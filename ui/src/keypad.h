// The on-screen keypad: which keys exist, where they sit, and what a touch
// at (x, y) hits. Drawing lives in screen.cpp.
//
// Landscape 480x320. The program list takes the left PANEL_WIDTH pixels and
// the keypad fills the rest as a 6x4 grid, arranged like the original:
//
//   CLR   CLS   CK   |  7   8   9
//   HOLD  FWD   RPT  |  4   5   6
//   LEFT  FIRE  RIGHT|  1   2   3
//   STOP  BACK  GO   |  0 (wide)
//
// Arrows form a diamond around FIRE, digits a phone-style block. CLR wipes
// the program and CLS clears the last step, as on the original. STOP isn't on
// the original keypad but it's in the protocol, so it gets a key.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace keypad {

constexpr int SCREEN_WIDTH = 480;
constexpr int SCREEN_HEIGHT = 320;

constexpr int PANEL_WIDTH = 150;  // program list + status, left side
constexpr int COLUMNS = 6;
constexpr int ROWS = 4;
constexpr int CELL_WIDTH = (SCREEN_WIDTH - PANEL_WIDTH) / COLUMNS;  // 55
constexpr int CELL_HEIGHT = SCREEN_HEIGHT / ROWS;                   // 80

// Gap drawn around each key. Touches in the gap still count for the nearest
// key, so there are no dead zones between buttons.
constexpr int KEY_INSET = 3;

enum class Key : uint8_t {
  None,
  D0, D1, D2, D3, D4, D5, D6, D7, D8, D9,
  Fwd, Back, Left, Right,
  Fire, Hold, Rpt,
  Go, Ck, Clr, Cls, Stop,
};

enum class Style : uint8_t {
  Digit,     // white with navy text
  Arrow,     // Big Trak blue, drawn as a triangle
  Function,  // slate with white text
  Go,        // orange
  Stop,      // red
};

struct KeyDef {
  Key key;
  int16_t x, y, w, h;  // full touch cell, screen coordinates
  const char* label;
  Style style;
};

const KeyDef* all();
size_t count();

// The key whose cell contains (x, y), or nullptr (the program panel).
const KeyDef* hitTest(int x, int y);

const KeyDef* find(Key key);

inline bool isDigit(Key key) { return key >= Key::D0 && key <= Key::D9; }
inline uint8_t digitValue(Key key) {
  return static_cast<uint8_t>(static_cast<uint8_t>(key) -
                              static_cast<uint8_t>(Key::D0));
}

}  // namespace keypad
