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

void driveChannel(int speed, bool invert, int in1, int in2, int pwmChannel) {
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

  ledcWrite(pwmChannel, abs(speed));
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

  ledcSetup(PWM_CHANNEL_LEFT, PWM_FREQ_HZ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_RIGHT, PWM_FREQ_HZ, PWM_RESOLUTION);
  ledcAttachPin(PIN_PWMA, PWM_CHANNEL_LEFT);
  ledcAttachPin(PIN_PWMB, PWM_CHANNEL_RIGHT);

  coast();
}

void drive(int left, int right) {
  setLeft(left);
  setRight(right);
}

void setLeft(int speed) {
  driveChannel(speed, INVERT_LEFT, PIN_AIN1, PIN_AIN2, PWM_CHANNEL_LEFT);
}

void setRight(int speed) {
  driveChannel(speed, INVERT_RIGHT, PIN_BIN1, PIN_BIN2, PWM_CHANNEL_RIGHT);
}

void coast() {
  drive(0, 0);
}

void brake() {
  digitalWrite(PIN_AIN1, HIGH);
  digitalWrite(PIN_AIN2, HIGH);
  digitalWrite(PIN_BIN1, HIGH);
  digitalWrite(PIN_BIN2, HIGH);
  ledcWrite(PWM_CHANNEL_LEFT, MOTOR_MAX_DUTY);
  ledcWrite(PWM_CHANNEL_RIGHT, MOTOR_MAX_DUTY);
}

}  // namespace motors
