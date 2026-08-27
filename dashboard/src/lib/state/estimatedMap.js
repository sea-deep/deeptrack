import { roverCalibration } from '../config/roverCalibration.js';

export const CELL_SIZE_M = roverCalibration.map.cellSizeM;
const FREE_DELTA = -0.45;
const OCCUPIED_DELTA = 0.90;
const FREE_LIMIT = -0.80;
const OCCUPIED_LIMIT = 0.80;

/** @param {number} value @param {number} min @param {number} max */
const clamp = (value, min, max) => Math.max(min, Math.min(max, value));
/** @param {number} x @param {number} y */
const keyOf = (x, y) => String(x) + ',' + String(y);
/** @param {number} meters */
const cellOf = (meters) => Math.floor(meters / CELL_SIZE_M);

export function createEstimatedMapState() {
  return {
    pose: {
      known: false, x_m: 0, y_m: 0, heading_rad: 0,
      confidence: 0, reason: 'ENCODER_CALIBRATION_REQUIRED'
    },
    previousTicks: null,
    calibration: null,
    cells: new Map(),
    trajectory: [],
    poseHistory: []
  };
}

/** @param {Record<string, any>} packet */
function calibrationFrom(packet, config = roverCalibration) {
  const liveTrack = packet.track_width_mm;
  const liveLeftTick = packet.left_micrometers_per_tick ?? packet.micrometers_per_tick;
  const liveRightTick = packet.right_micrometers_per_tick ?? packet.micrometers_per_tick;
  const liveChassis = packet.chassis_width_mm;
  const useLive = config.preferLiveTelemetry &&
    Number.isFinite(liveTrack) && liveTrack >= 100 &&
    Number.isFinite(liveLeftTick) && liveLeftTick >= 100 &&
    Number.isFinite(liveRightTick) && liveRightTick >= 100 &&
    Number.isFinite(liveChassis) && liveChassis >= 100;
  const track = useLive ? liveTrack : config.assumeCalibrated
    ? config.geometry.trackWidthMm : null;
  const leftTick = useLive ? liveLeftTick : config.assumeCalibrated
    ? config.geometry.leftMicrometersPerTick : null;
  const rightTick = useLive ? liveRightTick : config.assumeCalibrated
    ? config.geometry.rightMicrometersPerTick : null;
  const chassis = useLive ? liveChassis : config.assumeCalibrated
    ? config.geometry.chassisWidthMm : null;
  if (!Number.isFinite(track) || track < 100 ||
      !Number.isFinite(leftTick) || leftTick < 100 ||
      !Number.isFinite(rightTick) || rightTick < 100 ||
      !Number.isFinite(chassis) || chassis < 100) return null;
  return {
    track_width_m: track / 1000,
    left_meters_per_tick: leftTick / 1_000_000 * config.geometry.leftDistanceScale,
    right_meters_per_tick: rightTick / 1_000_000 * config.geometry.rightDistanceScale,
    chassis_width_m: chassis / 1000,
    source: useLive ? 'ROVER_NVS' : 'DASHBOARD_ASSUMED'
  };
}

/** @param {number} angle */
function normalizeAngle(angle) {
  let result = angle;
  while (result > Math.PI) result -= Math.PI * 2;
  while (result <= -Math.PI) result += Math.PI * 2;
  return result;
}

/** @param {number} from @param {number} to */
function shortestAngleDelta(from, to) {
  return normalizeAngle(to - from);
}

/** @param {Record<string, any>} packet */
function packetTimestamp(packet) {
  return Number.isFinite(packet.timestamp_ms) ? packet.timestamp_ms : null;
}

