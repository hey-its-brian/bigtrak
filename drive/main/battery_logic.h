// Battery state machine. Pure logic, fed voltages and timestamps, so the
// native unit tests can drive it without an ADC.
#pragma once

#include <stdint.h>

namespace battery_logic {

enum class Level {
  Absent,  // no pack, bench power over USB
  Ok,
  Low,     // warn the UI, keep driving
  Cutoff,  // motors disabled until power cycle
};

struct Thresholds {
  float presentV;
  float warnV;
  float cutoffV;
  uint32_t cutoffHoldMs;
};

class Monitor {
 public:
  explicit Monitor(Thresholds t) : t_(t) {}

  // Call with each filtered reading. Returns the level after this sample.
  Level update(float volts, uint32_t nowMs) {
    volts_ = volts;

    // Cutoff latches. Take the load off a sagging pack and it bounces back
    // above the threshold, so un-latching on voltage would just oscillate.
    if (level_ == Level::Cutoff) return level_;

    if (volts < t_.presentV) {
      level_ = Level::Absent;
      underSince_ = 0;
      underTiming_ = false;
      return level_;
    }

    if (volts < t_.cutoffV) {
      if (!underTiming_) {
        underTiming_ = true;
        underSince_ = nowMs;
      } else if (nowMs - underSince_ >= t_.cutoffHoldMs) {
        level_ = Level::Cutoff;
        return level_;
      }
    } else {
      underTiming_ = false;
    }

    level_ = volts < t_.warnV ? Level::Low : Level::Ok;
    return level_;
  }

  Level level() const { return level_; }
  float volts() const { return volts_; }
  bool motorsAllowed() const { return level_ != Level::Cutoff; }

 private:
  Thresholds t_;
  Level level_ = Level::Absent;
  float volts_ = 0.0f;
  bool underTiming_ = false;
  uint32_t underSince_ = 0;
};

}  // namespace battery_logic
