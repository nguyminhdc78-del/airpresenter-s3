// Keeps the gyro zero-rate offset (bias) right while the remote is idle in the hand or on a
// table: each 0.75 s window in which the remote was truly still moves the offset toward that
// window's mean. "Truly still" is strict, because a steady slow turn looks still to a spread
// test alone and learning it as offset makes the cursor drift on the next AIR press:
//   - gyro peak-to-peak small (no turning or tremor bursts),
//   - accelerometer steady (gravity did not move: no tilting, no carrying),
//   - a trusted offset is never pulled far at once (a steady pure yaw keeps gravity still).
// Pure logic, no Arduino calls: host tests drive it with synthetic samples.
#pragma once

#include <stdint.h>
#include <stdlib.h>

namespace gyro_bias {
constexpr float kLsbPerDps = 65.5f;                                       // +-500 deg/s
constexpr int32_t kStillSpreadLsb = static_cast<int32_t>(5.0f * kLsbPerDps);     // hand tremor ok
constexpr int32_t kVeryStillSpreadLsb = static_cast<int32_t>(1.0f * kLsbPerDps); // on a table
constexpr int32_t kAccelSteadyLsb = 819;            // 0.05 g peak-to-peak at +-2 g (16384 LSB/g)
constexpr float kMaxStepLsb = 0.5f * kLsbPerDps;    // per window when hand-held and trusted
constexpr float kMaxPullLsb = 1.5f * kLsbPerDps;    // hand-held window this far off: a slow turn
constexpr float kMaxTablePullLsb = 4.0f * kLsbPerDps;
constexpr uint32_t kWindowMs = 750;
constexpr uint16_t kWindowMinSamples = 50;
// A trusted offset that is really wrong (calibrated during a smooth turn, big temperature
// change) would otherwise never heal: this many still windows in a row that agree with each
// other (7.5 s) are believed over the trusted value. A slow turn never lasts that long.
constexpr uint8_t kAgreeingWindows = 10;
constexpr float kAgreeLsb = 0.3f * kLsbPerDps;
}  // namespace gyro_bias

// Min / max / sum of raw samples over one window.
struct ImuSampleWindow {
  int16_t gyroLo[3], gyroHi[3], accelLo[3], accelHi[3];
  int32_t gyroSum[3];
  uint16_t count;

  void reset() {
    for (int i = 0; i < 3; ++i) {
      gyroLo[i] = accelLo[i] = INT16_MAX;
      gyroHi[i] = accelHi[i] = INT16_MIN;
      gyroSum[i] = 0;
    }
    count = 0;
  }
  void add(const int16_t gyro[3], const int16_t accel[3]) {
    for (int i = 0; i < 3; ++i) {
      if (gyro[i] < gyroLo[i]) gyroLo[i] = gyro[i];
      if (gyro[i] > gyroHi[i]) gyroHi[i] = gyro[i];
      if (accel[i] < accelLo[i]) accelLo[i] = accel[i];
      if (accel[i] > accelHi[i]) accelHi[i] = accel[i];
      gyroSum[i] += gyro[i];
    }
    ++count;
  }
  int32_t gyroSpread() const { return spread(gyroLo, gyroHi); }
  int32_t accelSpread() const { return spread(accelLo, accelHi); }
  float gyroMean(int axis) const { return static_cast<float>(gyroSum[axis]) / count; }

 private:
  int32_t spread(const int16_t lo[3], const int16_t hi[3]) const {
    int32_t s = 0;
    for (int i = 0; i < 3 && count > 0; ++i) {
      const int32_t axis = static_cast<int32_t>(hi[i]) - lo[i];
      if (axis > s) s = axis;
    }
    return s;
  }
};

class GyroBiasTracker {
 public:
  void set(const float bias[3], bool trusted) {
    for (int i = 0; i < 3; ++i) bias_[i] = bias[i];
    trusted_ = trusted;
  }
  void distrust() { trusted_ = false; }  // offset may be off (sensor reset, temperature jump)
  const float* bias() const { return bias_; }
  bool trusted() const { return trusted_; }

  // Cursor in use (or a fresh start): drop the current window.
  void restart(uint32_t nowMs) {
    window_.reset();
    startMs_ = nowMs;
  }

  // One raw sample while the cursor is not being moved. Returns true if the offset changed.
  bool add(const int16_t gyro[3], const int16_t accel[3], uint32_t nowMs) {
    window_.add(gyro, accel);
    if (window_.count < gyro_bias::kWindowMinSamples || nowMs - startMs_ < gyro_bias::kWindowMs) return false;
    const bool changed = learnFromWindow();
    restart(nowMs);
    return changed;
  }

 private:
  bool learnFromWindow() {
    using namespace gyro_bias;
    const int32_t spread = window_.gyroSpread();
    if (spread > kStillSpreadLsb || window_.accelSpread() > kAccelSteadyLsb) {
      rejectedRun_ = 0;  // turning, shaking, tilted or carried meanwhile
      return false;
    }
    const bool veryStill = spread <= kVeryStillSpreadLsb;
    if (trusted_ && farFromBias(veryStill ? kMaxTablePullLsb : kMaxPullLsb) && !keepsAgreeing()) {
      return false;  // most likely a steady turn, not an offset
    }
    rejectedRun_ = 0;
    const bool believeMean = veryStill || !trusted_ || farFromBias(kMaxPullLsb);
    // Lying on a table, no trusted offset yet, or a run of agreeing windows: take the mean.
    // Hand-held: small steps, so whatever slipped through moves the offset by 0.5 deg/s at
    // most (hidden by the dead-zone).
    const float maxStep = believeMean ? 1.0e6f : kMaxStepLsb;
    for (int i = 0; i < 3; ++i) {
      float step = window_.gyroMean(i) - bias_[i];
      if (step > maxStep) step = maxStep;
      if (step < -maxStep) step = -maxStep;
      bias_[i] += step;
    }
    trusted_ = true;
    return true;
  }

  bool farFromBias(float limitLsb) const {
    for (int i = 0; i < 3; ++i) {
      const float off = window_.gyroMean(i) - bias_[i];
      if (off > limitLsb || off < -limitLsb) return true;
    }
    return false;
  }

  // Counts far-off windows in a row whose means agree; true once there are enough of them.
  bool keepsAgreeing() {
    bool agree = rejectedRun_ > 0;
    for (int i = 0; i < 3; ++i) {
      const float d = window_.gyroMean(i) - rejectedMean_[i];
      if (d > gyro_bias::kAgreeLsb || d < -gyro_bias::kAgreeLsb) agree = false;
      rejectedMean_[i] = window_.gyroMean(i);
    }
    rejectedRun_ = agree ? rejectedRun_ + 1 : 1;
    return rejectedRun_ >= gyro_bias::kAgreeingWindows;
  }

  float bias_[3] = {0.0f, 0.0f, 0.0f};  // raw LSB
  bool trusted_ = false;
  uint8_t rejectedRun_ = 0;
  float rejectedMean_[3] = {0.0f, 0.0f, 0.0f};
  ImuSampleWindow window_ = {};
  uint32_t startMs_ = 0;
};
