#include "air-nose-axis-nvs-storage.h"

#include <Preferences.h>

namespace {

constexpr const char* kNamespace = "airmouse";
constexpr const char* kNoseKey = "nose";  // axis * 2 + (sign < 0 ? 1 : 0)

}  // namespace

bool loadSavedNoseAxis(uint8_t& axis, int8_t& sign) {
  Preferences prefs;
  // Read-write: a read-only open of a namespace never written logs an error at every boot.
  if (!prefs.begin(kNamespace, false)) return false;
  const uint8_t code = prefs.getUChar(kNoseKey, 0xFF);
  prefs.end();
  if (code > 5) return false;
  axis = code / 2;
  sign = (code & 1) ? -1 : 1;
  return true;
}

bool saveNoseAxis(uint8_t axis, int8_t sign) {
  if (axis > 2) return false;
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) return false;
  const size_t written = prefs.putUChar(kNoseKey, static_cast<uint8_t>(axis * 2 + (sign < 0 ? 1 : 0)));
  prefs.end();
  return written == 1;
}
