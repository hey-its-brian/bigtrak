// UART2 link to the CYD (GPIO17 TX, GPIO16 RX, 115200, newline-terminated
// ASCII per docs/SPEC.md).
#pragma once

#include <Print.h>

namespace cyd_link {

void begin();

// Returns the next complete line from the CYD, or nullptr if none is ready.
// The pointer is valid until the next call.
const char* poll();

// Sends one status line to the CYD and echoes it to USB serial, so a serial
// monitor on the Mac sees the same traffic the touchscreen does.
void send(const char* line);
void sendf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

// The raw port, for replies to CYD commands.
Print& port();

}  // namespace cyd_link
