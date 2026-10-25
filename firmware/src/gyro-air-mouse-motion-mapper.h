// Converts cursor-oriented turn rates (see gravity-referenced-screen-axes.h) into cursor
// movement: smoothing, dead-zone (hand tremor) and an acceleration curve
// (slow = precise, fast = far). Slow turns are smoothed harder than fast ones.
// Dead-zone and curve use the combined turn speed, so the cursor always moves in the
// direction the hand turns. All filters use time constants: tick length doesn't matter.
#pragma once

class GyroAirMouseMapper {
 public:
  // Adds this tick's movement (mouse counts) to dx/dy while the clutch is held.
  // rightDps / downDps: deg/s, positive = cursor right / down.
  void update(float rightDps, float downDps, bool clutchHeld, float dtSec, float& dx, float& dy);

 private:
  static float rateToCountsPerSec(float rateDps);

  float rightFiltered_ = 0.0f;  // smoothed turn rate (deg/s)
  float downFiltered_ = 0.0f;
};
