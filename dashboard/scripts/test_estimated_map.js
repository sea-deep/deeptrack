import assert from 'node:assert/strict';
import { describe, test } from 'node:test';
import {
  addScanEvidence, branchPoseMatches, createEstimatedMapState,
  detectFrontierClusters, interpolatePose, isKnownTraversable,
  planKnownFreePath, renderableEvidence, scoreFrontierClusters,
  updateEstimatedPose, validateGoal
} from '../src/lib/state/estimatedMap.js';
import { roverCalibration } from '../src/lib/config/roverCalibration.js';

const calibrated = {
  chassis_width_mm: 220, track_width_mm: 180,
  micrometers_per_tick: 5000, left_ticks: 0, right_ticks: 0
};

describe('confidence-aware estimated map', () => {
  test('keeps pose unknown until measured encoder calibration arrives', () => {
    const state = updateEstimatedPose(createEstimatedMapState(), {
      left_ticks: 0, right_ticks: 0, chassis_width_mm: null,
      track_width_mm: null, micrometers_per_tick: null
    }, { ...roverCalibration, assumeCalibrated: false });
    assert.equal(state.pose.known, false);
    assert.equal(state.pose.reason, 'ENCODER_CALIBRATION_REQUIRED');
  });

  test('uses the external assumed calibration when rover NVS is empty', () => {
    const state = updateEstimatedPose(createEstimatedMapState(), {
      left_ticks: 0, right_ticks: 0, chassis_width_mm: null,
      track_width_mm: null, micrometers_per_tick: null
    });
    assert.equal(state.pose.known, true);
    assert.equal(state.calibration.source, 'DASHBOARD_ASSUMED');
    assert.equal(state.pose.reason, 'ASSUMED_DASHBOARD_CALIBRATION');
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

  test('applies bounded gyro heading correction without replacing encoders', () => {
    let state = updateEstimatedPose(createEstimatedMapState(), {
      ...calibrated, timestamp_ms: 0, heading_deg: 0
    });
    state = updateEstimatedPose(state, {
      ...calibrated, left_ticks: 10, right_ticks: 10,
      timestamp_ms: 100, heading_deg: 10
    });
    assert.ok(Math.abs(state.pose.heading_rad + 2 * Math.PI / 180) < 0.0001);
  });

  test('interpolates timestamped poses across heading wrap', () => {
    const pose = interpolatePose([
      { timestamp_ms: 100, x_m: 0, y_m: 0,
        heading_rad: 170 * Math.PI / 180, confidence: 0.8 },
      { timestamp_ms: 200, x_m: 1, y_m: 2,
        heading_rad: -170 * Math.PI / 180, confidence: 0.6 }
    ], 150);
    assert.ok(Math.abs(pose.x_m - 0.5) < 0.0001);
    assert.ok(Math.abs(Math.abs(pose.heading_rad) - Math.PI) < 0.0001);
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

  test('casts a scan from its timestamped pose rather than the latest pose', () => {
    const base = updateEstimatedPose(createEstimatedMapState(), calibrated);
    const state = {
      ...base,
      pose: { ...base.pose, x_m: 1 },
      poseHistory: [
        { timestamp_ms: 0, x_m: 0, y_m: 0, heading_rad: 0, confidence: 0.8 },
        { timestamp_ms: 1000, x_m: 1, y_m: 0, heading_rad: 0, confidence: 0.7 }
      ]
    };
    const mapped = addScanEvidence(state, [
      { angle_deg: 90, distance_mm: 500, valid: true, timestamp_ms: 500 },
      { angle_deg: 90, distance_mm: 500, valid: true, timestamp_ms: 500 }
    ], 1000);
    assert.equal(mapped.cells.get('20,0')?.classification, 'OCCUPIED');
    assert.equal(mapped.cells.has('30,0'), false);
  });

  test('clusters reachable frontiers and exposes score breakdowns', () => {
    const state = createEstimatedMapState();
    state.calibration = {
      track_width_m: 0.18, left_meters_per_tick: 0.005,
      right_meters_per_tick: 0.005, chassis_width_m: 0.22
    };
    for (let x = -2; x <= 2; ++x) {
      for (let y = -2; y <= 2; ++y) {
        state.cells.set(`${x},${y}`, {
          x, y, classification: 'KNOWN_FREE', observations: 2, log_odds: -1
        });
      }
    }
    const clusters = detectFrontierClusters(state, {
      minCells: 2, start: { x: 0, y: 0 }
    });
    assert.equal(clusters.length, 1);
    assert.ok(clusters[0].informationGain > 0);
    const scored = scoreFrontierClusters(state, clusters, {
      communicationConfidence: 0.8, poseUncertainty: 0.1
    });
    assert.equal(typeof scored[0].score, 'number');
    assert.equal(typeof scored[0].breakdown.pathLength, 'number');
  });

  test('validates goals with bounded safe snapping and A-star planning', () => {
    const state = createEstimatedMapState();
    state.calibration = {
      track_width_m: 0.18, left_meters_per_tick: 0.005,
      right_meters_per_tick: 0.005, chassis_width_m: 0.22
    };
    for (let x = 0; x <= 6; ++x) {
      state.cells.set(`${x},0`, {
        x, y: 0, classification: 'KNOWN_FREE', observations: 2, log_odds: -1
      });
    }
    const validation = validateGoal(state, { x: 6, y: 1 }, 2);
    assert.equal(validation.accepted, true);
    assert.deepEqual(validation.goal, { x: 6, y: 0 });
    const path = planKnownFreePath(state, { x: 0, y: 0 }, validation.goal);
    assert.equal(path.length, 7);
    assert.equal(validateGoal(state, { x: 50, y: 50 }, 2).accepted, false);
  });

  test('matches failed branches with position and wrapped-heading tolerances', () => {
    assert.equal(branchPoseMatches(
      { x: 1, y: 1, headingRad: 179 * Math.PI / 180 },
      { x: 1.1, y: 1.05, headingRad: -179 * Math.PI / 180 }
    ), true);
    assert.equal(branchPoseMatches(
      { x: 1, y: 1, headingRad: 0 },
      { x: 2, y: 1, headingRad: 0 }
    ), false);
  });
});
