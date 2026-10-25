// Collects fractional cursor movement (from gyro + D-pad) and hands out whole
// mouse counts that fit an 8-bit HID report, carrying the remainder to the next tick.
#pragma once

#include <math.h>
#include <stdint.h>

class CursorMotionAccumulator {
 public:
  // Returns true when there is a non-zero movement to send.
  bool take(float dx, float dy, int8_t& outX, int8_t& outY) {
    remainderX_ += dx;
    remainderY_ += dy;
    float wholeX = truncf(remainderX_);
    float wholeY = truncf(remainderY_);
    remainderX_ -= wholeX;  // keep only the fractions
    remainderY_ -= wholeY;
    // Movement beyond one report is dropped, not queued: the cursor must stop the moment
    // the hand stops. Both axes shrink alike, so a fast diagonal keeps its direction.
    if (fabsf(wholeX) > 127.0f || fabsf(wholeY) > 127.0f) {
      if (fabsf(wholeX) >= fabsf(wholeY)) {
        wholeY = truncf(wholeY * 127.0f / fabsf(wholeX));
        wholeX = copysignf(127.0f, wholeX);
      } else {
        wholeX = truncf(wholeX * 127.0f / fabsf(wholeY));
        wholeY = copysignf(127.0f, wholeY);
      }
    }
    outX = static_cast<int8_t>(wholeX);
    outY = static_cast<int8_t>(wholeY);
    return outX != 0 || outY != 0;
  }

 private:
  float remainderX_ = 0.0f;
  float remainderY_ = 0.0f;
};
