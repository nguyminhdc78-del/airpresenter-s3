// Rejects IMU burst reads that a flaky I2C link garbled (loose GY-521 wire: errors cluster
// while the remote is being swung). One garbled read used to poison the gyro calibration or
// the bias tracking, leaving the cursor pulled to one side until the next reboot.
// Pure logic, no Arduino calls: host tests drive it with synthetic samples.
#pragma once

#include <stdint.h>
#include <stdlib.h>

namespace imu_sanity {
// Raw words of one burst: accel x,y,z, temperature, gyro x,y,z.
constexpr int kTempIndex = 3;
constexpr int32_t kMinTempLsb = -19500;  // about -20 deg C on the 6050 and the 6500
constexpr int32_t kMaxTempLsb = 21500;   // about +85 deg C
// Between reads a few ms apart a hand cannot change its turn rate by 250 deg/s nor the
// specific force by 1.5 g; a bit error easily does.
constexpr int32_t kMaxGyroJumpLsb = 16375;   // 250 deg/s at 65.5 LSB/(deg/s)
constexpr int32_t kMaxAccelJumpLsb = 24576;  // 1.5 g at 16384 LSB/g
constexpr uint32_t kJumpWindowMs = 20;       // older reference: a jump may be real
}  // namespace imu_sanity

class ImuSampleSanityCheck {
 public:
  // True if the 7 raw words look like a real reading; then they become the new reference.
  bool plausible(const int16_t v[7], uint32_t nowMs) {
    using namespace imu_sanity;
    bool allZero = true;
    bool allOnes = true;  // SDA stuck high reads 0xFFFF everywhere
    for (int i = 0; i < 7; ++i) {
      if (v[i] != 0) allZero = false;
      if (v[i] != -1) allOnes = false;
    }
    if (allZero || allOnes) return false;
    if (v[kTempIndex] < kMinTempLsb || v[kTempIndex] > kMaxTempLsb) return false;
    if (haveLast_ && nowMs - lastMs_ <= kJumpWindowMs) {
      for (int i = 0; i < 7; ++i) {
        if (i == kTempIndex) continue;
        const int32_t limit = i < kTempIndex ? kMaxAccelJumpLsb : kMaxGyroJumpLsb;
        if (abs(static_cast<int32_t>(v[i]) - last_[i]) > limit) return false;
      }
    }
    for (int i = 0; i < 7; ++i) last_[i] = v[i];
    lastMs_ = nowMs;
    haveLast_ = true;
    return true;
  }

 private:
  int16_t last_[7] = {0, 0, 0, 0, 0, 0, 0};
  uint32_t lastMs_ = 0;
  bool haveLast_ = false;
};
