// bigtrak drive board, milestone 2: gamepad driving.
//
// A Bluetooth pad drives the tank (left stick throttle, right stick steer,
// R1 turbo, Cross/A fires the cannon). Disconnecting the pad stops the
// motors. The battery is watched the whole time: BATT goes to the CYD every
// few seconds, and a flat pack cuts the motors until power is cycled.
//
// USB serial (115200) takes lowercase console commands (type "help"), and
// also accepts protocol lines exactly as the CYD would send them, so this
// board can be tested with the touchscreen unplugged.
//
// Entry point is app_main() in main.c, which starts Bluetooth and then runs
// setup()/loop() on the Arduino task.

#include <Arduino.h>

#include "battery.h"
#include "cannon.h"
#include "config.h"
#include "cyd_link.h"
#include "drive_mix.h"
#include "gamepad.h"
#include "line_reader.h"
#include "motion_test.h"
#include "motors.h"
#include "protocol.h"

namespace {

constexpr uint32_t DRIVE_TICK_MS = 10;

// Current wheel outputs after ramping, -1..1.
float leftOut = 0.0f;
float rightOut = 0.0f;
uint32_t lastDriveTickAt = 0;
uint32_t lastBattReportAt = 0;

LineReader consoleReader;

// Hard stop: no ramp, brake both motors, drop any cannon volley.
void stopAll() {
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
  } else {
    // The failsafe. The pad is gone, so nobody is steering.
    stopAll();
    cyd_link::send("PAD DISCONNECTED");
  }
}

void driveFromPad(float dtSec) {
  gamepad::Input in;
  if (!gamepad::read(in)) return;

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
      stopAll();
      motors::setInhibit(true);
      gamepad::signalWarning();
      Serial.println("[batt] *** CUTOFF: motors disabled until power cycle ***");
    }
    reportBattery();
  }

  if (millis() - lastBattReportAt >= BATT_REPORT_MS) reportBattery();
}

// Executes one protocol line from either port. Replies go back to `reply`.
void handleCommand(const char* line, Print& reply) {
  protocol::Parsed p = protocol::parse(line);
  if (p.command == protocol::Command::Invalid) {
    reply.printf("ERR %s\n", p.error);
    return;
  }

  switch (p.command) {
    case protocol::Command::Stop:
    case protocol::Command::Cls:
      stopAll();
      reply.println("ACK");
      break;
    default:
      // Queue commands are valid but have nowhere to go until the program
      // engine lands (milestone 5).
      reply.println("ERR not implemented");
      break;
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

void printStatus() {
  Serial.printf("pad:     %s (%s)\n", gamepad::connected() ? "connected" : "none",
                gamepad::modelName());
  Serial.printf("battery: %.2fV, %s\n", battery::volts(),
                battery::levelName(battery::level()));
  Serial.printf("motors:  L=%+.2f R=%+.2f%s\n", leftOut, rightOut,
                motors::isInhibited() ? " (INHIBITED)" : "");
  Serial.printf("heap:    %u bytes free\n", ESP.getFreeHeap());
}

void printHelp() {
  Serial.println("console commands:");
  Serial.println("  status   pad, battery and motor state");
  Serial.println("  test     milestone 1 motion test (wheels OFF the bench, no pad)");
  Serial.println("  forget   erase stored gamepad pairings");
  Serial.println("  help     this list");
  Serial.println("protocol lines (as the CYD sends them) also work, e.g. STOP");
}

void handleConsole(const char* line) {
  if (strcmp(line, "help") == 0) {
    printHelp();
  } else if (strcmp(line, "status") == 0) {
    printStatus();
  } else if (strcmp(line, "forget") == 0) {
    gamepad::forgetPairings();
    Serial.println("pairings erased; put the pad in pairing mode to reconnect");
  } else if (strcmp(line, "test") == 0) {
    if (gamepad::connected()) {
      Serial.println("disconnect the pad first; it owns the motors");
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
  Serial.println(" bigtrak drive board, milestone 2");
  Serial.println(" gamepad driving");
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
  cyd_link::begin();
  gamepad::begin(onPadChange);

  printBanner();
  Serial.printf("[batt] %.2fV, level %s\n", battery::volts(),
                battery::levelName(battery::level()));
  reportBattery();
}

void loop() {
  gamepad::update();  // connect/disconnect callbacks fire in here
  updateBattery();
  updateDrive();
  cannon::update();
  pollPorts();

  // Bluepad32 runs on another task; give it (and the idle task) room.
  vTaskDelay(1);
}
