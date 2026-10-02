#!/usr/bin/env bash
# Fetch the ESP-IDF components the drive firmware builds against: Bluepad32
# and BTstack from Ricardo Quesada's esp-idf-arduino-bluepad32-template at a
# pinned commit, and the Arduino core at a pinned tag, so every checkout builds
# against the same versions.
#
# The template pins Arduino 3.2.1, which needs ESP-IDF 5.4 and therefore the
# 54.x pioarduino platform. That platform's SCons doesn't work with current
# PlatformIO core, so we take the template's Bluepad32/BTstack but use the
# Arduino release that matches the platform in platformio.ini.
#
# Run from anywhere:  drive/tools/fetch_components.sh
# Re-run with --force to replace components that are already there.
set -euo pipefail

TEMPLATE_REPO="https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template.git"
TEMPLATE_COMMIT="d07a9385f46f7215f51fc3eb5e40c5a484cfe102"
ARDUINO_REPO="https://github.com/espressif/arduino-esp32.git"
ARDUINO_TAG="3.3.12"  # matches pioarduino platform 55.03.312 (ESP-IDF 5.5.5)
COMPONENTS=(arduino bluepad32 bluepad32_arduino btstack cmd_nvs cmd_system)

DRIVE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="$DRIVE_DIR/components"

if [[ -d "$DEST" && "${1:-}" != "--force" ]]; then
  echo "components/ already exists. Use --force to fetch again."
  exit 0
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

echo "Fetching template @ ${TEMPLATE_COMMIT:0:7}"
git -C "$WORK" init -q tpl
git -C "$WORK/tpl" remote add origin "$TEMPLATE_REPO"
git -C "$WORK/tpl" fetch -q --depth 1 origin "$TEMPLATE_COMMIT"
git -C "$WORK/tpl" checkout -q FETCH_HEAD

echo "Fetching Arduino core $ARDUINO_TAG (about 60 MB)"
rm -rf "$WORK/tpl/components/arduino"
git clone -q --depth 1 --branch "$ARDUINO_TAG" "$ARDUINO_REPO" "$WORK/tpl/components/arduino"

rm -rf "$DEST"
mkdir -p "$DEST"
for c in "${COMPONENTS[@]}"; do
  cp -R "$WORK/tpl/components/$c" "$DEST/$c"
  rm -rf "$DEST/$c/.git"
done

# The template targets ESP-IDF 5.4, where driver/rtc_io.h happened to pull in
# driver/gpio.h. On 5.5 it doesn't, and cmd_system.c fails to compile on
# gpio_wakeup_enable(). Add the include it always needed.
CMD_SYSTEM="$DEST/cmd_system/cmd_system.c"
if ! grep -q '<driver/gpio.h>' "$CMD_SYSTEM"; then
  sed -i.bak 's|#include <driver/rtc_io.h>|#include <driver/gpio.h>\
#include <driver/rtc_io.h>|' "$CMD_SYSTEM"
  rm -f "$CMD_SYSTEM.bak"
fi

echo "Done: ${COMPONENTS[*]} -> $DEST"
