// Milestone 1 — drive board on blocks.
//
// No gamepad, no UART link, no encoders yet. This just proves the wiring:
// both motors spin, both directions, both sides, at a duty you can hear and
// see. Put the tank on blocks so the wheels are off the bench before running.
//
// Serial (115200): any key starts the sequence, any key during the run aborts.

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
// straight, one motor is wired backwards — flip INVERT_LEFT/INVERT_RIGHT in
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

constexpr size_t SEQUENCE_LENGTH = sizeof(SEQUENCE) / sizeof(SEQUENCE[0]);

void drainSerial() {
  while (Serial.available()) Serial.read();
}

// Any keypress mid-run is a stop request — no need to hunt for the right key
// while the tank is doing something you don't like.
bool abortRequested() {
  if (!Serial.available()) return false;
  drainSerial();
  return true;
}

// Returns false if the run was aborted partway through.
bool holdFor(uint32_t durationMs) {
  uint32_t startedAt = millis();
  while (millis() - startedAt < durationMs) {
    if (abortRequested()) return false;
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

// Slow ramp to full duty — this is where a marginal buck converter or a sagging
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

void runSequence() {
  Serial.println("\n--- running (press any key to abort) ---");

  for (size_t i = 0; i < SEQUENCE_LENGTH; i++) {
    if (!runStep(SEQUENCE[i])) {
      motors::brake();
      Serial.println("\n*** ABORTED — motors braked ***");
      return;
    }
  }

  if (!runRamp()) {
    motors::brake();
    Serial.println("\n*** ABORTED — motors braked ***");
    return;
  }

  motors::coast();
  Serial.println("--- sequence complete ---");
}

void printBanner() {
  Serial.println();
  Serial.println("=========================================");
  Serial.println(" bigtrak drive board — milestone 1");
  Serial.println(" motion test (wheels OFF the bench)");
  Serial.println("=========================================");
  Serial.println(" press any key to start");
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);  // let the USB serial port come up before the banner

  motors::begin();  // leaves both motors coasting

  printBanner();
}

void loop() {
  if (!Serial.available()) {
    delay(20);
    return;
  }

  drainSerial();
  runSequence();

  delay(300);
  drainSerial();  // ignore keys pressed during the run itself
  Serial.println("\npress any key to run again");
}
