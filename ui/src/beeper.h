// Keypad beeps on the CYD's speaker (GPIO26, onboard amp).
// Non-blocking: a beep starts a tone and update() stops it when it's done, so
// the touch loop never stalls waiting on the speaker.
#pragma once

#include <stdint.h>

namespace beeper {

void begin();

// Call every loop; ends the current tone when its time is up.
void update();

void key();    // accepted keypress
void error();  // rejected keypress (low buzz)
void go();     // GO / CK, a touch higher and longer

void tone(uint32_t hz, uint32_t durationMs);

}  // namespace beeper
