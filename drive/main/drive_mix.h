// Stick-to-wheel math. Pure functions, no Arduino, so the native unit tests
// can cover it.
#pragma once

#include <math.h>

namespace drive_mix {

struct Wheels {
  float left;   // -1..1, positive is forward
  float right;
};

// Bluepad32 reports sticks as -511..512, with up being negative on Y.
inline float normalizeAxis(int raw) {
  float v = raw / 512.0f;
  if (v > 1.0f) return 1.0f;
  if (v < -1.0f) return -1.0f;
  return v;
}

// Zero inside the deadzone, then rescaled so output still starts at 0 just
// past the edge and reaches 1 at full travel (no jump at the boundary).
inline float applyDeadzone(float v, float deadzone) {
  float magnitude = fabsf(v);
  if (magnitude <= deadzone) return 0.0f;
  float scaled = (magnitude - deadzone) / (1.0f - deadzone);
  if (scaled > 1.0f) scaled = 1.0f;
  return v > 0 ? scaled : -scaled;
}

// Arcade mix for skid steer: left = throttle + steer, right = throttle - steer.
// When the sum overshoots, both sides are scaled down together instead of
// clamped, so full throttle plus a little steer still turns.
inline Wheels mix(float throttle, float steer) {
  float left = throttle + steer;
  float right = throttle - steer;
  float biggest = fmaxf(fabsf(left), fabsf(right));
  if (biggest > 1.0f) {
    left /= biggest;
    right /= biggest;
  }
  return {left, right};
}

// Moves `current` toward `target`, no faster than the given rates (full scale
// per second). "Decel" is any move toward zero, including the first half of a
// reversal, so stopping is always the quick direction.
inline float slew(float current, float target, float accelPerSec,
                  float decelPerSec, float dtSec) {
  bool towardZero = fabsf(target) < fabsf(current) ||
                    (current > 0 && target < 0) || (current < 0 && target > 0);
  float maxStep = (towardZero ? decelPerSec : accelPerSec) * dtSec;
  float delta = target - current;
  if (delta > maxStep) delta = maxStep;
  if (delta < -maxStep) delta = -maxStep;
  return current + delta;
}

inline int toDuty(float v, int maxDuty) {
  return static_cast<int>(lroundf(v * maxDuty));
}

}  // namespace drive_mix
