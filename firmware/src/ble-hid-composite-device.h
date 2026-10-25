// BLE HID peripheral that shows up on the PC as ONE device with keyboard, mouse and
// media-key functions. Handles advertising, bonding and report delivery.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ble-connection-interval-policy.h"
#include "ble-link-event-log.h"

class NimBLEServer;
class NimBLEServerCallbacks;
class NimBLEHIDDevice;
class NimBLECharacteristic;

class BleHidCompositeDevice {
 public:
  void begin(const char* deviceName);

  // Call every loop: prints connection events, re-sends reports the radio could not
  // take yet and drops a link that never got encrypted (so the real PC can connect).
  void service(uint32_t now);

  // True only once the link is paired + encrypted; a bare connection does not count.
  bool isConnected() const { return secureHandle_ != kNoConnection; }
  uint32_t connectionIntervalUs() const { return interval_.intervalUs(); }  // 0 = no link

  // Press + release in one call. The host never sees a held key, so its own
  // auto-repeat cannot fire (holding Ctrl+L would toggle the laser on/off forever).
  void keyTap(uint8_t modifiers, uint8_t keycode);
  void consumerTap(uint16_t usage);

  void mouseButtonDown(uint8_t mask);
  void mouseButtonUp(uint8_t mask);
  void mouseWheel(int8_t notches);  // positive = scroll up
  void mouseMove(int8_t dx, int8_t dy);  // best effort: skipped if the radio is busy

  void setBatteryLevel(uint8_t percent);
  void forgetAllHosts();  // delete stored pairings; restart afterwards

 private:
  static constexpr uint16_t kNoConnection = 0xFFFF;  // BLE_HS_CONN_HANDLE_NONE

  // Latest state of one input report. "dirty" = not delivered yet: kept and retried,
  // so a lost key release can never leave a key stuck down on the PC.
  struct ReportChannel {
    NimBLECharacteristic* chr;
    uint8_t data[8];
    uint8_t len;
    bool dirty;
  };

  class ServerCallbacks;  // NimBLE connection events: ble-hid-connection-callbacks.cpp
  NimBLEServerCallbacks* createServerCallbacks();

  bool publish(ReportChannel& ch, const uint8_t* data, uint8_t len, uint8_t attempts);
  bool flush(ReportChannel& ch, uint8_t attempts);
  void tap(ReportChannel& ch, const uint8_t* pressed, const uint8_t* released, uint8_t len);
  bool publishMouse(int8_t wheel);

  NimBLEServer* server_ = nullptr;
  NimBLEHIDDevice* hid_ = nullptr;
  ReportChannel keyboard_ = {};
  ReportChannel mouse_ = {};
  ReportChannel consumer_ = {};
  uint8_t mouseButtons_ = 0;

  // Written by the NimBLE host task (core 0), read by loop() (core 1).
  volatile uint16_t linkHandle_ = kNoConnection;    // any connection
  volatile uint16_t secureHandle_ = kNoConnection;  // encrypted: reports go here
  volatile uint32_t linkUpMs_ = 0;
  volatile uint16_t selfDroppedHandle_ = kNoConnection;  // link we closed ourselves
  BleLinkEventLog events_;
  BleConnectionIntervalPolicy interval_;
};
