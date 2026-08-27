import {
  CELL_SIZE_M, detectFrontierClusters, planKnownFreePath,
  scoreFrontierClusters, validateGoal
} from './estimatedMap.js';
import { roverCalibration } from '../config/roverCalibration.js';

const TURN_THRESHOLD_RAD = 20 * Math.PI / 180;
const GOAL_TOLERANCE_M = 0.09;
const ROUTE_PROGRESS_TIMEOUT_MS = 2500;

/** @typedef {{x:number,y:number}} Cell */
/** @typedef {{mode:string,status:string,reason:string,goal:Cell|null,path:Cell[],pathIndex:number,failedRoutes:any[],lastProgressAtMs:number,lastDistanceM:number,targetKey:string|null}} MissionState */

/** @param {number} value @param {number} low @param {number} high */
const clamp = (value, low, high) => Math.max(low, Math.min(high, value));
/** @param {number} value */
const normalizeAngle = (value) => {
  let angle = value;
  while (angle > Math.PI) angle -= 2 * Math.PI;
  while (angle <= -Math.PI) angle += 2 * Math.PI;
  return angle;
};
/** @param {{x_m:number,y_m:number}} pose */
const poseCell = (pose) => ({
  x: Math.floor(pose.x_m / CELL_SIZE_M),
  y: Math.floor(pose.y_m / CELL_SIZE_M)
});

/** @returns {MissionState} */
export function createMissionControllerState() {
  return {
    mode: 'IDLE', status: 'IDLE', reason: 'OPERATOR_CONTROL',
    goal: null, path: [], pathIndex: 0, failedRoutes: [],
    lastProgressAtMs: 0, lastDistanceM: Infinity, targetKey: null
  };
}

/** Start NAVIGATE, EXPLORE, or RETURN_HOME without producing motor output. */
/** @param {MissionState} state @param {string} mode @param {any} mapState @param {Cell|null} requestedGoal @param {number} nowMs @returns {MissionState} */
export function startMission(state, mode, mapState, requestedGoal = null,
                             nowMs = 0) {
  if (!mapState?.calibration || !mapState?.pose?.known)
    return { ...state, mode: 'IDLE', status: 'BLOCKED',
      reason: 'CALIBRATED_POSE_REQUIRED' };
  const start = poseCell(mapState.pose);
  /** @type {Cell|null} */
  let goal = requestedGoal;
  if (mode === 'RETURN_HOME') goal = {
    x: Math.floor(roverCalibration.pose.startXM / CELL_SIZE_M),
    y: Math.floor(roverCalibration.pose.startYM / CELL_SIZE_M)
  };
  if (mode === 'EXPLORE') {
    const clusters = detectFrontierClusters(mapState, { minCells: 2, start });
    const ranked = scoreFrontierClusters(mapState, clusters, {
      poseUncertainty: 1 - mapState.pose.confidence,
      failedRoutes: state.failedRoutes
    });
    goal = ranked[0]?.approach || null;
    if (!goal) return { ...state, mode: 'IDLE', status: 'BLOCKED',
      reason: 'NO_REACHABLE_FRONTIER' };
  }
  if (!goal) return { ...state, mode: 'IDLE', status: 'BLOCKED',
    reason: 'GOAL_REQUIRED' };
  const validated = validateGoal(mapState, goal, 4, start);
  if (!validated.accepted) return { ...state, mode: 'IDLE', status: 'BLOCKED',
    reason: validated.reason };
  if (!validated.goal) return { ...state, mode: 'IDLE', status: 'BLOCKED',
    reason: validated.reason };
  const path = planKnownFreePath(mapState, start, validated.goal);
  if (!path) return { ...state, mode: 'IDLE', status: 'BLOCKED',
    reason: 'NO_KNOWN_FREE_PATH' };
  return {
    ...state, mode, status: 'RUNNING', reason: validated.reason,
    goal: validated.goal, path, pathIndex: Math.min(1, path.length - 1),
    lastProgressAtMs: nowMs, lastDistanceM: Infinity, targetKey: null
  };
}

