# bigtrak

Firmware for a 1979 Big Trak rebuilt around two ESP32 boards: a Bluetooth
gamepad for live driving, and a touchscreen recreation of the original
programmable keypad.

The full design (architecture, BOM, pin map, UART protocol) is in
[docs/SPEC.md](docs/SPEC.md).

```
[CYD touchscreen]  <--UART-->  [ESP32 DevKit]
   ui/                            drive/
```

## Milestones

| # | Milestone | Status |
|---|---|---|
| 1 | Drive board on blocks: hardcoded motion test | done, untested on hardware |
| 2 | Gamepad driving (Bluepad32, mixing, disconnect failsafe) | **done, untested on hardware** |
| 3 | CYD standalone: keypad UI, touch, beeps | done, untested on hardware |
| 4 | UART integration | drive side done; CYD still talks over USB (`LINK_OVER_USB`) |
| 5 | Program engine (queue execution, timed moves) | **done, untested on hardware** |
| 6 | Encoders: closed-loop distance and turns | **done, needs calibration on the tank** |
| 7 | Polish: cannon LED, sounds, battery display | not started |

## drive/ (ESP32 DevKit V1)

Bluepad32 needs the BTstack Bluetooth stack, which the stock Arduino core
doesn't ship, so `drive/` is an ESP-IDF project with Arduino as a component
(the layout of Ricardo Quesada's
[esp-idf-arduino-bluepad32-template](https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template)).
Setup is once per checkout:

```bash
cd drive
./tools/fetch_components.sh
```

That pulls Bluepad32 and BTstack at a pinned template commit and the Arduino
core at tag 3.3.12 into `drive/components/` (not checked in). The platform is
pioarduino 55.03.312 (ESP-IDF 5.5.5); the first `pio run` downloads it and
takes several minutes.

```bash
pio run -t upload && pio device monitor
```

Unit tests for the hardware-free logic (stick mixing, ramping, battery
thresholds, protocol parsing, program queue and RPT expansion, move
planning) run on the Mac:

```bash
pio test -e native
```

### Driving

