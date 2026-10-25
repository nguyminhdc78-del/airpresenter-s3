#include "gyro-air-mouse-motion-mapper.h"

#include <math.h>

#include "low-pass-filter-weight.h"
#include "remote-user-config.h"

void GyroAirMouseMapper::update(float rightDps, float downDps, bool clutchHeld, float dtSec,
                                float& dx, float& dy) {
  if (!clutchHeld) {
    // Restart the filter from zero next time so the cursor never jumps on press.
    rightFiltered_ = downFiltered_ = 0.0f;
    return;
  }

  // 1. Speed-adaptive low-pass (the idea of the 1-euro pointer filter): slow turns (precise
  //    aiming, hand tremor) get a low cutoff, fast turns a high one (little lag), blended in
  //    frequency. It stays a single first-order filter, so speeding up never overshoots and
  //    a stop never restarts. Its own output sets the speed, so tremor can't flicker it.
  const float speed = sqrtf(rightFiltered_ * rightFiltered_ + downFiltered_ * downFiltered_);
  float fast = (speed - cfg::kAirTierSlowDps) / (cfg::kAirTierFastDps - cfg::kAirTierSlowDps);
  if (fast < 0.0f) fast = 0.0f;
  if (fast > 1.0f) fast = 1.0f;
  const float slowHz = 1.0f / cfg::kAirSlowSmoothingSec;  // 2*pi*cutoff, in rad/s
  const float fastHz = 1.0f / cfg::kAirSmoothingSec;
  const float weight = lowPassWeight(dtSec, 1.0f / (slowHz + (fastHz - slowHz) * fast));
  rightFiltered_ += weight * (rightDps - rightFiltered_);
  downFiltered_ += weight * (downDps - downFiltered_);
  const float right = rightFiltered_;
  const float down = downFiltered_;

  // 2. Dead-zone and acceleration act on the turn speed, then scale both axes alike, so a
  //    diagonal keeps its direction. Per axis, the curve bent diagonals toward the stronger
  //    axis and the dead-zone snapped slow diagonals to pure horizontal / vertical.
  const float rate = sqrtf(right * right + down * down);
  if (rate <= 0.0f) return;
  const float countsPerDegree = rateToCountsPerSec(rate) / rate;
  dx += right * countsPerDegree * dtSec;
  dy += down * countsPerDegree * dtSec;
}

float GyroAirMouseMapper::rateToCountsPerSec(float rateDps) {
  // Soft dead-zone: subtract it instead of clamping so movement starts smoothly.
  const float magnitude = fabsf(rateDps) - cfg::kAirDeadzoneDps;
  if (magnitude <= 0.0f) return 0.0f;

  // speed = sensitivity * rate * (1 + rate / knee): linear when slow, faster when flicked.
  const float speed = cfg::kAirSensitivity * magnitude * (1.0f + magnitude / cfg::kAirAccelKneeDps);
  return rateDps > 0.0f ? speed : -speed;
}
