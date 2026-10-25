// AirPresenter S3 - ESP32-S3 Bluetooth presenter remote + gyro air mouse.
// Shows up on the PC as a BLE keyboard + mouse + media keys (no dongle, no driver).
//
// Loop: scan buttons -> dispatch actions -> every motion tick (half the BLE connection
// interval: 3.75 or 7.5 ms) mix gyro + D-pad motion
// into a mouse report -> battery / LED / sleep housekeeping.
#include <Arduino.h>

#include "action-dispatcher.h"
#include "air-gesture-debug-summary.h"
#include "air-nose-axis-setup.h"
#include "battery-monitor.h"
#include "ble-hid-composite-device.h"
#include "cursor-motion-accumulator.h"
#include "debounced-button-scanner.h"
#include "gravity-referenced-screen-axes.h"
#include "gyro-air-mouse-motion-mapper.h"
#include "low-pass-filter-weight.h"
#include "mpu6050-gyro-sensor.h"
#include "power-sleep-manager.h"
#include "remote-action-types.h"
#include "remote-user-config.h"
#include "status-led-indicator.h"

namespace {

BleHidCompositeDevice bleHid;
DebouncedButtonScanner buttons;
ActionDispatcher dispatcher(bleHid, kButtonBindings, kButtonCount);
Mpu6050GyroSensor gyro;
GyroAirMouseMapper airMapper;
GravityReference gravity;
AirNoseAxisSetup noseAxis;
AirGestureDebugSummary gestureLog;
CursorMotionAccumulator cursorMotion;
BatteryMonitor battery;
StatusLedIndicator statusLed;
PowerSleepManager power;

uint8_t buttonPins[kMaxButtons];
bool gyroReady = false;
uint32_t nextMotionTickUs = 0;  // fixed tick grid: one late tick doesn't shift the next ones
uint32_t lastMotionTickUs = 0;  // when the previous tick really ran (its dt)
bool wasClutchHeld = false;     // AIR press edge: confirms the nose pose during nose setup
bool airLocked = false;         // the AIR press that confirmed a new nose must not move the cursor

// Gyro (while AIR is held) + D-pad -> one relative mouse report per tick.
void runMotionTick(uint32_t now, uint32_t nowUs) {
  float dtSec = (nowUs - lastMotionTickUs) / 1e6f;  // real elapsed time, not the nominal tick
  if (dtSec > 0.05f) dtSec = 0.05f;  // first tick / long stall: avoid a jump
  lastMotionTickUs = nowUs;

  float dx = 0.0f;
  float dy = 0.0f;
  dispatcher.addNudgeMotion(now, dtSec, dx, dy);

  const bool clutchHeld = dispatcher.isAirClutchHeld();
  const bool airPressed = clutchHeld && !wasClutchHeld;
  if (wasClutchHeld && !clutchHeld) {
    gestureLog.printAndReset(gravity.up(), noseAxis.forward());
    airLocked = false;
  }
  wasClutchHeld = clutchHeld;
  // While teaching the nose, AIR only confirms the pose: the cursor stays put.
  const bool airMoves = clutchHeld && !noseAxis.active() && !airLocked;
  // A click jolts the remote: the filter keeps running but the cursor holds still briefly.
  const bool clickSettling = now - dispatcher.lastMouseButtonEdgeMs() < cfg::kClickSettleMs;
  float rates[3];
  // Read even when the clutch is released: lets the bias follow temperature drift.
  if (gyroReady && gyro.readRatesDps(rates, !clutchHeld)) {
    gravity.rotate(rates, dtSec);
    gravity.addAccel(gyro.accelG(), lowPassWeight(dtSec, cfg::kGravityTauSec));
    const ScreenRates screen = gravity.toScreen(rates, noseAxis.forward());
    float airX = 0.0f;
    float airY = 0.0f;
    airMapper.update(screen.right, screen.down, airMoves, dtSec, airX, airY);
    if (!clickSettling) {
      dx += airX;
      dy += airY;
    }
    if (airMoves) gestureLog.add(rates, screen, dtSec);
  } else {
    airMapper.update(0.0f, 0.0f, false, dtSec, dx, dy);  // no read: no stale motion on the next press
  }
  if (noseAxis.active()) {
    using Result = NoseUpPointingAxisDetector::Result;
    const Result result = noseAxis.tick(gravity.up(), airPressed, dtSec);
    if (result != Result::Waiting) airLocked = true;  // the confirming press never moves the cursor
    if (result != Result::Waiting) statusLed.startResultFlash(result == Result::Found && noseAxis.saved(), now);
  }

  int8_t moveX = 0;
  int8_t moveY = 0;
  if (cursorMotion.take(dx, dy, moveX, moveY) && bleHid.isConnected()) {
    bleHid.mouseMove(moveX, moveY);
    power.notifyActivity(now);  // only real use counts: a remote jiggling in a bag must sleep
  }
}

void updateStatusLed(uint32_t now) {
  if (statusLed.drawResultFlash(now)) return;  // nose setup result owns the LED for ~0.6 s
  if (noseAxis.active() && !dispatcher.isFnLatched()) {
    statusLed.showNoseSetup(now);
  } else if (gyroReady && dispatcher.isFnHeldAloneFor(now, cfg::kNoseSetupHoldMs)) {
    statusLed.showNoseSetupArmed();
  } else {
    statusLed.update(now, bleHid.isConnected(), battery.isLow(), dispatcher.isFnLatched());
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  setCpuFrequencyMhz(cfg::kCpuFreqMhz);
  PowerSleepManager::logWakeReason();

  statusLed.begin(cfg::kPinStatusLed, cfg::kStatusLedIsRgb, cfg::kStatusLedBrightness);

  const uint8_t count = kButtonCount < kMaxButtons ? kButtonCount : kMaxButtons;
  for (uint8_t i = 0; i < count; ++i) buttonPins[i] = kButtonBindings[i].pin;
  power.begin(buttonPins, count);  // first: releases wake-up pins from the RTC domain
  buttons.begin(buttonPins, count);

  // BLE first: the PC can reconnect while the gyro calibrates.
  bleHid.begin(cfg::kDeviceName);

  statusLed.showCalibrating();
  gyroReady = gyro.begin(cfg::kPinI2cSda, cfg::kPinI2cScl, cfg::kMpuI2cAddress);
  if (gyroReady) gyro.calibrateBias(cfg::kGyroCalibrationAttempts, PowerSleepManager::wokeFromDeepSleep());
  Serial.printf("[gyro] %s\n", gyroReady ? "ready - hold AIR to move the cursor" : "absent - D-pad only");

  noseAxis.begin();

  battery.begin(cfg::kPinBatteryAdc, cfg::kBatteryDividerRatio);
  bleHid.setBatteryLevel(battery.percent());
  power.notifyActivity(millis());
}

void loop() {
  const uint32_t now = millis();

  bleHid.service(now);
  buttons.scan(now);
  if (buttons.anyEdge()) power.notifyActivity(now);
  dispatcher.update(buttons, now);

  if (dispatcher.clearBondsRequested()) {
    statusLed.flashPairingsCleared();
    bleHid.forgetAllHosts();
    // Restarting with BOOT (GPIO0) still held could boot into download mode.
    power.waitForButtonsReleased(10000);
    ESP.restart();
  }

  if (dispatcher.takeNoseSetupRequest() && gyroReady) noseAxis.start(gravity.up());  // not the FN jolt

  const uint32_t halfIntervalUs = bleHid.connectionIntervalUs() / 2;  // two reports per event
  const uint32_t tickUs = halfIntervalUs ? constrain(halfIntervalUs, cfg::kMinMotionTickUs, cfg::kMotionTickUs)
                                         : cfg::kMotionTickUs;
  const uint32_t nowUs = micros();
  if (static_cast<int32_t>(nowUs - nextMotionTickUs) >= 0) {
    nextMotionTickUs += tickUs;
    // Fell behind (first tick, LED flash, long BLE call): restart the grid from now.
    if (static_cast<int32_t>(nowUs - nextMotionTickUs) >= 0) nextMotionTickUs = nowUs + tickUs;
    runMotionTick(now, nowUs);
  }

  if (battery.update(now)) bleHid.setBatteryLevel(battery.percent());
  updateStatusLed(now);

  if (power.shouldSleep(now, bleHid.isConnected())) {
    if (power.userCameBack()) {
      power.notifyActivity(millis());  // pressed right at the timeout: stay awake
    } else {
      statusLed.off();
      if (gyroReady) gyro.sleep();
      power.enterDeepSleep();  // does not return
    }
  }

  // Sleep in whole RTOS ticks (1 ms) only while the next motion tick is far; close to it,
  // wait the exact microseconds. Ticks rounded to 1 ms made 7.5 ms connection events
  // alternate 1 and 3 reports in some phases (a judder).
  const int32_t untilTickUs = static_cast<int32_t>(nextMotionTickUs - micros());
  if (untilTickUs > 1500) {
    delay(1);
  } else if (untilTickUs > 0) {
    delayMicroseconds(untilTickUs);
  }
}
