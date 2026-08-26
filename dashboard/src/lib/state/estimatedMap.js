const CELL_SIZE_M = 0.06;
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
    trajectory: []
  };
}

/** @param {Record<string, any>} packet */
function calibrationFrom(packet) {
  const track = packet.track_width_mm;
  const tick = packet.micrometers_per_tick;
  const chassis = packet.chassis_width_mm;
  if (!Number.isFinite(track) || track < 100 ||
      !Number.isFinite(tick) || tick < 100 ||
      !Number.isFinite(chassis) || chassis < 100) return null;
  return {
    track_width_m: track / 1000,
    meters_per_tick: tick / 1_000_000,
    chassis_width_m: chassis / 1000
  };
}

/** @param {any} state @param {Record<string, any>} packet @returns {any} */
export function updateEstimatedPose(state, packet) {
  const calibration = calibrationFrom(packet);
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
        known: true, x_m: 0, y_m: 0, heading_rad: 0,
        confidence: 0.72, reason: 'DEAD_RECKONING_ESTIMATE'
      },
      trajectory: [{ x_m: 0, y_m: 0 }]
    };
  }
  const deltaLeftTicks = ticks.left - state.previousTicks.left;
  const deltaRightTicks = ticks.right - state.previousTicks.right;
  if (Math.abs(deltaLeftTicks) > 10000 || Math.abs(deltaRightTicks) > 10000) {
    return {
      ...state,
      previousTicks: ticks,
      calibration,
      pose: { ...state.pose, known: false, confidence: 0,
        reason: 'ENCODER_DISCONTINUITY' }
    };
  }
  const leftM = deltaLeftTicks * calibration.meters_per_tick;
  const rightM = deltaRightTicks * calibration.meters_per_tick;
  const distanceM = (leftM + rightM) / 2;
  const deltaHeading = (rightM - leftM) / calibration.track_width_m;
  const midHeading = state.pose.heading_rad + deltaHeading / 2;
  const pose = {
    known: true,
    x_m: state.pose.x_m + distanceM * Math.cos(midHeading),
    y_m: state.pose.y_m + distanceM * Math.sin(midHeading),
    heading_rad: state.pose.heading_rad + deltaHeading,
    confidence: clamp(state.pose.confidence - Math.abs(distanceM) * 0.015 -
                      Math.abs(deltaHeading) * 0.02, 0.20, 0.95),
    reason: 'DEAD_RECKONING_ESTIMATE'
  };
  return {
    ...state,
    previousTicks: ticks,
    calibration,
    pose,
    trajectory: [...state.trajectory.slice(-499),
      { x_m: pose.x_m, y_m: pose.y_m }]
  };
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
  const originX = cellOf(state.pose.x_m);
  const originY = cellOf(state.pose.y_m);
  for (const point of scanPoints) {
    if (point.valid !== true || !Number.isFinite(point.distance_mm) ||
        point.distance_mm < 30 || point.distance_mm > 1200 ||
        (Number.isFinite(point.confidence_pct) && point.confidence_pct < 50)) {
      continue;
    }
    const relative = (point.angle_deg - 90) * Math.PI / 180;
    const angle = state.pose.heading_rad + relative;
    const distanceM = point.distance_mm / 1000;
    const endX = cellOf(state.pose.x_m + Math.cos(angle) * distanceM);
    const endY = cellOf(state.pose.y_m + Math.sin(angle) * distanceM);
    traceRay(cells, originX, originY, endX, endY, timestampMs);
  }
  return { ...state, cells };
}

/** @param {any} state @returns {Set<string>} */
function inflatedOccupiedKeys(state) {
  const keys = new Set();
  if (!state.calibration) return keys;
  const radius = Math.ceil(
    (state.calibration.chassis_width_m / 2 + 0.05) / CELL_SIZE_M
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
  const queue = [start];
  /** @type {Map<string, string | null>} */
  const previous = new Map([[startKey, null]]);
  for (let index = 0; index < queue.length && index < maxVisited; ++index) {
    const current = queue[index];
    const currentKey = keyOf(current.x, current.y);
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
      if (previous.has(nextKey) ||
          (nextKey !== startKey &&
           !isKnownTraversableWithInflation(state, inflated, next.x, next.y)))
        continue;
      previous.set(nextKey, currentKey);
      queue.push(next);
    }
  }
  return null;
}
