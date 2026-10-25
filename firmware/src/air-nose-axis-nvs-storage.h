// Keeps the taught nose axis (see nose-up-pointing-axis-detector.h) in flash (NVS):
// it survives power-off, deep sleep and firmware updates.
#pragma once

#include <stdint.h>

bool loadSavedNoseAxis(uint8_t& axis, int8_t& sign);  // false = never taught
bool saveNoseAxis(uint8_t axis, int8_t sign);
