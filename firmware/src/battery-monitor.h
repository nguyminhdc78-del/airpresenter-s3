// Measures the LiPo voltage through a resistor divider and converts it to a
// percentage for the BLE battery service (Windows/Android show it in settings).
#pragma once

#include <stdint.h>

class BatteryMonitor {
 public:
  void begin(int8_t adcPin, float dividerRatio);

  // Samples every cfg::kBatterySampleIntervalMs. Returns true when percent() changed.
  bool update(uint32_t now);

  uint8_t percent() const { return percent_; }
  bool isLow() const;

 private:
  bool enabled() const { return pin_ >= 0; }
  uint16_t readBatteryMillivolts() const;
  static uint8_t millivoltsToPercent(float mv);

  int8_t pin_ = -1;
  float dividerRatio_ = 2.0f;
  float filteredMv_ = 0.0f;
  uint32_t lastSampleMs_ = 0;
  uint8_t percent_ = 100;
  bool low_ = false;  // latched below kLowBatteryPercent, cleared 3 % above it
};
