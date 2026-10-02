#include "beeper.h"

#include <Arduino.h>

#include "config.h"

namespace {

bool playing = false;
uint32_t stopAt = 0;

}  // namespace

namespace beeper {

void begin() {
  // Arduino core 2.x LEDC API, same as the drive board's motor PWM.
  ledcSetup(BEEP_PWM_CHANNEL, BEEP_KEY_HZ, 8);
  ledcAttachPin(PIN_SPEAKER, BEEP_PWM_CHANNEL);
  ledcWriteTone(BEEP_PWM_CHANNEL, 0);  // silent until asked
}

void update() {
  // Signed difference so a millis() wrap doesn't leave a tone stuck on.
  if (playing && static_cast<int32_t>(millis() - stopAt) >= 0) {
    ledcWriteTone(BEEP_PWM_CHANNEL, 0);
    playing = false;
  }
}

void tone(uint32_t hz, uint32_t durationMs) {
  // A new beep cuts off the old one; fast typing should still click per key.
  ledcWriteTone(BEEP_PWM_CHANNEL, hz);
  stopAt = millis() + durationMs;
  playing = true;
}

void key() { tone(BEEP_KEY_HZ, BEEP_KEY_MS); }
void error() { tone(BEEP_ERROR_HZ, BEEP_ERROR_MS); }
void go() { tone(BEEP_GO_HZ, BEEP_GO_MS); }

}  // namespace beeper
