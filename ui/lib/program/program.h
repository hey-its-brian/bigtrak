// Program builder: the Big Trak keypad's memory, minus the keypad.
//
// Plain C++ with no Arduino or display dependencies so it runs under
// `pio test -e native`. The UI turns key presses into calls on this and
// redraws from what it reports.
//
// Entry works like the original: press a command key, then up to two digits.
// The step is committed when the next command key (or GO / CK) is pressed.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace program {

// Every queueable command in the UART protocol (docs/SPEC.md).
enum class Command : uint8_t { Fwd, Back, Left, Right, Fire, Hold, Rpt };

struct Step {
  Command command;
  uint8_t arg;
};

// The original's memory held 16 steps.
constexpr size_t MAX_STEPS = 16;

// Two digits on the keypad, so 1..99. Zero is rejected; a step with no digits
// typed gets DEFAULT_ARG (one unit, one shot, one repeat...).
constexpr uint8_t MAX_DIGITS = 2;
constexpr uint8_t DEFAULT_ARG = 1;

enum class Result : uint8_t {
  Ok,
  Full,           // already MAX_STEPS steps, nothing more fits
  NoCommand,      // digit pressed with no command key before it
  TooManyDigits,  // third digit; ignored
  BadArg,         // pending step can't commit (0, or RPT longer than program)
  Empty,          // nothing to clear / check
};

class Program {
 public:
  // Commits any pending step, then starts a new one. If the pending step is
  // bad it stays pending (so the user can see it and CLS it) and this returns
  // BadArg without starting the new command.
  Result beginStep(Command command);

  // Appends a digit (0..9) to the pending step's argument.
  Result enterDigit(uint8_t digit);

  // Commits the pending step, if any. Ok when nothing was pending.
  Result commit();

  // CLR: wipe the whole program, pending step included.
  void clearAll();

  // CLS: drop the pending step if there is one, otherwise the last committed
  // step. Empty if there was nothing to drop.
  Result clearLast();

  size_t size() const { return count_; }
  bool empty() const { return count_ == 0; }
  bool full() const { return count_ >= MAX_STEPS; }
  const Step& at(size_t index) const { return steps_[index]; }

  bool hasPending() const { return pending_; }
  Command pendingCommand() const { return pendingCommand_; }
  uint8_t pendingDigits() const { return pendingDigits_; }
  // What the pending step would commit with (DEFAULT_ARG if no digits yet).
  uint8_t pendingArg() const;

 private:
  bool argValid(Command command, uint8_t arg) const;

  Step steps_[MAX_STEPS] = {};
  size_t count_ = 0;

  bool pending_ = false;
  Command pendingCommand_ = Command::Fwd;
  uint8_t pendingValue_ = 0;
  uint8_t pendingDigits_ = 0;
};

}  // namespace program
