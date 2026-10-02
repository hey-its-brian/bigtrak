#include "protocol.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace protocol {

namespace {

bool equalsIgnoreCase(const char* a, const char* b) {
  while (*a && *b) {
    if (toupper(static_cast<unsigned char>(*a)) !=
        toupper(static_cast<unsigned char>(*b))) {
      return false;
    }
    a++;
    b++;
  }
  return *a == *b;
}

const char* skipSpace(const char* s) {
  while (*s && isspace(static_cast<unsigned char>(*s))) s++;
  return s;
}

// Splits "KEYWORD rest of line" in place: terminates the keyword and returns
// a pointer to the (trimmed) rest, which may be empty.
char* splitKeyword(char* s) {
  while (*s && !isspace(static_cast<unsigned char>(*s))) s++;
  if (*s) *s++ = '\0';
  return const_cast<char*>(skipSpace(s));
}

void trimRight(char* s) {
  size_t length = strlen(s);
  while (length > 0 && isspace(static_cast<unsigned char>(s[length - 1]))) {
    s[--length] = '\0';
  }
}

}  // namespace

const char* commandName(program::Command command) {
  switch (command) {
    case program::Command::Fwd: return "FWD";
    case program::Command::Back: return "BACK";
    case program::Command::Left: return "LEFT";
    case program::Command::Right: return "RIGHT";
    case program::Command::Fire: return "FIRE";
    case program::Command::Hold: return "HOLD";
    case program::Command::Rpt: return "RPT";
  }
  return "?";
}

size_t formatStep(const program::Step& step, char* out, size_t outSize) {
  if (out == nullptr || outSize == 0) return 0;
  int written = snprintf(out, outSize, "%s %u", commandName(step.command),
                         static_cast<unsigned>(step.arg));
  if (written < 0 || static_cast<size_t>(written) >= outSize) {
    out[0] = '\0';
    return 0;
  }
  return static_cast<size_t>(written);
}

bool parseStatus(const char* line, Status& out) {
  if (line == nullptr) return false;

  char work[MAX_LINE + 1];
  line = skipSpace(line);
  if (strlen(line) > MAX_LINE) return false;
  strcpy(work, line);
  trimRight(work);
  if (work[0] == '\0') return false;

  char* rest = splitKeyword(work);
  const char* keyword = work;

  if (equalsIgnoreCase(keyword, "ACK")) {
    if (*rest) return false;
    out.type = StatusType::Ack;
    return true;
  }

  if (equalsIgnoreCase(keyword, "DONE")) {
    if (*rest) return false;
    out.type = StatusType::Done;
    return true;
  }

  if (equalsIgnoreCase(keyword, "ERR")) {
    out.type = StatusType::Err;
    strcpy(out.reason, rest);  // fits: rest is a tail of work
    return true;
  }

  if (equalsIgnoreCase(keyword, "STEP")) {
    char* end = nullptr;
    long n = strtol(rest, &end, 10);
    if (end == rest || *end != '\0' || n < 0 || n > 255) return false;
    out.type = StatusType::Step;
    out.step = static_cast<int>(n);
    return true;
  }

  if (equalsIgnoreCase(keyword, "BATT")) {
    char* end = nullptr;
    float v = strtof(rest, &end);
    if (end == rest || *end != '\0' || v < 0.0f || v > 99.0f) return false;
    out.type = StatusType::Batt;
    out.volts = v;
    return true;
  }

  if (equalsIgnoreCase(keyword, "PAD")) {
    if (equalsIgnoreCase(rest, "CONNECTED")) {
      out.type = StatusType::PadConnected;
      return true;
    }
    if (equalsIgnoreCase(rest, "DISCONNECTED")) {
      out.type = StatusType::PadDisconnected;
      return true;
    }
    return false;
  }

  return false;
}

bool LineBuffer::feed(char c) {
  if (c == '\n' || c == '\r') {
    bool ready = !overflow_ && length_ > 0;
    buffer_[ready ? length_ : 0] = '\0';
    length_ = 0;
    overflow_ = false;
    return ready;
  }

  if (length_ >= MAX_LINE) {
    overflow_ = true;  // keep swallowing until the end of this line
    return false;
  }
  buffer_[length_++] = c;
  return false;
}

}  // namespace protocol
