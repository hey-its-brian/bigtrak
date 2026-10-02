// UART protocol: outgoing line formatting, incoming status parsing, and
// assembling lines out of a byte stream.
#include <string.h>
#include <unity.h>

#include "protocol.h"

using program::Command;
using protocol::LineBuffer;
using protocol::Status;
using protocol::StatusType;

void setUp() {}
void tearDown() {}

namespace {

void assertFormats(Command command, uint8_t arg, const char* expected) {
  char line[protocol::MAX_LINE + 1];
  size_t length = protocol::formatStep({command, arg}, line, sizeof(line));
  TEST_ASSERT_EQUAL_STRING(expected, line);
  TEST_ASSERT_EQUAL(strlen(expected), length);
}

// Feeds a whole string; returns the number of complete lines seen and leaves
// the last one in lastLine.
int feedAll(LineBuffer& buffer, const char* bytes, char* lastLine) {
  int lines = 0;
  for (const char* c = bytes; *c; c++) {
    if (buffer.feed(*c)) {
      lines++;
      strcpy(lastLine, buffer.line());
    }
  }
  return lines;
}

}  // namespace

void test_format_every_command() {
  assertFormats(Command::Fwd, 5, "FWD 5");
  assertFormats(Command::Back, 12, "BACK 12");
  assertFormats(Command::Left, 15, "LEFT 15");
  assertFormats(Command::Right, 30, "RIGHT 30");
  assertFormats(Command::Fire, 3, "FIRE 3");
  assertFormats(Command::Hold, 10, "HOLD 10");
  assertFormats(Command::Rpt, 2, "RPT 2");
  TEST_ASSERT_EQUAL_STRING("GO", protocol::GO);
  TEST_ASSERT_EQUAL_STRING("CLS", protocol::CLS);
  TEST_ASSERT_EQUAL_STRING("STOP", protocol::STOP);
}

void test_format_too_small_buffer() {
  char tiny[4];
  TEST_ASSERT_EQUAL(0, protocol::formatStep({Command::Fwd, 5}, tiny,
                                            sizeof(tiny)));
  TEST_ASSERT_EQUAL_STRING("", tiny);
}

void test_parse_simple_status() {
  Status s;
  TEST_ASSERT_TRUE(protocol::parseStatus("ACK", s));
  TEST_ASSERT_EQUAL(StatusType::Ack, s.type);
  TEST_ASSERT_TRUE(protocol::parseStatus("DONE", s));
  TEST_ASSERT_EQUAL(StatusType::Done, s.type);
  TEST_ASSERT_TRUE(protocol::parseStatus("PAD CONNECTED", s));
  TEST_ASSERT_EQUAL(StatusType::PadConnected, s.type);
  TEST_ASSERT_TRUE(protocol::parseStatus("PAD DISCONNECTED", s));
  TEST_ASSERT_EQUAL(StatusType::PadDisconnected, s.type);
}

void test_parse_step_and_batt() {
  Status s;
  TEST_ASSERT_TRUE(protocol::parseStatus("STEP 2", s));
  TEST_ASSERT_EQUAL(StatusType::Step, s.type);
  TEST_ASSERT_EQUAL(2, s.step);

  TEST_ASSERT_TRUE(protocol::parseStatus("BATT 11.80", s));
  TEST_ASSERT_EQUAL(StatusType::Batt, s.type);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 11.8f, s.volts);

  TEST_ASSERT_TRUE(protocol::parseStatus("BATT 12", s));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.0f, s.volts);
}

void test_parse_err_reason() {
  Status s;
  TEST_ASSERT_TRUE(protocol::parseStatus("ERR queue full", s));
  TEST_ASSERT_EQUAL(StatusType::Err, s.type);
  TEST_ASSERT_EQUAL_STRING("queue full", s.reason);

  TEST_ASSERT_TRUE(protocol::parseStatus("ERR", s));
  TEST_ASSERT_EQUAL_STRING("", s.reason);
}

