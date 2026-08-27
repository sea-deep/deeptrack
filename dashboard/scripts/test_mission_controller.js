import assert from 'node:assert/strict';
import {
  createMissionControllerState, startMission, stepMission, stopMission
} from '../src/lib/state/missionController.js';
import { createEstimatedMapState } from '../src/lib/state/estimatedMap.js';

const map = createEstimatedMapState();
map.calibration = { track_width_m: 0.3, left_meters_per_tick: 0.001,
  right_meters_per_tick: 0.001, chassis_width_m: 0.2 };
map.pose = { known: true, x_m: 0.025, y_m: 0.025, heading_rad: 0,
  confidence: 0.9 };
for (let x = 0; x <= 5; ++x)
  map.cells.set(`${x},0`, { x, y: 0, classification: 'KNOWN_FREE',
    observations: 2, log_odds: -1 });

let mission = startMission(createMissionControllerState(), 'NAVIGATE', map,
  { x: 5, y: 0 }, 100);
assert.equal(mission.status, 'RUNNING');
let tick = stepMission(mission, map, { controlReady: true, frontValid: true,
  frontFresh: true, frontBlocked: false, speedPercent: 35 }, 200);
assert.ok(tick.drive.left > 0 && tick.drive.right > 0);

tick = stepMission(mission, map, { controlReady: true, frontValid: false,
  frontFresh: false, frontBlocked: true }, 200);
assert.deepEqual(tick.drive, { left: 0, right: 0 });
assert.equal(tick.state.reason, 'FRONT_CLEARANCE_UNKNOWN');

const uncalibrated = createEstimatedMapState();
assert.equal(startMission(createMissionControllerState(), 'RETURN_HOME',
  uncalibrated, null, 0).reason, 'CALIBRATED_POSE_REQUIRED');
assert.equal(stopMission(mission).status, 'IDLE');
console.log('mission controller tests passed');
