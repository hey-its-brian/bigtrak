#include "program.h"

namespace program {

Result Program::beginStep(Command command) {
  Result committed = commit();
  if (committed != Result::Ok) return committed;
  if (full()) return Result::Full;

  pending_ = true;
  pendingCommand_ = command;
  pendingValue_ = 0;
  pendingDigits_ = 0;
  return Result::Ok;
}

Result Program::enterDigit(uint8_t digit) {
  if (!pending_) return Result::NoCommand;
  if (digit > 9) return Result::BadArg;
  if (pendingDigits_ >= MAX_DIGITS) return Result::TooManyDigits;

  pendingValue_ = static_cast<uint8_t>(pendingValue_ * 10 + digit);
  pendingDigits_++;
  return Result::Ok;
}

Result Program::commit() {
  if (!pending_) return Result::Ok;

  uint8_t arg = pendingArg();
  if (!argValid(pendingCommand_, arg)) return Result::BadArg;

  steps_[count_++] = {pendingCommand_, arg};
  pending_ = false;
  return Result::Ok;
}

void Program::clearAll() {
  count_ = 0;
  pending_ = false;
}

Result Program::clearLast() {
  if (pending_) {
    pending_ = false;
    return Result::Ok;
  }
  if (count_ == 0) return Result::Empty;
  count_--;
  return Result::Ok;
}

uint8_t Program::pendingArg() const {
  return pendingDigits_ == 0 ? DEFAULT_ARG : pendingValue_;
}

bool Program::argValid(Command command, uint8_t arg) const {
  if (arg == 0) return false;
  // RPT n replays the n steps before it, so they have to exist. A RPT inside
  // the replayed range is the drive board's problem, not the keypad's.
  if (command == Command::Rpt && arg > count_) return false;
  return true;
}

}  // namespace program
