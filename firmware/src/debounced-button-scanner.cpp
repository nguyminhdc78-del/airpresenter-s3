#include "debounced-button-scanner.h"

#include <Arduino.h>

#include "remote-user-config.h"

void DebouncedButtonScanner::begin(const uint8_t* pins, uint8_t count) {
  pins_ = pins;
  count_ = count > kMaxButtons ? kMaxButtons : count;
  for (uint8_t i = 0; i < count_; ++i) {
    pinMode(pins_[i], INPUT_PULLUP);
  }
  delay(5);  // let the pull-ups settle before the first read
  const uint32_t now = millis();
  for (uint8_t i = 0; i < count_; ++i) {
    // A button already held at boot (e.g. the one that woke us from deep sleep)
    // starts as "pressed" so it does not fire an action.
    const bool pressed = digitalRead(pins_[i]) == LOW;
    states_[i] = State{pressed, pressed, false, false, now};
  }
}

void DebouncedButtonScanner::scan(uint32_t now) {
  anyEdge_ = false;
  for (uint8_t i = 0; i < count_; ++i) {
    State& s = states_[i];
    s.pressedEdge = false;
    s.releasedEdge = false;

    const bool raw = digitalRead(pins_[i]) == LOW;
    if (raw != s.lastRaw) {
      // Contact is still bouncing: restart the stability timer.
      s.lastRaw = raw;
      s.lastChangeMs = now;
    } else if (raw != s.stable && now - s.lastChangeMs >= cfg::kDebounceMs) {
      s.stable = raw;
      if (raw) {
        s.pressedEdge = true;
      } else {
        s.releasedEdge = true;
      }
      anyEdge_ = true;
    }
  }
}
