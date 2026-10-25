#include "status-led-indicator.h"

#include <Arduino.h>

namespace {
constexpr uint32_t kFlashPhaseMs = 150;
}  // namespace

void StatusLedIndicator::begin(int8_t pin, bool isRgb, uint8_t brightness) {
  pin_ = pin;
  isRgb_ = isRgb;
  level_ = brightness;
  if (pin_ < 0) return;
  if (!isRgb_) pinMode(pin_, OUTPUT);
  off();
}

void StatusLedIndicator::showCalibrating() { setColor(level_, level_, 0); }

void StatusLedIndicator::showNoseSetup(uint32_t now) {
  if (now % 250 < 125) {
    setColor(0, level_, level_);
  } else {
    off();
  }
}

void StatusLedIndicator::startResultFlash(bool ok, uint32_t now) {
  resultOk_ = ok;
  resultStartMs_ = now;
  resultShowing_ = true;
}

bool StatusLedIndicator::drawResultFlash(uint32_t now) {
  if (!resultShowing_) return false;
  const uint32_t elapsed = now - resultStartMs_;
  if (elapsed >= 4 * kFlashPhaseMs) {  // on, off, on, off
    resultShowing_ = false;
    return false;
  }
  if ((elapsed / kFlashPhaseMs) % 2 == 0) {
    setColor(resultOk_ ? 0 : level_, resultOk_ ? level_ : 0, 0);
  } else {
    off();
  }
  return true;
}

void StatusLedIndicator::flash(uint8_t r, uint8_t g, uint8_t b, uint8_t times) {
  for (uint8_t i = 0; i < times; ++i) {
    setColor(r, g, b);
    delay(kFlashPhaseMs);
    off();
    delay(kFlashPhaseMs);
  }
}

void StatusLedIndicator::update(uint32_t now, bool connected, bool lowBattery, bool fnLatched) {
  if (connected && !wasConnected_) connectedAtMs_ = now;
  wasConnected_ = connected;

  if (fnLatched) {
    setColor(level_, 0, level_);
  } else if (!connected) {
    if (now % 1000 < 100) {
      setColor(0, 0, level_);
    } else {
      off();
    }
  } else if (now - connectedAtMs_ < 1500) {
    setColor(0, level_, 0);
  } else if (lowBattery && now % 4000 < 150) {
    setColor(level_, 0, 0);
  } else {
    off();
  }
}

void StatusLedIndicator::setColor(uint8_t r, uint8_t g, uint8_t b) {
  if (pin_ < 0) return;
  const uint32_t color = (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
  if (color == lastColor_) return;  // the RGB LED write is slow; skip repeats
  lastColor_ = color;

  if (!isRgb_) {
    digitalWrite(pin_, color != 0 ? HIGH : LOW);
    return;
  }
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  rgbLedWrite(pin_, r, g, b);
#else
  neopixelWrite(pin_, r, g, b);
#endif
}
