// User-tunable settings for the AirPresenter remote.
// Everything a builder may need to change (pins, gyro orientation, speeds,
// timeouts) lives here. Button pins + key bindings live in keymap-presenter-default.cpp.
#pragma once

#include <stdint.h>

namespace cfg {

// ---------------- BLE identity ----------------
// Keep the name <= 18 chars: it must fit in the 31-byte advertising packet.
constexpr const char* kDeviceName = "AirPresenter S3";
constexpr const char* kManufacturer = "DIY";
constexpr int8_t kBleTxPowerDbm = 9;  // +9 dBm: ~10-20 m indoors
// true: ask the PC for a 7.5 ms connection interval (133 reports/s, smoothest; falls back to
// 15 ms if refused). false: 15 ms (66.7 reports/s). Off while a laptop freeze is watched:
// its Qualcomm QCA9377 WiFi/BT combo froze once ~2 h after 7.5 ms was enabled.
constexpr bool kBleFastInterval = false;

// ---------------- Pins (ESP32-S3 SuperMini defaults) ----------------
constexpr int8_t kPinI2cSda = 12;      // MPU6050 SDA
constexpr int8_t kPinI2cScl = 13;      // MPU6050 SCL
constexpr int8_t kPinBatteryAdc = 1;   // ADC1 pin, -1 = no battery sensing
constexpr float kBatteryDividerRatio = 2.0f;  // 100k/100k divider => x2
constexpr int8_t kPinStatusLed = 48;   // onboard LED
constexpr bool kStatusLedIsRgb = true; // WS2812 RGB; false = plain LED
constexpr uint8_t kStatusLedBrightness = 24;  // 0-255, low saves battery

// ---------------- Gyro air mouse ----------------
constexpr uint8_t kMpuI2cAddress = 0x68;  // 0x69 if AD0 pulled high
// Which arrow printed on the GY-521 points at the screen: 0 = X, 1 = Y, 2 = Z,
// sign -1 if it points back at you. Up/down comes from gravity, so chip up or chip
// down and a rolled wrist need no setting. Only the default: teaching the nose
// (hold FN 2 s, release, point the nose at the ceiling, press AIR) overrides it and is saved.
constexpr uint8_t kAirForwardAxis = 0;
constexpr int8_t kAirForwardSign = 1;
constexpr float kGravityTauSec = 1.0f;      // accelerometer pull on "up" (the gyro turns it meanwhile)
constexpr float kAirSensitivity = 6.0f;     // counts per degree at slow speed
constexpr float kAirAccelKneeDps = 40.0f;   // lower = stronger acceleration
constexpr float kAirDeadzoneDps = 1.5f;     // ignore hand tremor below this
// Smoothing follows the turn speed: slow turns (precise aiming, hand tremor) are steadied,
// fast ones get little lag. Time constants in seconds: higher = smoother but laggier.
constexpr float kAirSlowSmoothingSec = 0.06f;  // at or below kAirTierSlowDps (~2.7 Hz cutoff)
constexpr float kAirSmoothingSec = 0.008f;     // at or above kAirTierFastDps (~20 Hz cutoff)
constexpr float kAirTierSlowDps = 4.0f;
constexpr float kAirTierFastDps = 60.0f;
static_assert(kAirTierSlowDps < kAirTierFastDps, "tiers must not overlap");
constexpr uint16_t kGyroCalibrationSamples = 150;  // x 5 ms = 0.75 s per attempt
constexpr uint8_t kGyroCalibrationAttempts = 4;    // retries while the remote moves

// ---------------- Buttons & cursor ----------------
constexpr uint16_t kDebounceMs = 15;
constexpr uint16_t kRepeatDelayMs = 400;     // hold time before auto-repeat
constexpr uint16_t kRepeatIntervalMs = 90;   // auto-repeat period
// Hold FN alone (or BOOT) to forget paired PCs. Far from the 2 s nose hold: at 5 s a user
// teaching the nose wiped the pairing by accident and the PC then looped connect/disconnect.
constexpr uint16_t kClearBondsHoldMs = 10000;
constexpr uint16_t kNoseSetupHoldMs = 2000;  // hold FN alone, release before 10 s: teach the nose
static_assert(kNoseSetupHoldMs < kClearBondsHoldMs, "nose setup must be reachable before clear-bonds");
constexpr uint16_t kClickSettleMs = 100;     // AIR held: cursor holds still this long after a click edge
constexpr uint16_t kFnTapMaxMs = 500;        // FN released sooner = "tap": arms FN for one press
constexpr uint16_t kFnLatchTimeoutMs = 3000; // armed FN expires if no button follows
constexpr float kNudgeMinCountsPerSec = 150.0f;   // D-pad start speed
constexpr float kNudgeMaxCountsPerSec = 1500.0f;  // D-pad top speed
constexpr uint16_t kNudgeRampMs = 1000;      // time to reach top speed
// 7.5 ms = two reports per 15 ms BLE connection event. At exactly the event rate the
// two clocks drift: some events carry no report (30 ms freeze) and the next carries two
// (double jump). Measured on the PC: 17% of reports arrived bunched that way.
// The tick follows the link: half the BLE connection interval (two reports per event),
// so 3.75 ms at 7.5 ms and 7.5 ms at 15 ms; 7.5 ms while no PC is connected.
constexpr uint32_t kMotionTickUs = 7500;     // longest tick, and the default
constexpr uint32_t kMinMotionTickUs = 3750;  // shortest tick

// ---------------- Power ----------------
constexpr uint32_t kCpuFreqMhz = 80;  // BLE works at 80 MHz, saves ~20 mA
constexpr uint32_t kIdleSleepTimeoutMs = 30UL * 60UL * 1000UL;        // connected, no use (long Q&A)
constexpr uint32_t kUnconnectedSleepTimeoutMs = 3UL * 60UL * 1000UL;  // no PC found
constexpr uint8_t kLowBatteryPercent = 15;
constexpr uint32_t kBatterySampleIntervalMs = 10000;

}  // namespace cfg
