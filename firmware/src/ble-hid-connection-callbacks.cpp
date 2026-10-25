// NimBLE connection callbacks of BleHidCompositeDevice. They run on the NimBLE host task
// (core 0), so they only record state: no Serial output, nothing that can block.
#include <Arduino.h>
#include <NimBLEDevice.h>

#include "ble-hid-composite-device.h"
#include "remote-user-config.h"

// Connection events. Runs on the NimBLE host task.
class BleHidCompositeDevice::ServerCallbacks : public NimBLEServerCallbacks {
 public:
  explicit ServerCallbacks(BleHidCompositeDevice& owner) : owner_(owner) {}

  void onConnect(NimBLEServer*, NimBLEConnInfo& info) override {
    owner_.linkUpMs_ = millis();
    owner_.linkHandle_ = info.getConnHandle();
    owner_.interval_.linkUp(info.getConnInterval());
    // Ask the host to encrypt now (bonded PC: re-encrypt, new PC: pair).
    NimBLEDevice::startSecurity(info.getConnHandle());
    owner_.events_.connected();  // printed by service(): no Serial I/O on the host task
  }

  void onDisconnect(NimBLEServer*, NimBLEConnInfo& info, int reason) override {
    const bool wasEncrypted = owner_.secureHandle_ == info.getConnHandle();
    if (wasEncrypted) owner_.secureHandle_ = kNoConnection;
    if (owner_.linkHandle_ == info.getConnHandle()) {
      owner_.linkHandle_ = kNoConnection;
      owner_.interval_.linkDown();
    }
    // Advertising restarts automatically (advertiseOnDisconnect).
    const bool droppedByUs = owner_.selfDroppedHandle_ == info.getConnHandle();
    if (droppedByUs) owner_.selfDroppedHandle_ = kNoConnection;
    const bool quick = millis() - owner_.linkUpMs_ < BleLinkEventLog::kQuickDropMs;
    owner_.events_.disconnected(reason, !wasEncrypted && !droppedByUs && quick);
  }

  void onAuthenticationComplete(NimBLEConnInfo& info) override {
    NimBLEServer* server = NimBLEDevice::getServer();
    if (!info.isEncrypted()) {
      owner_.events_.pairingFailed();
      server->disconnect(info.getConnHandle());
      return;
    }
    owner_.secureHandle_ = info.getConnHandle();
    // 15 ms, or 7.5 ms first when cfg::kBleFastInterval (see ble-connection-interval-policy.h).
    // Latency 0: with peripheral latency the ESP32 radio sleeps through events even with
    // reports queued, which showed up as 45-60 ms stalls followed by bursts (a jerky
    // cursor). Supervision timeout 300 x 10 ms = 3 s.
    constexpr uint16_t kUnits = cfg::kBleFastInterval ? BleConnectionIntervalPolicy::kFastUnits
                                                      : BleConnectionIntervalPolicy::kSafeUnits;
    server->updateConnParams(info.getConnHandle(), kUnits, kUnits, 0, 300);
    if (cfg::kBleFastInterval) owner_.interval_.fastRequested(millis());  // 15 ms fallback armed
    owner_.events_.encrypted();
  }

  void onConnParamsUpdate(NimBLEConnInfo& info) override {
    owner_.interval_.changed(info.getConnInterval());
    owner_.events_.linkParams(info.getConnInterval(), info.getConnLatency(), info.getConnTimeout());
  }

 private:
  BleHidCompositeDevice& owner_;
};

NimBLEServerCallbacks* BleHidCompositeDevice::createServerCallbacks() { return new ServerCallbacks(*this); }
