// The UART link to the drive board (protocol in docs/SPEC.md).
//
// Milestone 3 runs it over USB Serial so the UI can be driven from a serial
// monitor: outgoing lines are printed there, and typing `STEP 2` or
// `BATT 11.8` feeds the status handler. Milestone 4 sets LINK_OVER_USB to
// false in config.h and the same calls go out on UART1 instead.
#pragma once

#include "protocol.h"

namespace drive_link {

using StatusHandler = void (*)(const protocol::Status& status);

// handler is called from poll() for every status line that parses.
void begin(StatusHandler handler);

// Sends one protocol line; the newline is added here.
void send(const char* line);
void send(const program::Step& step);

// Reads whatever has arrived and dispatches complete lines. Call every loop.
void poll();

}  // namespace drive_link
