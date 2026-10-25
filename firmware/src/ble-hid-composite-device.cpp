#include "ble-hid-composite-device.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <string.h>

#include "hid-report-descriptor-and-usages.h"
#include "remote-user-config.h"

namespace {

constexpr uint8_t kReliableAttempts = 30;  // x 2 ms = up to 60 ms (~4 connection events)
// Longer than the 30 s SMP timeout: phones show a "Pair?" dialog first.
constexpr int32_t kUnencryptedLinkTimeoutMs = 35000;

constexpr size_t constLength(const char* s) { return *s ? 1 + constLength(s + 1) : 0; }
// flags(3) + appearance(4) + HID UUID(4) + name header(2) + name must fit in 31 bytes.
static_assert(constLength(cfg::kDeviceName) <= 18, "kDeviceName longer than 18 chars is cut from advertising");

}  // namespace

void BleHidCompositeDevice::begin(const char* deviceName) {
  NimBLEDevice::init(deviceName);
  NimBLEDevice::setPower(cfg::kBleTxPowerDbm);
  // Bonding + LE Secure Connections, "Just Works" pairing (remote has no screen).
  NimBLEDevice::setSecurityAuth(true, false, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  server_ = NimBLEDevice::createServer();
  server_->setCallbacks(createServerCallbacks());
  server_->advertiseOnDisconnect(true);

  hid_ = new NimBLEHIDDevice(server_);
  hid_->setManufacturer(cfg::kManufacturer);
  hid_->setPnp(0x02, 0xe502, 0xa111, 0x0210);  // vendor source=USB, ids are arbitrary
  hid_->setHidInfo(0x00, 0x01);                // country=none, flags=remote wake
  hid_->setReportMap(const_cast<uint8_t*>(hid::kReportMap), sizeof(hid::kReportMap));
  keyboard_.chr = hid_->getInputReport(hid::kReportIdKeyboard);
  hid_->getOutputReport(hid::kReportIdKeyboard);  // Caps/Num lock LEDs: accepted, ignored
  mouse_.chr = hid_->getInputReport(hid::kReportIdMouse);
  consumer_.chr = hid_->getInputReport(hid::kReportIdConsumer);
  hid_->setBatteryLevel(100);

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->setAppearance(HID_KEYBOARD);
  adv->addServiceUUID(hid_->getHidService()->getUUID());
  adv->setName(deviceName);
  adv->start();
  Serial.printf("[ble] advertising as \"%s\"\n", deviceName);
}

void BleHidCompositeDevice::service(uint32_t now) {
  events_.print();
  flush(keyboard_, 1);
  flush(consumer_, 1);
  flush(mouse_, 1);

  const uint16_t link = linkHandle_;
  // Signed difference: linkUpMs_ may be set by the host task just after `now` was read.
  if (link != kNoConnection && secureHandle_ == kNoConnection &&
      static_cast<int32_t>(now - linkUpMs_) > kUnencryptedLinkTimeoutMs) {
    Serial.println("[ble] link never encrypted, dropping it");
    linkUpMs_ = now;  // one attempt per timeout period
    selfDroppedHandle_ = link;  // our own drop: not a sign of a stale pairing on the PC
    server_->disconnect(link);
  }
  const uint16_t secure = secureHandle_;
  if (secure != kNoConnection && interval_.fallbackDue(now)) {
    Serial.println("[ble] PC did not grant 7.5 ms, asking for 15 ms");
    constexpr uint16_t kSafe = BleConnectionIntervalPolicy::kSafeUnits;
    server_->updateConnParams(secure, kSafe, kSafe, 0, 300);
  }
}

bool BleHidCompositeDevice::publish(ReportChannel& ch, const uint8_t* data, uint8_t len,
                                    uint8_t attempts) {
  memcpy(ch.data, data, len);
  ch.len = len;
  ch.dirty = true;
  return flush(ch, attempts);
}

bool BleHidCompositeDevice::flush(ReportChannel& ch, uint8_t attempts) {
  for (uint8_t i = 0; ch.dirty && i < attempts; ++i) {
    const uint16_t handle = secureHandle_;
    if (handle == kNoConnection || ch.chr == nullptr) {
      ch.dirty = false;  // no host: on disconnect it releases every key by itself
      return false;
    }
    // Pass the bytes explicitly: NimBLE copies them right away. The value-less
    // notify() reads the characteristic later from the host task, so a press
    // quickly followed by a release would reach the PC as release only.
    if (ch.chr->notify(ch.data, ch.len, handle)) {
      ch.chr->setValue(ch.data, ch.len);  // what a GATT read returns
      ch.dirty = false;
    } else if (i + 1 < attempts) {
      delay(2);  // radio queue full: let the next connection event drain it
    }
  }
  return !ch.dirty;
}

void BleHidCompositeDevice::keyTap(uint8_t modifiers, uint8_t keycode) {
  const uint8_t pressed[8] = {modifiers, 0, keycode, 0, 0, 0, 0, 0};  // [mods][rsvd][keys x6]
  const uint8_t released[8] = {0};
  tap(keyboard_, pressed, released, sizeof(pressed));
}

void BleHidCompositeDevice::consumerTap(uint16_t usage) {
  const uint8_t pressed[2] = {static_cast<uint8_t>(usage & 0xFF), static_cast<uint8_t>(usage >> 8)};
  const uint8_t released[2] = {0, 0};
  tap(consumer_, pressed, released, sizeof(pressed));
}

void BleHidCompositeDevice::tap(ReportChannel& ch, const uint8_t* pressed, const uint8_t* released,
                                uint8_t len) {
  if (!isConnected()) return;
  // An older release still waiting must reach the PC first. Overwriting it would
  // leave that key held there, and the PC's auto-repeat would run away with it.
  if (ch.dirty && !flush(ch, kReliableAttempts)) return;  // still stuck: drop this tap
  if (!publish(ch, pressed, len, kReliableAttempts)) {
    ch.dirty = false;  // press never left: drop the keystroke rather than send it late
    return;
  }
  publish(ch, released, len, kReliableAttempts);  // if this fails, service() keeps retrying
}

void BleHidCompositeDevice::mouseButtonDown(uint8_t mask) {
  mouseButtons_ |= mask;
  publishMouse(0);
}

void BleHidCompositeDevice::mouseButtonUp(uint8_t mask) {
  mouseButtons_ &= static_cast<uint8_t>(~mask);
  publishMouse(0);
}

void BleHidCompositeDevice::mouseWheel(int8_t notches) { publishMouse(notches); }

bool BleHidCompositeDevice::publishMouse(int8_t wheel) {
  if (!isConnected()) return false;
  const uint8_t report[4] = {mouseButtons_, 0, 0, static_cast<uint8_t>(wheel)};
  return publish(mouse_, report, sizeof(report), kReliableAttempts);
}

void BleHidCompositeDevice::mouseMove(int8_t dx, int8_t dy) {
  const uint16_t handle = secureHandle_;
  if ((dx == 0 && dy == 0) || handle == kNoConnection || mouse_.dirty) return;  // button state first
  const uint8_t report[4] = {mouseButtons_, static_cast<uint8_t>(dx), static_cast<uint8_t>(dy), 0};
  mouse_.chr->notify(report, sizeof(report), handle);  // a lost motion report is not worth a retry
}

void BleHidCompositeDevice::setBatteryLevel(uint8_t percent) {
  if (hid_ != nullptr) hid_->setBatteryLevel(percent, isConnected());
}

void BleHidCompositeDevice::forgetAllHosts() {
  Serial.println("[ble] deleting all bonds");
  NimBLEDevice::deleteAllBonds();
}
