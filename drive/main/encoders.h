// Wheel encoders via the ESP32 PCNT peripheral: hardware quadrature
// counting with a glitch filter, no interrupts per tick.
#pragma once

#include <stdint.h>

namespace encoders {

// Returns false if the PCNT units couldn't be set up; counts then stay 0.
bool begin();

// Signed tick counts since boot (or the last zero()). Positive is forward
// for each wheel.
int32_t left();
int32_t right();

void zero();

}  // namespace encoders
