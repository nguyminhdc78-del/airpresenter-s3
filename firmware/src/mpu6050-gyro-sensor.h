// Minimal MPU6050 gyroscope driver (also works with MPU6500/MPU9250 clones sold as
// "GY-521"): init, bias calibration, self-healing bias tracking (gyro-bias-tracker.h),
// recovery after a sensor glitch or reset, sleep.
#pragma once

#include <stdint.h>

#include "gyro-bias-tracker.h"
#include "imu-sample-sanity-check.h"

class Mpu6050GyroSensor {
 public:
  // Returns false when no sensor answers on I2C (remote then works with D-pad only).
  bool begin(int8_t sdaPin, int8_t sclPin, uint8_t address);

  // Finds the zero-rate offset, retrying while the remote is being moved.
  // reuseSaved: after a deep-sleep wake, reuse the offset saved by sleep() instead
  // (instant start). Returns false if it never saw the remote still; the runtime
  // tracking in readRatesDps() then corrects the offset later.
  bool calibrateBias(uint8_t maxAttempts, bool reuseSaved);

  // Angular rate in deg/s for X, Y, Z. With trackBias = true (cursor not moving)
  // the offset follows temperature drift and heals a bad calibration.
  bool readRatesDps(float out[3], bool trackBias);

  // Accelerometer from the last read, in g (at rest it points up, ~1 g long).
  const float* accelG() const { return accel_; }

  // Saves the offset for the next wake, then puts the sensor in ~5 uA sleep mode.
  void sleep();

 private:
  bool configure();                 // writes every register the driver relies on
  void startBus();                  // I2C driver at 400 kHz, 5 ms timeout
  void checkSettings(uint32_t now);  // re-configures a sensor that reset itself
  bool writeRegister(uint8_t reg, uint8_t value);
  bool readRegister(uint8_t reg, uint8_t& value);
  bool readRaw(int16_t gyro[3]);  // also refreshes accelRaw_, accel_ and tempRaw_

  uint8_t address_ = 0x68;
  int8_t sdaPin_ = -1;
  int8_t sclPin_ = -1;
  ImuSampleSanityCheck sanity_;
  uint8_t whoAmI_ = 0;
  bool present_ = false;
  int16_t accelRaw_[3] = {0, 0, 16384};
  int16_t tempRaw_ = 0;
  float accel_[3] = {0.0f, 0.0f, 1.0f};  // g
  GyroBiasTracker tracker_;
  uint16_t failedReads_ = 0;  // in a row
  uint32_t lastRecoveryMs_ = 0;
  uint32_t lastSettingsCheckMs_ = 0;
};
