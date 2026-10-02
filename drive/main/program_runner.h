// Runs a program, one step at a time, without blocking the main loop.
// Moves are closed-loop on the encoders (or timed when ENCODERS_ENABLED is
// false). Reports progress to the CYD as STEP n / DONE / ERR <reason>.
#pragma once

#include "program_logic.h"

namespace program_runner {

void begin();

// Flattens and starts `program`. Returns nullptr on success, or the ERR
// reason (the caller sends it).
const char* start(const program_logic::Program& program);

// Advance the running program. Call every loop.
void update();

bool running();

// Brake and drop the rest of the run. Sends nothing; the caller decides what
// to tell the CYD.
void abort();

}  // namespace program_runner
