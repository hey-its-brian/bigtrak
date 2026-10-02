// Milestone 3: CYD standalone.
//
// The keypad draws, takes touches, beeps, and builds a program exactly as it
// will once the drive board is attached. Protocol lines go out on USB serial
// (115200) instead of the UART, and status lines typed into the serial
// monitor drive the display, so with nothing else plugged in you can try:
//
//   STEP 2           highlight step 2 of the program
//   DONE             clear the highlight
//   BATT 11.8        battery readout (green / amber / red)
//   PAD CONNECTED    pad indicator
//   ERR queue full   error line
//
// Lines the firmware prints for humans start with '#' so they can't be
// mistaken for protocol.
//
// Program sync: GO sends CLS, then every step, then GO. The drive board's
// queue is rebuilt from scratch on each run, so the two boards can't drift
// apart (a drive board reset, a dropped line) between runs.

#include <Arduino.h>

#include "beeper.h"
#include "config.h"
#include "drive_link.h"
#include "keypad.h"
#include "program.h"
#include "protocol.h"
#include "screen.h"

using keypad::Key;
using program::Command;
using program::Result;

namespace {

program::Program prog;

// Index into prog of the step the drive board is executing, or -1.
int highlight = -1;

// STEP n numbers steps within the queue that was sent. GO sends the whole
// program (offset 0); CK sends only the last step (offset = its index).
int runOffset = 0;

// ------------------------------------------------------------- touch ----
bool fingerDown = false;
uint32_t lastContactMs = 0;
const keypad::KeyDef* heldKey = nullptr;

void showReady() {
  if (prog.full()) {
    screen::drawMessage("FULL (16 STEPS)", screen::Tone::Busy);
  } else {
    screen::drawMessage("READY", screen::Tone::Normal);
  }
}

void redrawProgram() { screen::drawProgram(prog, highlight); }

// A key that couldn't be accepted: say why and buzz.
void reject(Result result) {
  const char* why = "";
  switch (result) {
    case Result::Full: why = "FULL (16 STEPS)"; break;
    case Result::BadArg: why = "BAD NUMBER"; break;
    case Result::NoCommand: why = "COMMAND FIRST"; break;
    case Result::TooManyDigits: why = "2 DIGITS MAX"; break;
    case Result::Empty: why = "NO PROGRAM"; break;
    case Result::Ok: return;
  }
  beeper::error();
  screen::drawMessage(why, screen::Tone::Error);
}

Command commandFor(Key key) {
  switch (key) {
    case Key::Fwd: return Command::Fwd;
    case Key::Back: return Command::Back;
    case Key::Left: return Command::Left;
    case Key::Right: return Command::Right;
    case Key::Fire: return Command::Fire;
    case Key::Hold: return Command::Hold;
    case Key::Rpt:
    default: return Command::Rpt;
  }
}

void startRun(int offset) {
  runOffset = offset;
  highlight = -1;
  drive_link::send(protocol::GO);
  beeper::go();
  screen::drawMessage("RUNNING", screen::Tone::Busy);
  redrawProgram();
}

// GO: commit what's being typed, then send the whole program and run it.
void onGo() {
  Result result = prog.commit();
  if (result != Result::Ok) return reject(result);
  if (prog.empty()) return reject(Result::Empty);

  drive_link::send(protocol::CLS);
  for (size_t i = 0; i < prog.size(); i++) drive_link::send(prog.at(i));
  startRun(0);
}

// CK (check): run just the last step, so you can see what it does before
// adding more. A lone RPT has nothing to repeat, so that's refused.
void onCheck() {
  Result result = prog.commit();
  if (result != Result::Ok) return reject(result);
  if (prog.empty()) return reject(Result::Empty);

  const program::Step& last = prog.at(prog.size() - 1);
  if (last.command == Command::Rpt) return reject(Result::BadArg);

  drive_link::send(protocol::CLS);
  drive_link::send(last);
  startRun(static_cast<int>(prog.size()) - 1);
}

void onKey(Key key) {
  if (keypad::isDigit(key)) {
    Result result = prog.enterDigit(keypad::digitValue(key));
    if (result != Result::Ok) return reject(result);
    beeper::key();
    redrawProgram();
    return;
  }

  switch (key) {
    case Key::Fwd:
    case Key::Back:
    case Key::Left:
    case Key::Right:
    case Key::Fire:
    case Key::Hold:
    case Key::Rpt: {
      Result result = prog.beginStep(commandFor(key));
      if (result != Result::Ok) {
        reject(result);
        redrawProgram();  // a full program may just have committed its 16th
        return;
      }
      beeper::key();
      showReady();
      redrawProgram();
      return;
    }

    case Key::Go:
      onGo();
      return;

    case Key::Ck:
      onCheck();
      return;

    case Key::Clr:
      // Wipe the program, and the drive board's queue with it (CLS also
      // aborts a run in progress).
      prog.clearAll();
      highlight = -1;
      drive_link::send(protocol::CLS);
      beeper::key();
      screen::drawMessage("CLEARED", screen::Tone::Normal);
      redrawProgram();
      return;

    case Key::Cls: {
      // Clear last step: local only. The drive board gets the edited program
      // on the next GO.
      Result result = prog.clearLast();
      if (result != Result::Ok) return reject(result);
      if (highlight >= static_cast<int>(prog.size())) highlight = -1;
      beeper::key();
      showReady();
      redrawProgram();
      return;
    }

    case Key::Stop:
      drive_link::send(protocol::STOP);
      highlight = -1;
      beeper::key();
      screen::drawMessage("STOPPED", screen::Tone::Error);
      redrawProgram();
      return;

    default:
      return;
  }
}

// Fires once per touch, on touch-down. Holding a key does nothing more, and
// the finger has to leave the glass for TOUCH_RELEASE_MS before the next
// press registers.
void pollTouch() {
  static uint32_t lastPollMs = 0;
  uint32_t now = millis();
  if (now - lastPollMs < TOUCH_POLL_MS) return;
  lastPollMs = now;

  int x, y;
  if (screen::readTouch(x, y)) {
    lastContactMs = now;
    if (fingerDown) return;  // still the same press

    fingerDown = true;
    heldKey = keypad::hitTest(x, y);
    if (heldKey) {
      screen::drawKey(*heldKey, true);
      onKey(heldKey->key);
    }
    return;
  }

  if (fingerDown && now - lastContactMs >= TOUCH_RELEASE_MS) {
    fingerDown = false;
    if (heldKey) screen::drawKey(*heldKey, false);
    heldKey = nullptr;
  }
}

// ----------------------------------------------------- drive -> UI ----

void onStatus(const protocol::Status& status) {
  switch (status.type) {
    case protocol::StatusType::Ack:
      break;  // nothing to show; the absence of ERR is the news

    case protocol::StatusType::Err: {
      char text[protocol::MAX_LINE + 8];
      snprintf(text, sizeof(text), "ERR %s", status.reason);
      screen::drawMessage(text, screen::Tone::Error);
      break;
    }

    case protocol::StatusType::Step: {
      int index = runOffset + status.step - STEP_INDEX_BASE;
      highlight = (index >= 0 && index < static_cast<int>(prog.size()))
                      ? index
                      : -1;
      char text[24];
      snprintf(text, sizeof(text), "RUNNING %d", status.step);
      screen::drawMessage(text, screen::Tone::Busy);
      redrawProgram();
      break;
    }

    case protocol::StatusType::Done:
      highlight = -1;
      screen::drawMessage("DONE", screen::Tone::Good);
      redrawProgram();
      break;

    case protocol::StatusType::Batt:
      screen::drawBattery(true, status.volts);
      break;

    case protocol::StatusType::PadConnected:
      screen::drawPad(screen::Pad::Connected);
      break;

    case protocol::StatusType::PadDisconnected:
      screen::drawPad(screen::Pad::Disconnected);
      break;
  }
}

void printBanner() {
  Serial.println();
  Serial.println("# =========================================");
  Serial.println("# bigtrak UI board, milestone 3");
  Serial.println("# protocol lines print here; type status");
  Serial.println("# lines (STEP 2, BATT 11.8, DONE) to test");
  Serial.println("# =========================================");
}

}  // namespace

void setup() {
  Serial.begin(USB_BAUD);
  delay(300);  // let the USB serial port come up before the banner

  beeper::begin();
  screen::begin();
  screen::drawKeypad();
  screen::drawBattery(false, 0.0f);
  screen::drawPad(screen::Pad::Unknown);
  showReady();
  redrawProgram();

  drive_link::begin(onStatus);
  printBanner();
}

void loop() {
  pollTouch();
  drive_link::poll();
  beeper::update();
  delay(1);
}
