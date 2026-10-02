// bigtrak drive board, milestones 2, 5 and 6: gamepad driving and the
// program engine with closed-loop moves.
//
// A Bluetooth pad drives the tank (left stick throttle, right stick steer,
// R1 turbo, Cross/A fires the cannon). Disconnecting the pad mid-drive stops
// the motors.
//
// The CYD builds a program (FWD 5, LEFT 15, FIRE 2, ...) and sends GO. Moves
// run on the wheel encoders, and progress goes back as STEP n / DONE. Touching
// the pad during a program aborts it: the gamepad always outranks a program.
//
// The battery is watched the whole time: BATT goes to the CYD every few
// seconds, and a flat pack cuts the motors until power is cycled.
//
// USB serial (115200) takes lowercase console commands (type "help"), and
// also accepts protocol lines exactly as the CYD would send them, so this
// board can be tested with the touchscreen unplugged.
//
// Entry point is app_main() in main.c, which starts Bluetooth and then runs
// setup()/loop() on the Arduino task.

#include <Arduino.h>
#include <stdlib.h>

#include "battery.h"
#include "cannon.h"
#include "config.h"
#include "cyd_link.h"
#include "drive_mix.h"
#include "encoders.h"
#include "gamepad.h"
#include "line_reader.h"
#include "motion_test.h"
#include "motors.h"
#include "program_logic.h"
#include "program_runner.h"
#include "protocol.h"
#include "settings.h"

