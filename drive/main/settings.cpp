#include "settings.h"

#include <Preferences.h>

#include "config.h"

namespace {

Preferences prefs;
int32_t perUnit = DEFAULT_TICKS_PER_UNIT;
int32_t perMinute = DEFAULT_TICKS_PER_MINUTE;

constexpr const char* NAMESPACE = "bigtrak";
constexpr const char* KEY_PER_UNIT = "tpu";
constexpr const char* KEY_PER_MINUTE = "tpm";

}  // namespace

namespace settings {

void begin() {
  prefs.begin(NAMESPACE, false);
  perUnit = prefs.getInt(KEY_PER_UNIT, DEFAULT_TICKS_PER_UNIT);
  perMinute = prefs.getInt(KEY_PER_MINUTE, DEFAULT_TICKS_PER_MINUTE);
}

int32_t ticksPerUnit() {
  return perUnit;
}

int32_t ticksPerMinute() {
  return perMinute;
}

void setTicksPerUnit(int32_t ticks) {
  perUnit = ticks;
  prefs.putInt(KEY_PER_UNIT, ticks);
}

void setTicksPerMinute(int32_t ticks) {
  perMinute = ticks;
  prefs.putInt(KEY_PER_MINUTE, ticks);
}

void reset() {
  prefs.remove(KEY_PER_UNIT);
  prefs.remove(KEY_PER_MINUTE);
  perUnit = DEFAULT_TICKS_PER_UNIT;
  perMinute = DEFAULT_TICKS_PER_MINUTE;
}

}  // namespace settings
