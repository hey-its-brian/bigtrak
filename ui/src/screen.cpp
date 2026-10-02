#include "screen.h"

#include <Arduino.h>

#include "config.h"
#include "display.h"
#include "protocol.h"

namespace {

Display tft;

// Lines are drawn into small sprites and pushed in one go, so updating the
// list or the status corner doesn't flicker. About 6 KB and 5 KB at 16 bits.
LGFX_Sprite statusLine(&tft);
LGFX_Sprite listRow(&tft);

// ------------------------------------------------------------ palette ----
// 1979 Big Trak: charcoal body, blue and white keypad, orange accents.
// uint32_t so LovyanGFX reads them as RGB888.
constexpr uint32_t COLOR_BG = 0x0B1222;
constexpr uint32_t COLOR_PANEL = 0x111B30;
constexpr uint32_t COLOR_RULE = 0x1E64C8;
constexpr uint32_t COLOR_TEXT = 0xE8ECF4;
constexpr uint32_t COLOR_DIM = 0x4A5A78;
constexpr uint32_t COLOR_PENDING = 0xF0B020;
constexpr uint32_t COLOR_GOOD = 0x38C060;
constexpr uint32_t COLOR_WARN = 0xF0B020;
constexpr uint32_t COLOR_BAD = 0xE04040;

constexpr uint32_t KEY_BLUE = 0x1E64C8;
constexpr uint32_t KEY_WHITE = 0xF2F2EA;
constexpr uint32_t KEY_NAVY = 0x0B1A3A;
constexpr uint32_t KEY_SLATE = 0x34445E;
constexpr uint32_t KEY_ORANGE = 0xF07818;
constexpr uint32_t KEY_RED = 0xC42828;
constexpr uint32_t KEY_EDGE = 0x05080F;

// ------------------------------------------------------------- layout ----
constexpr int PANEL_W = keypad::PANEL_WIDTH;
constexpr int LINE_H = 20;               // status lines
constexpr int STATUS_H = 3 * LINE_H;     // battery, pad, message
constexpr int RULE_Y = STATUS_H + 1;     // 2px blue rule under the status
constexpr int LIST_Y = STATUS_H + 4;     // 64
constexpr int ROW_H = (keypad::SCREEN_HEIGHT - LIST_Y) /
                      static_cast<int>(program::MAX_STEPS);  // 16
constexpr int TEXT_X = 6;
constexpr int KEY_RADIUS = 8;

static_assert(ROW_H >= 16, "program rows too short for Font2");

struct KeyColors {
  uint32_t face;
  uint32_t text;
};

KeyColors colorsFor(keypad::Style style, bool pressed) {
  KeyColors c;
  switch (style) {
    case keypad::Style::Digit: c = {KEY_WHITE, KEY_NAVY}; break;
    case keypad::Style::Arrow: c = {KEY_BLUE, KEY_WHITE}; break;
    case keypad::Style::Go: c = {KEY_ORANGE, KEY_WHITE}; break;
    case keypad::Style::Stop: c = {KEY_RED, KEY_WHITE}; break;
    case keypad::Style::Function:
    default: c = {KEY_SLATE, KEY_WHITE}; break;
  }
  // Pressed keys light up: digits go blue, everything else goes white.
  if (pressed) {
    if (style == keypad::Style::Digit) {
      c = {KEY_BLUE, KEY_WHITE};
    } else {
      c = {KEY_WHITE, c.face};
    }
  }
  return c;
}

void drawArrow(keypad::Key key, int cx, int cy, int size, uint32_t color) {
  int s = size / 2;
  switch (key) {
    case keypad::Key::Fwd:
      tft.fillTriangle(cx, cy - s, cx - s, cy + s, cx + s, cy + s, color);
      break;
    case keypad::Key::Back:
      tft.fillTriangle(cx, cy + s, cx - s, cy - s, cx + s, cy - s, color);
      break;
    case keypad::Key::Left:
      tft.fillTriangle(cx - s, cy, cx + s, cy - s, cx + s, cy + s, color);
      break;
    case keypad::Key::Right:
      tft.fillTriangle(cx + s, cy, cx - s, cy - s, cx - s, cy + s, color);
      break;
    default:
      break;
  }
}

// Draws label centred at (cx, cy), squeezed horizontally if it would
// otherwise overrun maxWidth ("HOLD" and "STOP" in a 49 px key).
void drawFittedLabel(const char* label, const lgfx::IFont* font, int cx,
                     int cy, int maxWidth) {
  tft.setFont(font);
  tft.setTextSize(1.0f);
  int width = tft.textWidth(label);
  if (width > maxWidth) {
    tft.setTextSize(static_cast<float>(maxWidth) / width, 1.0f);
  }
  tft.drawString(label, cx, cy);
  tft.setTextSize(1.0f);
}

void pushStatusLine(int line, const char* text, uint32_t color) {
  statusLine.fillScreen(COLOR_PANEL);
  statusLine.setTextColor(color, COLOR_PANEL);
  statusLine.drawString(text, TEXT_X, 2);
  statusLine.pushSprite(0, line * LINE_H);
}

}  // namespace