namespace {

constexpr uint32_t DRIVE_TICK_MS = 10;

// Current wheel outputs after ramping, -1..1.
float leftOut = 0.0f;
float rightOut = 0.0f;
uint32_t lastDriveTickAt = 0;
uint32_t lastBattReportAt = 0;

program_logic::Program program;
LineReader consoleReader;

// Hard stop: no ramp, abort any program, brake both motors, drop any cannon
// volley.
void stopAll() {
  program_runner::abort();
  leftOut = 0.0f;
  rightOut = 0.0f;
  motors::brake();
  cannon::cancel();
}

void reportBattery() {
  cyd_link::sendf("BATT %.2f", battery::volts());
  lastBattReportAt = millis();
}

void onPadChange(bool connected) {
  if (connected) {
    cyd_link::send("PAD CONNECTED");
    return;
  }

  // The failsafe. If the pad was driving, nobody is steering now. A running
  // program doesn't need the pad, so it carries on.
  if (!program_runner::running()) stopAll();
  cyd_link::send("PAD DISCONNECTED");
}

bool padIsActive(const gamepad::Input& in) {
  return in.throttle != 0.0f || in.steer != 0.0f || in.firePressed;
}

void driveFromPad(float dtSec) {
  gamepad::Input in;
  if (!gamepad::read(in)) return;

  if (program_runner::running()) {
    // Hands off the motors while the program runs, unless someone grabs the
    // sticks, in which case the program is over.
    if (!padIsActive(in)) return;
    stopAll();
    cyd_link::send("ERR gamepad override");
  }

  float cap = in.turbo ? SPEED_TURBO : SPEED_NORMAL;
  drive_mix::Wheels target =
      drive_mix::mix(in.throttle * cap, in.steer * STEER_GAIN * cap);

  leftOut = drive_mix::slew(leftOut, target.left, ACCEL_PER_SEC, DECEL_PER_SEC, dtSec);
  rightOut = drive_mix::slew(rightOut, target.right, ACCEL_PER_SEC, DECEL_PER_SEC, dtSec);

  motors::drive(drive_mix::toDuty(leftOut, MOTOR_MAX_DUTY),
                drive_mix::toDuty(rightOut, MOTOR_MAX_DUTY));

  if (in.firePressed) cannon::fire(1);
}

void updateDrive() {
  uint32_t now = millis();
  uint32_t elapsed = now - lastDriveTickAt;
  if (elapsed < DRIVE_TICK_MS) return;
  lastDriveTickAt = now;

  if (gamepad::connected()) driveFromPad(elapsed / 1000.0f);
}

void updateBattery() {
  if (battery::update()) {
    battery_logic::Level level = battery::level();
    Serial.printf("[batt] %.2fV, level %s\n", battery::volts(), battery::levelName(level));

    if (level == battery_logic::Level::Low) {
      gamepad::signalWarning();
    } else if (level == battery_logic::Level::Cutoff) {
      bool wasRunning = program_runner::running();
      stopAll();
      motors::setInhibit(true);
      gamepad::signalWarning();
      if (wasRunning) cyd_link::send("ERR battery cutoff");
      Serial.println("[batt] *** CUTOFF: motors disabled until power cycle ***");
    }
    reportBattery();
  }

  if (millis() - lastBattReportAt >= BATT_REPORT_MS) reportBattery();
}

// Executes one protocol line from either port. Replies go back to `reply`;
// STEP/DONE from a run always go to the CYD.
void handleCommand(const char* line, Print& reply) {
  protocol::Parsed p = protocol::parse(line);
  if (p.command == protocol::Command::Invalid) {
    reply.printf("ERR %s\n", p.error);
    return;
  }

  switch (p.command) {
    case protocol::Command::Stop:
      stopAll();
      reply.println("ACK");
      return;

    case protocol::Command::Cls:
      stopAll();
      program.clear();
      reply.println("ACK");
      return;

    default:
      break;
  }

  if (program_runner::running()) {
    reply.println("ERR busy");
    return;
  }

  if (p.command == protocol::Command::Go) {
    if (motors::isInhibited()) {
      reply.println("ERR battery cutoff");
      return;
    }
    // Make sure no leftover pad output is still applied.
    leftOut = 0.0f;
    rightOut = 0.0f;
    const char* error = program_runner::start(program);
    if (error) {
      reply.printf("ERR %s\n", error);
    } else {
      reply.println("ACK");
    }
    return;
  }

  const char* error = program.add({p.command, p.arg});
  if (error) {
    reply.printf("ERR %s\n", error);
  } else {
    reply.println("ACK");
  }
}

// ----------------------------------------------------------- USB console ----

bool motionTestShouldAbort() {
  gamepad::update();
  if (gamepad::connected()) {
    Serial.println("  (pad connected)");
    return true;
  }
  if (Serial.available()) {
    while (Serial.available()) Serial.read();
    return true;
  }
  const char* line = cyd_link::poll();
  return line != nullptr && strcmp(line, "STOP") == 0;
}

const char* commandName(protocol::Command c) {
  for (const protocol::Spec& spec : protocol::SPECS) {
    if (spec.command == c) return spec.name;
  }
  return "?";
}

void printProgram() {
  if (program.empty()) {
    Serial.println("program: empty");
    return;
  }
  Serial.printf("program: %u step(s)\n", static_cast<unsigned>(program.size()));
  for (size_t i = 0; i < program.size(); i++) {
    const program_logic::Step& s = program.at(i);
    Serial.printf("  %2u  %s %d\n", static_cast<unsigned>(i + 1),
                  commandName(s.command), s.arg);
  }
}

void printStatus() {
  Serial.printf("pad:      %s (%s)\n", gamepad::connected() ? "connected" : "none",
                gamepad::modelName());
  Serial.printf("battery:  %.2fV, %s\n", battery::volts(),
                battery::levelName(battery::level()));
  Serial.printf("motors:   L=%+.2f R=%+.2f%s\n", leftOut, rightOut,
                motors::isInhibited() ? " (INHIBITED)" : "");
  Serial.printf("program:  %u step(s), %s\n", static_cast<unsigned>(program.size()),
                program_runner::running() ? "RUNNING" : "idle");
  Serial.printf("encoders: L=%ld R=%ld%s\n", static_cast<long>(encoders::left()),
                static_cast<long>(encoders::right()),
                ENCODERS_ENABLED ? "" : " (disabled, moves are timed)");
  Serial.printf("cal:      %ld ticks/unit, %ld ticks/minute\n",
                static_cast<long>(settings::ticksPerUnit()),
                static_cast<long>(settings::ticksPerMinute()));
  Serial.printf("heap:     %u bytes free\n", ESP.getFreeHeap());
}

void printHelp() {
  Serial.println("console commands:");
  Serial.println("  status            pad, battery, motor, program and encoder state");
  Serial.println("  prog              list the queued program");
  Serial.println("  enc               encoder counts");
  Serial.println("  enc zero          zero both encoder counts");
  Serial.println("  cal unit <ticks>  set ticks per unit (13\"), saved in flash");
  Serial.println("  cal min <ticks>   set ticks per clock minute (15 = 90 deg), saved");
  Serial.println("  cal reset         back to the config.h defaults");
  Serial.println("  test              milestone 1 motion test (wheels OFF the bench, no pad)");
  Serial.println("  forget            erase stored gamepad pairings");
  Serial.println("  help              this list");
  Serial.println("protocol lines (as the CYD sends them) also work: FWD 1, GO, STOP");
}

// "cal unit 2150" / "cal min 64". Returns false if the line didn't parse.
bool handleCal(const char* args) {
  if (strcmp(args, "reset") == 0) {
    settings::reset();
    Serial.println("calibration reset to defaults");
    return true;
  }

  char which[8] = {};
  long ticks = 0;
  if (sscanf(args, "%7s %ld", which, &ticks) != 2 || ticks <= 0) return false;

  if (strcmp(which, "unit") == 0) {
    settings::setTicksPerUnit(static_cast<int32_t>(ticks));
  } else if (strcmp(which, "min") == 0) {
    settings::setTicksPerMinute(static_cast<int32_t>(ticks));
  } else {
    return false;
  }
  Serial.printf("saved: %ld ticks/unit, %ld ticks/minute\n",
                static_cast<long>(settings::ticksPerUnit()),
                static_cast<long>(settings::ticksPerMinute()));
  return true;
}

void handleConsole(const char* line) {
  if (strcmp(line, "help") == 0) {
    printHelp();
  } else if (strcmp(line, "status") == 0) {
    printStatus();
  } else if (strcmp(line, "prog") == 0) {
    printProgram();
  } else if (strcmp(line, "enc") == 0) {
    Serial.printf("L=%ld R=%ld\n", static_cast<long>(encoders::left()),
                  static_cast<long>(encoders::right()));
  } else if (strcmp(line, "enc zero") == 0) {
    encoders::zero();
    Serial.println("encoders zeroed");
  } else if (strcmp(line, "cal") == 0) {
    Serial.printf("%ld ticks/unit, %ld ticks/minute\n",
                  static_cast<long>(settings::ticksPerUnit()),
                  static_cast<long>(settings::ticksPerMinute()));
  } else if (strncmp(line, "cal ", 4) == 0) {
    if (!handleCal(line + 4)) Serial.println("usage: cal unit <ticks> | cal min <ticks> | cal reset");
  } else if (strcmp(line, "forget") == 0) {
    gamepad::forgetPairings();
    Serial.println("pairings erased; put the pad in pairing mode to reconnect");
  } else if (strcmp(line, "test") == 0) {
    if (gamepad::connected()) {
      Serial.println("disconnect the pad first; it owns the motors");
    } else if (program_runner::running()) {
      Serial.println("a program is running; send STOP first");
    } else if (motors::isInhibited()) {
      Serial.println("battery cutoff is active; charge and power cycle");
    } else {
      motion_test::run(motionTestShouldAbort);
    }
  } else {
    handleCommand(line, Serial);
  }
}

void pollPorts() {
  while (Serial.available()) {
    if (consoleReader.feed(static_cast<char>(Serial.read()))) {
      handleConsole(consoleReader.line());
    }
  }

  const char* line = cyd_link::poll();
  if (line) handleCommand(line, cyd_link::port());
}

void printBanner() {
  Serial.println();
  Serial.println("=========================================");
  Serial.println(" bigtrak drive board");
  Serial.println(" gamepad driving + program engine");
  Serial.println("=========================================");
  Serial.println(" put a pad in pairing mode to connect");
  Serial.println(" type \"help\" for console commands");
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);  // let the USB serial port come up before the banner

  motors::begin();  // leaves both motors coasting
  cannon::begin();
  battery::begin();
  settings::begin();
  if (ENCODERS_ENABLED) encoders::begin();
  program_runner::begin();
  cyd_link::begin();
  gamepad::begin(onPadChange);

  printBanner();
  Serial.printf("[batt] %.2fV, level %s\n", battery::volts(),
                battery::levelName(battery::level()));
  Serial.printf("[cal] %ld ticks/unit, %ld ticks/minute\n",
                static_cast<long>(settings::ticksPerUnit()),
                static_cast<long>(settings::ticksPerMinute()));
  reportBattery();
}

void loop() {
  gamepad::update();  // connect/disconnect callbacks fire in here
  updateBattery();
  updateDrive();
  program_runner::update();
  cannon::update();
  pollPorts();

  // Bluepad32 runs on another task; give it (and the idle task) room.
  vTaskDelay(1);
}
