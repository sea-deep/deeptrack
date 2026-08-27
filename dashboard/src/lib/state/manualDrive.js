/** @param {number} value */
const clampPercent = (value) => Math.max(-100, Math.min(100,
  Math.round(Number.isFinite(value) ? value : 0)));

/** Match the rover's directional front-gate definition. */
/** @param {number} left @param {number} right */
export const requestsForward = (left, right) => left + right > 0;

/**
 * Resolve a requested vector before it reaches the serial writer. Firmware
 * remains authoritative, but known blocked/invalid forward motion is also
 * neutralized at the UI boundary.
 * @param {number} left
 * @param {number} right
 * @param {{controlReady:boolean,manualMode:boolean,forwardAllowed:boolean}} state
 */
export function gateManualDrive(left, right, state) {
  const requested = { left: clampPercent(left), right: clampPercent(right) };
  if (!state.controlReady || !state.manualMode)
    return { left: 0, right: 0, allowed: false, reason: 'CONTROL_LOCKED' };
  if (requestsForward(requested.left, requested.right) &&
      !state.forwardAllowed)
    return { left: 0, right: 0, allowed: false, reason: 'FORWARD_HELD' };
  return { ...requested, allowed: true, reason: 'ALLOWED' };
}

/** @param {{w:boolean,a:boolean,s:boolean,d:boolean}} keys @param {number} speed @param {boolean} forwardAllowed */
export function keyboardDriveVector(keys, speed, forwardAllowed) {
  const bounded = Math.max(0, Math.min(100, speed));
  const linear = (keys.w && forwardAllowed ? bounded : 0) -
    (keys.s ? bounded : 0);
  const turn = (keys.d ? bounded : 0) - (keys.a ? bounded : 0);
  return {
    left: clampPercent(linear + turn),
    right: clampPercent(linear - turn)
  };
}
