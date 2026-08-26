#pragma once

#include <stdint.h>

namespace DeepTrack {
namespace Safety {

enum class RangeValidity : uint8_t {
  UNKNOWN = 0,
  VALID = 1,
  INVALID = 2,
};

enum class RangeFreshness : uint8_t {
  NEVER = 0,
  FRESH = 1,
  STALE = 2,
};

struct FrontSafetySnapshot {
  RangeValidity latest_sample;
  RangeFreshness freshness;
  bool forward_blocked;
  bool has_valid_distance;
  float last_valid_cm;
  uint32_t last_valid_age_ms;
};

// Pure state machine shared by the rover firmware and host-side tests.
// UNKNOWN and STALE are fail-closed. Hysteresis changes the gate only at the
// stop/clear boundaries, preventing chatter around a single threshold.
class FrontSafetyGate {
 public:
  constexpr FrontSafetyGate(float stop_cm, float clear_cm,
                            uint32_t stale_after_ms)
      : stop_cm_(stop_cm),
        clear_cm_(clear_cm),
        stale_after_ms_(stale_after_ms) {}

  void observe(bool valid, float distance_cm, uint32_t now_ms) {
    latest_sample_ = valid ? RangeValidity::VALID : RangeValidity::INVALID;

    if (valid) {
      has_valid_distance_ = true;
      last_valid_cm_ = distance_cm;
      last_valid_at_ms_ = now_ms;

      if (distance_cm <= stop_cm_) {
        forward_blocked_ = true;
      } else if (distance_cm >= clear_cm_) {
        forward_blocked_ = false;
      }
    }

    refresh(now_ms);
  }

  void refresh(uint32_t now_ms) {
    if (!has_valid_distance_) {
      freshness_ = RangeFreshness::NEVER;
      forward_blocked_ = true;
      return;
    }

    const uint32_t age_ms = now_ms - last_valid_at_ms_;
    freshness_ = age_ms <= stale_after_ms_ ? RangeFreshness::FRESH
                                           : RangeFreshness::STALE;
    if (freshness_ == RangeFreshness::STALE) forward_blocked_ = true;
  }

  bool allows(int16_t left_request, int16_t right_request,
              uint32_t now_ms) {
    refresh(now_ms);
    return !requestsForward(left_request, right_request) || !forward_blocked_;
  }

  FrontSafetySnapshot snapshot(uint32_t now_ms) {
    refresh(now_ms);
    return FrontSafetySnapshot{
        latest_sample_,
        freshness_,
        forward_blocked_,
        has_valid_distance_,
        last_valid_cm_,
        has_valid_distance_ ? now_ms - last_valid_at_ms_ : 0,
    };
  }

  static constexpr bool requestsForward(int16_t left_request,
                                        int16_t right_request) {
    // A single forward bank can still advance the chassis. Opposite signed,
    // equal-magnitude requests are an in-place pivot and have zero sum.
    return static_cast<int32_t>(left_request) +
               static_cast<int32_t>(right_request) >
           0;
  }

 private:
  float stop_cm_;
  float clear_cm_;
  uint32_t stale_after_ms_;
  RangeValidity latest_sample_ = RangeValidity::UNKNOWN;
  RangeFreshness freshness_ = RangeFreshness::NEVER;
  bool forward_blocked_ = true;
  bool has_valid_distance_ = false;
  float last_valid_cm_ = 0.0f;
  uint32_t last_valid_at_ms_ = 0;
};

inline const char* rangeValidityName(RangeValidity value) {
  switch (value) {
    case RangeValidity::UNKNOWN:
      return "UNKNOWN";
    case RangeValidity::VALID:
      return "VALID";
    case RangeValidity::INVALID:
      return "INVALID";
  }
  return "UNKNOWN";
}

inline const char* rangeFreshnessName(RangeFreshness value) {
  switch (value) {
    case RangeFreshness::NEVER:
      return "NEVER";
    case RangeFreshness::FRESH:
      return "FRESH";
    case RangeFreshness::STALE:
      return "STALE";
  }
  return "NEVER";
}

}  // namespace Safety
}  // namespace DeepTrack
