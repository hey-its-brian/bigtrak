// Host-side tests for the drive board's hardware-free logic.
//   cd drive && pio test -e native

#include <unity.h>

#include "battery_logic.h"
#include "drive_mix.h"
#include "line_reader.h"
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

  return UNITY_END();
}
