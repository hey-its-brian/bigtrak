// Everything that touches the display: the keypad, the program list, and the
// status corner. Holds no UI state of its own; main.cpp decides what to show
// and calls in here to draw it.
#pragma once

#include "keypad.h"
#include "program.h"

namespace screen {

enum class Pad : uint8_t { Unknown, Connected, Disconnected };

// Colour of the message line in the status corner.
enum class Tone : uint8_t { Normal, Busy, Good, Error };

// Display, backlight, touch. Clears to the background colour.
void begin();

void drawKeypad();
void drawKey(const keypad::KeyDef& key, bool pressed);

// The 16-row program list. highlight is the executing step's index, or -1.
void drawProgram(const program::Program& program, int highlight);

void drawBattery(bool known, float volts);
void drawPad(Pad pad);
void drawMessage(const char* text, Tone tone);

// True while a finger is on the glass, with (x, y) in screen coordinates.
bool readTouch(int& x, int& y);

}  // namespace screen
