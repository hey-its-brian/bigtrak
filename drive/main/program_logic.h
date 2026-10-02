// The Big Trak program: a queue of up to 16 steps, and the flattening of RPT
// steps into the sequence that actually runs. Pure logic, covered by the
// native unit tests.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "protocol.h"

namespace program_logic {

struct Step {
  protocol::Command command;
  int arg;
};

// One entry of the flattened run. `programIndex` is where the step sits in
// the program (0-based), so a repeated step reports the row the CYD shows.
struct RunStep {
  Step step;
  uint8_t programIndex;
};

// Nested repeats can multiply a 16-step program well past anything the tank
// should be doing unattended. Refuse to run anything longer.
constexpr size_t MAX_RUN_STEPS = 256;

class Program {
 public:
  // Returns nullptr on success, or the ERR reason.
  const char* add(Step step) {
    if (count_ >= static_cast<size_t>(protocol::MAX_STEPS)) return "queue full";
    // "RPT n" repeats the n steps before it, so they have to exist. Same rule
    // the CYD applies, checked again here in case a line came from a
    // serial monitor.
    if (step.command == protocol::Command::Rpt &&
        static_cast<size_t>(step.arg) > count_) {
      return "nothing to repeat";
    }
    steps_[count_++] = step;
    return nullptr;
  }

  void clear() { count_ = 0; }
  size_t size() const { return count_; }
  bool empty() const { return count_ == 0; }
  const Step& at(size_t i) const { return steps_[i]; }

  // Flattens the program into `out`. RPT n at index i runs steps i-n..i-1
  // once more; if that range holds an RPT, it expands too, the way the
  // original replayed its memory. Returns the number of entries, or 0 if the
  // program is empty or the result would exceed `capacity`.
  size_t flatten(RunStep* out, size_t capacity) const {
    size_t n = 0;
    for (size_t i = 0; i < count_; i++) {
      if (!expand(i, out, capacity, n)) return 0;
    }
    return n;
  }

 private:
  bool expand(size_t i, RunStep* out, size_t capacity, size_t& n) const {
    const Step& s = steps_[i];
    if (s.command != protocol::Command::Rpt) {
      if (n >= capacity) return false;
      out[n++] = {s, static_cast<uint8_t>(i)};
      return true;
    }
    for (size_t j = i - s.arg; j < i; j++) {
      if (!expand(j, out, capacity, n)) return false;
    }
    return true;
  }

  Step steps_[protocol::MAX_STEPS] = {};
  size_t count_ = 0;
};

// ----------------------------------------------------------- move plans ----

// What a movement step asks of the wheels. Directions are +1 forward, -1
// back, per wheel; `ticks` is how far each wheel travels.
struct MovePlan {
  int leftDir;
  int rightDir;
  int32_t ticks;
  uint32_t timedMs;  // duration when running without encoders
};

inline bool isMove(protocol::Command c) {
  return c == protocol::Command::Fwd || c == protocol::Command::Back ||
         c == protocol::Command::Left || c == protocol::Command::Right;
}

inline MovePlan planMove(const Step& s, int32_t ticksPerUnit,
                         int32_t ticksPerMinute, uint32_t msPerUnit,
                         uint32_t msPerMinute) {
  MovePlan p = {0, 0, 0, 0};
  switch (s.command) {
    case protocol::Command::Fwd:
      p = {1, 1, s.arg * ticksPerUnit, s.arg * msPerUnit};
      break;
    case protocol::Command::Back:
      p = {-1, -1, s.arg * ticksPerUnit, s.arg * msPerUnit};
      break;
    // Skid steer spins in place: one side forward, the other back.
    case protocol::Command::Left:
      p = {-1, 1, s.arg * ticksPerMinute, s.arg * msPerMinute};
      break;
    case protocol::Command::Right:
      p = {1, -1, s.arg * ticksPerMinute, s.arg * msPerMinute};
      break;
    default:
      break;
  }
  return p;
}

// Speed for a move given how far along it is: cruise, then slow for the
// final stretch so the tank stops on its mark.
inline float moveSpeed(int32_t done, int32_t total, float cruise, float slow,
                       float slowdownFraction, int32_t slowdownMaxTicks) {
  int32_t slowdown = static_cast<int32_t>(total * slowdownFraction);
  if (slowdown > slowdownMaxTicks) slowdown = slowdownMaxTicks;
  return (total - done) <= slowdown ? slow : cruise;
}

}  // namespace program_logic
