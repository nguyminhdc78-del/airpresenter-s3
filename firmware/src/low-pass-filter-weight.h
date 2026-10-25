// Weight for one step of a one-pole low-pass (exponential moving average) given the real
// step length and the wanted time constant. With it a filter responds the same at any tick
// length: 7.5 ms or 3.75 ms ticks, a short catch-up tick, or a long tick after a stall.
#pragma once

#include <math.h>

inline float lowPassWeight(float dtSec, float tauSec) {
  if (tauSec <= 0.0f) return 1.0f;  // no smoothing
  if (dtSec <= 0.0f) return 0.0f;   // no time passed: no change
  return 1.0f - expf(-dtSec / tauSec);
}
