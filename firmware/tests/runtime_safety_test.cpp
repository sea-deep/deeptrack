#include <assert.h>
#include <stdint.h>

#include "../shared/DeeptrackRuntimeSafety.h"

using DeepTrack::Safety::dutyRequiresEncoderMotion;
using DeepTrack::Safety::encoderDirectionForDuty;
using DeepTrack::Safety::encoderGateWindowOpen;
using DeepTrack::Safety::encoderPulseTooSoon;
using DeepTrack::Safety::sideHasStalled;
using DeepTrack::Safety::boundedBalanceTrim;

int main() {
  assert(encoderDirectionForDuty(145) == 1);
  assert(encoderDirectionForDuty(-145) == -1);
  assert(encoderDirectionForDuty(0) == 0);

  assert(!dutyRequiresEncoderMotion(99, 100));
  assert(dutyRequiresEncoderMotion(100, 100));
  assert(dutyRequiresEncoderMotion(-100, 100));

  assert(!sideHasStalled(145, 100, 649, 0, 650));
  assert(sideHasStalled(145, 100, 650, 0, 650));
  assert(!sideHasStalled(0, 100, 5000, 0, 650));

  // Unsigned subtraction is intentionally wrap-safe for millis().
  assert(!sideHasStalled(145, 100, 50, UINT32_MAX - 100, 650));
  assert(sideHasStalled(145, 100, 600, UINT32_MAX - 100, 650));

  assert(!encoderPulseTooSoon(1000, 0, 1500));
  assert(encoderPulseTooSoon(2000, 1000, 1500));
  assert(!encoderPulseTooSoon(2500, 1000, 1500));
  assert(encoderGateWindowOpen(1000, 1080));
  assert(!encoderGateWindowOpen(1081, 1080));
  assert(encoderGateWindowOpen(UINT32_MAX - 20, 40));
  assert(boundedBalanceTrim(0.11f, 0.10f, 0.0f, true, false,
                            100.0f, 0.0f, 8.0f) > 0.99f);
  assert(boundedBalanceTrim(0.20f, 0.0f, 0.0f, true, false,
                            100.0f, 0.0f, 8.0f) == 8.0f);
  assert(boundedBalanceTrim(0.0f, 0.0f, -10.0f, false, true,
                            0.0f, 0.5f, 8.0f) == -5.0f);
  return 0;
}
