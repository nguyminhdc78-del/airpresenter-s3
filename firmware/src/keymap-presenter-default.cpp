// Default button layout for presenting. Edit this table to remap buttons.
// Every button: one leg to the GPIO, the other leg to GND (internal pull-ups are used).
// FN is a physical button on the remote (GPIO9). Its layer is used while FN is held,
// or for one press after FN is tapped. Holding FN alone for 10 s forgets pairings.
//
// Pins 43/44 cannot wake the remote from deep sleep (not RTC pins) - keep them for
// less important buttons. Do not hold the GPIO43 button while powering on (it is the
// UART TX pin during boot). GPIO0 is the onboard BOOT button.
#include "hid-report-descriptor-and-usages.h"
#include "remote-action-types.h"

using namespace hid;

const ButtonBinding kButtonBindings[] = {
    // GPIO  label    normal action                      action while FN is held
    {2,  "AIR",   act::airClutch(),                   act::key(kKeyL, kModCtrl)},   // Fn: PowerPoint laser pointer
    {3,  "CLICK", act::mouseButton(kMouseLeft),       act::mouseButton(kMouseRight)},
    {4,  "NEXT",  act::key(kKeyPageDown),             act::key(kKeyF5)},            // Fn: start slideshow
    {5,  "PREV",  act::key(kKeyPageUp),               act::key(kKeyEsc)},           // Fn: end slideshow
    {6,  "PLAY",  act::media(kMediaPlayPause),        act::media(kMediaMute)},
    {7,  "VOL+",  act::media(kMediaVolumeUp, true),   act::wheel(act::kUp)},        // Fn: scroll up
    {8,  "VOL-",  act::media(kMediaVolumeDown, true), act::wheel(act::kDown)},      // Fn: scroll down
    {9,  "FN",    act::fnLayer(),                     act::none()},
    {10, "UP",    act::nudge(act::kUp),               act::key(kKeyB)},             // Fn: black screen
    {11, "DOWN",  act::nudge(act::kDown),             act::key(kKeyTab, kModAlt)},  // Fn: Alt+Tab
    {44, "LEFT",  act::nudge(act::kLeft),             act::media(kMediaPrev)},      // Fn: previous track
    {43, "RIGHT", act::nudge(act::kRight),            act::media(kMediaNext)},      // Fn: next track
    {0,  "BOOT",  act::clearBonds(),                  act::clearBonds()},           // hold 10 s: re-pair
};

const uint8_t kButtonCount = sizeof(kButtonBindings) / sizeof(kButtonBindings[0]);
