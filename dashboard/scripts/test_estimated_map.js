import assert from 'node:assert/strict';
import { describe, test } from 'node:test';
import {
  addScanEvidence, createEstimatedMapState, isKnownTraversable,
  planKnownFreePath, renderableEvidence, updateEstimatedPose
} from '../src/lib/state/estimatedMap.js';

const calibrated = {
  chassis_width_mm: 220, track_width_mm: 180,
  micrometers_per_tick: 5000, left_ticks: 0, right_ticks: 0
};

describe('confidence-aware estimated map', () => {
  test('keeps pose unknown until measured encoder calibration arrives', () => {
    const state = updateEstimatedPose(createEstimatedMapState(), {
      left_ticks: 0, right_ticks: 0, chassis_width_mm: null,
      track_width_mm: null, micrometers_per_tick: null
    });
    assert.equal(state.pose.known, false);
    assert.equal(state.pose.reason, 'ENCODER_CALIBRATION_REQUIRED');
  });

  test('integrates differential ticks only after calibration', () => {
    let state = updateEstimatedPose(createEstimatedMapState(), calibrated);
    state = updateEstimatedPose(state, {
      ...calibrated, left_ticks: 20, right_ticks: 20
    });
    assert.equal(state.pose.known, true);
    assert.ok(Math.abs(state.pose.x_m - 0.1) < 0.0001);
    assert.equal(state.pose.reason, 'DEAD_RECKONING_ESTIMATE');
  });

  test('ignores invalid ranges and records valid rays as evidence', () => {
    let state = updateEstimatedPose(createEstimatedMapState(), calibrated);
    state = addScanEvidence(state, [
      { angle_deg: 90, distance_mm: null, valid: false },
      { angle_deg: 90, distance_mm: 600, valid: true, confidence_pct: 100 },
      { angle_deg: 90, distance_mm: 600, valid: true, confidence_pct: 100 }
    ], 1000);
    const evidence = renderableEvidence(state);
    assert.ok(evidence.occupied.length > 0);
    assert.ok(evidence.knownFree.length > 0);
  });

  test('never treats an unknown cell as traversable or plans through it', () => {
    const state = updateEstimatedPose(createEstimatedMapState(), calibrated);
    assert.equal(isKnownTraversable(state, 20, 20), false);
    assert.equal(planKnownFreePath(state, { x: 0, y: 0 }, { x: 20, y: 20 }), null);
  });
});
