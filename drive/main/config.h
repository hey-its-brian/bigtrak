// Pin map and tuning constants for the drive board.
// Pin assignments come straight from docs/SPEC.md (carrier PCB rev 1). Change
// them there too.
#pragma once

#include <stdint.h>

// ---------------------------------------------------------------- pins ----
// GPIO 0, 2, 12, 15 are boot-strap pins. Nothing that drives current at reset
// goes on them or the board won't boot (and the motors twitch on power-up).

// TB6612FNG: left motor (channel A)
constexpr int PIN_PWMA = 25;
constexpr int PIN_AIN1 = 26;
constexpr int PIN_AIN2 = 27;

// TB6612FNG: right motor (channel B)
constexpr int PIN_PWMB = 32;
constexpr int PIN_BIN1 = 33;
constexpr int PIN_BIN2 = 23;

// STBY is tied high on the carrier PCB. Set this to a GPIO if you'd rather
// cut the driver in software; -1 means "assume it's already high".
constexpr int PIN_STBY = -1;

// Encoders. A channels are on input-only pins (no internal pullups); the PCB
// has 10k pullups on all four lines. GPIO39 is deliberately not used for an
// encoder: errata 3.11 glitches 36/39 whenever the ADC powers up.
constexpr int PIN_ENC_LEFT_A = 34;
constexpr int PIN_ENC_LEFT_B = 4;
constexpr int PIN_ENC_RIGHT_A = 35;
constexpr int PIN_ENC_RIGHT_B = 13;

// Photon cannon: drives Q1 on the PCB, which switches the LED from 5V.
constexpr int PIN_CANNON_LED = 18;

// DFPlayer Mini on UART1 (1k series resistors on the PCB)
constexpr int PIN_DFPLAYER_RX = 19;    // board TX -> player RX
constexpr int PIN_DFPLAYER_TX = 5;     // player TX -> board RX
constexpr int PIN_DFPLAYER_BUSY = 39;  // input-only, low while playing

// J8 expansion header (4.7k pullups on the PCB)
constexpr int PIN_I2C_SDA = 21;
constexpr int PIN_I2C_SCL = 22;

// Battery sense: 100k/22k divider into VP (ADC1, so it works alongside BT)
constexpr int PIN_BATT_SENSE = 36;

// UART2 link to the CYD
constexpr int PIN_UART_TX = 17;
constexpr int PIN_UART_RX = 16;
constexpr unsigned long LINK_BAUD = 115200;

// ------------------------------------------------------------ motor PWM ----
constexpr int PWM_FREQ_HZ = 20000;  // above audible, no gearmotor whine
constexpr int PWM_RESOLUTION = 8;   // 0..255 duty

// Below this duty the 25GA-370s buzz instead of turning. Anything nonzero and
// under the floor gets pushed up to it.
constexpr int MOTOR_MIN_DUTY = 40;
constexpr int MOTOR_MAX_DUTY = 255;

// If a motor spins backwards relative to the chassis, flip its flag here
// rather than rewiring.
constexpr bool INVERT_LEFT = false;
constexpr bool INVERT_RIGHT = false;

// -------------------------------------------------------------- gamepad ----
// Fraction of full stick travel ignored around center. Cheap pads drift.
constexpr float STICK_DEADZONE = 0.10f;

// On 3S the 12V motors run at full rated voltage, which is a lot of tank for
// a living room. Normal driving is capped; hold R1 for everything.
constexpr float SPEED_NORMAL = 0.60f;
constexpr float SPEED_TURBO = 1.00f;

// Steering authority relative to throttle. Below 1.0 keeps a full-stick spin
// from being violent.
constexpr float STEER_GAIN = 0.70f;

// Ramp limits, in full-scale duty per second. Speeding up is gentle to spare
// the gearboxes and the TB6612 (1.2A continuous); slowing down is quick.
// Failsafe stops skip the ramp entirely.
constexpr float ACCEL_PER_SEC = 2.5f;
constexpr float DECEL_PER_SEC = 8.0f;

// ------------------------------------------------------------- battery ----
// 3S Li-ion: 12.6V full, 11.1V nominal.
constexpr float BATT_DIVIDER_RATIO = 122.0f / 22.0f;  // (100k + 22k) / 22k
// Trim against a multimeter: multiply the reading by this.
constexpr float BATT_CALIBRATION = 1.00f;

constexpr float BATT_WARN_V = 10.5f;
constexpr float BATT_CUTOFF_V = 9.9f;  // 3.3V per cell
// Voltage has to stay under the cutoff this long before the motors are cut,
// so a hard launch sagging the pack for a moment doesn't trip it.
constexpr uint32_t BATT_CUTOFF_HOLD_MS = 2000;
// Below this there's no pack at all (bench power over USB). Don't cut, just
// report it: the motors have no VM anyway.
constexpr float BATT_PRESENT_V = 5.0f;

constexpr uint32_t BATT_SAMPLE_MS = 100;
constexpr uint32_t BATT_REPORT_MS = 5000;

// --------------------------------------------------------------- cannon ----
constexpr uint32_t CANNON_ON_MS = 120;
constexpr uint32_t CANNON_OFF_MS = 120;

// ------------------------------------------------------------- encoders ----
// Hall encoders on the 25GA-370s, counted by the PCNT peripheral in full
// quadrature (4 counts per encoder line per pulse). Set false to run
// programs on timing alone, e.g. before the encoder motors arrive.
constexpr bool ENCODERS_ENABLED = true;

// If driving forward counts down on a side, flip it here.
constexpr bool ENC_INVERT_LEFT = false;
constexpr bool ENC_INVERT_RIGHT = false;

// Pulses narrower than this are noise from the motor leads, not ticks.
constexpr uint32_t ENC_GLITCH_NS = 1000;

// ---------------------------------------------------- programmed moves ----
// Calibration. These are starting guesses; the real values come from the
// procedure in the README and are stored in flash with the "cal" console
// command, so they survive reflashing without editing this file.
//
// One Big Trak unit is about 13 inches. Turns are in "clock minutes":
// 15 is a quarter turn, 60 is a full spin.
constexpr int32_t DEFAULT_TICKS_PER_UNIT = 2000;
constexpr int32_t DEFAULT_TICKS_PER_MINUTE = 60;

// Timed fallback when ENCODERS_ENABLED is false. Drifts with battery sag,
// which is the whole reason the encoders exist.
constexpr uint32_t MS_PER_UNIT = 1200;
constexpr uint32_t MS_PER_MINUTE = 50;

// Cruise and approach speeds for programmed moves, as a fraction of full
// duty. The last PROGRAM_SLOWDOWN_FRACTION of each move (at most
// PROGRAM_SLOWDOWN_TICKS) runs at the slow speed so the tank stops on the
// mark instead of coasting past it.
constexpr float PROGRAM_SPEED = 0.50f;
constexpr float PROGRAM_SLOW_SPEED = 0.25f;
constexpr float PROGRAM_SLOWDOWN_FRACTION = 0.25f;
constexpr int32_t PROGRAM_SLOWDOWN_TICKS = 600;

// Keeps straight moves straight: duty correction per tick of left/right
// difference. Too high and it wobbles; too low and it drifts.
constexpr float PROGRAM_STRAIGHT_KP = 0.004f;

// Brake pause between steps, so one move's momentum doesn't leak into the
// next (especially a forward run followed by a spin).
constexpr uint32_t PROGRAM_SETTLE_MS = 250;

// Stall protection for the TB6612 (1.2A continuous per channel): if a move
// is under power and the encoders haven't advanced in this long, abort.
constexpr uint32_t STALL_TIMEOUT_MS = 500;
