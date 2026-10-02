#include "gamepad.h"

#include <Arduino.h>
#include <Bluepad32.h>

#include "config.h"
#include "drive_mix.h"

namespace {

ControllerPtr active = nullptr;
gamepad::ConnectionCallback onChangeCallback = nullptr;
bool fireWasDown = false;

void onConnected(ControllerPtr ctl) {
  if (active != nullptr) {
    Serial.printf("[pad] second controller (%s) refused\n", ctl->getModelName().c_str());
    ctl->disconnect();
    return;
  }

  active = ctl;
  fireWasDown = false;
  Serial.printf("[pad] connected: %s\n", ctl->getModelName().c_str());

  // Stop advertising for new pads while one is driving.
  BP32.enableNewBluetoothConnections(false);

  ctl->setPlayerLEDs(0x01);
  ctl->setColorLED(0, 80, 255);  // Big Trak blue on a DualShock light bar
  ctl->playDualRumble(0, 200, 0x60, 0x60);

  if (onChangeCallback) onChangeCallback(true);
}

void onDisconnected(ControllerPtr ctl) {
  if (ctl != active) return;  // a refused extra pad going away

  active = nullptr;
  Serial.println("[pad] disconnected");
  BP32.enableNewBluetoothConnections(true);

  if (onChangeCallback) onChangeCallback(false);
}

}  // namespace

namespace gamepad {

void begin(ConnectionCallback onChange) {
  onChangeCallback = onChange;

  const uint8_t* addr = BP32.localBdAddress();
  Serial.printf("[pad] Bluepad32 %s, BT addr %02X:%02X:%02X:%02X:%02X:%02X\n",
                BP32.firmwareVersion(), addr[0], addr[1], addr[2], addr[3],
                addr[4], addr[5]);

  BP32.setup(&onConnected, &onDisconnected, true);
  BP32.enableVirtualDevice(false);  // DS4 touchpad as a mouse: no thanks
  BP32.enableBLEService(false);
}

void update() {
  BP32.update();
}

bool connected() {
  return active != nullptr && active->isConnected();
}

bool read(Input& out) {
  out = {};
  if (!connected() || !active->hasData() || !active->isGamepad()) return false;

  // Stick up is negative Y. Flip it so forward is positive throttle.
  float rawThrottle = -drive_mix::normalizeAxis(active->axisY());
  float rawSteer = drive_mix::normalizeAxis(active->axisRX());

  out.throttle = drive_mix::applyDeadzone(rawThrottle, STICK_DEADZONE);
  out.steer = drive_mix::applyDeadzone(rawSteer, STICK_DEADZONE);
  out.turbo = active->r1();

  bool fireDown = active->a();
  out.firePressed = fireDown && !fireWasDown;
  fireWasDown = fireDown;

  return true;
}

void signalWarning() {
  if (!connected()) return;
  active->setColorLED(255, 0, 0);
  active->playDualRumble(0, 600, 0xFF, 0xFF);
}

void forgetPairings() {
  BP32.forgetBluetoothKeys();
}

const char* modelName() {
  static String name;
  if (!connected()) return "none";
  name = active->getModelName();
  return name.c_str();
}

}  // namespace gamepad
