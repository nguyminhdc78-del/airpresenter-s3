// HID report map for one composite BLE device: keyboard + mouse + consumer (media) keys,
// plus the HID usage codes the keymap refers to.
// NOTE: Windows caches this descriptor per pairing. After changing it, remove the
// device in Bluetooth settings and pair again.
#pragma once

#include <stdint.h>

namespace hid {

constexpr uint8_t kReportIdKeyboard = 1;
constexpr uint8_t kReportIdMouse = 2;
constexpr uint8_t kReportIdConsumer = 3;

// Keyboard modifier bits (first byte of the keyboard report).
constexpr uint8_t kModCtrl = 0x01;
constexpr uint8_t kModShift = 0x02;
constexpr uint8_t kModAlt = 0x04;
constexpr uint8_t kModGui = 0x08;  // Windows / Command key

// Keyboard usages (HID usage page 0x07).
constexpr uint8_t kKeyB = 0x05;
constexpr uint8_t kKeyL = 0x0F;
constexpr uint8_t kKeyW = 0x1A;
constexpr uint8_t kKeyEnter = 0x28;
constexpr uint8_t kKeyEsc = 0x29;
constexpr uint8_t kKeyTab = 0x2B;
constexpr uint8_t kKeySpace = 0x2C;
constexpr uint8_t kKeyF5 = 0x3E;
constexpr uint8_t kKeyHome = 0x4A;
constexpr uint8_t kKeyPageUp = 0x4B;
constexpr uint8_t kKeyEnd = 0x4D;
constexpr uint8_t kKeyPageDown = 0x4E;
constexpr uint8_t kKeyRight = 0x4F;
constexpr uint8_t kKeyLeft = 0x50;
constexpr uint8_t kKeyDown = 0x51;
constexpr uint8_t kKeyUp = 0x52;

// Consumer usages (HID usage page 0x0C).
constexpr uint16_t kMediaNext = 0x00B5;
constexpr uint16_t kMediaPrev = 0x00B6;
constexpr uint16_t kMediaStop = 0x00B7;
constexpr uint16_t kMediaPlayPause = 0x00CD;
constexpr uint16_t kMediaMute = 0x00E2;
constexpr uint16_t kMediaVolumeUp = 0x00E9;
constexpr uint16_t kMediaVolumeDown = 0x00EA;

// Mouse button bits.
constexpr uint8_t kMouseLeft = 0x01;
constexpr uint8_t kMouseRight = 0x02;
constexpr uint8_t kMouseMiddle = 0x04;

// Report layouts:
//  ID1 keyboard in : [modifiers][reserved][key1..key6]   (8 bytes)
//  ID1 keyboard out: [LED bits]                          (1 byte, host -> device)
//  ID2 mouse in    : [buttons][x][y][wheel]              (4 bytes, signed deltas)
//  ID3 consumer in : [usage lo][usage hi]                (2 bytes, 0 = released)
static const uint8_t kReportMap[] = {
    // ---- Keyboard ----
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, kReportIdKeyboard,
    0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
    0x19, 0xE0,        //   Usage Minimum (Left Control)
    0x29, 0xE7,        //   Usage Maximum (Right GUI)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute) - modifiers
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Constant) - reserved byte
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data, Variable, Absolute) - LED bits
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x01,        //   Output (Constant) - LED padding
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
    0x19, 0x00,        //   Usage Minimum (0)
    0x29, 0x65,        //   Usage Maximum (101)
    0x81, 0x00,        //   Input (Data, Array) - 6 key slots
    0xC0,              // End Collection

    // ---- Mouse ----
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, kReportIdMouse,
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (1)
    0x29, 0x03,        //     Usage Maximum (3)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x95, 0x03,        //     Report Count (3)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data, Variable, Absolute) - buttons
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x05,        //     Report Size (5)
    0x81, 0x03,        //     Input (Constant) - padding
    0x05, 0x01,        //     Usage Page (Generic Desktop)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x06,        //     Input (Data, Variable, Relative) - x, y, wheel
    0xC0,              //   End Collection
    0xC0,              // End Collection

    // ---- Consumer control (media keys) ----
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, kReportIdConsumer,
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
    0x19, 0x00,        //   Usage Minimum (0)
    0x2A, 0xFF, 0x03,  //   Usage Maximum (1023)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x00,        //   Input (Data, Array, Absolute)
    0xC0,              // End Collection
};

}  // namespace hid
