#include "drive_link.h"

#include <Arduino.h>

#include "config.h"

namespace {

drive_link::StatusHandler onStatus = nullptr;
protocol::LineBuffer incoming;

Stream& port() {
  if (LINK_OVER_USB) return Serial;
  return Serial1;
}

}  // namespace

namespace drive_link {

void begin(StatusHandler handler) {
  onStatus = handler;
  if (!LINK_OVER_USB) {
    // Any free GPIO works through the ESP32 pin matrix.
    Serial1.begin(LINK_BAUD, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
  }
}

void send(const char* line) {
  port().print(line);
  port().print('\n');  // spec is bare newline; println would add \r
  if (!LINK_OVER_USB) Serial.printf("# > %s\n", line);
}

void send(const program::Step& step) {
  char line[protocol::MAX_LINE + 1];
  if (protocol::formatStep(step, line, sizeof(line)) > 0) send(line);
}

void poll() {
  Stream& in = port();
  while (in.available()) {
    if (!incoming.feed(static_cast<char>(in.read()))) continue;

    protocol::Status status;
    if (protocol::parseStatus(incoming.line(), status)) {
      if (onStatus) onStatus(status);
    } else {
      // Over USB this is usually a typo; over UART, noise on a floating RX
      // line. Either way it goes to the USB log, never back down the link.
      Serial.printf("# ? %s\n", incoming.line());
    }
  }
}

}  // namespace drive_link
