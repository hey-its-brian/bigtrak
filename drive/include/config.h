// Pin map and tuning constants for the drive board.
// Pin assignments come straight from docs/SPEC.md — change them there too.
#pragma once

// ---------------------------------------------------------------- pins ----
// GPIO 0, 2, 12, 15 are boot-strap pins. Nothing that drives current at reset
// goes on them or the board won't boot (and the motors twitch on power-up).

// TB6612FNG — left motor (channel A)
constexpr int PIN_PWMA = 25;
constexpr int PIN_AIN1 = 26;
constexpr int PIN_AIN2 = 27;

// TB6612FNG — right motor (channel B)
constexpr int PIN_PWMB = 32;
constexpr int PIN_BIN1 = 33;
constexpr int PIN_BIN2 = 23;

// STBY is tied to 3.3V in the current wiring. Set this to a GPIO if you'd
// rather cut the driver in software; -1 means "assume it's already high".
constexpr int PIN_STBY = -1;

// Encoders (input-only pins — no internal pullups available)
constexpr int PIN_ENC_LEFT_A = 34;
constexpr int PIN_ENC_RIGHT_A = 35;

// Extras
constexpr int PIN_CANNON_LED = 18;
constexpr int PIN_DFPLAYER_TX = 19;
constexpr int PIN_BATT_SENSE = 36;  // VP, ADC1_CH0

// UART2 link to the CYD
constexpr int PIN_UART_TX = 17;
constexpr int PIN_UART_RX = 16;
constexpr unsigned long LINK_BAUD = 115200;

// ------------------------------------------------------------ motor PWM ----
constexpr int PWM_FREQ_HZ = 20000;  // above audible — no gearmotor whine
constexpr int PWM_RESOLUTION = 8;   // 0..255 duty
constexpr int PWM_CHANNEL_LEFT = 0;
constexpr int PWM_CHANNEL_RIGHT = 1;

// Below this duty the 25GA-370s buzz instead of turning. Anything nonzero and
// under the floor gets pushed up to it.
constexpr int MOTOR_MIN_DUTY = 40;
constexpr int MOTOR_MAX_DUTY = 255;

// If a motor spins backwards relative to the chassis, flip its flag here
// rather than rewiring.
constexpr bool INVERT_LEFT = false;
constexpr bool INVERT_RIGHT = false;
