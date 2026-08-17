# Big Trak Rebuild — Technical Spec

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

- **UI board (CYD):** ESP32-3248S035C — 3.5" 480x320 capacitive touch, ST7796 display, ESP32-WROOM-32. Mounted in the original keypad recess with a 3D-printed bezel.
- **Drive board:** 30-pin ESP32 DevKit V1 (ESP32-WROOM-32, classic ESP32 — NOT S3/C3/C6; Bluetooth Classic is required for gamepad support).

## Hardware Bill of Materials

| Item | Part | Notes |
|---|---|---|
| UI board | ESP32-3248S035C (CYD 3.5" capacitive) | "C" suffix = capacitive. Buy 2 (clone QC) |
| Drive board | ESP32 DevKit V1, 30-pin, WROOM-32 | USB-C variant preferred. Buy 2–3 |
| Motor driver | TB6612FNG breakout | 1.2A cont / 3.2A peak per channel |
| Motors | 2x 25GA-370 12V gearmotors **with hall encoders** | ~130–200 RPM winding (runs at 7.4V) |
| Battery | 2S: 2x 18650 + 2S BMS (or 2S LiPo, XT30) | ~7.4V nominal |
| Buck converter | MP1584 (3A) or Mini 360 | Battery → 5V for both boards |
| Sound (optional) | DFPlayer Mini + small speaker | Photon cannon / motor sounds |
| Misc | Toggle switch, JST/XT30 connectors, 18–20AWG wire, blue LED (cannon) | |

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
| Left encoder A | 34 | input-only, needs external pullup if open-collector |
| Right encoder A | 35 | input-only, same |
| Photon cannon LED | 18 | |
| DFPlayer TX (board→player) | 19 | UART1 or SoftwareSerial |
| Battery voltage divider | 36 (VP) | ADC; divider sized for 8.4V max → <3.3V |

Avoid GPIO 0, 2, 12, 15 (boot-strap pins — motor driver load on these can prevent boot or cause twitch at power-on).

### UI board (CYD 3248S035C)

Display + capacitive touch pins are fixed by the board (use known-good LovyanGFX/TFT_eSPI config for 3248S035C). Additional:

| Function | GPIO | Notes |
|---|---|---|
| UART TX → drive RX | 22 | software-assigned UART (ESP32 pin matrix) |
| UART RX ← drive TX | 27 | verify free on this board rev; any free extension-connector pin OK |
| Speaker | 26 | onboard amp — keypad beeps |

Both boards are 3.3V logic — direct UART connection, no level shifting. Common ground required.

## Power

```
2S battery → toggle switch → ┬→ TB6612 VM (motor power, ~6.4–8.4V)
                             └→ buck (5V) → ┬→ DevKit VIN
                                            └→ CYD 5V pin
```

- Budget ~250–400mA at 5V for both boards; 3A buck removes all doubt.
- Rule: power off the tank before connecting USB to either board (avoid backfeed through cheap bucks).

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
| `ERR <reason>` | rejected (bad arg, queue full) |
| `STEP n` | now executing queue step n (UI highlights it) |
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
- **Bluepad32** (Arduino version) — BT Classic gamepad pairing (PS4/PS5/8BitDo etc.)
- LEDC PWM for motors (~20kHz, above audible)
- Drive mixing: `left = throttle + steer; right = throttle - steer;` clamp; ~10% stick deadzone
- Encoder ISRs counting ticks; programmed moves are **closed-loop on distance/angle** (calibrate ticks-per-unit and ticks-per-clock-minute empirically)
- Battery ADC read + low-voltage warn (send `BATT`, cut motors below ~6.0V)

### UI board firmware
- Arduino framework
- **LovyanGFX** (auto-config works well for 3248S035C) — or TFT_eSPI with known-good setup
- **No LVGL** — hand-drawn fixed button grid; simpler and lighter
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
- Wheelbase / wheel bore / recess dimensions: **TODO — measure** and record here before modeling mounts.

## Constraints & Gotchas (learned the hard way, pre-emptively)

- ESP32-S3/C3/C6 have **no Bluetooth Classic** → most gamepads won't pair. Classic ESP32 only.
- CYD clones sometimes ship inverted-color panels → one config flag fixes (`TFT_INVERSION_ON` or LovyanGFX equivalent).
- CYD "R" = resistive, "C" = capacitive. This project uses **C**.
- TB6612 STBY must be high or nothing moves.
- GPIO 34/35/36/39 are input-only, no internal pullups.
- Timed (non-encoder) moves drift with battery sag — encoders are what make programmed mode faithful.
