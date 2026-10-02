#include "keypad.h"

namespace keypad {

namespace {

// Grid cell (column, row), optionally spanning several columns.
constexpr KeyDef cell(Key key, int column, int row, const char* label,
                      Style style, int span = 1) {
  return {key,
          static_cast<int16_t>(PANEL_WIDTH + column * CELL_WIDTH),
          static_cast<int16_t>(row * CELL_HEIGHT),
          static_cast<int16_t>(span * CELL_WIDTH),
          static_cast<int16_t>(CELL_HEIGHT),
          label,
          style};
}

constexpr KeyDef KEYS[] = {
    // Row 0
    cell(Key::Clr, 0, 0, "CLR", Style::Function),
    cell(Key::Cls, 1, 0, "CLS", Style::Function),
    cell(Key::Ck, 2, 0, "CK", Style::Function),
    cell(Key::D7, 3, 0, "7", Style::Digit),
    cell(Key::D8, 4, 0, "8", Style::Digit),
    cell(Key::D9, 5, 0, "9", Style::Digit),
    // Row 1
    cell(Key::Hold, 0, 1, "HOLD", Style::Function),
    cell(Key::Fwd, 1, 1, "FWD", Style::Arrow),
    cell(Key::Rpt, 2, 1, "RPT", Style::Function),
    cell(Key::D4, 3, 1, "4", Style::Digit),
    cell(Key::D5, 4, 1, "5", Style::Digit),
    cell(Key::D6, 5, 1, "6", Style::Digit),
    // Row 2
    cell(Key::Left, 0, 2, "LEFT", Style::Arrow),
    cell(Key::Fire, 1, 2, "FIRE", Style::Function),
    cell(Key::Right, 2, 2, "RIGHT", Style::Arrow),
    cell(Key::D1, 3, 2, "1", Style::Digit),
    cell(Key::D2, 4, 2, "2", Style::Digit),
    cell(Key::D3, 5, 2, "3", Style::Digit),
    // Row 3
    cell(Key::Stop, 0, 3, "STOP", Style::Stop),
    cell(Key::Back, 1, 3, "BACK", Style::Arrow),
    cell(Key::Go, 2, 3, "GO", Style::Go),
    cell(Key::D0, 3, 3, "0", Style::Digit, 3),
};

constexpr size_t KEY_COUNT = sizeof(KEYS) / sizeof(KEYS[0]);

}  // namespace

const KeyDef* all() { return KEYS; }
size_t count() { return KEY_COUNT; }

const KeyDef* hitTest(int x, int y) {
  for (const KeyDef& k : KEYS) {
    if (x >= k.x && x < k.x + k.w && y >= k.y && y < k.y + k.h) return &k;
  }
  return nullptr;
}

const KeyDef* find(Key key) {
  for (const KeyDef& k : KEYS) {
    if (k.key == key) return &k;
  }
  return nullptr;
}

}  // namespace keypad
