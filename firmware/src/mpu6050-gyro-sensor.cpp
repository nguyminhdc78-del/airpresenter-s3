#include "mpu6050-gyro-sensor.h"

#include <Arduino.h>
#include <Wire.h>
#include <stdlib.h>

#include "remote-user-config.h"

namespace {

// Registers. kRegAccelConfig2 (accelerometer low-pass) exists on the MPU-6500/9250 only;
// a burst from kRegAccelXoutH gives accel x,y,z, temperature, gyro x,y,z.
enum : uint8_t { kRegSampleRateDiv = 0x19, kRegConfig = 0x1A, kRegGyroConfig = 0x1B, kRegAccelConfig2 = 0x1D,
                 kRegAccelXoutH = 0x3B, kRegPwrMgmt1 = 0x6B, kRegPwrMgmt2 = 0x6C, kRegWhoAmI = 0x75 };

constexpr uint8_t kGyroConfigValue = 0x08;  // +-500 deg/s
constexpr float kLsbPerG = 16384.0f;        // +-2 g full scale (power-on default)
constexpr int32_t kSameTempLsb = 1000;      // ~3 deg C (6500: 334 LSB/C, 6050: 340 LSB/C)
constexpr uint16_t kFailuresBeforeRecovery = 20;
constexpr uint32_t kRecoveryIntervalMs = 500;
constexpr uint32_t kSettingsCheckIntervalMs = 1000;

// Survives deep sleep (RTC slow memory): lets a wake-up skip calibration.
RTC_DATA_ATTR float gSavedBias[3] = {0.0f, 0.0f, 0.0f};
RTC_DATA_ATTR bool gSavedBiasValid = false;  // reset to false on power-on
RTC_DATA_ATTR bool gSavedBiasTrusted = false;
RTC_DATA_ATTR int16_t gSavedTempRaw = 0;

}  // namespace

bool Mpu6050GyroSensor::begin(int8_t sdaPin, int8_t sclPin, uint8_t address) {
  address_ = address;
  sdaPin_ = sdaPin;
  sclPin_ = sclPin;
  present_ = false;
  tracker_.restart(millis());
  if (sdaPin < 0 || sclPin < 0) return false;

  startBus();

  if (!readRegister(kRegWhoAmI, whoAmI_) || whoAmI_ == 0x00 || whoAmI_ == 0xFF) {
    Serial.println("[gyro] no MPU6050 on I2C");
    return false;
  }
  Serial.printf("[gyro] WHO_AM_I=0x%02X\n", whoAmI_);  // 0x68 genuine, 0x70/0x71 = 6500/9250

  writeRegister(kRegPwrMgmt1, 0x80);  // device reset
  delay(100);
  present_ = configure();
  delay(50);  // gyro start-up time
  return present_;
}

void Mpu6050GyroSensor::startBus() {
  Wire.begin(static_cast<int>(sdaPin_), static_cast<int>(sclPin_));
  Wire.setClock(400000);
  Wire.setTimeOut(5);  // a read takes ~0.4 ms: a stuck bus must not stall every tick 50 ms
}

bool Mpu6050GyroSensor::configure() {
  bool ok = writeRegister(kRegPwrMgmt1, 0x01)              // wake, clock = gyro X PLL
            && writeRegister(kRegPwrMgmt2, 0x00)           // accelerometer on: gives "up"
            && writeRegister(kRegConfig, 0x03)             // gyro low-pass 41 Hz, 5.9 ms delay
            && writeRegister(kRegSampleRateDiv, 0)         // 1 kHz: every tick gets new data
            && writeRegister(kRegGyroConfig, kGyroConfigValue);
  if (ok && (whoAmI_ == 0x70 || whoAmI_ == 0x71)) {
    ok = writeRegister(kRegAccelConfig2, 0x04);  // accel low-pass ~20 Hz (6500 has its own)
  }
  return ok;
}

