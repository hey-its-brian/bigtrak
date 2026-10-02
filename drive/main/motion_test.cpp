#include "motion_test.h"

#include <Arduino.h>

#include "config.h"
#include "motors.h"

namespace {

struct TestStep {
  const char* label;
  int left;
  int right;
  uint32_t durationMs;
};

// Forward is positive on both sides. If the tank turns when it should go
// straight, one motor is wired backwards: flip INVERT_LEFT/INVERT_RIGHT in
// config.h rather than reversing the wires.
const TestStep SEQUENCE[] = {
    {"forward, slow", 120, 120, 1500},
    {"stop", 0, 0, 800},
    {"forward, fast", 220, 220, 1500},
    {"stop", 0, 0, 800},
    {"reverse, slow", -120, -120, 1500},
    {"stop", 0, 0, 800},
    {"left motor only (forward)", 150, 0, 1200},
    {"stop", 0, 0, 800},
    {"right motor only (forward)", 0, 150, 1200},
    {"stop", 0, 0, 800},
    {"spin left", -150, 150, 1200},
    {"stop", 0, 0, 800},
    {"spin right", 150, -150, 1200},
    {"stop", 0, 0, 800},
};

motion_test::AbortCheck abortCheck = nullptr;

// Returns false if the run was aborted partway through.
bool holdFor(uint32_t durationMs) {
  uint32_t startedAt = millis();
  while (millis() - startedAt < durationMs) {
    if (abortCheck()) return false;
    delay(5);
  }
  return true;
}

bool runStep(const TestStep& step) {
  Serial.printf("  %-28s L=%4d R=%4d  %lums\n", step.label, step.left,
                step.right, static_cast<unsigned long>(step.durationMs));
  motors::drive(step.left, step.right);
  return holdFor(step.durationMs);
}

// Slow ramp to full duty. This is where a marginal regulator or a sagging
// pack shows itself (brownout reset instead of a smooth speed-up).
bool runRamp() {
  Serial.println("  ramp 0 -> 255 forward");
  for (int duty = 0; duty <= MOTOR_MAX_DUTY; duty += 5) {
    motors::drive(duty, duty);
    if (!holdFor(60)) return false;
  }
  motors::drive(0, 0);
  return holdFor(500);
}

}  // namespace

namespace motion_test {

bool run(AbortCheck shouldAbort) {
  abortCheck = shouldAbort;
  Serial.println("\n--- motion test (any key aborts) ---");

  bool completed = true;
  for (const TestStep& step : SEQUENCE) {
    if (!runStep(step)) {
      completed = false;
      break;
    }
  }
  if (completed) completed = runRamp();

  if (completed) {
    motors::coast();
    Serial.println("--- motion test complete ---");
  } else {
    motors::brake();
    Serial.println("\n*** ABORTED: motors braked ***");
  }
  return completed;
}

}  // namespace motion_test
