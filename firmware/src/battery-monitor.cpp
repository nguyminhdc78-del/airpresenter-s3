#include "battery-monitor.h"

#include <Arduino.h>

#include "remote-user-config.h"

namespace {

// Rough LiPo discharge curve under light load: {millivolts, percent}, descending.
struct CurvePoint {
  uint16_t mv;
  uint8_t percent;
};
constexpr CurvePoint kLipoCurve[] = {
    {4200, 100}, {4100, 90}, {4000, 80}, {3900, 65}, {3800, 50}, {3750, 40},
    {3700, 30},  {3650, 20}, {3600, 12}, {3500, 5},  {3300, 0},
};
constexpr uint8_t kCurveSize = sizeof(kLipoCurve) / sizeof(kLipoCurve[0]);

// Below this the ADC sees no battery: assume USB power and report full.
constexpr uint16_t kNoBatteryMv = 2500;

}  // namespace

void BatteryMonitor::begin(int8_t adcPin, float dividerRatio) {
  pin_ = adcPin;
  dividerRatio_ = dividerRatio;
  if (!enabled()) return;
  filteredMv_ = readBatteryMillivolts();
  percent_ = millivoltsToPercent(filteredMv_);
  low_ = percent_ <= cfg::kLowBatteryPercent;
  lastSampleMs_ = millis();
  Serial.printf("[bat] %.0f mV -> %u%%\n", filteredMv_, percent_);
}

bool BatteryMonitor::update(uint32_t now) {
  if (!enabled() || now - lastSampleMs_ < cfg::kBatterySampleIntervalMs) return false;
  lastSampleMs_ = now;

  // Heavy smoothing: BLE radio bursts make single readings jump around.
  filteredMv_ = filteredMv_ * 0.7f + readBatteryMillivolts() * 0.3f;
  const uint8_t previous = percent_;
  percent_ = millivoltsToPercent(filteredMv_);
  // Hysteresis: near the threshold the reading wanders, the warning must not flicker.
  if (percent_ <= cfg::kLowBatteryPercent) low_ = true;
  if (percent_ >= cfg::kLowBatteryPercent + 3) low_ = false;
  return percent_ != previous;
}

bool BatteryMonitor::isLow() const { return enabled() && low_; }

uint16_t BatteryMonitor::readBatteryMillivolts() const {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < 8; ++i) sum += analogReadMilliVolts(pin_);  // factory-calibrated
  return static_cast<uint16_t>((sum / 8) * dividerRatio_);
}

uint8_t BatteryMonitor::millivoltsToPercent(float mv) {
  if (mv < kNoBatteryMv) return 100;
  if (mv >= kLipoCurve[0].mv) return 100;
  for (uint8_t i = 1; i < kCurveSize; ++i) {
    const CurvePoint& hi = kLipoCurve[i - 1];
    const CurvePoint& lo = kLipoCurve[i];
    if (mv >= lo.mv) {
      // Linear interpolation between the two surrounding curve points.
      const float t = (mv - lo.mv) / static_cast<float>(hi.mv - lo.mv);
      return static_cast<uint8_t>(lo.percent + t * (hi.percent - lo.percent) + 0.5f);
    }
  }
  return 0;
}
