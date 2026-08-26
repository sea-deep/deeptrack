#include <assert.h>
#include <stdint.h>

#include "../shared/DeeptrackFrontSafety.h"

using DeepTrack::Safety::FrontSafetyGate;
using DeepTrack::Safety::RangeFreshness;
using DeepTrack::Safety::RangeValidity;

int main() {
  FrontSafetyGate gate(24.0f, 32.0f, 300);

  auto initial = gate.snapshot(0);
  assert(initial.latest_sample == RangeValidity::UNKNOWN);
  assert(initial.freshness == RangeFreshness::NEVER);
  assert(initial.forward_blocked);
  assert(!gate.allows(145, 145, 0));
  assert(!gate.allows(145, 0, 0));
  assert(gate.allows(-145, -145, 0));
  assert(gate.allows(-145, 145, 0));

  gate.observe(true, 40.0f, 10);
  auto clear = gate.snapshot(10);
  assert(clear.latest_sample == RangeValidity::VALID);
  assert(clear.freshness == RangeFreshness::FRESH);
  assert(!clear.forward_blocked);
  assert(gate.allows(145, 145, 10));

  // The hysteresis band preserves the previous clear state.
  gate.observe(true, 28.0f, 90);
  assert(!gate.snapshot(90).forward_blocked);

  gate.observe(true, 24.0f, 170);
  assert(gate.snapshot(170).forward_blocked);
  assert(!gate.allows(145, 0, 170));

  // The same hysteresis band now preserves the blocked state.
  gate.observe(true, 28.0f, 210);
  assert(gate.snapshot(210).forward_blocked);
  gate.observe(true, 32.0f, 250);
  assert(!gate.snapshot(250).forward_blocked);

  // One failed ping does not erase a still-fresh last-valid observation.
  gate.observe(false, 0.0f, 330);
  auto transient = gate.snapshot(330);
  assert(transient.latest_sample == RangeValidity::INVALID);
  assert(transient.freshness == RangeFreshness::FRESH);
  assert(!transient.forward_blocked);

  // Once the bounded freshness window expires, forward fails closed.
  auto stale = gate.snapshot(551);
  assert(stale.freshness == RangeFreshness::STALE);
  assert(stale.forward_blocked);
  assert(!gate.allows(145, 145, 551));

  // Unsigned elapsed-time arithmetic remains correct across millis() wrap.
  FrontSafetyGate wrap_gate(24.0f, 32.0f, 300);
  wrap_gate.observe(true, 40.0f, UINT32_MAX - 100);
  assert(!wrap_gate.snapshot(50).forward_blocked);
  assert(wrap_gate.snapshot(250).forward_blocked);

  return 0;
}
