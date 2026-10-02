// Bluetooth gamepad via Bluepad32. One pad drives the tank; any others that
// connect are turned away.
//
// Controls (PS4 names, Xbox/8BitDo equivalents in brackets):
//   left stick Y   throttle
//   right stick X  steer
//   R1 [RB] held   turbo, full speed instead of SPEED_NORMAL
//   Cross [A]      fire the photon cannon
#pragma once

namespace gamepad {

struct Input {
  float throttle;  // -1..1 after deadzone, positive is forward
  float steer;     // -1..1 after deadzone, positive is right
  bool turbo;
  bool firePressed;  // true once per press, not while held
};

// connected is true on connect, false on disconnect. Runs on the main loop
// task (from inside update()), so it's safe to touch the motors from it.
using ConnectionCallback = void (*)(bool connected);

void begin(ConnectionCallback onChange);

// Pumps Bluepad32. Call every loop; connect/disconnect callbacks fire here.
void update();

bool connected();

// Latest input from the active pad. Returns false (and zeroes `out`) when no
// pad is connected.
bool read(Input& out);

// Buzz and recolor the pad, e.g. for a low-battery warning.
void signalWarning();

// Erase stored pairings, so a pad that was paired before has to pair fresh.
void forgetPairings();

const char* modelName();

}  // namespace gamepad
