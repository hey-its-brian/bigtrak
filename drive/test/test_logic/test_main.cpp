// Host-side tests for the drive board's hardware-free logic.
//   cd drive && pio test -e native

#include <unity.h>

#include <initializer_list>

#include "battery_logic.h"
#include "drive_mix.h"
#include "line_reader.h"
#include "program_logic.h"
#include "protocol.h"

void setUp() {}
void tearDown() {}

// ------------------------------------------------------------ drive_mix ----

void test_normalize_axis_clamps_and_scales() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, drive_mix::normalizeAxis(512));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.998f, drive_mix::normalizeAxis(-511));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, drive_mix::normalizeAxis(0));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, drive_mix::normalizeAxis(9999));
}

void test_deadzone_zeroes_small_inputs() {
  TEST_ASSERT_EQUAL_FLOAT(0.0f, drive_mix::applyDeadzone(0.05f, 0.1f));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, drive_mix::applyDeadzone(-0.1f, 0.1f));
}

void test_deadzone_rescales_without_a_jump() {
  // Just past the edge is near zero, full travel is still full.
  TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.0f, drive_mix::applyDeadzone(0.11f, 0.1f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, drive_mix::applyDeadzone(1.0f, 0.1f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, drive_mix::applyDeadzone(-1.0f, 0.1f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, drive_mix::applyDeadzone(0.55f, 0.1f));
}

void test_mix_straight_and_spin() {
  drive_mix::Wheels w = drive_mix::mix(0.5f, 0.0f);
  TEST_ASSERT_EQUAL_FLOAT(0.5f, w.left);
  TEST_ASSERT_EQUAL_FLOAT(0.5f, w.right);

  // Steer right with no throttle: spin in place clockwise.
  w = drive_mix::mix(0.0f, 0.6f);
  TEST_ASSERT_EQUAL_FLOAT(0.6f, w.left);
  TEST_ASSERT_EQUAL_FLOAT(-0.6f, w.right);
}

void test_mix_normalizes_instead_of_clipping() {
  // Full throttle plus some right steer must still turn right.
  drive_mix::Wheels w = drive_mix::mix(1.0f, 0.5f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, w.left);
  TEST_ASSERT_TRUE(w.right < w.left);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f / 1.5f, w.right);
}

void test_slew_accelerates_gently() {
  float v = drive_mix::slew(0.0f, 1.0f, 2.0f, 8.0f, 0.1f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.2f, v);
}

void test_slew_stops_quickly() {
  float v = drive_mix::slew(1.0f, 0.0f, 2.0f, 8.0f, 0.1f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.2f, v);
}

void test_slew_reversal_decelerates_first() {
  // Heading from +0.5 to -1: the move toward zero uses the fast rate.
  float v = drive_mix::slew(0.5f, -1.0f, 2.0f, 8.0f, 0.05f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.1f, v);
}

void test_slew_does_not_overshoot() {
  TEST_ASSERT_EQUAL_FLOAT(0.3f, drive_mix::slew(0.25f, 0.3f, 2.0f, 8.0f, 1.0f));
}

void test_to_duty() {
  TEST_ASSERT_EQUAL(255, drive_mix::toDuty(1.0f, 255));
  TEST_ASSERT_EQUAL(-128, drive_mix::toDuty(-0.5f, 255));
  TEST_ASSERT_EQUAL(0, drive_mix::toDuty(0.0f, 255));
}

// -------------------------------------------------------- battery_logic ----

using battery_logic::Level;

battery_logic::Monitor makeMonitor() {
  return battery_logic::Monitor({5.0f, 10.5f, 9.9f, 2000});
}

void test_battery_levels() {
  battery_logic::Monitor m = makeMonitor();
  TEST_ASSERT_EQUAL(Level::Absent, m.update(0.3f, 0));
  TEST_ASSERT_EQUAL(Level::Ok, m.update(12.4f, 100));
  TEST_ASSERT_EQUAL(Level::Low, m.update(10.3f, 200));
  TEST_ASSERT_EQUAL(Level::Ok, m.update(11.0f, 300));
}

void test_battery_brief_sag_does_not_cut() {
  battery_logic::Monitor m = makeMonitor();
  m.update(11.0f, 0);
  TEST_ASSERT_EQUAL(Level::Low, m.update(9.5f, 1000));
  TEST_ASSERT_EQUAL(Level::Low, m.update(9.5f, 2500));
  TEST_ASSERT_EQUAL(Level::Ok, m.update(10.8f, 2600));  // recovered: timer resets
  TEST_ASSERT_EQUAL(Level::Low, m.update(9.5f, 3000));
  TEST_ASSERT_EQUAL(Level::Low, m.update(9.5f, 4900));
  TEST_ASSERT_TRUE(m.motorsAllowed());
}

void test_battery_sustained_low_cuts_and_latches() {
  battery_logic::Monitor m = makeMonitor();
  m.update(11.0f, 0);
  m.update(9.7f, 1000);
  TEST_ASSERT_EQUAL(Level::Cutoff, m.update(9.7f, 3000));
  TEST_ASSERT_FALSE(m.motorsAllowed());
  // Load comes off, the pack bounces back. Still cut.
  TEST_ASSERT_EQUAL(Level::Cutoff, m.update(10.6f, 4000));
}

void test_battery_cutoff_timer_survives_millis_wrap() {
  battery_logic::Monitor m = makeMonitor();
  m.update(11.0f, 0xFFFFF000u);
  m.update(9.7f, 0xFFFFFF00u);
  TEST_ASSERT_EQUAL(Level::Cutoff, m.update(9.7f, 0x00000800u));
}

// ---------------------------------------------------------- line_reader ----

bool feedAll(LineReader& r, const char* s) {
  bool ready = false;
  for (const char* c = s; *c; c++) ready = r.feed(*c);
  return ready;
}

void test_line_reader_lf_and_crlf() {
  LineReader r;
  TEST_ASSERT_TRUE(feedAll(r, "STOP\n"));
  TEST_ASSERT_EQUAL_STRING("STOP", r.line());
  TEST_ASSERT_TRUE(feedAll(r, "FWD 5\r\n"));
  TEST_ASSERT_EQUAL_STRING("FWD 5", r.line());
}

void test_line_reader_ignores_blank_lines() {
  LineReader r;
  TEST_ASSERT_FALSE(feedAll(r, "\n\r\n"));
}

void test_line_reader_drops_overlong_line_whole() {
  LineReader r;
  for (int i = 0; i < 100; i++) r.feed('X');
  TEST_ASSERT_FALSE(r.feed('\n'));
  // And recovers for the next one.
  TEST_ASSERT_TRUE(feedAll(r, "GO\n"));
  TEST_ASSERT_EQUAL_STRING("GO", r.line());
}

// ------------------------------------------------------------- protocol ----

using protocol::Command;

void test_protocol_commands_with_args() {
  protocol::Parsed p = protocol::parse("FWD 5");
  TEST_ASSERT_EQUAL(Command::Fwd, p.command);
  TEST_ASSERT_EQUAL(5, p.arg);

  p = protocol::parse("LEFT 15");
  TEST_ASSERT_EQUAL(Command::Left, p.command);
  TEST_ASSERT_EQUAL(15, p.arg);

  TEST_ASSERT_EQUAL(Command::Back, protocol::parse("BACK 99").command);
  TEST_ASSERT_EQUAL(Command::Right, protocol::parse("RIGHT 1").command);
  TEST_ASSERT_EQUAL(Command::Fire, protocol::parse("FIRE 2").command);
  TEST_ASSERT_EQUAL(Command::Hold, protocol::parse("HOLD 10").command);
  TEST_ASSERT_EQUAL(Command::Rpt, protocol::parse("RPT 3").command);
}

void test_protocol_bare_commands() {
  TEST_ASSERT_EQUAL(Command::Go, protocol::parse("GO").command);
  TEST_ASSERT_EQUAL(Command::Cls, protocol::parse("CLS").command);
  TEST_ASSERT_EQUAL(Command::Stop, protocol::parse("STOP").command);
}

void test_protocol_rejects_malformed() {
  TEST_ASSERT_EQUAL_STRING("unknown command", protocol::parse("JUMP 3").error);
  TEST_ASSERT_EQUAL_STRING("unknown command", protocol::parse("FWDX 3").error);
  TEST_ASSERT_EQUAL_STRING("unknown command", protocol::parse("fwd 3").error);
  TEST_ASSERT_EQUAL_STRING("missing argument", protocol::parse("FWD").error);
  TEST_ASSERT_EQUAL_STRING("missing argument", protocol::parse("FWD ").error);
  TEST_ASSERT_EQUAL_STRING("bad argument", protocol::parse("FWD 5x").error);
  TEST_ASSERT_EQUAL_STRING("bad argument", protocol::parse("FWD -5").error);
  TEST_ASSERT_EQUAL_STRING("argument out of range", protocol::parse("FWD 0").error);
  TEST_ASSERT_EQUAL_STRING("argument out of range", protocol::parse("FWD 100").error);
  TEST_ASSERT_EQUAL_STRING("argument out of range", protocol::parse("RPT 17").error);
  TEST_ASSERT_EQUAL_STRING("argument out of range", protocol::parse("FWD 99999999999").error);
  TEST_ASSERT_EQUAL_STRING("unexpected argument", protocol::parse("GO 2").error);
  TEST_ASSERT_EQUAL(Command::Invalid, protocol::parse("").command);
}

// -------------------------------------------------------- program_logic ----

using program_logic::Program;
using program_logic::RunStep;

Program makeProgram(std::initializer_list<program_logic::Step> steps) {
  Program p;
  for (const program_logic::Step& s : steps) p.add(s);
  return p;
}

void test_program_add_and_clear() {
  Program p;
  TEST_ASSERT_NULL(p.add({Command::Fwd, 5}));
  TEST_ASSERT_NULL(p.add({Command::Left, 15}));
  TEST_ASSERT_EQUAL(2, p.size());
  p.clear();
  TEST_ASSERT_TRUE(p.empty());
}

void test_program_caps_at_16_steps() {
  Program p;
  for (int i = 0; i < 16; i++) TEST_ASSERT_NULL(p.add({Command::Fwd, 1}));
  TEST_ASSERT_EQUAL_STRING("queue full", p.add({Command::Fwd, 1}));
  TEST_ASSERT_EQUAL(16, p.size());
}

void test_program_rpt_needs_steps_before_it() {
  Program p;
  TEST_ASSERT_EQUAL_STRING("nothing to repeat", p.add({Command::Rpt, 1}));
  p.add({Command::Fwd, 1});
  TEST_ASSERT_EQUAL_STRING("nothing to repeat", p.add({Command::Rpt, 2}));
  TEST_ASSERT_NULL(p.add({Command::Rpt, 1}));
}

void test_flatten_without_repeats_is_the_program() {
  Program p = makeProgram({{Command::Fwd, 2}, {Command::Fire, 1}});
  RunStep out[8];
  TEST_ASSERT_EQUAL(2, p.flatten(out, 8));
  TEST_ASSERT_EQUAL(Command::Fwd, out[0].step.command);
  TEST_ASSERT_EQUAL(0, out[0].programIndex);
  TEST_ASSERT_EQUAL(Command::Fire, out[1].step.command);
  TEST_ASSERT_EQUAL(1, out[1].programIndex);
}

void test_flatten_rpt_replays_previous_steps_with_their_rows() {
  // FWD 2, RIGHT 15, RPT 2 -> FWD, RIGHT, FWD, RIGHT
  Program p = makeProgram({{Command::Fwd, 2}, {Command::Right, 15}, {Command::Rpt, 2}});
  RunStep out[8];
  TEST_ASSERT_EQUAL(4, p.flatten(out, 8));
  uint8_t rows[] = {0, 1, 0, 1};
  for (int i = 0; i < 4; i++) TEST_ASSERT_EQUAL(rows[i], out[i].programIndex);
  TEST_ASSERT_EQUAL(Command::Right, out[3].step.command);
}

void test_flatten_nested_rpt_expands_recursively() {
  // A, RPT 1, RPT 2 -> A, A, (A, A)
  Program p = makeProgram({{Command::Fwd, 1}, {Command::Rpt, 1}, {Command::Rpt, 2}});
  RunStep out[16];
  TEST_ASSERT_EQUAL(4, p.flatten(out, 16));
  for (int i = 0; i < 4; i++) TEST_ASSERT_EQUAL(0, out[i].programIndex);
}

void test_flatten_refuses_runaway_programs() {
  // Each RPT doubles everything before it: 2^15 steps.
  Program p;
  p.add({Command::Fwd, 1});
  for (int i = 1; i < 16; i++) p.add({Command::Rpt, static_cast<int>(i)});
  static RunStep out[program_logic::MAX_RUN_STEPS];
  TEST_ASSERT_EQUAL(0, p.flatten(out, program_logic::MAX_RUN_STEPS));
}

void test_plan_moves() {
  program_logic::MovePlan m =
      program_logic::planMove({Command::Fwd, 3}, 2000, 60, 1200, 50);
  TEST_ASSERT_EQUAL(1, m.leftDir);
  TEST_ASSERT_EQUAL(1, m.rightDir);
  TEST_ASSERT_EQUAL(6000, m.ticks);
  TEST_ASSERT_EQUAL(3600, m.timedMs);

  m = program_logic::planMove({Command::Back, 1}, 2000, 60, 1200, 50);
  TEST_ASSERT_EQUAL(-1, m.leftDir);
  TEST_ASSERT_EQUAL(-1, m.rightDir);

  // Left turn: left wheel back, right wheel forward.
  m = program_logic::planMove({Command::Left, 15}, 2000, 60, 1200, 50);
  TEST_ASSERT_EQUAL(-1, m.leftDir);
  TEST_ASSERT_EQUAL(1, m.rightDir);
  TEST_ASSERT_EQUAL(900, m.ticks);

  m = program_logic::planMove({Command::Right, 15}, 2000, 60, 1200, 50);
  TEST_ASSERT_EQUAL(1, m.leftDir);
  TEST_ASSERT_EQUAL(-1, m.rightDir);
}

void test_move_speed_slows_for_the_final_stretch() {
  // 1000-tick move, slowdown is 25% = 250 ticks.
  TEST_ASSERT_EQUAL_FLOAT(0.5f, program_logic::moveSpeed(0, 1000, 0.5f, 0.25f, 0.25f, 600));
  TEST_ASSERT_EQUAL_FLOAT(0.5f, program_logic::moveSpeed(749, 1000, 0.5f, 0.25f, 0.25f, 600));
  TEST_ASSERT_EQUAL_FLOAT(0.25f, program_logic::moveSpeed(750, 1000, 0.5f, 0.25f, 0.25f, 600));
  // Long move: slowdown capped at 600 ticks.
  TEST_ASSERT_EQUAL_FLOAT(0.5f, program_logic::moveSpeed(9399, 10000, 0.5f, 0.25f, 0.25f, 600));
  TEST_ASSERT_EQUAL_FLOAT(0.25f, program_logic::moveSpeed(9400, 10000, 0.5f, 0.25f, 0.25f, 600));
}

int main() {
  UNITY_BEGIN();

  RUN_TEST(test_normalize_axis_clamps_and_scales);
  RUN_TEST(test_deadzone_zeroes_small_inputs);
  RUN_TEST(test_deadzone_rescales_without_a_jump);
  RUN_TEST(test_mix_straight_and_spin);
  RUN_TEST(test_mix_normalizes_instead_of_clipping);
  RUN_TEST(test_slew_accelerates_gently);
  RUN_TEST(test_slew_stops_quickly);
  RUN_TEST(test_slew_reversal_decelerates_first);
  RUN_TEST(test_slew_does_not_overshoot);
  RUN_TEST(test_to_duty);

  RUN_TEST(test_battery_levels);
  RUN_TEST(test_battery_brief_sag_does_not_cut);
  RUN_TEST(test_battery_sustained_low_cuts_and_latches);
  RUN_TEST(test_battery_cutoff_timer_survives_millis_wrap);

  RUN_TEST(test_line_reader_lf_and_crlf);
  RUN_TEST(test_line_reader_ignores_blank_lines);
  RUN_TEST(test_line_reader_drops_overlong_line_whole);

  RUN_TEST(test_protocol_commands_with_args);
  RUN_TEST(test_protocol_bare_commands);
  RUN_TEST(test_protocol_rejects_malformed);

  RUN_TEST(test_program_add_and_clear);
  RUN_TEST(test_program_caps_at_16_steps);
  RUN_TEST(test_program_rpt_needs_steps_before_it);
  RUN_TEST(test_flatten_without_repeats_is_the_program);
  RUN_TEST(test_flatten_rpt_replays_previous_steps_with_their_rows);
  RUN_TEST(test_flatten_nested_rpt_expands_recursively);
  RUN_TEST(test_flatten_refuses_runaway_programs);
  RUN_TEST(test_plan_moves);
  RUN_TEST(test_move_speed_slows_for_the_final_stretch);

  return UNITY_END();
}
