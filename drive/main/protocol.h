// Parser for the CYD -> drive command lines in docs/SPEC.md. Pure logic, no
// Arduino, covered by the native unit tests.
//
// The queue commands are parsed and validated now so the CYD gets a precise
// ERR for a malformed line; executing them arrives with the program engine
// (milestone 5).
#pragma once

#include <stdlib.h>
#include <string.h>

namespace protocol {

enum class Command {
  Invalid,
  Fwd,
  Back,
  Left,
  Right,
  Fire,
  Hold,
  Rpt,
  Go,
  Cls,
  Stop,
};

struct Parsed {
  Command command = Command::Invalid;
  int arg = 0;
  const char* error = nullptr;  // set when command is Invalid
};

// The original Big Trak took two-digit entries, and a program held 16 steps.
constexpr int MAX_ARG = 99;
constexpr int MAX_STEPS = 16;

struct Spec {
  const char* name;
  Command command;
  bool takesArg;
  int maxArg;
};

constexpr Spec SPECS[] = {
    {"FWD", Command::Fwd, true, MAX_ARG},
    {"BACK", Command::Back, true, MAX_ARG},
    {"LEFT", Command::Left, true, MAX_ARG},
    {"RIGHT", Command::Right, true, MAX_ARG},
    {"FIRE", Command::Fire, true, MAX_ARG},
    {"HOLD", Command::Hold, true, MAX_ARG},
    {"RPT", Command::Rpt, true, MAX_STEPS},
    {"GO", Command::Go, false, 0},
    {"CLS", Command::Cls, false, 0},
    {"STOP", Command::Stop, false, 0},
};

inline Parsed fail(const char* why) {
  Parsed p;
  p.error = why;
  return p;
}

// Accepts exactly "NAME" or "NAME n", single space, as the spec writes them.
// Being strict here catches a corrupted line instead of guessing at it.
inline Parsed parse(const char* line) {
  const char* space = strchr(line, ' ');
  size_t nameLen = space ? static_cast<size_t>(space - line) : strlen(line);

  for (const Spec& spec : SPECS) {
    if (strlen(spec.name) != nameLen || strncmp(line, spec.name, nameLen) != 0)
      continue;

    if (!spec.takesArg) {
      if (space) return fail("unexpected argument");
      Parsed p;
      p.command = spec.command;
      return p;
    }

    if (!space || space[1] == '\0') return fail("missing argument");
    const char* digits = space + 1;
    for (const char* c = digits; *c; c++) {
      if (*c < '0' || *c > '9') return fail("bad argument");
    }
    if (strlen(digits) > 3) return fail("argument out of range");
    int n = atoi(digits);
    if (n < 1 || n > spec.maxArg) return fail("argument out of range");

    Parsed p;
    p.command = spec.command;
    p.arg = n;
    return p;
  }

  return fail("unknown command");
}

}  // namespace protocol
