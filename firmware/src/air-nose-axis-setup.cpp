#include "air-nose-axis-setup.h"

#include <Arduino.h>

#include "air-nose-axis-nvs-storage.h"
#include "remote-user-config.h"

static_assert(cfg::kAirForwardAxis < 3, "kAirForwardAxis must be 0 (X), 1 (Y) or 2 (Z)");

void AirNoseAxisSetup::begin() {
  uint8_t axis = cfg::kAirForwardAxis;
  int8_t sign = cfg::kAirForwardSign;
  const bool taught = loadSavedNoseAxis(axis, sign);
  set(axis, sign);
  log(taught ? "taught" : "default - teach it: hold FN 2 s, release, nose up, press AIR");
}

void AirNoseAxisSetup::start(const float up[3]) {
  detector_.start(up);
  Serial.println("[air] nose setup: point the nose at the ceiling, then press AIR");
}

NoseUpPointingAxisDetector::Result AirNoseAxisSetup::tick(const float up[3], bool airPressed, float dtSec) {
  using Result = NoseUpPointingAxisDetector::Result;
  const Result result = detector_.update(up, airPressed, dtSec);
  if (result == Result::Found) {
    const uint8_t axis = detector_.axis();
    const int8_t sign = detector_.sign();
    // Same axis, other sign: most likely the nose pointed at the floor (gravity can't tell).
    const bool flipped = forward_[axis] == -static_cast<float>(sign);
    set(axis, sign);
    saved_ = saveNoseAxis(axis, sign);
    log(saved_ ? "taught, saved" : "taught, SAVE FAILED: lost at power-off");
    if (flipped) Serial.println("[air] nose reversed: if up/down is now inverted, teach again pointing at the ceiling");
  } else if (result == Result::Rejected) {
    Serial.println("[air] nose setup: nose was not pointing up, nothing changed");
  } else if (result == Result::TimedOut) {
    Serial.println("[air] nose setup timed out, nothing changed");
  }
  return result;
}

void AirNoseAxisSetup::set(uint8_t axis, int8_t sign) {
  for (uint8_t i = 0; i < 3; ++i) forward_[i] = i == axis ? static_cast<float>(sign) : 0.0f;
}

void AirNoseAxisSetup::log(const char* source) const {
  for (uint8_t i = 0; i < 3; ++i) {
    if (forward_[i] != 0.0f) Serial.printf("[air] nose = %c%c (%s)\n", forward_[i] < 0 ? '-' : '+', "XYZ"[i], source);
  }
}
