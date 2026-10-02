// The UART protocol from docs/SPEC.md: formatting the lines the UI sends and
// parsing the status lines the drive board sends back.
//
// Newline-terminated ASCII at 115200. No Arduino dependencies, so this is
// tested on the host with `pio test -e native`.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "program.h"

namespace protocol {

// Longest line either side sends, newline excluded. Anything longer is
// dropped as garbage.
constexpr size_t MAX_LINE = 48;

// Non-step commands, sent as-is.
constexpr const char* GO = "GO";
constexpr const char* CLS = "CLS";  // clear queue / abort
constexpr const char* STOP = "STOP";

// "FWD", "BACK", "LEFT", "RIGHT", "FIRE", "HOLD", "RPT".
const char* commandName(program::Command command);

// Writes "FWD 5" (no newline) into out. Returns the length written, or 0 if
// it didn't fit.
size_t formatStep(const program::Step& step, char* out, size_t outSize);

// ------------------------------------------------------------- status ----

enum class StatusType : uint8_t {
  Ack,
  Err,
  Step,
  Done,
  Batt,
  PadConnected,
  PadDisconnected,
};

struct Status {
  StatusType type = StatusType::Ack;
  int step = 0;        // STEP n
  float volts = 0.0f;  // BATT v.vv
  char reason[MAX_LINE + 1] = {};  // ERR <reason>, may be empty
};

// Parses one line (no newline) into out. Leading/trailing whitespace is
// ignored and keywords are case-insensitive, so a human typing `step 2` into
// a serial monitor works too. Returns false for anything unrecognised or
// malformed (STEP without a number, BATT without a voltage...).
bool parseStatus(const char* line, Status& out);

// Accumulates bytes from a serial port into lines. Handles \n, \r\n and bare
// \r endings, skips empty lines, and throws away lines longer than MAX_LINE.
class LineBuffer {
 public:
  // Feed one byte. Returns true when a complete line is ready in line().
  bool feed(char c);
  const char* line() const { return buffer_; }

 private:
  char buffer_[MAX_LINE + 1] = {};
  size_t length_ = 0;
  bool overflow_ = false;
};

}  // namespace protocol
