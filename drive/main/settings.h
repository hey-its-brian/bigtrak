// Calibration values kept in flash (NVS), so dialing in the encoders doesn't
// mean editing config.h and reflashing for every attempt.
#pragma once

#include <stdint.h>

namespace settings {

void begin();

int32_t ticksPerUnit();
int32_t ticksPerMinute();

void setTicksPerUnit(int32_t ticks);
void setTicksPerMinute(int32_t ticks);

// Back to the config.h defaults.
void reset();

}  // namespace settings
