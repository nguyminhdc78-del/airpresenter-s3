// BLE connection interval policy. Asks the PC for 7.5 ms (133 cursor report slots per
// second instead of 66.7: on a 60 Hz screen 15 ms made every ~9th frame carry a double step)
// and falls back to 15 ms, which Windows, macOS, Android and iOS all accept, when 7.5 ms is
// not granted within a few seconds (Apple hosts refuse intervals under 11.25 ms).
// The hooks run on the NimBLE host task (core 0); the getters on loop() (core 1).
#pragma once

#include <stdint.h>

class BleConnectionIntervalPolicy {
 public:
  static constexpr uint16_t kFastUnits = 6;   // x 1.25 ms = 7.5 ms
  static constexpr uint16_t kSafeUnits = 12;  // 15 ms
  static constexpr uint32_t kGrantWaitMs = 3000;

  // ---- core 0 (NimBLE callbacks) ----
  void linkUp(uint16_t units) { intervalUnits_ = units; requestedAtMs_ = 0; fallbackSent_ = false; }
  void linkDown() { intervalUnits_ = 0; requestedAtMs_ = 0; }
  void fastRequested(uint32_t now) { requestedAtMs_ = now | 1; }  // | 1: 0 means "none"
  void changed(uint16_t units) { intervalUnits_ = units; }

  // ---- core 1 (loop) ----
  // True once per link when the 7.5 ms request was ignored or refused and the link is
  // slower than 15 ms: ask for 15 ms. A host that picked 10-15 ms is left alone.
  bool fallbackDue(uint32_t now) {
    const uint32_t requested = requestedAtMs_;
    const uint16_t units = intervalUnits_;
    if (requested == 0 || fallbackSent_ || (units != 0 && units <= kSafeUnits)) return false;
    if (static_cast<int32_t>(now - requested) < static_cast<int32_t>(kGrantWaitMs)) return false;
    fallbackSent_ = true;
    return true;
  }
  uint32_t intervalUs() const { return intervalUnits_ * 1250UL; }  // 0 = no link

 private:
  volatile uint16_t intervalUnits_ = 0;
  volatile uint32_t requestedAtMs_ = 0;
  volatile bool fallbackSent_ = false;
};
