import assert from 'node:assert/strict';
import { describe, test } from 'node:test';
import {
  gateManualDrive, keyboardDriveVector, requestsForward
} from '../src/lib/state/manualDrive.js';

describe('manual drive arbitration', () => {
  test('matches the rover forward-direction definition', () => {
    assert.equal(requestsForward(50, 50), true);
    assert.equal(requestsForward(-50, -50), false);
    assert.equal(requestsForward(-50, 50), false);
  });

  test('blocks forward but retains reverse and in-place pivot', () => {
    const state = { controlReady: true, manualMode: true,
      forwardAllowed: false };
    assert.equal(gateManualDrive(55, 55, state).reason, 'FORWARD_HELD');
    assert.equal(gateManualDrive(-55, -55, state).allowed, true);
    assert.equal(gateManualDrive(-55, 55, state).allowed, true);
  });

  test('neutralizes every vector while control is locked', () => {
    assert.deepEqual(gateManualDrive(-55, -55, {
      controlReady: false, manualMode: true, forwardAllowed: false
    }), { left: 0, right: 0, allowed: false, reason: 'CONTROL_LOCKED' });
  });

  test('recomputes remaining keyboard keys without a zero-command gap', () => {
    assert.deepEqual(keyboardDriveVector(
      { w: true, a: true, s: false, d: false }, 55, true),
    { left: 0, right: 100 });
    assert.deepEqual(keyboardDriveVector(
      { w: false, a: true, s: false, d: false }, 55, true),
    { left: -55, right: 55 });
  });

  test('removes forward contribution when the gate closes', () => {
    assert.deepEqual(keyboardDriveVector(
      { w: true, a: false, s: false, d: false }, 55, false),
    { left: 0, right: 0 });
  });
});