/** @param {any} state @param {Record<string, any>} packet @returns {any} */
export function updateEstimatedPose(state, packet, config = roverCalibration) {
  const calibration = calibrationFrom(packet, config);
  if (!calibration || !Number.isInteger(packet.left_ticks) ||
      !Number.isInteger(packet.right_ticks)) {
    return {
      ...state,
      pose: { ...state.pose, known: false, confidence: 0,
        reason: 'ENCODER_CALIBRATION_REQUIRED' },
      calibration
    };
  }
  const ticks = { left: packet.left_ticks, right: packet.right_ticks };
  if (!state.previousTicks) {
    return {
      ...state,
      previousTicks: ticks,
      calibration,
      pose: {
        known: true, x_m: config.pose.startXM, y_m: config.pose.startYM,
        heading_rad: config.pose.startHeadingDeg * Math.PI / 180,
        confidence: config.pose.initialConfidence,
        reason: calibration.source === 'ROVER_NVS'
          ? 'DEAD_RECKONING_ESTIMATE' : 'ASSUMED_DASHBOARD_CALIBRATION'
      },
      trajectory: [{ x_m: config.pose.startXM, y_m: config.pose.startYM }],
      poseHistory: packetTimestamp(packet) === null ? [] : [{
        timestamp_ms: packetTimestamp(packet), x_m: config.pose.startXM,
        y_m: config.pose.startYM,
        heading_rad: config.pose.startHeadingDeg * Math.PI / 180,
        confidence: config.pose.initialConfidence
      }]
    };
  }
  const deltaLeftTicks = ticks.left - state.previousTicks.left;
  const deltaRightTicks = ticks.right - state.previousTicks.right;
  if (Math.abs(deltaLeftTicks) > config.pose.discontinuityTicks ||
      Math.abs(deltaRightTicks) > config.pose.discontinuityTicks) {
    return {
      ...state,
      previousTicks: ticks,
      calibration,
      pose: { ...state.pose, known: false, confidence: 0,
        reason: 'ENCODER_DISCONTINUITY' }
    };
  }
  const leftM = deltaLeftTicks * calibration.left_meters_per_tick;
  const rightM = deltaRightTicks * calibration.right_meters_per_tick;
  const distanceM = (leftM + rightM) / 2;
  const encoderDeltaHeading = config.pose.encoderHeadingSign *
    (rightM - leftM) / calibration.track_width_m;
  let fusedHeading = state.pose.heading_rad + encoderDeltaHeading;
  if (Number.isFinite(packet.heading_deg)) {
    const imuHeading = (config.pose.imuHeadingSign * packet.heading_deg +
      config.pose.imuHeadingOffsetDeg) * Math.PI / 180;
    // MPU6050 yaw is relative and drifting, so it provides a bounded
    // complementary correction rather than replacing encoder geometry.
    fusedHeading += config.pose.imuFusionWeight *
      shortestAngleDelta(fusedHeading, imuHeading);
  }
  fusedHeading = normalizeAngle(fusedHeading);
  const fusedDeltaHeading = shortestAngleDelta(state.pose.heading_rad, fusedHeading);
  const midHeading = state.pose.heading_rad + fusedDeltaHeading / 2;
  const pose = {
    known: true,
    x_m: state.pose.x_m + distanceM * Math.cos(midHeading),
    y_m: state.pose.y_m + distanceM * Math.sin(midHeading),
    heading_rad: fusedHeading,
    confidence: clamp(state.pose.confidence - Math.abs(distanceM) *
                      config.pose.confidenceLossPerMeter -
                      Math.abs(fusedDeltaHeading) *
                      config.pose.confidenceLossPerRadian,
                      config.pose.minimumConfidence,
                      config.pose.maximumConfidence),
    reason: 'DEAD_RECKONING_ESTIMATE'
  };
  return {
    ...state,
    previousTicks: ticks,
    calibration,
    pose,
    trajectory: [...state.trajectory.slice(-499),
      { x_m: pose.x_m, y_m: pose.y_m }],
    poseHistory: packetTimestamp(packet) === null
      ? state.poseHistory
      : [...state.poseHistory.slice(-255), {
          timestamp_ms: packetTimestamp(packet), x_m: pose.x_m, y_m: pose.y_m,
          heading_rad: pose.heading_rad, confidence: pose.confidence
        }]
  };
}

/** @param {Array<Record<string, number>>} history @param {number} timestampMs */
export function interpolatePose(history, timestampMs) {
  if (!history.length || !Number.isFinite(timestampMs)) return null;
  if (timestampMs <= history[0].timestamp_ms) return { ...history[0] };
  const last = history[history.length - 1];
  if (timestampMs >= last.timestamp_ms) return { ...last };
  for (let index = 1; index < history.length; ++index) {
    const after = history[index];
    if (after.timestamp_ms < timestampMs) continue;
    const before = history[index - 1];
    const span = after.timestamp_ms - before.timestamp_ms;
    const ratio = span > 0 ? (timestampMs - before.timestamp_ms) / span : 0;
    return {
      timestamp_ms: timestampMs,
      x_m: before.x_m + (after.x_m - before.x_m) * ratio,
      y_m: before.y_m + (after.y_m - before.y_m) * ratio,
      heading_rad: normalizeAngle(
        before.heading_rad + shortestAngleDelta(
          before.heading_rad, after.heading_rad
        ) * ratio
      ),
      confidence: before.confidence +
        (after.confidence - before.confidence) * ratio
    };
  }
  return null;
}

