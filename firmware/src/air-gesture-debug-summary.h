// One serial line per AIR gesture, printed on release: how far the sensor turned about each
// of its own axes, and how much of that became horizontal / vertical cursor motion. A nod in
// the log shows at once whether the nose axis is right: it must turn the sensor about its
// left-right axis and come out as "vert", not vanish or come out as "horiz".
#pragma once

#include <Arduino.h>
#include <math.h>

#include "gravity-referenced-screen-axes.h"

class AirGestureDebugSummary {
 public:
  void add(const float rates[3], const ScreenRates& screen, float dtSec) {
    for (int i = 0; i < 3; ++i) sensorTurnDeg_[i] += fabsf(rates[i]) * dtSec;
    horizDeg_ += fabsf(screen.right) * dtSec;
    vertDeg_ += fabsf(screen.down) * dtSec;
    seconds_ += dtSec;
  }

  // `up` = low-passed gravity and `nose` = forward axis, both in sensor coordinates.
  void printAndReset(const float up[3], const float nose[3]) {
    if (seconds_ > 0.1f) {
      Serial.printf("[air] %.1fs up(%+.2f %+.2f %+.2f) nose(%+.0f %+.0f %+.0f) | turned X%.0f Y%.0f Z%.0f deg"
                    " -> horiz %.0f vert %.0f deg\n",
                    seconds_, up[0], up[1], up[2], nose[0], nose[1], nose[2], sensorTurnDeg_[0],
                    sensorTurnDeg_[1], sensorTurnDeg_[2], horizDeg_, vertDeg_);
    }
    *this = AirGestureDebugSummary();
  }

 private:
  float sensorTurnDeg_[3] = {0.0f, 0.0f, 0.0f};
  float horizDeg_ = 0.0f;
  float vertDeg_ = 0.0f;
  float seconds_ = 0.0f;
};
