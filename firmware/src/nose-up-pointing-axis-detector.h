// Finds which sensor axis is the remote's nose, however the GY-521 is mounted (chip up or
// down, turned any way). The user points the nose straight at the ceiling and presses AIR:
// the sensor axis that is then vertical (from the low-passed gravity "up") is the nose.
// The press is required: gravity alone cannot tell a deliberate nose-up from an arm
// hanging down or a remote lying on its side. Pressing AIR while holding the remote
// normally (top face up) is rejected, so an accidental setup cancels itself on next use.
// Pure logic, no Arduino calls: host tests drive it with synthetic samples.
#pragma once

#include <math.h>
#include <stdint.h>

namespace nose_setup {
constexpr float kTimeoutSec = 15.0f;
constexpr float kMinAxisAlignment = 0.82f;  // "up" within ~35 deg of one sensor axis
}  // namespace nose_setup

class NoseUpPointingAxisDetector {
 public:
  enum class Result : uint8_t { Waiting, Found, Rejected, TimedOut };

  // `up` = gravity direction (sensor frame, any length) while the remote is held normally.
  // Its main axis is the top face, which can never be the nose.
  void start(const float up[3]) {
    topAxis_ = dominantAxis(up);
    elapsedSec_ = 0.0f;
    active_ = true;
  }

  bool active() const { return active_; }

  // Call every motion tick. `confirmed` = AIR was just pressed: "the nose points up now".
  Result update(const float up[3], bool confirmed, float dtSec) {
    if (!active_) return Result::Waiting;
    elapsedSec_ += dtSec;
    if (!confirmed) {
      if (elapsedSec_ <= nose_setup::kTimeoutSec) return Result::Waiting;
      active_ = false;
      return Result::TimedOut;
    }
    active_ = false;
    const uint8_t k = dominantAxis(up);
    const float n = sqrtf(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);
    // Between two axes (tilted, or still swinging), top face up, or no sensor data.
    if (n < 0.5f || fabsf(up[k]) < nose_setup::kMinAxisAlignment * n || k == topAxis_) {
      return Result::Rejected;
    }
    axis_ = k;
    sign_ = up[k] > 0.0f ? 1 : -1;
    return Result::Found;
  }

  uint8_t axis() const { return axis_; }  // 0 = X, 1 = Y, 2 = Z (valid after Found)
  int8_t sign() const { return sign_; }   // +1 / -1

 private:
  static uint8_t dominantAxis(const float v[3]) {
    uint8_t k = 0;
    for (uint8_t i = 1; i < 3; ++i) {
      if (fabsf(v[i]) > fabsf(v[k])) k = i;
    }
    return k;
  }

  float elapsedSec_ = 0.0f;
  uint8_t topAxis_ = 2;
  uint8_t axis_ = 0;
  int8_t sign_ = 1;
  bool active_ = false;
};
