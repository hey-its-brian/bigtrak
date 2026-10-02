// TB6612FNG dual motor control.
// Speeds are signed duty: -255 (full reverse) .. 0 (stop) .. +255 (full forward).
#pragma once

namespace motors {

void begin();

// Signed duty per side. Values are clamped, and any nonzero magnitude below
// MOTOR_MIN_DUTY is raised to it so the motors turn instead of buzzing.
void drive(int left, int right);

void setLeft(int speed);
void setRight(int speed);

// Both outputs floating: the tank rolls to a stop.
void coast();

// Both outputs shorted: the motors resist turning. Use this for a hard stop.
void brake();

// While inhibited every drive request becomes a stop. The battery cutoff uses
// this so nothing upstream can accidentally run a flat pack.
void setInhibit(bool on);
bool isInhibited();

}  // namespace motors