bool Mpu6050GyroSensor::calibrateBias(uint8_t maxAttempts, bool reuseSaved) {
  if (!present_) return false;
  if (reuseSaved && gSavedBiasValid) {
    // Trust the saved offset only if the chip is about as warm as when it was saved.
    int16_t gyro[3];
    const bool sameTemp = readRaw(gyro) && abs(tempRaw_ - gSavedTempRaw) < kSameTempLsb;
    tracker_.set(gSavedBias, gSavedBiasTrusted && sameTemp);
    Serial.printf("[gyro] bias restored from before sleep%s\n",
                  tracker_.trusted() ? "" : " (not trusted: it is re-learnt when the remote is still)");
    return true;
  }

  float best[3] = {0.0f, 0.0f, 0.0f};
  int32_t bestSpread = INT32_MAX;
  for (uint8_t attempt = 0; attempt < maxAttempts; ++attempt) {
    ImuSampleWindow w;
    w.reset();
    for (uint16_t n = 0; n < cfg::kGyroCalibrationSamples; ++n) {
      int16_t gyro[3];
      if (readRaw(gyro)) w.add(gyro, accelRaw_);
      delay(5);  // 150 samples spread over 0.75 s
    }
    if (w.count == 0) return false;  // I2C dead
    const int32_t spread = w.gyroSpread();
    if (spread < bestSpread) {  // keep the calmest attempt as the best guess
      bestSpread = spread;
      for (uint8_t i = 0; i < 3; ++i) best[i] = w.gyroMean(i);
    }
    // Gravity must not have moved either: a smooth slow turn would otherwise be trusted.
    if (spread <= gyro_bias::kStillSpreadLsb && w.accelSpread() <= gyro_bias::kAccelSteadyLsb) {
      tracker_.set(best, true);
      const float g = sqrtf(accel_[0] * accel_[0] + accel_[1] * accel_[1] + accel_[2] * accel_[2]);
      Serial.printf("[gyro] bias %.1f %.1f %.1f LSB, |a| at rest %.2f g\n", best[0], best[1], best[2], g);
      return true;
    }
    Serial.println("[gyro] remote moved during calibration, retrying");
  }
  tracker_.set(best, false);  // first still window at runtime may replace it outright
  Serial.println("[gyro] calibration never saw the remote still (or I2C errors): offset re-learnt when it lies still");
  return false;
}

bool Mpu6050GyroSensor::readRatesDps(float out[3], bool trackBias) {
  if (!present_) return false;
  const uint32_t now = millis();
  int16_t gyro[3];
  if (!readRaw(gyro)) {
    // Loose wire or bus glitch: once it answers again, give it its settings back.
    if (++failedReads_ >= kFailuresBeforeRecovery && now - lastRecoveryMs_ >= kRecoveryIntervalMs) {
      lastRecoveryMs_ = now;
      Wire.end();  // a stuck bus / driver state is only cleared by a fresh start
      startBus();
      if (configure()) {
        failedReads_ = 0;
        tracker_.distrust();
        Serial.println("[gyro] sensor answers again, re-configured");
      }
    }
    return false;
  }
  failedReads_ = 0;
  const float* bias = tracker_.bias();
  for (uint8_t i = 0; i < 3; ++i) out[i] = (gyro[i] - bias[i]) / gyro_bias::kLsbPerDps;

  if (!trackBias) {
    tracker_.restart(now);  // cursor is being moved: these samples say nothing about the offset
    return true;
  }
  tracker_.add(gyro, accelRaw_, now);
  checkSettings(now);
  return true;
}

// A supply dip can reset the sensor to +-250 deg/s and no low-pass: reads keep working but
// the cursor would run at double speed. Checked once a second while the cursor is idle.
void Mpu6050GyroSensor::checkSettings(uint32_t now) {
  if (now - lastSettingsCheckMs_ < kSettingsCheckIntervalMs) return;
  lastSettingsCheckMs_ = now;
  uint8_t gyroConfig = 0;
  if (!readRegister(kRegGyroConfig, gyroConfig) || gyroConfig == kGyroConfigValue) return;
  Serial.println("[gyro] sensor lost its settings (reset?), re-configuring");
  if (configure()) tracker_.distrust();
}

void Mpu6050GyroSensor::sleep() {
  if (!present_) return;
  const float* bias = tracker_.bias();
  for (uint8_t i = 0; i < 3; ++i) gSavedBias[i] = bias[i];
  gSavedBiasValid = true;
  gSavedBiasTrusted = tracker_.trusted();
  gSavedTempRaw = tempRaw_;
  writeRegister(kRegPwrMgmt1, 0x40);
}

bool Mpu6050GyroSensor::writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address_);
  Wire.write(reg), Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool Mpu6050GyroSensor::readRegister(uint8_t reg, uint8_t& value) {
  Wire.beginTransmission(address_);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(static_cast<int>(address_), 1) != 1) return false;
  value = Wire.read();
  return true;
}

bool Mpu6050GyroSensor::readRaw(int16_t gyro[3]) {
  Wire.beginTransmission(address_);
  Wire.write(kRegAccelXoutH);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(static_cast<int>(address_), 14) != 14) return false;
  int16_t v[7];
  for (uint8_t i = 0; i < 7; ++i) {
    const uint8_t hi = Wire.read();
    const uint8_t lo = Wire.read();
    v[i] = static_cast<int16_t>((hi << 8) | lo);  // big-endian registers
  }
  if (!sanity_.plausible(v, millis())) return false;  // garbled by a flaky I2C link
  for (uint8_t i = 0; i < 3; ++i) {
    accelRaw_[i] = v[i];
    accel_[i] = v[i] / kLsbPerG;
    gyro[i] = v[4 + i];
  }
  tempRaw_ = v[3];
  return true;
}
