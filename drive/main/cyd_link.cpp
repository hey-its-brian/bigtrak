#include "cyd_link.h"

#include <Arduino.h>
#include <stdarg.h>

#include "config.h"
#include "line_reader.h"

namespace {

HardwareSerial& uart = Serial2;
LineReader reader;

}  // namespace

namespace cyd_link {

void begin() {
  uart.begin(LINK_BAUD, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
}

const char* poll() {
  while (uart.available()) {
    if (reader.feed(static_cast<char>(uart.read()))) return reader.line();
  }
  return nullptr;
}

void send(const char* line) {
  uart.println(line);
  Serial.printf("[link] > %s\n", line);
}

void sendf(const char* fmt, ...) {
  char buf[LineReader::MAX_LINE + 1];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  send(buf);
}

Print& port() {
  return uart;
}

}  // namespace cyd_link