/** @param {Map<string, any>} cells @param {number} x @param {number} y @param {number} delta @param {number} timestampMs */
function addEvidence(cells, x, y, delta, timestampMs) {
  const key = keyOf(x, y);
  const previous = cells.get(key);
  const logOdds = clamp((previous?.log_odds || 0) + delta, -3, 3);
  cells.set(key, {
    x, y, log_odds: logOdds,
    classification: logOdds >= OCCUPIED_LIMIT
      ? 'OCCUPIED' : logOdds <= FREE_LIMIT ? 'KNOWN_FREE' : 'UNCERTAIN',
    observations: (previous?.observations || 0) + 1,
    updated_at_ms: timestampMs
  });
}

/** @param {Map<string, any>} cells @param {number} x0 @param {number} y0 @param {number} x1 @param {number} y1 @param {number} timestampMs */
function traceRay(cells, x0, y0, x1, y1, timestampMs) {
  let x = x0;
  let y = y0;
  const dx = Math.abs(x1 - x0);
  const sx = x0 < x1 ? 1 : -1;
  const dy = -Math.abs(y1 - y0);
  const sy = y0 < y1 ? 1 : -1;
  let error = dx + dy;
  while (x !== x1 || y !== y1) {
    addEvidence(cells, x, y, FREE_DELTA, timestampMs);
    const doubled = 2 * error;
    if (doubled >= dy) { error += dy; x += sx; }
    if (doubled <= dx) { error += dx; y += sy; }
  }
  addEvidence(cells, x1, y1, OCCUPIED_DELTA, timestampMs);
}

/** @param {any} state @param {Array<Record<string, any>>} scanPoints @param {number} timestampMs @returns {any} */
export function addScanEvidence(state, scanPoints, timestampMs) {
  if (!state.pose.known) return state;
  const cells = new Map(state.cells);
  for (const point of scanPoints) {
    if (point.valid !== true || !Number.isFinite(point.distance_mm) ||
        point.distance_mm < roverCalibration.scanner.minimumRangeMm ||
        point.distance_mm > roverCalibration.scanner.maximumRangeMm ||
        (Number.isFinite(point.confidence_pct) &&
         point.confidence_pct < roverCalibration.scanner.minimumConfidencePct)) {
      continue;
    }
    const observationTime = Number.isFinite(point.timestamp_ms)
      ? point.timestamp_ms : timestampMs;
    const samplePose = interpolatePose(state.poseHistory, observationTime) ||
      state.pose;
    const originX = cellOf(samplePose.x_m);
    const originY = cellOf(samplePose.y_m);
    const relative = (point.angle_deg -
      roverCalibration.scanner.forwardAngleDeg +
      roverCalibration.scanner.bearingOffsetDeg) * Math.PI / 180;
    const angle = samplePose.heading_rad + relative;
    const distanceM = point.distance_mm / 1000;
    const endX = cellOf(samplePose.x_m + Math.cos(angle) * distanceM);
    const endY = cellOf(samplePose.y_m + Math.sin(angle) * distanceM);
    traceRay(cells, originX, originY, endX, endY, observationTime);
  }
  return { ...state, cells };
}

/** @param {any} state @returns {Set<string>} */
function inflatedOccupiedKeys(state) {
  const keys = new Set();
  if (!state.calibration) return keys;
  const radius = Math.ceil(
    (state.calibration.chassis_width_m / 2 +
      roverCalibration.map.footprintMarginM) / CELL_SIZE_M
  );
  for (const cell of state.cells.values()) {
    if (cell.classification !== 'OCCUPIED') continue;
    for (let dx = -radius; dx <= radius; ++dx) {
      for (let dy = -radius; dy <= radius; ++dy) {
        if (dx * dx + dy * dy <= radius * radius)
          keys.add(keyOf(cell.x + dx, cell.y + dy));
      }
    }
  }
  return keys;
}