Put the pad in pairing mode (PS4: hold Share + PS until the light bar
flashes; 8BitDo: check the model's manual for its Switch/Android mode).
It connects, rumbles, and the DualShock light bar turns blue.

| Control | Does |
|---|---|
| Left stick up/down | throttle |
| Right stick left/right | steer (full deflection with no throttle spins in place) |
| R1 / RB held | turbo: full speed instead of 60% |
| Cross / A | fire the photon cannon |

Speed-ups are ramped and slow-downs are quick (`ACCEL_PER_SEC` and
`DECEL_PER_SEC` in `config.h`). If the pad disconnects, the motors brake
immediately. Only one pad drives; a second one is refused.

### Programs

The CYD sends a program one step per line, then `GO` (see the protocol in
the spec). Each line gets `ACK` or `ERR <reason>`. While it runs, the drive
board reports `STEP n` (the program row, counting from 1) as each step
starts and `DONE` at the end.

- `FWD n` / `BACK n`: n units of about 13 inches.
- `LEFT n` / `RIGHT n`: spin in place n clock minutes (15 is 90 degrees).
- `FIRE n`: n cannon flashes. `HOLD n`: pause n tenths of a second.
- `RPT n`: replay the n steps before it once. Nested repeats expand too; a
  program that would expand past 256 steps is refused.

Moves are closed-loop on the wheel encoders: cruise at 50%, slow to 25% for
the last stretch, and trim the faster wheel so straight runs stay straight.
There's a short brake between steps. If a wheel stops turning under power
for half a second (a wall, a jammed track, an unplugged encoder), the run
stops with `ERR stall` to protect the TB6612. Before the encoder motors are
fitted, set `ENCODERS_ENABLED = false` in `config.h` and moves are timed
instead (`MS_PER_UNIT`, `MS_PER_MINUTE`).

The gamepad always wins: touching a stick or Cross/A during a program brakes,
drops the program and sends `ERR gamepad override`, and the pad has the tank.
A pad disconnecting mid-program doesn't stop it, since the program isn't
using the pad. `STOP` and `CLS` abort a run from the CYD; anything else sent
during a run gets `ERR busy`.

### Calibrating the encoders

The tick counts per unit and per clock minute depend on the motors' gear
ratio and the wheel size, so they're measured on the tank and saved to flash
with the console (no reflashing). Lay out a tape measure.

1. **Rough distance.** `enc zero`, drive straight along the tape with the pad
   for exactly 5 units (65 inches), then `enc`. Average the two counts,
   divide by 5: `cal unit <that>`.
2. **Rough turns.** `enc zero`, spin in place with the pad through 4 full
   turns (240 clock minutes), then `enc`. Average the two counts (ignore
   the signs), divide by 240: `cal min <that>`.
3. **Refine distance.** From the console: `CLS`, `FWD 5`, `GO`. Measure how
   far it went in inches. New value = old value x 65 / measured. Repeat
   until it's within half an inch.
4. **Refine turns.** `CLS`, `RIGHT 60`, `GO` (one full spin). If it
   overshoots by d degrees, new value = old value x 360 / (360 + d); if it
   comes up short, use (360 - d).

`cal` shows the stored values and `cal reset` goes back to the `config.h`
defaults. Turns depend on the floor (carpet scrubs more than wood), so
calibrate where the tank will actually run. If straight runs wobble, lower
`PROGRAM_STRAIGHT_KP`; if they drift, raise it.

### Battery

The 3S pack is read on GPIO36 and sent to the CYD as `BATT v.vv` every five
seconds. Below 10.5V the pad rumbles and turns red. Below 9.9V for two
seconds straight the motors are cut, and they stay cut until you power
cycle, since a resting pack bounces back above the threshold. On USB power
with no pack it reads near zero and reports "absent" rather than cutting.
Trim `BATT_CALIBRATION` against a multimeter.

### USB console

Type into the serial monitor at 115200:

| Command | Does |
|---|---|
| `status` | pad, battery, motor, program and encoder state |
| `prog` | list the queued program |
| `enc` / `enc zero` | show or zero the encoder counts |
| `cal unit <ticks>` / `cal min <ticks>` / `cal reset` | encoder calibration, saved in flash |
| `test` | the milestone 1 motion test (wheels off the bench, pad disconnected) |
| `forget` | erase stored pad pairings |
| `help` | the list |

Protocol lines work too, exactly as the CYD sends them (`FWD 2`, `RIGHT 15`,
`GO`, `STOP`), so the drive board can be tested with the touchscreen
unplugged. Replies come back on USB; `STEP`/`DONE` go to the CYD link and
are echoed to USB as `[link] > ...`.

### Before the first run

- **Wheels off the bench.** The motion test runs at full duty by the end.
- TB6612 `STBY` is tied high on the carrier PCB. On a breadboard, tie it to
  3.3V or set `PIN_STBY` in `main/config.h`. Low means nothing moves.
- Common ground between the battery, the TB6612, and the DevKit.
- The carrier PCB's 1N5822 blocks USB backfeed, but powering the tank off
  before plugging in USB is still good practice.

### What the motion test tells you

| Symptom | Fix |
|---|---|
| Tank curves during "forward" | One motor is wired backwards: flip `INVERT_LEFT` / `INVERT_RIGHT` in `config.h` |
| "left motor only" spins the right wheel | Channel A/B swapped at the TB6612 |
| Buzzing, no rotation at slow speed | Raise `MOTOR_MIN_DUTY` |
| Board resets when the ramp gets going | Regulator or battery sag: check `VM` vs the 5V rail under load |
| Nothing moves at all | `STBY` low, `VM` not connected, or battery cutoff latched (`status`) |
| Every programmed move ends in `ERR stall` | Encoders not connected or not counting: check `enc` while driving with the pad |

Pin assignments and tuning constants live in
[drive/main/config.h](drive/main/config.h) and mirror the spec.

## ui/ (CYD, ESP32-3248S035C)

Milestone 3: builds and passes its native tests, untested on hardware.
LovyanGFX configured by hand in `include/display.h` (its autodetect only
knows the 2.8" CYD), with a hand-drawn button grid (no LVGL: no PSRAM on this
board and the keypad is a fixed layout).

```bash
cd ui
pio run -t upload && pio device monitor
pio test -e native
```

Program list and status on the left, keypad on the right:

```
CLR   CLS   CK    | 7 8 9
HOLD  FWD   RPT   | 4 5 6
LEFT  FIRE  RIGHT | 1 2 3
STOP  BACK  GO    | 0
```

Entry works like the original: a command key, then up to two digits; the
step commits on the next command key, GO, or CK. No digits means 1. CLR
wipes the program, CLS removes the last step, CK test-runs the last step, and
GO sends `CLS`, every step, then `GO`, so the drive board's queue is rebuilt
on each run. STOP isn't on the original keypad; it sends `STOP`. Every key
beeps, and refused keys buzz and show why.

Until milestone 4, `LINK_OVER_USB` in `include/config.h` sends protocol lines
to USB serial instead of the drive board, and status lines typed into the
monitor (`STEP 2`, `BATT 11.80`, `PAD CONNECTED`, `DONE`) update the screen.

Things to check on first power-up: rotation and touch mapping, colour
inversion or red/blue swap (both flags in `config.h`), the GT911 address
(0x5D, some answer at 0x14), and 40 MHz SPI (drop to 27 MHz if it glitches).

## cad/ (printed parts)

Parametric OpenSCAD for the drive pod, drive wheels, and CYD bezel and back
cover. See [cad/README.md](cad/README.md); start with the fit tests.

## License

MIT, see [LICENSE](LICENSE). The exception is `drive/main/main.c`, adapted
from Ricardo Quesada's
[esp-idf-arduino-bluepad32-template](https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template),
which stays under Apache-2.0 as marked in its header. The Bluepad32, BTstack
and Arduino components fetched by `drive/tools/fetch_components.sh` keep
their own licenses.
