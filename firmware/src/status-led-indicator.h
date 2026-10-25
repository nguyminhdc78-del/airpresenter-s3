// Onboard LED feedback:
//   yellow solid      = calibrating gyro (keep the remote still)
//   blue blink 1 Hz   = not connected, waiting for the PC
//   green 1.5 s       = just connected, then off to save battery
//   red short blink   = battery low
//   purple solid      = FN tapped, waiting for the next button
//   cyan solid        = FN held 2 s: release now to teach the nose direction
//   cyan fast blink   = nose setup: point the nose at the ceiling, then press AIR
//   green x2 / red x2 = nose direction saved / nothing changed
//   white x3          = pairings deleted, rebooting
#pragma once

#include <stdint.h>

class StatusLedIndicator {
 public:
  void begin(int8_t pin, bool isRgb, uint8_t brightness);
  void showCalibrating();
  void flashPairingsCleared() { flash(level_, level_, level_, 3); }  // blocking, ~1 s
  void showNoseSetupArmed() { setColor(0, level_, level_); }
  void showNoseSetup(uint32_t now);
  // Green x2 (ok) or red x2, non-blocking: drawResultFlash() shows it for ~0.6 s.
  void startResultFlash(bool ok, uint32_t now);
  bool drawResultFlash(uint32_t now);  // true while the flash owns the LED
  void update(uint32_t now, bool connected, bool lowBattery, bool fnLatched);
  void off() { setColor(0, 0, 0); }

 private:
  void flash(uint8_t r, uint8_t g, uint8_t b, uint8_t times);
  void setColor(uint8_t r, uint8_t g, uint8_t b);

  bool resultShowing_ = false;
  bool resultOk_ = false;
  uint32_t resultStartMs_ = 0;

  int8_t pin_ = -1;
  bool isRgb_ = true;
  uint8_t level_ = 24;
  uint32_t lastColor_ = 0xFFFFFFFF;  // forces the first write
  bool wasConnected_ = false;
  uint32_t connectedAtMs_ = 0;
};
