#pragma once

#include <stdint.h>

namespace DeepTrack {
namespace Safety {

// A one-channel encoder cannot sense direction itself. Associate each pulse
// with the currently applied logical motor direction so reverse and pivots do
// not become fabricated forward odometry.
constexpr int8_t encoderDirectionForDuty(int16_t duty) {
  return duty > 0 ? 1 : duty < 0 ? -1 : 0;
}

constexpr bool dutyRequiresEncoderMotion(int16_t duty, int16_t minimumDuty) {
  return duty >= minimumDuty || duty <= -minimumDuty;
}

constexpr bool sideHasStalled(int16_t duty, int16_t minimumDuty,
                              uint32_t nowMs, uint32_t lastMotionMs,
                              uint32_t windowMs) {
  return dutyRequiresEncoderMotion(duty, minimumDuty) &&
         nowMs - lastMotionMs >= windowMs;
}

}  // namespace Safety
}  // namespace DeepTrack
