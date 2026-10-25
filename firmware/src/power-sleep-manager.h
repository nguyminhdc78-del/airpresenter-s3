// Inactivity tracking and deep sleep. Any button on an RTC-capable GPIO (0-21,
// except the BOOT pin) wakes the remote; it then reboots and reconnects by itself.
#pragma once

#include <stdint.h>

class PowerSleepManager {
 public:
  // Call BEFORE the button scanner's begin() (restores pins used for wake-up).
  void begin(const uint8_t* buttonPins, uint8_t count);
  void notifyActivity(uint32_t now) { lastActivityMs_ = now; }

  // True when idle longer than the timeout (shorter while no PC is connected). Losing the
  // link restarts the count: the PC gets the full unconnected timeout to come back.
  bool shouldSleep(uint32_t now, bool connected);

  // A button went down just as the remote was about to sleep and came back up: the user
  // is back, so don't sleep. A button still held after 2 s is stuck (bag) and ignored.
  bool userCameBack() const;

  // Returns true once no button is held, false on timeout.
  bool waitForButtonsReleased(uint32_t timeoutMs) const;

  // Arms button wake-up and powers down. Never returns. Turn off LEDs and
  // sensors before calling.
  void enterDeepSleep();

  static bool wokeFromDeepSleep();
  static void logWakeReason();  // crash / watchdog / brownout last time, wake-up button

 private:
  bool anyButtonPressed() const;

  const uint8_t* pins_ = nullptr;
  uint8_t count_ = 0;
  uint32_t lastActivityMs_ = 0;
  bool wasConnected_ = false;
};
