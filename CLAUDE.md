# bigtrak: project brief

Read [docs/SPEC.md](docs/SPEC.md) first. It holds the architecture, BOM, pin
assignments, UART protocol, and the milestone order, and it is the source of
truth; when a pin or a protocol message changes, change it there too.

## Layout

- `drive/`: PlatformIO project for the ESP32 DevKit V1 (motors, encoders,
  gamepad, failsafe). Currently at milestone 2. It is an ESP-IDF project with
  Arduino as a component (Bluepad32 needs BTstack), so sources live in
  `drive/main/` and every new .cpp must be added to `drive/main/CMakeLists.txt`.
  The arduino/bluepad32/btstack components are fetched by
  `drive/tools/fetch_components.sh` and are not checked in. Arduino core is
  3.x: `ledcAttach(pin, ...)` / `ledcWrite(pin, ...)`, not channels.
  Hardware-free logic (mixing, battery thresholds, protocol parsing) lives in
  header-only files with native tests: `pio test -e native`.
- `ui/`: PlatformIO project for the CYD touchscreen (milestone 3).
- `docs/SPEC.md`: copy of the spec from the Obsidian vault
  (`01 Projects/Big Trak Rebuild/`). Keep both in sync.

## Hard constraints

- **Classic ESP32 only** on the drive board. The S3/C3/C6 parts have no
  Bluetooth Classic and gamepads won't pair.
- Don't put motor-driver signals on GPIO 0, 2, 12, or 15 (boot-strap pins).
- GPIO 34/35/36/39 are input-only with no internal pullups.
- The gamepad always outranks a running program, and a gamepad disconnect
  stops the motors. Safety beats fidelity.

## Conventions

- Both boards build with `pio run` from their own directory. Keep them
  independently flashable and independently testable: the UART protocol is
  plain newline-terminated ASCII specifically so either half can be driven
  from a serial monitor with the other half unplugged.
- Motor speeds are signed duty, -255..255, positive is forward.
- Tuning constants go in `config.h`, not scattered through the source.
- The project moved from `~/development/my_dev/bigtrak` to `~/projects/bigtrak`.
- No em dashes in code, comments, or docs.
