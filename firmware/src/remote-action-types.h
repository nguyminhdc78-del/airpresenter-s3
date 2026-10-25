// Data model for the keymap: what a button does (Action) and which GPIO it sits on
// (ButtonBinding). The actual table lives in keymap-presenter-default.cpp.
#pragma once

#include <stdint.h>

constexpr uint8_t kMaxButtons = 16;

enum class ActionType : uint8_t {
  None,
  Key,          // keyboard key (+ modifiers), tapped on press (+ optional auto-repeat)
  Media,        // media/consumer key, tapped on press (+ optional auto-repeat)
  MouseButton,  // mouse button, held while the button is held (allows drag)
  Wheel,        // scroll one notch on press (+ auto-repeat while held)
  CursorNudge,  // move the cursor while held, speeding up (D-pad)
  AirClutch,    // the gyro moves the cursor only while this is held
  FnLayer,      // while held, other buttons use their "withFn" action
  ClearBonds,   // hold for cfg::kClearBondsHoldMs to forget all paired PCs
};

struct Action {
  ActionType type;
  uint16_t code;      // key usage / media usage / mouse mask / act::Direction
  uint8_t modifiers;  // keyboard modifier bits (Key only)
  bool repeat;        // auto-repeat while held (Key, Media, Wheel)
};

struct ButtonBinding {
  uint8_t pin;        // GPIO, button wired between this pin and GND
  const char* label;  // shown in serial logs
  Action normal;
  Action withFn;      // used while FN is held; act::none() = fall back to normal
};

// Builders so the keymap table stays readable.
namespace act {

enum Direction : uint16_t { kUp, kDown, kLeft, kRight };

constexpr Action none() { return Action{ActionType::None, 0, 0, false}; }
constexpr Action key(uint8_t keycode, uint8_t modifiers = 0, bool repeat = false) {
  return Action{ActionType::Key, keycode, modifiers, repeat};
}
constexpr Action media(uint16_t usage, bool repeat = false) {
  return Action{ActionType::Media, usage, 0, repeat};
}
constexpr Action mouseButton(uint8_t mask) { return Action{ActionType::MouseButton, mask, 0, false}; }
constexpr Action wheel(Direction dir) { return Action{ActionType::Wheel, dir, 0, true}; }
constexpr Action nudge(Direction dir) { return Action{ActionType::CursorNudge, dir, 0, false}; }
constexpr Action airClutch() { return Action{ActionType::AirClutch, 0, 0, false}; }
constexpr Action fnLayer() { return Action{ActionType::FnLayer, 0, 0, false}; }
constexpr Action clearBonds() { return Action{ActionType::ClearBonds, 0, 0, false}; }

}  // namespace act

extern const ButtonBinding kButtonBindings[];
extern const uint8_t kButtonCount;
