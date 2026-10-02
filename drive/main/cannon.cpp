#include "cannon.h"

#include <Arduino.h>

#include "config.h"

namespace {

int pendingShots = 0;
bool ledOn = false;
uint32_t phaseStartedAt = 0;

void setLed(bool on) {
  ledOn = on;
  digitalWrite(PIN_CANNON_LED, on ? HIGH : LOW);
  phaseStartedAt = millis();
}

}  // namespace

namespace cannon {

void begin() {
  pinMode(PIN_CANNON_LED, OUTPUT);
  setLed(false);
}

void fire(int shots) {
  if (shots <= 0) return;
  bool idle = !busy();
  pendingShots += shots;
  if (idle) {
    pendingShots--;
    setLed(true);
  }
}

void update() {
  uint32_t elapsed = millis() - phaseStartedAt;
  if (ledOn) {
    if (elapsed >= CANNON_ON_MS) setLed(false);
  } else if (pendingShots > 0 && elapsed >= CANNON_OFF_MS) {
    pendingShots--;
    setLed(true);
  }
}

bool busy() {
  return ledOn || pendingShots > 0;
}

void cancel() {
  pendingShots = 0;
  setLed(false);
}

}  // namespace cannon
