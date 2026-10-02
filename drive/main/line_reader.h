// Assembles newline-terminated lines from a byte stream. Used for both the
// CYD link and the USB serial console. No Arduino dependency.
#pragma once

#include <stddef.h>

class LineReader {
 public:
  static constexpr size_t MAX_LINE = 64;

  // Feed one byte. Returns true when a complete, non-empty line is ready in
  // line(). CR is ignored so both "\n" and "\r\n" senders work. A line that
  // overruns the buffer is thrown away whole rather than cut in half, since
  // half a command could be a different valid command.
  bool feed(char c) {
    if (c == '\r') return false;
    if (c == '\n') {
      bool ready = len_ > 0 && !overflow_;
      buf_[ready ? len_ : 0] = '\0';
      len_ = 0;
      overflow_ = false;
      return ready;
    }
    if (len_ >= MAX_LINE) {
      overflow_ = true;
      return false;
    }
    buf_[len_++] = c;
    return false;
  }

  const char* line() const { return buf_; }

 private:
  char buf_[MAX_LINE + 1] = {};
  size_t len_ = 0;
  bool overflow_ = false;
};
