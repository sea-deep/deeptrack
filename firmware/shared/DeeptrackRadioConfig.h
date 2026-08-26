#pragma once

#include <stdint.h>

// A local file is required before encrypted ESP-NOW is enabled. The fallback
// keeps builds reproducible while failing closed at runtime.
#if __has_include("DeeptrackRadioConfig.local.h")
#include "DeeptrackRadioConfig.local.h"
#else
namespace DeepTrack {
namespace RadioConfig {
constexpr bool ENABLED = false;
constexpr uint8_t CHANNEL = 1;
constexpr uint8_t ROVER_MAC[6] = {0, 0, 0, 0, 0, 0};
constexpr uint8_t GATEWAY_MAC[6] = {0, 0, 0, 0, 0, 0};
constexpr uint8_t PMK[16] = {0};
constexpr uint8_t LMK[16] = {0};
}  // namespace RadioConfig
}  // namespace DeepTrack
#endif
