#include "battery.h"

#include <Arduino.h>

#include "config.h"

namespace {

using battery_logic::Level;

battery_logic::Monitor monitor({BATT_PRESENT_V, BATT_WARN_V, BATT_CUTOFF_V,
                                BATT_CUTOFF_HOLD_MS});

float filtered = 0.0f;
bool primed = false;
uint32_t lastSampleAt = 0;

// The divider is 100k/22k, high impedance for the ADC's sample cap, and the
// motors put PWM ripple on the pack. Average a burst, then smooth over time.
float readPackVolts() {
  constexpr int SAMPLES = 16;
  uint32_t totalMv = 0;
  for (int i = 0; i < SAMPLES; i++) totalMv += analogReadMilliVolts(PIN_BATT_SENSE);
  float adcVolts = (totalMv / static_cast<float>(SAMPLES)) / 1000.0f;
  return adcVolts * BATT_DIVIDER_RATIO * BATT_CALIBRATION;
}

}  // namespace

namespace battery {

void begin() {
  analogSetPinAttenuation(PIN_BATT_SENSE, ADC_11db);
  filtered = readPackVolts();
  primed = true;
  monitor.update(filtered, millis());
  lastSampleAt = millis();
}

bool update() {
  uint32_t now = millis();
  if (now - lastSampleAt < BATT_SAMPLE_MS) return false;
  lastSampleAt = now;

  float sample = readPackVolts();
  // Exponential moving average, about a one second time constant at 10 Hz.
  filtered = primed ? filtered + 0.1f * (sample - filtered) : sample;
  primed = true;

  Level before = monitor.level();
  return monitor.update(filtered, now) != before;
}

float volts() {
  return monitor.volts();
}

Level level() {
  return monitor.level();
}

const char* levelName(Level level) {
  switch (level) {
    case Level::Absent: return "absent";
    case Level::Ok: return "ok";
    case Level::Low: return "low";
    case Level::Cutoff: return "CUTOFF";
  }
  return "?";
}

}  // namespace battery