/** @param {any} state @param {number} x @param {number} y */
export function isKnownTraversable(state, x, y) {
  const cell = state.cells.get(keyOf(x, y));
  return cell?.classification === 'KNOWN_FREE' &&
         !inflatedOccupiedKeys(state).has(keyOf(x, y));
}

/** @param {any} state @param {Set<string>} inflated @param {number} x @param {number} y */
function isKnownTraversableWithInflation(state, inflated, x, y) {
  const cell = state.cells.get(keyOf(x, y));
  return cell?.classification === 'KNOWN_FREE' &&
         !inflated.has(keyOf(x, y));
}

/** @param {any} state */
export function renderableEvidence(state) {
  const inflated = inflatedOccupiedKeys(state);
  const frontierClusters = detectFrontierClusters(state, { minCells: 2 });
  return {
    occupied: [...state.cells.values()]
      .filter((cell) => cell.classification === 'OCCUPIED'),
    knownFree: [...state.cells.values()]
      .filter((cell) => cell.classification === 'KNOWN_FREE' &&
                        !inflated.has(keyOf(cell.x, cell.y))),
    inflated: [...inflated].map((key) => {
      const [x, y] = key.split(',').map(Number);
      return { x, y };
    }),
    frontiers: frontierClusters.flatMap((cluster) =>
      cluster.cells.map((cell) => ({ ...cell, clusterId: cluster.id }))
    ),
    trajectory: state.trajectory.map((/** @type {any} */ point) => ({ ...point })),
    cellSizeM: CELL_SIZE_M
  };
}

/** @param {any} state @param {{x:number,y:number}} start @param {{x:number,y:number}} goal @param {number} maxVisited */
export function planKnownFreePath(state, start, goal, maxVisited = 4000) {
  const inflated = inflatedOccupiedKeys(state);
  if (!isKnownTraversableWithInflation(state, inflated, goal.x, goal.y))
    return null;
  const startKey = keyOf(start.x, start.y);
  const goalKey = keyOf(goal.x, goal.y);
  if (startKey === goalKey) return [{ ...start }];
  const open = [{ ...start, score: Math.abs(goal.x - start.x) +
    Math.abs(goal.y - start.y) }];
  const openKeys = new Set([startKey]);
  const closed = new Set();
  const cost = new Map([[startKey, 0]]);
  /** @type {Map<string, string | null>} */
  const previous = new Map([[startKey, null]]);
  let visited = 0;
  while (open.length && visited++ < maxVisited) {
    let bestIndex = 0;
    for (let index = 1; index < open.length; ++index) {
      if (open[index].score < open[bestIndex].score) bestIndex = index;
    }
    const current = open.splice(bestIndex, 1)[0];
    const currentKey = keyOf(current.x, current.y);
    openKeys.delete(currentKey);
    if (closed.has(currentKey)) continue;
    closed.add(currentKey);
    if (currentKey === goalKey) {
      const path = [];
      /** @type {string | null | undefined} */
      let cursor = currentKey;
      while (cursor) {
        const [x, y] = cursor.split(',').map(Number);
        path.push({ x, y });
        cursor = previous.get(cursor);
      }
      return path.reverse();
    }
    for (const [dx, dy] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
      const next = { x: current.x + dx, y: current.y + dy };
      const nextKey = keyOf(next.x, next.y);
      if (closed.has(nextKey) ||
          (nextKey !== startKey &&
           !isKnownTraversableWithInflation(state, inflated, next.x, next.y)))
        continue;
      const tentativeCost = (cost.get(currentKey) ?? Infinity) + 1;
      if (tentativeCost >= (cost.get(nextKey) ?? Infinity)) continue;
      cost.set(nextKey, tentativeCost);
      previous.set(nextKey, currentKey);
      const score = tentativeCost + Math.abs(goal.x - next.x) +
        Math.abs(goal.y - next.y);
      if (!openKeys.has(nextKey)) {
        open.push({ ...next, score });
        openKeys.add(nextKey);
      } else {
        const item = open.find((candidate) =>
          candidate.x === next.x && candidate.y === next.y
        );
        if (item) item.score = score;
      }
    }
  }
  return null;
}

