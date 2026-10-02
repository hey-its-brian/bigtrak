// The milestone 1 bench test: a fixed motor sequence for checking wiring.
// Wheels off the bench, it runs at full duty by the end.
#pragma once

namespace motion_test {

// Polled throughout the run; return true to abort (motors brake at once).
using AbortCheck = bool (*)();

// Blocks until the sequence finishes or is aborted. Returns false if aborted.
bool run(AbortCheck shouldAbort);

}  // namespace motion_test
