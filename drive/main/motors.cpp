#include "motors.h"

#include <Arduino.h>

#include "config.h"

namespace {

int clampSpeed(int speed) {
  if (speed > MOTOR_MAX_DUTY) return MOTOR_MAX_DUTY;
  if (speed < -MOTOR_MAX_DUTY) return -MOTOR_MAX_DUTY;
  return speed;
}

// Nonzero-but-too-small duty just makes the gearmotors sing, so push it up to
// the floor where they actually move.
int applyFloor(int speed) {
  if (speed == 0) return 0;
  int magnitude = abs(speed);
  if (magnitude < MOTOR_MIN_DUTY) magnitude = MOTOR_MIN_DUTY;
  return speed > 0 ? magnitude : -magnitude;
}

bool inhibited = false;

void driveChannel(int speed, bool invert, int in1, int in2, int pwmPin) {
  if (inhibited) speed = 0;
  speed = applyFloor(clampSpeed(speed));
  if (invert) speed = -speed;

  if (speed > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else if (speed < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else {
    // Coast: both inputs low leaves the H-bridge outputs floating.
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }

  ledcWrite(pwmPin, abs(speed));
}

}  // namespace

namespace motors {

void begin() {
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT);
  pinMode(PIN_BIN2, OUTPUT);

  if (PIN_STBY >= 0) {
    pinMode(PIN_STBY, OUTPUT);
    digitalWrite(PIN_STBY, HIGH);  // low = driver disabled, nothing moves
  }

  // Arduino core 3.x: LEDC channels are allocated per pin, and ledcWrite()
  // takes the pin rather than a channel number.
  ledcAttach(PIN_PWMA, PWM_FREQ_HZ, PWM_RESOLUTION);
  ledcAttach(PIN_PWMB, PWM_FREQ_HZ, PWM_RESOLUTION);

  coast();
}

void drive(int left, int right) {
  setLeft(left);
  setRight(right);
}

void setLeft(int speed) {
  driveChannel(speed, INVERT_LEFT, PIN_AIN1, PIN_AIN2, PIN_PWMA);
}

void setRight(int speed) {
  driveChannel(speed, INVERT_RIGHT, PIN_BIN1, PIN_BIN2, PIN_PWMB);
}

void coast() {
  drive(0, 0);
}

void brake() {
  digitalWrite(PIN_AIN1, HIGH);
  digitalWrite(PIN_AIN2, HIGH);
  digitalWrite(PIN_BIN1, HIGH);
  digitalWrite(PIN_BIN2, HIGH);
  ledcWrite(PIN_PWMA, MOTOR_MAX_DUTY);
  ledcWrite(PIN_PWMB, MOTOR_MAX_DUTY);
}

void setInhibit(bool on) {
  inhibited = on;
  if (on) brake();
}

bool isInhibited() {
  return inhibited;
}

}  // namespace motors