const CARDINAL_NEIGHBOURS = [[1, 0], [-1, 0], [0, 1], [0, -1]];
const EIGHT_NEIGHBOURS = [
  ...CARDINAL_NEIGHBOURS,
  [1, 1], [1, -1], [-1, 1], [-1, -1]
];

/** @param {any} state @param {number} x @param {number} y */
function isUnknown(state, x, y) {
  const cell = state.cells.get(keyOf(x, y));
  return !cell || cell.classification === 'UNCERTAIN';
}

/**
 * Frontier cells are safe known-free approach cells bordering unknown space.
 * Clustering is geometric; optional reachability is checked with A* through
 * already-known, inflated free space.
 * @param {any} state
 * @param {{minCells?:number,start?:{x:number,y:number}|null}} options
 */
export function detectFrontierClusters(
  state, { minCells = 2, start = null } = {}
) {
  const inflated = inflatedOccupiedKeys(state);
  const frontierKeys = new Set();
  for (const cell of state.cells.values()) {
    if (!isKnownTraversableWithInflation(state, inflated, cell.x, cell.y))
      continue;
    if (CARDINAL_NEIGHBOURS.some(([dx, dy]) =>
      isUnknown(state, cell.x + dx, cell.y + dy)
    )) frontierKeys.add(keyOf(cell.x, cell.y));
  }

  const clusters = [];
  const remaining = new Set(frontierKeys);
  while (remaining.size) {
    const seed = remaining.values().next().value;
    remaining.delete(seed);
    const queue = [seed];
    const cells = [];
    for (let index = 0; index < queue.length; ++index) {
      const current = queue[index];
      const [x, y] = current.split(',').map(Number);
      cells.push({ x, y });
      for (const [dx, dy] of EIGHT_NEIGHBOURS) {
        const neighbour = keyOf(x + dx, y + dy);
        if (!remaining.delete(neighbour)) continue;
        queue.push(neighbour);
      }
    }
    if (cells.length < minCells) continue;
    const centroid = {
      x: cells.reduce((sum, cell) => sum + cell.x, 0) / cells.length,
      y: cells.reduce((sum, cell) => sum + cell.y, 0) / cells.length
    };
    const approach = cells.reduce((best, cell) =>
      Math.hypot(cell.x - centroid.x, cell.y - centroid.y) <
      Math.hypot(best.x - centroid.x, best.y - centroid.y) ? cell : best
    , cells[0]);
    const path = start ? planKnownFreePath(state, start, approach) : null;
    if (start && !path) continue;
    const unknown = new Set();
    for (const cell of cells) {
      for (const [dx, dy] of CARDINAL_NEIGHBOURS) {
        if (isUnknown(state, cell.x + dx, cell.y + dy))
          unknown.add(keyOf(cell.x + dx, cell.y + dy));
      }
    }
    clusters.push({
      id: `F${clusters.length + 1}`,
      cells, centroid, approach, path,
      informationGain: unknown.size
    });
  }
  return clusters;
}

/** @param {any} state @param {{x:number,y:number}} point @param {Set<string>} inflated */
function clearanceCells(state, point, inflated) {
  for (let radius = 0; radius <= 10; ++radius) {
    for (let dx = -radius; dx <= radius; ++dx) {
      for (let dy = -radius; dy <= radius; ++dy) {
        if (Math.max(Math.abs(dx), Math.abs(dy)) !== radius) continue;
        if (inflated.has(keyOf(point.x + dx, point.y + dy))) return radius;
      }
    }
  }
  return 11;
}

/**
 * Returns the complete score breakdown so route choice is inspectable.
 * @param {any} state @param {Array<any>} clusters
 * @param {{communicationConfidence?:number,poseUncertainty?:number,failedRoutes?:Array<any>,weights?:Record<string,number>}} context
 */