namespace screen {

void begin() {
  tft.init();
  tft.setRotation(TFT_ROTATION);
  tft.setBrightness(BACKLIGHT_LEVEL);
  tft.fillScreen(COLOR_BG);

  tft.fillRect(0, 0, PANEL_W, keypad::SCREEN_HEIGHT, COLOR_PANEL);
  tft.fillRect(0, RULE_Y, PANEL_W, 2, COLOR_RULE);
  tft.fillRect(PANEL_W - 1, 0, 1, keypad::SCREEN_HEIGHT, COLOR_RULE);

  // PANEL_W - 1 wide so pushes never touch the divider line.
  for (LGFX_Sprite* sprite : {&statusLine, &listRow}) {
    sprite->setColorDepth(16);
    sprite->createSprite(PANEL_W - 1, sprite == &statusLine ? LINE_H : ROW_H);
    sprite->setFont(&fonts::Font2);
    sprite->setTextDatum(lgfx::top_left);
  }
}

void drawKeypad() {
  tft.fillRect(keypad::PANEL_WIDTH, 0,
               keypad::SCREEN_WIDTH - keypad::PANEL_WIDTH,
               keypad::SCREEN_HEIGHT, COLOR_BG);
  for (size_t i = 0; i < keypad::count(); i++) {
    drawKey(keypad::all()[i], false);
  }
}

void drawKey(const keypad::KeyDef& key, bool pressed) {
  int x = key.x + keypad::KEY_INSET;
  int y = key.y + keypad::KEY_INSET;
  int w = key.w - 2 * keypad::KEY_INSET;
  int h = key.h - 2 * keypad::KEY_INSET;
  int cx = x + w / 2;
  int cy = y + h / 2;

  KeyColors c = colorsFor(key.style, pressed);
  tft.fillRoundRect(x, y, w, h, KEY_RADIUS, c.face);
  tft.drawRoundRect(x, y, w, h, KEY_RADIUS, KEY_EDGE);
  // A thin lighter lip along the top for a moulded-plastic look.
  if (!pressed) {
    tft.drawFastHLine(x + KEY_RADIUS, y + 2, w - 2 * KEY_RADIUS, KEY_WHITE);
  }

  tft.setTextColor(c.text);
  tft.setTextDatum(lgfx::middle_center);

  if (key.style == keypad::Style::Arrow) {
    drawArrow(key.key, cx, cy - 6, (w < h ? w : h) / 2, c.text);
    drawFittedLabel(key.label, &fonts::Font0, cx, y + h - 10, w - 6);
    return;
  }

  const lgfx::IFont* font = &fonts::FreeSansBold9pt7b;
  if (key.style == keypad::Style::Digit) {
    font = &fonts::FreeSansBold18pt7b;
  } else if (strlen(key.label) <= 2) {
    font = &fonts::FreeSansBold12pt7b;
  }
  drawFittedLabel(key.label, font, cx, cy, w - 8);
}

void drawProgram(const program::Program& prog, int highlight) {
  char text[protocol::MAX_LINE + 8];

  for (size_t i = 0; i < program::MAX_STEPS; i++) {
    uint32_t bg = COLOR_PANEL;
    uint32_t fg = COLOR_DIM;

    if (i < prog.size()) {
      char step[protocol::MAX_LINE + 1];
      protocol::formatStep(prog.at(i), step, sizeof(step));
      snprintf(text, sizeof(text), "%2u  %s", static_cast<unsigned>(i + 1),
               step);
      fg = COLOR_TEXT;
      if (static_cast<int>(i) == highlight) {
        bg = COLOR_RULE;
        fg = KEY_WHITE;
      }
    } else if (i == prog.size() && prog.hasPending()) {
      // The step being typed: command plus whatever digits so far, then a
      // cursor while there's room for another digit.
      const char* name = protocol::commandName(prog.pendingCommand());
      if (prog.pendingDigits() == 0) {
        snprintf(text, sizeof(text), "%2u  %s _", static_cast<unsigned>(i + 1),
                 name);
      } else {
        snprintf(text, sizeof(text), "%2u  %s %u%s",
                 static_cast<unsigned>(i + 1), name,
                 static_cast<unsigned>(prog.pendingArg()),
                 prog.pendingDigits() < program::MAX_DIGITS ? "_" : "");
      }
      fg = COLOR_PENDING;
    } else {
      snprintf(text, sizeof(text), "%2u", static_cast<unsigned>(i + 1));
    }

    listRow.fillScreen(bg);
    listRow.setTextColor(fg, bg);
    listRow.drawString(text, TEXT_X, 0);
    listRow.pushSprite(0, LIST_Y + static_cast<int>(i) * ROW_H);
  }
}

void drawBattery(bool known, float volts) {
  char text[24];
  uint32_t color = COLOR_DIM;
  if (!known) {
    snprintf(text, sizeof(text), "BATT  --.--V");
  } else {
    snprintf(text, sizeof(text), "BATT  %.2fV", static_cast<double>(volts));
    if (volts >= BATT_OK_VOLTS) {
      color = COLOR_GOOD;
    } else if (volts >= BATT_WARN_VOLTS) {
      color = COLOR_WARN;
    } else {
      color = COLOR_BAD;
    }
  }
  pushStatusLine(0, text, color);
}

void drawPad(Pad pad) {
  switch (pad) {
    case Pad::Connected: pushStatusLine(1, "PAD   ON", COLOR_GOOD); break;
    case Pad::Disconnected: pushStatusLine(1, "PAD   OFF", COLOR_WARN); break;
    case Pad::Unknown:
    default: pushStatusLine(1, "PAD   --", COLOR_DIM); break;
  }
}

void drawMessage(const char* text, Tone tone) {
  uint32_t color = COLOR_TEXT;
  switch (tone) {
    case Tone::Busy: color = COLOR_PENDING; break;
    case Tone::Good: color = COLOR_GOOD; break;
    case Tone::Error: color = COLOR_BAD; break;
    case Tone::Normal:
    default: break;
  }
  pushStatusLine(2, text, color);
}

bool readTouch(int& x, int& y) {
  lgfx::touch_point_t point;
  if (tft.getTouch(&point, 1) == 0) return false;
  x = point.x;
  y = point.y;
  return true;
}

}  // namespace screen
