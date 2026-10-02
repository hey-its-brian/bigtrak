#include "program_runner.h"

#include <Arduino.h>
#include <stdlib.h>

#include "cannon.h"
#include "config.h"
#include "cyd_link.h"
#include "drive_mix.h"
#include "encoders.h"
#include "motors.h"
#include "settings.h"

namespace {

using program_logic::MovePlan;
using program_logic::RunStep;
using protocol::Command;

enum class Phase {
  Idle,
  Settling,  // brake pause before the next step
  Moving,
  Firing,
  Holding,
};

RunStep run[program_logic::MAX_RUN_STEPS];
size_t runLength = 0;
size_t current = 0;  // index into run[] of the step in progress

Phase phase = Phase::Idle;
uint32_t phaseStartedAt = 0;

// Moving state
MovePlan plan = {};
int32_t leftStart = 0;
int32_t rightStart = 0;
float speed = 0.0f;  // ramped cruise speed, 0..1
uint32_t lastTickAt = 0;
int32_t lastProgress = 0;
uint32_t lastProgressAt = 0;

uint32_t holdMs = 0;

void enter(Phase next) {
  phase = next;
  phaseStartedAt = millis();
}

void finish(const char* error) {
  motors::brake();
  phase = Phase::Idle;
  runLength = 0;
  if (error) {
    cyd_link::sendf("ERR %s", error);
  } else {
    cyd_link::send("DONE");
  }
}

void beginStep() {
  const RunStep& rs = run[current];
  cyd_link::sendf("STEP %u", static_cast<unsigned>(rs.programIndex + 1));

  const program_logic::Step& s = rs.step;
  if (program_logic::isMove(s.command)) {
    plan = program_logic::planMove(s, settings::ticksPerUnit(),
                                   settings::ticksPerMinute(), MS_PER_UNIT,
                                   MS_PER_MINUTE);
    leftStart = encoders::left();
    rightStart = encoders::right();
    speed = 0.0f;
    lastTickAt = millis();
    lastProgress = 0;
    lastProgressAt = millis();
    enter(Phase::Moving);
  } else if (s.command == Command::Fire) {
    cannon::fire(s.arg);
    enter(Phase::Firing);
  } else {
    // HOLD n: n tenths of a second, as on the original.
    holdMs = static_cast<uint32_t>(s.arg) * 100;
    enter(Phase::Holding);
  }
}

// Advances to the next step after a settle, or finishes the run.
void stepComplete() {
  current++;
  motors::brake();
  if (current >= runLength) {
    finish(nullptr);
    return;
  }
  enter(Phase::Settling);
}

void updateMove() {
  uint32_t now = millis();
  float dt = (now - lastTickAt) / 1000.0f;
  lastTickAt = now;

  int32_t leftDone = abs(encoders::left() - leftStart);
  int32_t rightDone = abs(encoders::right() - rightStart);

  int32_t progress;
  int32_t total;
  if (ENCODERS_ENABLED) {
    progress = (leftDone + rightDone) / 2;
    total = plan.ticks;
  } else {
    // Timed: treat milliseconds as the unit of progress.
    progress = static_cast<int32_t>(now - phaseStartedAt);
    total = static_cast<int32_t>(plan.timedMs);
  }

  if (progress >= total) {
    stepComplete();
    return;
  }

  if (ENCODERS_ENABLED) {
    if (progress != lastProgress) {
      lastProgress = progress;
      lastProgressAt = now;
    } else if (now - lastProgressAt >= STALL_TIMEOUT_MS) {
      // Under power and not turning: a wall, a jammed track, or an encoder
      // that came unplugged. Either way, stop before the driver cooks.
      finish("stall");
      return;
    }
  }

  float target = program_logic::moveSpeed(
      progress, total, PROGRAM_SPEED, PROGRAM_SLOW_SPEED,
      PROGRAM_SLOWDOWN_FRACTION, PROGRAM_SLOWDOWN_TICKS);
  speed = drive_mix::slew(speed, target, ACCEL_PER_SEC, DECEL_PER_SEC, dt);

  // Hold the two sides to the same distance. Whichever wheel is ahead gets
  // a little less power.
  float leftPower = speed;
  float rightPower = speed;
  if (ENCODERS_ENABLED) {
    float correction = (leftDone - rightDone) * PROGRAM_STRAIGHT_KP;
    leftPower -= correction;
    rightPower += correction;
    if (leftPower < 0.0f) leftPower = 0.0f;
    if (rightPower < 0.0f) rightPower = 0.0f;
  }

  motors::drive(drive_mix::toDuty(leftPower * plan.leftDir, MOTOR_MAX_DUTY),
                drive_mix::toDuty(rightPower * plan.rightDir, MOTOR_MAX_DUTY));
}

}  // namespace

namespace program_runner {

void begin() {
  phase = Phase::Idle;
}

const char* start(const program_logic::Program& program) {
  if (program.empty()) return "empty program";
  runLength = program.flatten(run, program_logic::MAX_RUN_STEPS);
  if (runLength == 0) return "program too long";

  // Start with a settle rather than the first step, so the caller's ACK for
  // GO reaches the CYD before STEP 1 does.
  current = 0;
  motors::brake();
  enter(Phase::Settling);
  return nullptr;
}

void update() {
  uint32_t elapsed = millis() - phaseStartedAt;
  switch (phase) {
    case Phase::Idle:
      break;
    case Phase::Settling:
      if (elapsed >= PROGRAM_SETTLE_MS) beginStep();
      break;
    case Phase::Moving:
      updateMove();
      break;
    case Phase::Firing:
      if (!cannon::busy()) stepComplete();
      break;
    case Phase::Holding:
      if (elapsed >= holdMs) stepComplete();
      break;
  }
}

bool running() {
  return phase != Phase::Idle;
}

void abort() {
  if (!running()) return;
  phase = Phase::Idle;
  runLength = 0;
  motors::brake();
  cannon::cancel();
}

}  // namespace program_runner
