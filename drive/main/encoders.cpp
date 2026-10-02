#include "encoders.h"

#include <Arduino.h>
#include <driver/pulse_cnt.h>

#include "config.h"

namespace {

// The hardware counter is 16-bit. With accum_count set and watch points on
// both limits, the driver folds each overflow into the value
// pcnt_unit_get_count() returns, so callers see one continuous count.
constexpr int COUNT_LIMIT = 32767;

pcnt_unit_handle_t leftUnit = nullptr;
pcnt_unit_handle_t rightUnit = nullptr;

// Standard x4 quadrature decode (the ESP-IDF rotary encoder example): each
// channel counts edges on one line and uses the other line for direction.
bool makeQuadratureUnit(int pinA, int pinB, pcnt_unit_handle_t* out) {
  pcnt_unit_config_t unitConfig = {};
  unitConfig.low_limit = -COUNT_LIMIT;
  unitConfig.high_limit = COUNT_LIMIT;
  unitConfig.flags.accum_count = 1;

  pcnt_unit_handle_t unit = nullptr;
  if (pcnt_new_unit(&unitConfig, &unit) != ESP_OK) return false;

  pcnt_glitch_filter_config_t filter = {};
  filter.max_glitch_ns = ENC_GLITCH_NS;
  pcnt_unit_set_glitch_filter(unit, &filter);

  pcnt_chan_config_t chanAConfig = {};
  chanAConfig.edge_gpio_num = pinA;
  chanAConfig.level_gpio_num = pinB;
  pcnt_channel_handle_t chanA = nullptr;
  if (pcnt_new_channel(unit, &chanAConfig, &chanA) != ESP_OK) return false;

  pcnt_chan_config_t chanBConfig = {};
  chanBConfig.edge_gpio_num = pinB;
  chanBConfig.level_gpio_num = pinA;
  pcnt_channel_handle_t chanB = nullptr;
  if (pcnt_new_channel(unit, &chanBConfig, &chanB) != ESP_OK) return false;

  pcnt_channel_set_edge_action(chanA, PCNT_CHANNEL_EDGE_ACTION_DECREASE,
                               PCNT_CHANNEL_EDGE_ACTION_INCREASE);
  pcnt_channel_set_level_action(chanA, PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
  pcnt_channel_set_edge_action(chanB, PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                               PCNT_CHANNEL_EDGE_ACTION_DECREASE);
  pcnt_channel_set_level_action(chanB, PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

  pcnt_unit_add_watch_point(unit, COUNT_LIMIT);
  pcnt_unit_add_watch_point(unit, -COUNT_LIMIT);

  if (pcnt_unit_enable(unit) != ESP_OK) return false;
  pcnt_unit_clear_count(unit);
  pcnt_unit_start(unit);

  *out = unit;
  return true;
}

int32_t read(pcnt_unit_handle_t unit, bool invert) {
  if (unit == nullptr) return 0;
  int value = 0;
  pcnt_unit_get_count(unit, &value);
  return invert ? -value : value;
}

}  // namespace

namespace encoders {

bool begin() {
  // The PCNT driver turns on internal pullups for every encoder pin. GPIO4
  // and 13 have them; 34 and 35 don't and rely on the PCB's 10k resistors.
  bool ok = makeQuadratureUnit(PIN_ENC_LEFT_A, PIN_ENC_LEFT_B, &leftUnit) &&
            makeQuadratureUnit(PIN_ENC_RIGHT_A, PIN_ENC_RIGHT_B, &rightUnit);
  if (!ok) Serial.println("[enc] PCNT setup failed; counts will read 0");
  return ok;
}

int32_t left() {
  return read(leftUnit, ENC_INVERT_LEFT);
}

int32_t right() {
  return read(rightUnit, ENC_INVERT_RIGHT);
}

void zero() {
  if (leftUnit) pcnt_unit_clear_count(leftUnit);
  if (rightUnit) pcnt_unit_clear_count(rightUnit);
}

}  // namespace encoders