export function scoreFrontierClusters(state, clusters, context = {}) {
  const inflated = inflatedOccupiedKeys(state);
  const weights = {
    informationGain: 1.0, clearance: 0.7, untriedBranchBonus: 2.0,
    communicationConfidence: 1.0, pathLength: 0.45, turnCost: 0.3,
    poseUncertainty: 2.0, revisitPenalty: 0.25, failedRoutePenalty: 3.0,
    ...(context.weights || {})
  };
  return clusters.map((cluster) => {
    const pathLength = Math.max(0, (cluster.path?.length || 1) - 1);
    const path = cluster.path || [];
    let turnCost = 0;
    for (let index = 2; index < path.length; ++index) {
      const a = path[index - 2];
      const b = path[index - 1];
      const point = path[index];
      if (b.x - a.x !== point.x - b.x ||
          b.y - a.y !== point.y - b.y) ++turnCost;
    }
    let revisitTotal = 0;
    for (const cell of cluster.cells) {
      revisitTotal += Math.max(0,
        (state.cells.get(keyOf(cell.x, cell.y))?.observations || 1) - 1
      );
    }
    const revisitPenalty = revisitTotal / cluster.cells.length;
    const failedRoutePenalty = (context.failedRoutes || []).reduce(
      (sum, failed) => sum +
        (Math.hypot(failed.x - cluster.approach.x,
                    failed.y - cluster.approach.y) <= (failed.toleranceCells || 2)
          ? (failed.attempts || 1) : 0), 0
    );
    const breakdown = {
      informationGain: cluster.informationGain * weights.informationGain,
      clearance: clearanceCells(state, cluster.approach, inflated) * weights.clearance,
      untriedBranchBonus: weights.untriedBranchBonus,
      communicationConfidence: (context.communicationConfidence ?? 1) *
        weights.communicationConfidence,
      pathLength: -pathLength * weights.pathLength,
      turnCost: -turnCost * weights.turnCost,
      poseUncertainty: -(context.poseUncertainty ?? 0) * weights.poseUncertainty,
      revisitPenalty: -revisitPenalty * weights.revisitPenalty,
      failedRoutePenalty: -failedRoutePenalty * weights.failedRoutePenalty
    };
    return {
      ...cluster, breakdown,
      score: Object.values(breakdown).reduce((sum, value) => sum + value, 0)
    };
  }).sort((a, b) => b.score - a.score);
}

/** @param {any} state @param {{x:number,y:number}} goal @param {number} snapRadius @param {{x:number,y:number}|null} start */
export function validateGoal(state, goal, snapRadius = 4, start = null) {
  if (!Number.isInteger(goal?.x) || !Number.isInteger(goal?.y))
    return { accepted: false, reason: 'INVALID_MAP_COORDINATE', goal: null };
  if (isKnownTraversable(state, goal.x, goal.y) &&
      (!start || planKnownFreePath(state, start, goal)))
    return { accepted: true, reason: 'KNOWN_FREE', goal: { ...goal } };
  for (let radius = 1; radius <= snapRadius; ++radius) {
    const candidates = [];
    for (let dx = -radius; dx <= radius; ++dx) {
      for (let dy = -radius; dy <= radius; ++dy) {
        if (Math.max(Math.abs(dx), Math.abs(dy)) !== radius) continue;
        const candidate = { x: goal.x + dx, y: goal.y + dy };
        if (isKnownTraversable(state, candidate.x, candidate.y) &&
            (!start || planKnownFreePath(state, start, candidate)))
          candidates.push(candidate);
      }
    }
    if (candidates.length) {
      candidates.sort((a, b) =>
        Math.hypot(a.x - goal.x, a.y - goal.y) -
        Math.hypot(b.x - goal.x, b.y - goal.y)
      );
      return { accepted: true, reason: 'SNAPPED_TO_KNOWN_FREE',
        goal: candidates[0] };
    }
  }
  const cell = state.cells.get(keyOf(goal.x, goal.y));
  return { accepted: false,
    reason: cell?.classification === 'OCCUPIED'
      ? 'OCCUPIED_OR_INFLATED' : 'UNKNOWN_OR_UNREACHABLE', goal: null };
}

/** @param {{x:number,y:number,headingRad:number}} first @param {{x:number,y:number,headingRad:number}} second @param {number} positionTolerance @param {number} headingTolerance */
export function branchPoseMatches(first, second, positionTolerance = 0.20,
                                  headingTolerance = Math.PI / 8) {
  return Math.hypot(first.x - second.x, first.y - second.y) <=
      positionTolerance &&
    Math.abs(shortestAngleDelta(first.headingRad, second.headingRad)) <=
      headingTolerance;
}
