// Shared helpers for the GyroAirMouseMapper host tests. Expected values come from the speed
// law in gyro-air-mouse-motion-mapper.cpp with the cfg:: constants, never hand-picked, and the
// filters are checked by their properties (steady state, symmetry, tick independence), not by
// re-implementing them here.
#pragma once

#include <cmath>

#include "gyro-air-mouse-motion-mapper.h"
#include "remote-user-config.h"

namespace mapper_test {

constexpr float kDt = cfg::kMotionTickUs / 1e6f;  // default motion tick

// rateToCountsPerSec(): dead-zone subtracted, then sens * m * (1 + m / knee), keeping the sign.
inline float speedFor(float rateDps) {
  const float m = std::fabs(rateDps) - cfg::kAirDeadzoneDps;
  if (m <= 0.0f) return 0.0f;
  const float speed = cfg::kAirSensitivity * m * (1.0f + m / cfg::kAirAccelKneeDps);
  return rateDps > 0.0f ? speed : -speed;
}

// Tolerance: relative 1e-4 plus a small absolute floor for float accumulation.
inline float tolFor(float expected) { return 1e-4f * std::fabs(expected) + 1e-5f; }

// Feeds constant cursor rates (deg/s, + = right / down) into dx/dy for `ticks` ticks.
inline void runConstant(GyroAirMouseMapper& m, float right, float down, bool clutch, int ticks,
                        float& dx, float& dy, float dt = kDt) {
  for (int i = 0; i < ticks; ++i) m.update(right, down, clutch, dt, dx, dy);
}

// Total counts from a fresh mapper after `seconds` of constant rates, in ticks of `dt`.
inline void totalsOver(float right, float down, float seconds, float dt, float& dx, float& dy) {
  GyroAirMouseMapper m;
  dx = 0.0f;
  dy = 0.0f;
  runConstant(m, right, down, true, static_cast<int>(seconds / dt + 0.5f), dx, dy, dt);
}

// Signed counts emitted by one tick of a constant downward rate, once every filter settled.
inline float settledDownStep(float rateDps) {
  GyroAirMouseMapper m;
  float dx = 0.0f, dy = 0.0f;
  runConstant(m, 0.0f, rateDps, true, 800, dx, dy);  // 6 s: far past every time constant
  dx = 0.0f;
  dy = 0.0f;
  m.update(0.0f, rateDps, true, kDt, dx, dy);
  return dy;
}

}  // namespace mapper_test
