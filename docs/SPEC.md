# Big Trak Rebuild: Technical Spec

> Project brief for Claude Code. Drop this file (or copy it as `CLAUDE.md`) into the firmware project root.
> Goal: modernize a 1979 Big Trak with two-board ESP32 internals. Bluetooth gamepad driving + touchscreen recreation of the original programmable keypad.

## Architecture Overview

Two boards connected by UART, one battery, tank-style (skid-steer) drive.

```
[CYD touchscreen]  <--UART-->  [ESP32 DevKit]
   UI / keypad                    drive, BT gamepad,
   program builder                motors, encoders,
   beeps, status display          LEDs, failsafe
```

- **UI board (CYD):** ESP32-3248S035C: 3.5" 480x320 capacitive touch, ST7796 display, ESP32-WROOM-32. Mounted in the original keypad recess with a 3D-printed bezel.
- **Drive board:** 30-pin ESP32 DevKit V1 (ESP32-WROOM-32, classic ESP32, NOT S3/C3/C6; Bluetooth Classic is required for gamepad support).

## Hardware Bill of Materials

| Item             | Part                                                                 | Notes                                     |
| ---------------- | -------------------------------------------------------------------- | ----------------------------------------- |
| UI board         | ESP32-3248S035C (CYD 3.5" capacitive)                                | "C" suffix = capacitive. Buy 2 (clone QC) |
| Drive board      | ESP32 DevKit V1, 30-pin, WROOM-32                                    | USB-C variant preferred. Buy 2–3          |
| Motor driver     | TB6612FNG breakout                                                   | 1.2A cont / 3.2A peak per channel         |
| Motors           | 2x 25GA-370 12V gearmotors **with hall encoders**                    | ~130–200 RPM winding (full speed on 3S)   |
| Battery          | 3S Li-ion/LiPo, XT30                                                 | 11.1V nominal, 12.6V full, never 4S       |
| 5V regulator     | RECOM R-78B5.0-2.0 (on carrier PCB)                                  | Battery → 5V for both boards              |
| Carrier PCB      | `PCB/` folder: KiCad project + Gerbers (rev 1)                      | DevKit, TB6612, DFPlayer plug into sockets |
| Sound (optional) | DFPlayer Mini + small speaker                                        | Photon cannon / motor sounds              |
| Misc             | Toggle switch, JST/XT30 connectors, 18–20AWG wire, blue LED (cannon) |                                           |

3D printed (Bambu P1S): motor mounts, wheel hubs/adapters, CYD bezel for keypad recess, 18650 holder, ESP32 cradle.

## Pin Assignments

### Drive board (ESP32 DevKit)

| Function | GPIO | Notes |
|---|---|---|
| UART2 TX → CYD RX | 17 | |
| UART2 RX ← CYD TX | 16 | |
| TB6612 PWMA | 25 | Left motor PWM (LEDC) |
| TB6612 AIN1 | 26 | |
| TB6612 AIN2 | 27 | |
| TB6612 PWMB | 32 | Right motor PWM (LEDC) |
| TB6612 BIN1 | 33 | |
| TB6612 BIN2 | 23 | |
| TB6612 STBY | tie to 3.3V | (or a GPIO if SW-controlled standby wanted) |
| Left encoder A | 34 | input-only, 10k pullup on PCB |
| Left encoder B | 4 | 10k pullup on PCB |
| Right encoder A | 35 | input-only, 10k pullup on PCB |
| Right encoder B | 13 | 10k pullup on PCB |
| Photon cannon LED | 18 | |
| DFPlayer RX (board→player) | 19 | UART1, 1k series on PCB |
| DFPlayer TX (player→board) | 5 | via 1k |
| DFPlayer BUSY | 39 | input-only, 10k pullup; low while playing |
| I2C SDA / SCL | 21 / 22 | J8 expansion, 4.7k pullups |
| Battery voltage divider | 36 (VP) | ADC; 100k/22k, 12.6V → 2.27V. vbat = v_adc × 122/22 |

GPIO39 carries only the slow BUSY signal: ESP32 errata 3.11 glitches GPIO36/39 whenever the ADC powers up, so no encoder goes there.

Avoid GPIO 0, 2, 12, 15 (boot-strap pins; motor driver load on these can prevent boot or cause twitch at power-on).

### UI board (CYD 3248S035C)

Display + capacitive touch pins are fixed by the board: ST7796 on SPI (SCLK 14, MOSI 13, MISO 12, DC 2, CS 15), backlight 27, GT911 touch on I2C (SDA 33, SCL 32, INT 21, RST 25). See `ui/include/config.h`. Additional:

| Function | GPIO | Notes |
|---|---|---|
| UART TX → drive RX | 22 | software-assigned UART (ESP32 pin matrix) |
| UART RX ← drive TX | 35 | P3 connector. Input-only, no internal pullup: floats (junk bytes) with the drive board unplugged; a 10k pullup to 3.3V on the cable fixes it. Not 27: that's the backlight on the 3248S035C. Not 21: GT911 INT. |
| Speaker | 26 | onboard amp, keypad beeps |

Carrier PCB connector J5 pin order is 5V, RX, TX, GND (drive side), so it crosses correctly to a VIN, TX, RX, GND port with a straight cable.

Both boards are 3.3V logic: direct UART connection, no level shifting. Common ground required.

## Power

```
3S battery → XT30 → 3A PTC → toggle switch (J2) → ┬→ TB6612 VM (motor power, 9.0–12.6V)
                                                  └→ R-78B5.0-2.0 → 1N5822 → ┬→ DevKit VIN
                                                                             ├→ CYD 5V (J5)
                                                                             └→ DFPlayer
```

- Budget ~250–400mA at 5V for both boards plus DFPlayer peaks; the 2A regulator covers it.
- The 1N5822 blocks DevKit USB 5V from back-feeding the regulator. Powering off before plugging in USB is still good practice.
- TB6612 VM is rated 13.5V max. 3S (12.6V full) is the ceiling.

## UART Protocol (the contract between boards)

Newline-terminated ASCII text, 115200 baud. Human-readable by design so either board can be driven/debugged from a serial monitor.

### CYD → Drive (commands)

| Message | Meaning |
|---|---|
| `FWD n` | queue: forward n units (1 unit ≈ 13", per original) |
| `BACK n` | queue: backward n units |
| `LEFT n` / `RIGHT n` | queue: turn n "clock minutes" (15 = 90°, per original) |
| `FIRE n` | queue: fire photon cannon n times (LED flash + sound) |
| `HOLD n` | queue: pause n × 0.1s (original semantics) |
| `RPT n` | repeat last n steps once |
| `GO` | execute queue |
| `CLS` | clear queue / abort execution |
| `STOP` | immediate halt (also aborts) |

### Drive → CYD (status)

| Message | Meaning |
|---|---|
| `ACK` | command accepted/queued |
| `ERR <reason>` | rejected or aborted. Reasons: `unknown command`, `missing argument`, `bad argument`, `argument out of range`, `unexpected argument`, `queue full`, `nothing to repeat`, `empty program`, `program too long`, `busy` (program running), `stall`, `gamepad override`, `battery cutoff` |
| `STEP n` | now executing queue step n, counting from 1 (UI highlights it) |
| `DONE` | program finished |
| `BATT v.vv` | battery voltage, sent every ~5s |
| `PAD CONNECTED` / `PAD DISCONNECTED` | gamepad state |

### Mode arbitration

- Gamepad input **pauses/overrides** a running program (safety first).
- Gamepad disconnect while driving → motors stop immediately (Bluepad32 disconnect callback = failsafe).
- Drive board watchdog: if UI hangs, motors are unaffected; if drive board resets mid-program, motors stop.

## Software Stack

### Drive board firmware
- Arduino framework (PlatformIO preferred)
- **Bluepad32** (Arduino version): BT Classic gamepad pairing (PS4/PS5/8BitDo etc.)
- LEDC PWM for motors (~20kHz, above audible)
- Drive mixing: `left = throttle + steer; right = throttle - steer;` clamp; ~10% stick deadzone
- Encoder ISRs counting ticks; programmed moves are **closed-loop on distance/angle** (calibrate ticks-per-unit and ticks-per-clock-minute empirically)
- Battery ADC read + low-voltage warn at ~10.5V (send `BATT`), cut motors below ~9.9V (3.3V/cell)
- Encoders via PCNT (ESP32Encoder) with glitch filter on; stall detect (PWM high, no ticks) to protect the TB6612 (1.2A continuous per channel)

### UI board firmware
- Arduino framework
- **LovyanGFX**, configured by hand in `ui/include/display.h` (autodetect only covers the 2.8" 2432S028, not the 3248S035C)
- **No LVGL**: hand-drawn fixed button grid; simpler and lighter
- Keypad layout mimics original: arrows, 0–9, CLS, CK, FIRE, HOLD, RPT, GO
- Beep on every keypress (GPIO 26 tone)
- Program list display with executing-step highlight; battery + pad status in corner

## Suggested Milestones (build/test order)

1. **Drive board on blocks:** motors wired, hardcoded motion test (fwd/back/spin)
2. **Gamepad driving:** Bluepad32 pairing + mixing + deadzone + disconnect failsafe
3. **CYD standalone:** keypad UI rendering + touch + beeps; commands printed to USB serial
4. **UART integration:** connect boards; typed serial commands first, then UI-driven
5. **Program engine:** queue execution with timed moves
6. **Encoders:** closed-loop distance/turns; calibrate units
7. **Polish:** cannon LED + DFPlayer sounds, battery display, printed mounts final fit

## Physical Notes

- Original gearbox/motor fully removed; two independent motors, skid-steer (no original single-motor turn mechanism).
- Keep the original membrane keypad intact and stored (collector value).
- USB access: route/expose USB-C via the battery-door area for reflashing without opening shell.
- Wheelbase / wheel bore / recess dimensions: **TODO: measure** and record here before modeling mounts.

## Constraints & Gotchas (learned the hard way, pre-emptively)

- ESP32-S3/C3/C6 have **no Bluetooth Classic** → most gamepads won't pair. Classic ESP32 only.
- CYD clones sometimes ship inverted-color panels → one config flag fixes (`TFT_INVERSION_ON` or LovyanGFX equivalent).
- CYD "R" = resistive, "C" = capacitive. This project uses **C**.
- TB6612 STBY must be high or nothing moves.
- GPIO 34/35/36/39 are input-only, no internal pullups.
- Timed (non-encoder) moves drift with battery sag; encoders are what make programmed mode faithful.
