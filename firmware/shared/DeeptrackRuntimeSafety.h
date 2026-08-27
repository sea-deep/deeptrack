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

constexpr bool encoderPulseTooSoon(uint32_t nowUs, uint32_t lastAcceptedUs,
                                   uint32_t minimumPulseUs) {
  return lastAcceptedUs != 0 && nowUs - lastAcceptedUs < minimumPulseUs;
}

// Wrap-safe deadline check used by the short, configurable coasting window.
constexpr bool encoderGateWindowOpen(uint32_t nowUs, uint32_t activeUntilUs) {
  return static_cast<int32_t>(activeUntilUs - nowUs) >= 0;
}

inline float boundedBalanceTrim(float leftDistanceM, float rightDistanceM,
                                float yawRateDps, bool useEncoder,
                                bool useGyro, float encoderKp, float gyroKp,
                                float maximumCorrection) {
  float correction = 0.0f;
  if (useEncoder)
    correction += (leftDistanceM - rightDistanceM) * encoderKp;
  if (useGyro) correction += yawRateDps * gyroKp;
  if (correction > maximumCorrection) return maximumCorrection;
  if (correction < -maximumCorrection) return -maximumCorrection;
  return correction;
}

}  // namespace Safety
}  // namespace DeepTrack
