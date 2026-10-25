// Turns gyro rates (sensor frame, deg/s) into cursor rates using the direction of gravity
// from the accelerometer. "Right/left" is the turn about the room's vertical axis and
// "up/down" the turn about the horizontal axis across the pointing direction, so the
// cursor follows the hand the same way whether the GY-521 sits chip up or chip down and
// however the remote is rolled in the hand. Only the pointing axis has to be known.
// Pure math (no Arduino), unit-tested on the host.
#pragma once

#include <math.h>

struct ScreenRates {
  float right;  // deg/s, positive = cursor moves right
  float down;   // deg/s, positive = cursor moves down
};

class GravityReference {
 public:
  // Pulls the "up" estimate toward the accelerometer (in g; at rest it reads +1 g pointing
  // up). alpha: 0..1 per sample; the first sample sets the estimate. Skipped while the hand
  // accelerates the remote (|a| far from 1 g): then the reading is not gravity.
  void addAccel(const float accel[3], float alpha) {
    if (ready_) {
      const float g = sqrtf(dot(accel, accel));
      if (g < 0.85f || g > 1.15f) return;
    }
    const float a = ready_ ? alpha : 1.0f;
    for (int i = 0; i < 3; ++i) up_[i] += a * (accel[i] - up_[i]);
    ready_ = true;
  }

  // Turns the estimate with the gyro (deg/s, sensor frame): seen from a turning body, a
  // fixed world direction turns the other way, d(up)/dt = up x w. "Up" then follows a nod
  // or a wrist roll at once instead of lagging behind the slow accelerometer filter.
  void rotate(const float ratesDps[3], float dtSec) {
    if (!ready_) return;
    const float k = dtSec * 0.017453293f;  // deg -> rad, times dt
    const float w[3] = {ratesDps[0] * k, ratesDps[1] * k, ratesDps[2] * k};
    const float turn[3] = {up_[1] * w[2] - up_[2] * w[1], up_[2] * w[0] - up_[0] * w[2],
                           up_[0] * w[1] - up_[1] * w[0]};
    for (int i = 0; i < 3; ++i) up_[i] += turn[i];
  }

  bool ready() const { return ready_; }
  const float* up() const { return up_; }  // gravity estimate (~1 g long), sensor frame

  // forward: the remote's pointing axis in sensor coordinates (unit vector).
  ScreenRates toScreen(const float rates[3], const float forward[3]) const {
    float up[3] = {up_[0], up_[1], up_[2]};
    if (!normalize(up)) return ScreenRates{0.0f, 0.0f};
    // left = up x forward: horizontal, across the pointing direction
    float left[3] = {up[1] * forward[2] - up[2] * forward[1],
                     up[2] * forward[0] - up[0] * forward[2],
                     up[0] * forward[1] - up[1] * forward[0]};
    const float yaw = dot(rates, up);  // + = turning left, seen from above
    if (!normalize(left)) return ScreenRates{-yaw, 0.0f};  // pointing straight up/down
    const float pitch = dot(rates, left);  // + = nose going down
    return ScreenRates{-yaw, pitch};
  }

 private:
  static float dot(const float a[3], const float b[3]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

  // Scales v to unit length; false when it is too short to have a direction.
  static bool normalize(float v[3]) {
    const float n = sqrtf(dot(v, v));
    if (n < 0.2f) return false;
    for (int i = 0; i < 3; ++i) v[i] /= n;
    return true;
  }

  float up_[3] = {0.0f, 0.0f, 1.0f};
  bool ready_ = false;
};
