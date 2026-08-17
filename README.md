# bigtrak

Firmware for a 1979 Big Trak rebuilt around two ESP32 boards: a Bluetooth
gamepad for live driving, and a touchscreen recreation of the original
programmable keypad.

Full design — architecture, BOM, pin map, UART protocol — is in
[docs/SPEC.md](docs/SPEC.md).

```
[CYD touchscreen]  <--UART-->  [ESP32 DevKit]
   ui/ (not started)              drive/
```

## Milestones

| # | Milestone | Status |
|---|---|---|
| 1 | Drive board on blocks — hardcoded motion test | **done, untested on hardware** |
| 2 | Gamepad driving (Bluepad32, mixing, disconnect failsafe) | not started |
| 3 | CYD standalone — keypad UI, touch, beeps | not started |
| 4 | UART integration | not started |
| 5 | Program engine (queue execution, timed moves) | not started |
| 6 | Encoders — closed-loop distance and turns | not started |
| 7 | Polish — cannon LED, sounds, battery display | not started |

## drive/ — ESP32 DevKit V1

```bash
cd drive && pio run -t upload && pio device monitor
```

Milestone 1 firmware runs a fixed motion test: press any key in the serial
monitor to start, any key again to abort (motors brake).

The sequence is forward slow → forward fast → reverse → each motor alone →
spin left → spin right → a slow ramp to full duty. Watch the ramp — that's
where an undersized buck converter or a sagging pack shows up as a brownout
reset instead of a smooth speed-up.

### Before the first run

- **Wheels off the bench.** It runs at full duty by the end.
- TB6612 `STBY` tied to 3.3V, or set `PIN_STBY` in `include/config.h`. Low
  means nothing moves at all.
- Common ground between the battery, the TB6612, and the DevKit.
- Tank powered off before plugging USB into either board — cheap bucks
  backfeed.

### What the test tells you

| Symptom | Fix |
|---|---|
| Tank curves during "forward" | One motor is wired backwards — flip `INVERT_LEFT` / `INVERT_RIGHT` in `config.h` |
| "left motor only" spins the right wheel | Channel A/B swapped at the TB6612 |
| Buzzing, no rotation at slow speed | Raise `MOTOR_MIN_DUTY` |
| Board resets when the ramp gets going | Buck converter or battery sag — check `VM` vs the 5V rail under load |
| Nothing moves at all | `STBY` low, or `VM` not connected |

Pin assignments and tuning constants live in
[drive/include/config.h](drive/include/config.h) and mirror the spec.

## ui/ — CYD (ESP32-3248S035C)

Not started; milestone 3. Will use LovyanGFX with a hand-drawn button grid
(no LVGL — no PSRAM on this board and the keypad is a fixed layout).