void test_parse_is_forgiving_of_humans() {
  Status s;
  TEST_ASSERT_TRUE(protocol::parseStatus("  step 3  ", s));
  TEST_ASSERT_EQUAL(StatusType::Step, s.type);
  TEST_ASSERT_EQUAL(3, s.step);
  TEST_ASSERT_TRUE(protocol::parseStatus("pad connected", s));
  TEST_ASSERT_EQUAL(StatusType::PadConnected, s.type);
}

void test_parse_rejects_garbage() {
  Status s;
  TEST_ASSERT_FALSE(protocol::parseStatus("", s));
  TEST_ASSERT_FALSE(protocol::parseStatus("   ", s));
  TEST_ASSERT_FALSE(protocol::parseStatus("HELLO", s));
  TEST_ASSERT_FALSE(protocol::parseStatus("STEP", s));
  TEST_ASSERT_FALSE(protocol::parseStatus("STEP x", s));
  TEST_ASSERT_FALSE(protocol::parseStatus("STEP 2x", s));
  TEST_ASSERT_FALSE(protocol::parseStatus("STEP -1", s));
  TEST_ASSERT_FALSE(protocol::parseStatus("BATT", s));
  TEST_ASSERT_FALSE(protocol::parseStatus("BATT abc", s));
  TEST_ASSERT_FALSE(protocol::parseStatus("PAD", s));
  TEST_ASSERT_FALSE(protocol::parseStatus("PAD MAYBE", s));
  TEST_ASSERT_FALSE(protocol::parseStatus("ACK extra", s));
  TEST_ASSERT_FALSE(protocol::parseStatus("STEPS 2", s));
  TEST_ASSERT_FALSE(protocol::parseStatus(nullptr, s));
}

void test_line_buffer_endings() {
  LineBuffer buffer;
  char last[protocol::MAX_LINE + 1] = {};

  TEST_ASSERT_EQUAL(1, feedAll(buffer, "ACK\n", last));
  TEST_ASSERT_EQUAL_STRING("ACK", last);

  // \r\n counts once; the \n after \r is an empty line and is skipped.
  TEST_ASSERT_EQUAL(1, feedAll(buffer, "STEP 4\r\n", last));
  TEST_ASSERT_EQUAL_STRING("STEP 4", last);

  TEST_ASSERT_EQUAL(2, feedAll(buffer, "DONE\rBATT 11.1\n\n\n", last));
  TEST_ASSERT_EQUAL_STRING("BATT 11.1", last);

  // Partial line: nothing until the newline arrives.
  TEST_ASSERT_EQUAL(0, feedAll(buffer, "PAD CONN", last));
  TEST_ASSERT_EQUAL(1, feedAll(buffer, "ECTED\n", last));
  TEST_ASSERT_EQUAL_STRING("PAD CONNECTED", last);
}

void test_line_buffer_drops_overlong_line() {
  LineBuffer buffer;
  char last[protocol::MAX_LINE + 1] = {};

  char junk[protocol::MAX_LINE + 20];
  memset(junk, 'x', sizeof(junk) - 2);
  junk[sizeof(junk) - 2] = '\n';
  junk[sizeof(junk) - 1] = '\0';
  TEST_ASSERT_EQUAL(0, feedAll(buffer, junk, last));

  // And recovers for the next line.
  TEST_ASSERT_EQUAL(1, feedAll(buffer, "ACK\n", last));
  TEST_ASSERT_EQUAL_STRING("ACK", last);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_format_every_command);
  RUN_TEST(test_format_too_small_buffer);
  RUN_TEST(test_parse_simple_status);
  RUN_TEST(test_parse_step_and_batt);
  RUN_TEST(test_parse_err_reason);
  RUN_TEST(test_parse_is_forgiving_of_humans);
  RUN_TEST(test_parse_rejects_garbage);
  RUN_TEST(test_line_buffer_endings);
  RUN_TEST(test_line_buffer_drops_overlong_line);
  return UNITY_END();
}
