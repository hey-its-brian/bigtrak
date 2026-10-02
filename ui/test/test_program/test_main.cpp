// Program builder: entry, commit, clear, and the 16-step cap.
#include <unity.h>

#include "program.h"

using program::Command;
using program::Program;
using program::Result;

void setUp() {}
void tearDown() {}

namespace {

// FWD 5, LEFT 15 style entry: command key, then digits.
void enter(Program& p, Command command, int value) {
  TEST_ASSERT_EQUAL(Result::Ok, p.beginStep(command));
  if (value >= 10) {
    TEST_ASSERT_EQUAL(Result::Ok, p.enterDigit(value / 10));
  }
  TEST_ASSERT_EQUAL(Result::Ok, p.enterDigit(value % 10));
}

}  // namespace

void test_starts_empty() {
  Program p;
  TEST_ASSERT_TRUE(p.empty());
  TEST_ASSERT_FALSE(p.hasPending());
}

void test_step_commits_on_next_command() {
  Program p;
  enter(p, Command::Fwd, 5);
  TEST_ASSERT_EQUAL(0, p.size());  // still pending
  TEST_ASSERT_TRUE(p.hasPending());
  TEST_ASSERT_EQUAL(5, p.pendingArg());

  enter(p, Command::Left, 15);
  TEST_ASSERT_EQUAL(1, p.size());
  TEST_ASSERT_EQUAL(Command::Fwd, p.at(0).command);
  TEST_ASSERT_EQUAL(5, p.at(0).arg);

  TEST_ASSERT_EQUAL(Result::Ok, p.commit());  // what GO does
  TEST_ASSERT_EQUAL(2, p.size());
  TEST_ASSERT_EQUAL(Command::Left, p.at(1).command);
  TEST_ASSERT_EQUAL(15, p.at(1).arg);
  TEST_ASSERT_FALSE(p.hasPending());
}

void test_commit_with_nothing_pending_is_ok() {
  Program p;
  TEST_ASSERT_EQUAL(Result::Ok, p.commit());
  TEST_ASSERT_EQUAL(0, p.size());
}

void test_no_digits_uses_default_arg() {
  Program p;
  p.beginStep(Command::Fire);
  TEST_ASSERT_EQUAL(0, p.pendingDigits());
  TEST_ASSERT_EQUAL(Result::Ok, p.commit());
  TEST_ASSERT_EQUAL(program::DEFAULT_ARG, p.at(0).arg);
}

void test_digit_without_command_rejected() {
  Program p;
  TEST_ASSERT_EQUAL(Result::NoCommand, p.enterDigit(3));
  TEST_ASSERT_TRUE(p.empty());
}

void test_third_digit_ignored() {
  Program p;
  enter(p, Command::Back, 42);
  TEST_ASSERT_EQUAL(Result::TooManyDigits, p.enterDigit(7));
  TEST_ASSERT_EQUAL(42, p.pendingArg());
}

void test_zero_arg_stays_pending() {
  Program p;
  enter(p, Command::Fwd, 0);
  TEST_ASSERT_EQUAL(Result::BadArg, p.beginStep(Command::Back));
  TEST_ASSERT_EQUAL(0, p.size());
  TEST_ASSERT_TRUE(p.hasPending());
  TEST_ASSERT_EQUAL(Command::Fwd, p.pendingCommand());  // BACK not started
}

void test_rpt_cannot_exceed_program() {
  Program p;
  enter(p, Command::Fwd, 2);
  enter(p, Command::Rpt, 2);  // only one step before it
  TEST_ASSERT_EQUAL(Result::BadArg, p.commit());
  TEST_ASSERT_EQUAL(1, p.size());

  p.clearLast();
  enter(p, Command::Rpt, 1);
  TEST_ASSERT_EQUAL(Result::Ok, p.commit());
  TEST_ASSERT_EQUAL(2, p.size());
}

void test_clear_last_drops_pending_first() {
  Program p;
  enter(p, Command::Fwd, 1);
  enter(p, Command::Right, 30);
  TEST_ASSERT_EQUAL(Result::Ok, p.clearLast());  // drops RIGHT 30
  TEST_ASSERT_FALSE(p.hasPending());
  TEST_ASSERT_EQUAL(1, p.size());

  TEST_ASSERT_EQUAL(Result::Ok, p.clearLast());  // drops FWD 1
  TEST_ASSERT_TRUE(p.empty());
  TEST_ASSERT_EQUAL(Result::Empty, p.clearLast());
}

void test_clear_all() {
  Program p;
  enter(p, Command::Fwd, 1);
  enter(p, Command::Back, 1);
  p.clearAll();
  TEST_ASSERT_TRUE(p.empty());
  TEST_ASSERT_FALSE(p.hasPending());
}

void test_cap_at_sixteen_steps() {
  Program p;
  for (size_t i = 0; i < program::MAX_STEPS; i++) {
    enter(p, Command::Hold, 3);
  }
  // 15 committed + 1 pending; the next command commits the 16th and is full.
  TEST_ASSERT_EQUAL(Result::Full, p.beginStep(Command::Fwd));
  TEST_ASSERT_EQUAL(program::MAX_STEPS, p.size());
  TEST_ASSERT_TRUE(p.full());
  TEST_ASSERT_FALSE(p.hasPending());
  TEST_ASSERT_EQUAL(Result::Full, p.beginStep(Command::Fwd));

  // Clearing one makes room again.
  p.clearLast();
  TEST_ASSERT_EQUAL(Result::Ok, p.beginStep(Command::Fwd));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_starts_empty);
  RUN_TEST(test_step_commits_on_next_command);
  RUN_TEST(test_commit_with_nothing_pending_is_ok);
  RUN_TEST(test_no_digits_uses_default_arg);
  RUN_TEST(test_digit_without_command_rejected);
  RUN_TEST(test_third_digit_ignored);
  RUN_TEST(test_zero_arg_stays_pending);
  RUN_TEST(test_rpt_cannot_exceed_program);
  RUN_TEST(test_clear_last_drops_pending_first);
  RUN_TEST(test_clear_all);
  RUN_TEST(test_cap_at_sixteen_steps);
  return UNITY_END();
}
