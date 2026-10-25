// Turns debounced button edges into HID actions using the keymap (normal / FN layer),
// including auto-repeat, mouse drag, D-pad cursor nudging and hold-to-trigger actions.
#pragma once

#include <stdint.h>

#include "ble-hid-composite-device.h"
#include "debounced-button-scanner.h"
#include "remote-action-types.h"

class ActionDispatcher {
 public:
  ActionDispatcher(BleHidCompositeDevice& hid, const ButtonBinding* bindings, uint8_t count);

  // Process press/release edges and held buttons. Call after buttons.scan().
  void update(const DebouncedButtonScanner& buttons, uint32_t now);

  // Adds this tick's D-pad cursor movement (in mouse counts) to dx/dy.
  void addNudgeMotion(uint32_t now, float dtSec, float& dx, float& dy) const;

  bool isAirClutchHeld() const { return isActive(ActionType::AirClutch); }
  bool isFnLatched() const { return fnLatched_; }
  // When a mouse button last went down or up (the click jolts the remote).
  uint32_t lastMouseButtonEdgeMs() const { return mouseButtonEdgeMs_; }
  bool clearBondsRequested() const { return clearBondsRequested_; }

  // FN held alone for at least `ms` (LED hint: releasing now starts the nose setup).
  bool isFnHeldAloneFor(uint32_t now, uint32_t ms) const;
  // True once after FN was held alone for cfg::kNoseSetupHoldMs, then released.
  bool takeNoseSetupRequest() {
    const bool requested = noseSetupRequested_;
    noseSetupRequested_ = false;
    return requested;
  }

 private:
  void onPress(uint8_t i, uint32_t now);
  void onRelease(uint8_t i, uint32_t now);
  void onHold(uint8_t i, uint32_t now);
  void fireTap(const Action& action);
  bool isActive(ActionType type) const;

  BleHidCompositeDevice& hid_;
  const ButtonBinding* bindings_;
  uint8_t count_;
  // Action chosen when each button was pressed. Releasing uses the same action even
  // if FN was let go in between, so keys never get stuck.
  Action active_[kMaxButtons];
  uint32_t pressedAtMs_[kMaxButtons] = {};
  uint32_t nextRepeatMs_[kMaxButtons] = {};
  bool clearBondsRequested_ = false;
  bool noseSetupRequested_ = false;

  // FN state for one-hand use: tap FN, then press the button within the timeout.
  bool fnLatched_ = false;
  uint32_t fnLatchedAtMs_ = 0;
  bool fnUsedWhileHeld_ = false;        // another button was pressed while FN was down
  bool fnPressCancelledLatch_ = false;  // this FN press only cancelled an armed latch
  bool fnJoinedOtherButton_ = false;    // FN went down while another button was held
  uint32_t mouseButtonEdgeMs_ = 0;
};
