// The remote's nose (pointing direction) in sensor coordinates: loaded from flash at boot,
// else the cfg default, and re-taught on request (FN held 2 s, released, nose pointed at
// the ceiling, AIR pressed; see nose-up-pointing-axis-detector.h).
#pragma once

#include <stdint.h>

#include "nose-up-pointing-axis-detector.h"

class AirNoseAxisSetup {
 public:
  void begin();  // load the saved nose (or the default) and log it
  const float* forward() const { return forward_; }

  void start(const float up[3]);  // up = current gravity estimate (remote held normally)
  bool active() const { return detector_.active(); }

  // One motion tick while active. Waiting until the setup ends; on Found the new nose is
  // already in use and saved (saved() tells whether the flash write worked).
  NoseUpPointingAxisDetector::Result tick(const float up[3], bool airPressed, float dtSec);
  bool saved() const { return saved_; }

 private:
  void set(uint8_t axis, int8_t sign);
  void log(const char* source) const;

  NoseUpPointingAxisDetector detector_;
  float forward_[3] = {1.0f, 0.0f, 0.0f};
  bool saved_ = false;
};
