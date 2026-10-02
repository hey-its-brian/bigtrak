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
| 3 | CYD standalone: keypad UI, touch, beeps | in progress |
| 4 | UART integration | not started |
| 5 | Program engine (queue execution, timed moves) | not started |
| 6 | Encoders: closed-loop distance and turns | not started |
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

That pulls the Arduino core 3.2.1, Bluepad32 and BTstack components at a
pinned commit into `drive/components/` (not checked in). The first
`pio run` then downloads ESP-IDF 5.4 and takes several minutes.

```bash
pio run -t upload && pio device monitor
```

Unit tests for the hardware-free logic (stick mixing, ramping, battery
thresholds, protocol parsing) run on the Mac:

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
| `status` | pad, battery and motor state |
| `test` | the milestone 1 motion test (wheels off the bench, pad disconnected) |
| `forget` | erase stored pad pairings |
| `help` | the list |

Protocol lines work too, exactly as the CYD sends them (`STOP`, `CLS`, ...),
so the drive board can be tested with the touchscreen unplugged. Queue
commands (`FWD 5`, `GO`) are parsed and validated but answer
`ERR not implemented` until the program engine lands in milestone 5.

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

## License

MIT, see [LICENSE](LICENSE). The exception is `drive/main/main.c`, adapted
from Ricardo Quesada's
[esp-idf-arduino-bluepad32-template](https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template),
which stays under Apache-2.0 as marked in its header. The Bluepad32, BTstack
and Arduino components fetched by `drive/tools/fetch_components.sh` keep
their own licenses.
