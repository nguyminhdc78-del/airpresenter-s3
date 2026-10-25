// Reads active-low push buttons (GPIO -> button -> GND) with time-based debouncing
// and exposes press/release edges for the current scan.
#pragma once

#include <stdint.h>

#include "remote-action-types.h"

class DebouncedButtonScanner {
 public:
  void begin(const uint8_t* pins, uint8_t count);
  void scan(uint32_t now);  // call every loop iteration

  // Edge flags are valid until the next scan().
  bool wasPressed(uint8_t i) const { return states_[i].pressedEdge; }
  bool wasReleased(uint8_t i) const { return states_[i].releasedEdge; }
  bool isHeld(uint8_t i) const { return states_[i].stable; }
  bool anyEdge() const { return anyEdge_; }
  uint8_t count() const { return count_; }

 private:
  struct State {
    bool stable;        // debounced "pressed" state
    bool lastRaw;       // last raw reading
    bool pressedEdge;
    bool releasedEdge;
    uint32_t lastChangeMs;
  };

  const uint8_t* pins_ = nullptr;
  uint8_t count_ = 0;
  bool anyEdge_ = false;
  State states_[kMaxButtons] = {};
};
