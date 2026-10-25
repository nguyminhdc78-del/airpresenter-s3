#include "action-dispatcher.h"

#include <Arduino.h>

#include "remote-user-config.h"

ActionDispatcher::ActionDispatcher(BleHidCompositeDevice& hid, const ButtonBinding* bindings,
                                   uint8_t count)
    : hid_(hid), bindings_(bindings), count_(count > kMaxButtons ? kMaxButtons : count) {
  for (Action& a : active_) a = act::none();
}

void ActionDispatcher::update(const DebouncedButtonScanner& buttons, uint32_t now) {
  if (fnLatched_ && now - fnLatchedAtMs_ >= cfg::kFnLatchTimeoutMs) fnLatched_ = false;

  const uint8_t n = buttons.count() < count_ ? buttons.count() : count_;
  // Pass 0 handles FN buttons, pass 1 the rest: FN + NEXT landing in the same scan
  // must still pick NEXT's FN action.
  for (uint8_t pass = 0; pass < 2; ++pass) {
    for (uint8_t i = 0; i < n; ++i) {
      const bool isFnButton = bindings_[i].normal.type == ActionType::FnLayer;
      if (isFnButton != (pass == 0)) continue;
      if (buttons.wasPressed(i)) {
        onPress(i, now);
      } else if (buttons.wasReleased(i)) {
        onRelease(i, now);
      } else if (buttons.isHeld(i)) {
        onHold(i, now);
      }
    }
  }
}

void ActionDispatcher::onPress(uint8_t i, uint32_t now) {
  const ButtonBinding& b = bindings_[i];
  const bool isFnButton = b.normal.type == ActionType::FnLayer;
  const bool fnHeld = isActive(ActionType::FnLayer);
  // FN works two ways: held together with a button, or tapped first (one-hand use).
  const bool useFn = !isFnButton && (fnHeld || fnLatched_) && b.withFn.type != ActionType::None;
  if (isFnButton) {
    fnPressCancelledLatch_ = fnLatched_;  // a second FN tap cancels the latch
    fnUsedWhileHeld_ = false;
    // A thumb resting on FN while aiming with AIR (or dragging) is not "FN alone": it must
    // not start the nose setup or wipe the pairings.
    fnJoinedOtherButton_ = false;
    for (uint8_t j = 0; j < count_; ++j) {
      if (j != i && active_[j].type != ActionType::None) fnJoinedOtherButton_ = true;
    }
  } else if (fnHeld) {
    fnUsedWhileHeld_ = true;
  }
  fnLatched_ = false;  // a latch covers exactly one button press
  const Action& a = useFn ? b.withFn : b.normal;

  active_[i] = a;
  pressedAtMs_[i] = now;
  nextRepeatMs_[i] = now + cfg::kRepeatDelayMs;
  Serial.printf("[btn] %s%s\n", useFn ? "FN+" : "", b.label);

  if (a.type == ActionType::MouseButton) {
    mouseButtonEdgeMs_ = now;
    hid_.mouseButtonDown(static_cast<uint8_t>(a.code));  // stays down until release: drag
  } else {
    fireTap(a);  // key / media / wheel; state-only actions are ignored there
  }
}

void ActionDispatcher::onRelease(uint8_t i, uint32_t now) {
  const Action& a = active_[i];
  if (a.type == ActionType::MouseButton) {
    mouseButtonEdgeMs_ = now;
    hid_.mouseButtonUp(static_cast<uint8_t>(a.code));
  } else if (a.type == ActionType::FnLayer) {
    const uint32_t heldMs = now - pressedAtMs_[i];
    // A short FN tap with nothing else pressed arms FN for the next button.
    if (heldMs < cfg::kFnTapMaxMs && !fnUsedWhileHeld_ && !fnPressCancelledLatch_) {
      fnLatched_ = true;
      fnLatchedAtMs_ = now;
    }
    // Held alone for a while, released before the clear-bonds hold: teach the nose.
    if (heldMs >= cfg::kNoseSetupHoldMs && !fnUsedWhileHeld_ && !fnJoinedOtherButton_) noseSetupRequested_ = true;
  }
  active_[i] = act::none();
}

void ActionDispatcher::onHold(uint8_t i, uint32_t now) {
  const Action& a = active_[i];
  if (a.repeat && static_cast<int32_t>(now - nextRepeatMs_[i]) >= 0) {
    fireTap(a);
    nextRepeatMs_[i] = now + cfg::kRepeatIntervalMs;
  }
  // Forget pairings: hold BOOT, or hold FN alone (reachable with the case closed).
  const bool clearsBonds =
      a.type == ActionType::ClearBonds ||
      (a.type == ActionType::FnLayer && !fnUsedWhileHeld_ && !fnJoinedOtherButton_);
  if (clearsBonds && now - pressedAtMs_[i] >= cfg::kClearBondsHoldMs) {
    clearBondsRequested_ = true;
  }
}

void ActionDispatcher::fireTap(const Action& a) {
  switch (a.type) {
    case ActionType::Key:
      hid_.keyTap(a.modifiers, static_cast<uint8_t>(a.code));
      break;
    case ActionType::Media:
      hid_.consumerTap(a.code);
      break;
    case ActionType::Wheel:
      hid_.mouseWheel(a.code == act::kUp ? 1 : -1);
      break;
    default:
      break;  // nudge / air / fn / clear-bonds are states, read elsewhere
  }
}

void ActionDispatcher::addNudgeMotion(uint32_t now, float dtSec, float& dx, float& dy) const {
  for (uint8_t i = 0; i < count_; ++i) {
    const Action& a = active_[i];
    if (a.type != ActionType::CursorNudge) continue;

    // Ease-in ramp: slow first for precise aiming, then speed up while held.
    float t = static_cast<float>(now - pressedAtMs_[i]) / cfg::kNudgeRampMs;
    if (t > 1.0f) t = 1.0f;
    const float speed =
        cfg::kNudgeMinCountsPerSec + (cfg::kNudgeMaxCountsPerSec - cfg::kNudgeMinCountsPerSec) * t * t;
    const float step = speed * dtSec;

    switch (a.code) {
      case act::kUp:    dy -= step; break;  // screen Y grows downwards
      case act::kDown:  dy += step; break;
      case act::kLeft:  dx -= step; break;
      case act::kRight: dx += step; break;
      default: break;
    }
  }
}

bool ActionDispatcher::isFnHeldAloneFor(uint32_t now, uint32_t ms) const {
  if (fnUsedWhileHeld_ || fnJoinedOtherButton_) return false;
  for (uint8_t i = 0; i < count_; ++i) {
    if (active_[i].type == ActionType::FnLayer && now - pressedAtMs_[i] >= ms) return true;
  }
  return false;
}

bool ActionDispatcher::isActive(ActionType type) const {
  for (uint8_t i = 0; i < count_; ++i) {
    if (active_[i].type == type) return true;
  }
  return false;
}
