// Pin map and tuning constants for the UI board (ESP32-3248S035C).
// The UART pins come from docs/SPEC.md; change them there too.
#pragma once

#include <stdint.h>

// ------------------------------------------------------------ display ----
// Fixed by the board. ST7796 on the HSPI bus, 320x480 native (portrait).
constexpr int PIN_TFT_SCLK = 14;
constexpr int PIN_TFT_MOSI = 13;
constexpr int PIN_TFT_MISO = 12;
constexpr int PIN_TFT_DC = 2;
constexpr int PIN_TFT_CS = 15;
constexpr int PIN_TFT_RST = -1;  // tied to the ESP32 EN line, no GPIO
constexpr int PIN_TFT_BL = 27;   // backlight PWM (this is why GPIO27 is taken)

constexpr int TFT_NATIVE_WIDTH = 320;
constexpr int TFT_NATIVE_HEIGHT = 480;

// 40 MHz is what most 3248S035 configs run. If the picture shows speckles or
// smeared lines, drop this to 27000000.
constexpr uint32_t TFT_SPI_WRITE_HZ = 40000000;
constexpr uint32_t TFT_SPI_READ_HZ = 16000000;

// 1 or 3 gives landscape (480x320). If the keypad comes up upside down, try
// the other one.
constexpr uint8_t TFT_ROTATION = 1;

// Clone panels vary. If black shows as white (inverted colors), flip
// TFT_INVERT. If blue and red are swapped, flip TFT_RGB_ORDER.
constexpr bool TFT_INVERT = false;
constexpr bool TFT_RGB_ORDER = false;

constexpr uint8_t BACKLIGHT_LEVEL = 255;  // 0..255
constexpr int BACKLIGHT_PWM_CHANNEL = 7;

// -------------------------------------------------------------- touch ----
// GT911 capacitive controller on its own I2C pins. GPIO21 is the touch INT on
// the "C" board, so it is not free for anything else here.
constexpr int PIN_TOUCH_SDA = 33;
constexpr int PIN_TOUCH_SCL = 32;
constexpr int PIN_TOUCH_INT = 21;
constexpr int PIN_TOUCH_RST = 25;
constexpr uint8_t TOUCH_I2C_ADDR = 0x5D;  // 0x14 on some boards; see display.h
constexpr uint32_t TOUCH_I2C_HZ = 400000;

// The GT911 reports in native portrait coordinates and LovyanGFX rotates them
// with the panel. If taps land on the wrong key (mirrored or rotated), change
// this (0..7: 0-3 rotate, 4-7 rotate plus mirror) until they line up.
constexpr uint8_t TOUCH_OFFSET_ROTATION = 0;

// A press fires once, on touch-down. The finger has to be off the glass for
// this long before the next press counts, which swallows the GT911's
// occasional one-frame dropouts during a held press.
constexpr uint32_t TOUCH_RELEASE_MS = 60;
constexpr uint32_t TOUCH_POLL_MS = 10;

// ------------------------------------------------------------ speaker ----
// GPIO26 feeds the onboard audio amp. LEDC square wave, 50% duty.
constexpr int PIN_SPEAKER = 26;
constexpr int BEEP_PWM_CHANNEL = 4;  // own LEDC timer, away from backlight's

constexpr uint32_t BEEP_KEY_HZ = 2400;  // every accepted keypress
constexpr uint32_t BEEP_KEY_MS = 35;
constexpr uint32_t BEEP_ERROR_HZ = 600;  // rejected key (program full, etc)
constexpr uint32_t BEEP_ERROR_MS = 180;
constexpr uint32_t BEEP_GO_HZ = 3200;  // GO and CK
constexpr uint32_t BEEP_GO_MS = 90;

// --------------------------------------------------------- UART link ----
// SPEC.md asks for RX on GPIO27, but GPIO27 is the backlight on the
// 3248S035C, so the RX line moves to GPIO35 on the P3 extension connector.
// GPIO35 is input-only with no internal pullup, which is fine for a UART RX
// line while the drive board's TX holds it high. With the drive board
// unplugged it floats and may read the odd garbage byte; the line parser
// ignores anything it doesn't recognise. A 10k pullup to 3.3V on the cable
// side fixes that for good.
constexpr int PIN_UART_TX = 22;  // CN1 / P3 extension connector
constexpr int PIN_UART_RX = 35;  // P3 extension connector, input-only
constexpr unsigned long LINK_BAUD = 115200;

// Milestone 3: protocol lines go out on USB Serial and status lines are read
// back from it, so the UI can be exercised from a serial monitor. Milestone 4
// flips this to false and the link moves to UART1 on the pins above.
constexpr bool LINK_OVER_USB = true;

constexpr unsigned long USB_BAUD = 115200;

// ----------------------------------------------------------- protocol ----
// SPEC.md doesn't say whether `STEP n` counts from 0 or 1. Assume 1 (matches
// the numbers on the program list); set to 0 if the drive board counts from 0.
constexpr int STEP_INDEX_BASE = 1;

// --------------------------------------------------------- status bar ----
// Battery colour thresholds for the 3S pack (volts).
constexpr float BATT_OK_VOLTS = 11.1f;    // above: green
constexpr float BATT_WARN_VOLTS = 10.5f;  // above: amber, below: red
