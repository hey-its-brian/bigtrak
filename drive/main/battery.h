// 3S pack voltage on GPIO36, with low-voltage warning and motor cutoff.
// The thresholds and latching live in battery_logic.h; this is the ADC side.
#pragma once

#include "battery_logic.h"

namespace battery {

void begin();

// Samples on its own schedule (BATT_SAMPLE_MS). Returns true when the level
// changed on this call, so the caller can react once.
bool update();

float volts();
battery_logic::Level level();
const char* levelName(battery_logic::Level level);

}  // namespace battery
