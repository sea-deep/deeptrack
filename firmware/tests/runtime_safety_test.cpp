#include <assert.h>
#include <stdint.h>

#include "../shared/DeeptrackRuntimeSafety.h"

using DeepTrack::Safety::dutyRequiresEncoderMotion;
using DeepTrack::Safety::encoderDirectionForDuty;
using DeepTrack::Safety::sideHasStalled;

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
  return 0;
}