/** @param {MissionState} state @param {string} reason @returns {MissionState} */
export function stopMission(state, reason = 'OPERATOR_STOP') {
  return { ...state, mode: 'IDLE', status: 'IDLE', reason,
    goal: null, path: [], pathIndex: 0, targetKey: null };
}

/**
 * One fail-closed navigation tick. The caller must refresh the returned drive
 * command within protocol v2's TTL; any missing tick therefore stops motion.
 */
/** @param {MissionState} state @param {any} mapState @param {any} safety @param {number} nowMs */
export function stepMission(state, mapState, safety, nowMs) {
  /** @param {MissionState} next @param {string} reason */
  const stopped = (next, reason) => ({
    state: { ...next, status: 'BLOCKED', reason }, drive: { left: 0, right: 0 }
  });
  if (state.status !== 'RUNNING') return stopped(state, state.reason);
  if (!safety?.controlReady) return stopped(state, 'CONTROL_LINK_NOT_READY');
  if (!safety.frontValid || !safety.frontFresh)
    return stopped(state, 'FRONT_CLEARANCE_UNKNOWN');
  if (safety.frontBlocked) return stopped(state, 'FRONT_CLEARANCE_BLOCKED');
  if (!mapState?.calibration || !mapState?.pose?.known)
    return stopped(state, 'CALIBRATED_POSE_REQUIRED');

  const pose = mapState.pose;
  const start = poseCell(pose);
  let next = state;
  let path = state.path;
  if (!path.length || !state.goal) return stopped(state, 'ROUTE_MISSING');

  // Re-plan against every new occupancy update. Unknown space is never driven.
  const replanned = planKnownFreePath(mapState, start, state.goal);
  if (!replanned) {
    const failedRoutes = [...state.failedRoutes.slice(-15), {
      ...state.goal, attempts: 1, toleranceCells: 2
    }];
    return stopped({ ...state, failedRoutes }, 'ROUTE_BLOCKED');
  }
  path = replanned;
  let index = Math.min(1, path.length - 1);
  let target = path[index];
  const targetKey = `${target.x},${target.y}`;
  let targetX = (target.x + 0.5) * CELL_SIZE_M;
  let targetY = (target.y + 0.5) * CELL_SIZE_M;
  let distance = Math.hypot(targetX - pose.x_m, targetY - pose.y_m);
  if (path.length === 1 ||
      Math.hypot((state.goal.x + 0.5) * CELL_SIZE_M - pose.x_m,
                 (state.goal.y + 0.5) * CELL_SIZE_M - pose.y_m) <=
        GOAL_TOLERANCE_M) {
    const completed = { ...state, status: 'COMPLETE', reason: 'GOAL_REACHED',
      path, pathIndex: path.length - 1 };
    return { state: completed, drive: { left: 0, right: 0 } };
  }

  const newTarget = targetKey !== state.targetKey;
  const madeProgress = newTarget || distance + 0.015 < state.lastDistanceM;
  if (madeProgress) next = { ...next, lastDistanceM: distance, targetKey,
    lastProgressAtMs: nowMs };
  else if (nowMs - state.lastProgressAtMs > ROUTE_PROGRESS_TIMEOUT_MS)
    return stopped(state, 'NO_ODOMETRY_PROGRESS');

  const desiredHeading = Math.atan2(targetY - pose.y_m, targetX - pose.x_m);
  const error = normalizeAngle(desiredHeading - pose.heading_rad);
  const turnSign = error >= 0 ? 1 : -1;
  let drive;
  if (Math.abs(error) > TURN_THRESHOLD_RAD) {
    const turn = clamp(25 + Math.abs(error) * 8, 25, 38);
    drive = { left: Math.round(-turnSign * turn),
      right: Math.round(turnSign * turn) };
  } else {
    const base = clamp(safety.speedPercent ?? 35, 30, 45);
    const correction = clamp(error * 32, -14, 14);
    drive = { left: Math.round(base - correction),
      right: Math.round(base + correction) };
  }
  return { state: { ...next, path, pathIndex: index, reason: 'FOLLOWING_ROUTE' },
    drive };
}
