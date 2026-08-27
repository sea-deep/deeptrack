const STATE_THRESHOLDS = Object.freeze({ WATCH: 30, HIGH: 55, CRITICAL: 75 });

/** @typedef {{feature:string,category:string,label:string,value:number,z:number,contribution:number}} FeatureContribution */

/** @param {number} value @param {number} min @param {number} max */
const clamp = (value, min, max) => Math.min(max, Math.max(min, value));
/** @param {unknown} value @returns {number | null} */
const finite = (value) => Number.isFinite(value) ? Number(value) : null;

/** @param {number[]} values */
function median(values) {
  if (!values.length) return 0;
  const sorted = [...values].sort((a, b) => a - b);
  const middle = Math.floor(sorted.length / 2);
  return sorted.length % 2
    ? sorted[middle]
    : (sorted[middle - 1] + sorted[middle]) / 2;
}

/** Expected unsuccessful-search path length used by Isolation Forest. @param {number} size */
export function expectedPathLength(size) {
  if (size <= 1) return 0;
  if (size === 2) return 1;
  const harmonic = Math.log(size - 1) + 0.5772156649;
  return 2 * harmonic - (2 * (size - 1)) / size;
}

/**
 * Convert cumulative telemetry into a one-second feature window.
 * Unknown inputs remain null; the engine never substitutes zero for a sensor.
 * @param {Record<string, any>} telemetry
 * @param {Record<string, any> | null} previous
 * @param {number} dtSeconds
 * @param {{heartbeatAgeMs?: number | null}} context
 */
export function createHazardFeatures(
  telemetry, previous = null, dtSeconds = 1, context = {}
) {
  /** @param {string} key */
  const rate = (key) => {
    const current = finite(telemetry[key]);
    const before = finite(previous?.[key]);
    return current === null || before === null || dtSeconds <= 0
      ? null : (current - before) / dtSeconds;
  };
  const pitch = finite(telemetry.pitchDeg);
  const roll = finite(telemetry.rollDeg);
  const leftDelta = rate('encoderL');
  const rightDelta = rate('encoderR');
  const packetGapRate = rate('packetGaps');
  const temperatureRate = rate('temperature');
  const humidityRate = rate('humidity');
  const encoderMagnitude = leftDelta === null || rightDelta === null
    ? null : Math.abs(leftDelta) + Math.abs(rightDelta);
  const encoderMismatch = encoderMagnitude === null || encoderMagnitude < 1
    ? null
    : Math.abs(Math.abs(Number(leftDelta)) - Math.abs(Number(rightDelta))) /
      encoderMagnitude;
  const tiltMagnitude = pitch === null || roll === null
    ? null : Math.hypot(pitch, roll);

  return {
    gasRaw: finite(telemetry.gasRaw),
    gasRate: rate('gasRaw'),
    temperature: finite(telemetry.temperature),
    temperatureRate,
    humidity: finite(telemetry.humidity),
    humidityRate,
    climateCoupling: temperatureRate === null || humidityRate === null
      ? null : Math.max(0, temperatureRate) * Math.max(0, humidityRate),
    tiltMagnitude,
    waterRaw: finite(telemetry.waterRaw),
    waterRate: rate('waterRaw'),
    frontClearanceCm: finite(telemetry.frontDistanceCm),
    encoderMismatch,
    tiltEncoderCoupling: tiltMagnitude === null || encoderMismatch === null
      ? null : tiltMagnitude * encoderMismatch,
    packetGapRate,
    heartbeatAgeMs: finite(context.heartbeatAgeMs),
    rssiWeakness: Number.isFinite(telemetry.rssi)
      ? Math.max(0, -70 - Number(telemetry.rssi)) : null
  };
}

/** @param {Record<string, any>} node @param {Record<string, number | null>} features @param {number} depth */
function pathLength(node, features, depth = 0) {
  if (!node || node.leaf === true)
    return depth + expectedPathLength(Math.max(1, Number(node?.size) || 1));
  const value = finite(features[node.feature]);
  if (value === null)
    return depth + expectedPathLength(Math.max(1, Number(node.size) || 1));
  return pathLength(value < node.threshold ? node.left : node.right,
    features, depth + 1);
}

/** @param {Record<string, any>} model @param {Record<string, number | null>} features */
function isolationForestScore(model, features) {
  const trees = Array.isArray(model?.isolationForest?.trees)
    ? model.isolationForest.trees : [];
  if (!trees.length) return 0;
  const sampleSize = Number(model.isolationForest.sampleSize) || 64;
  const averagePath = trees.reduce(
    (/** @type {number} */ sum, /** @type {Record<string, any>} */ tree) =>
      sum + pathLength(tree, features), 0) / trees.length;
  return Math.pow(2, -averagePath / expectedPathLength(sampleSize));
}

/** @param {number} score */
export function hazardState(score) {
  if (score >= STATE_THRESHOLDS.CRITICAL) return 'CRITICAL';
  if (score >= STATE_THRESHOLDS.HIGH) return 'HIGH';
  if (score >= STATE_THRESHOLDS.WATCH) return 'WATCH';
  return 'NORMAL';
}

const ACTIONS = Object.freeze({
  NORMAL: 'Continue monitoring. Deterministic rover safety remains authoritative.',
  WATCH: 'Reduce speed if moving and watch the contributing signals for continued change.',
  HIGH: 'Hold position when safe and have the operator investigate the emerging environmental anomaly.',
  CRITICAL: 'Operator should stop in a safe position, withdraw if needed, and verify conditions with calibrated instruments.'
});

/** @param {string} feature @param {number} z @param {Record<string, number | null>} values */
function explainFeature(feature, z, values) {
  const variation = `${z.toFixed(1)}× outside learned variation`;
  /** @type {Record<string, string>} */
  const explanations = {
    gasRaw: `MQ-4 raw signal is ${variation}${(values.gasRate ?? 0) > 20 ? ' and rising' : ''}.`,
    gasRate: `MQ-4 raw signal is changing unusually quickly (${variation}).`,
    temperature: `Temperature is ${variation}.`,
    temperatureRate: `Temperature is rising faster than the deployment baseline (${variation}).`,
    humidity: `Humidity is ${variation}.`,
    humidityRate: `Humidity is changing faster than expected (${variation}).`,
    climateCoupling: 'Temperature and humidity are rising together unusually.',
    tiltMagnitude: `Chassis tilt is ${variation}.`,
    waterRaw: `Water-contact raw signal is ${variation}; contact must be confirmed against calibration.`,
    waterRate: `Water-contact raw signal changed abruptly (${variation}).`,
    frontClearanceCm: `Front clearance is unusually narrow (${variation}).`,
    encoderMismatch: `Left/right encoder movement disagrees unusually (${variation}); possible slip or drivetrain asymmetry.`,
    tiltEncoderCoupling: 'Tilt and encoder disagreement occurred together, suggesting instability or wheel slip.',
    packetGapRate: `Packet-gap count is increasing unusually (${variation}).`,
    heartbeatAgeMs: `Telemetry delivery is delayed (${variation}).`,
    rssiWeakness: `Radio signal is weaker than the learned deployment baseline (${variation}).`
  };
  return explanations[feature] || `${feature} is ${variation}.`;
}

/** @param {Record<string, any>} [model] */
function unknownResult(model = {}) {
  return {
    score: 0,
    state: 'NORMAL',
    confidence: 0,
    trend: 'STABLE',
    reasons: ['Waiting for enough live sensor inputs.'],
    suggestedAction: ACTIONS.NORMAL,
    availableFeatures: 0,
    totalFeatures: Object.keys(model.features || {}).length,
    baselineProgress: 0,
    baselineReady: false,
    modelVersion: model.version || 'not loaded',
    trainingDate: model.trainingDate || null,
    modelScope: model.trainingScope || 'Offline advisory model',
    isolationScore: 0,
    featureContributions: [],
    scoreHistory: [],
    advisory: true,
    timestampMs: 0
  };
}

export function createUnavailableHazardResult() {
  return unknownResult();
}

/**
 * Offline explainable anomaly inference. The returned object has no command or
 * motor field by design: Sentinel is an advisory consumer of telemetry only.
 * @param {Record<string, any>} model
 * @param {{baselineSeconds?: number}} [options]
 */
export function createHazardEngine(model, options = {}) {
  if (!model || !model.features || !model.isolationForest)
    throw new Error('Invalid Sentinel hazard model.');

  const baselineTarget = Math.max(10,
    Number(options.baselineSeconds ?? model.baselineSeconds) || 60);
  /** @type {Record<string, {count:number, mean:number, m2:number}>} */
  const learned = {};
  /** @type {Record<string, any> | null} */
  let previousTelemetry = null;
  let previousTimestamp = 0;
  let baselineSamples = 0;
  /** @type {Array<{timestampMs:number, score:number}>} */
  let history = [];
  /** @type {any} */
  let latest = unknownResult(model);

  /** @param {string} name @param {Record<string, any>} spec */
  const baselineFor = (name, spec) => {
    const stats = learned[name];
    if (!stats || stats.count < 10)
      return { center: Number(spec.center) || 0,
        scale: Math.max(1e-6, Number(spec.scale) || 1) };
    const deviation = stats.count > 1
      ? Math.sqrt(stats.m2 / (stats.count - 1)) : Number(spec.scale) || 1;
    return {
      center: stats.mean,
      scale: Math.max(Number(spec.scale) * 0.2, deviation, 1e-6)
    };
  };

  /** @param {Record<string, number | null>} features */
  const updateBaseline = (features) => {
    for (const [name, value] of Object.entries(features)) {
      if (!Number.isFinite(value) || !model.features[name]) continue;
      const stats = learned[name] || { count: 0, mean: 0, m2: 0 };
      stats.count += 1;
      const delta = Number(value) - stats.mean;
      stats.mean += delta / stats.count;
      stats.m2 += delta * (Number(value) - stats.mean);
      learned[name] = stats;
    }
    baselineSamples += 1;
  };

  return {
    /**
     * @param {Record<string, any>} telemetry
     * @param {number} [timestampMs]
     * @param {{heartbeatAgeMs?: number | null}} [context]
     */
    observe(telemetry, timestampMs = Date.now(), context = {}) {
      const dtSeconds = previousTimestamp
        ? clamp((timestampMs - previousTimestamp) / 1000, 0.25, 5) : 1;
      const features = createHazardFeatures(
        telemetry, previousTelemetry, dtSeconds, context);
      previousTelemetry = { ...telemetry };
      previousTimestamp = timestampMs;

      /** @type {FeatureContribution[]} */
      const featureContributions = [];
      for (const [name, rawSpec] of Object.entries(model.features)) {
        const spec = /** @type {Record<string, any>} */ (rawSpec);
        const value = finite(
          /** @type {Record<string, number | null>} */ (features)[name]);
        if (value === null) continue;
        const baseline = baselineFor(name, spec);
        const raw = (value - baseline.center) / baseline.scale;
        const z = spec.direction === 'low' ? Math.max(0, -raw)
          : spec.direction === 'high' ? Math.max(0, raw)
            : Math.abs(raw);
        const contribution = clamp((z / 4) * 100, 0, 100) *
          clamp(Number(spec.weight) || 1, 0.1, 2);
        featureContributions.push({
          feature: name,
          category: spec.category || name,
          label: spec.label || name,
          value,
          z,
          contribution: clamp(contribution, 0, 100)
        });
      }
      featureContributions.sort((a, b) => b.contribution - a.contribution);

      const top = featureContributions.slice(0, 3);
      const robustScore = top.length
        ? top[0].contribution * 0.6 +
          top.slice(1).reduce((sum, item) => sum + item.contribution, 0) /
            Math.max(1, top.length - 1) * 0.4
        : 0;
      const rawIsolation = isolationForestScore(model, features);
      const isolationScore = clamp(
        (rawIsolation - Number(model.scoreCalibration.offset || 0.52)) /
        Number(model.scoreCalibration.range || 0.3) * 100, 0, 100);

      /** @type {Map<string, FeatureContribution>} */
      const distinct = new Map();
      for (const item of featureContributions) {
        if (!distinct.has(item.category)) distinct.set(item.category, item);
      }
      const ranked = [...distinct.values()];
      let combinationBonus = 0;
      /** @param {string} name */
      const contributionOf = (name) => featureContributions.find(
        (item) => item.feature === name)?.z || 0;
      if (contributionOf('gasRaw') > 2 &&
          (contributionOf('temperatureRate') > 1 ||
           contributionOf('climateCoupling') > 1)) combinationBonus += 8;
      if (contributionOf('tiltMagnitude') > 2 &&
          contributionOf('encoderMismatch') > 2) combinationBonus += 10;
      if (contributionOf('waterRaw') > 2 &&
          contributionOf('frontClearanceCm') > 2) combinationBonus += 8;

      const score = Math.round(clamp(
        robustScore * 0.58 + isolationScore * 0.42 + combinationBonus,
        0, 100));
      const state = hazardState(score);
      const availableFeatures = featureContributions.length;
      const totalFeatures = Object.keys(model.features).length;

      if (state === 'NORMAL' && availableFeatures >= 5 &&
          baselineSamples < baselineTarget) updateBaseline(features);

      history = [...history.filter(
        (item) => timestampMs - item.timestampMs <= 30000),
        { timestampMs, score }];
      const split = Math.max(1, Math.floor(history.length / 3));
      const early = median(history.slice(0, split).map((item) => item.score));
      const recent = median(history.slice(-split).map((item) => item.score));
      const change = recent - early;
      const trend = history.length < 4 || Math.abs(change) < 6
        ? 'STABLE' : change > 0 ? 'RISING' : 'FALLING';

      const reasons = ranked.filter((item) => item.z >= 1.4).slice(0, 3)
        .map((item) => explainFeature(item.feature, item.z, features));
      if (!reasons.length) reasons.push(baselineSamples < baselineTarget
        ? `Learning deployment baseline: ${baselineSamples}/${baselineTarget} one-second windows.`
        : 'Available sensor combination matches the learned deployment baseline.');

      const confidence = Math.round(clamp(
        (availableFeatures / Math.max(1, totalFeatures)) *
        (0.7 + 0.3 * Math.min(1, baselineSamples / baselineTarget)) * 100,
        0, 99));
      latest = {
        score, state, confidence, trend, reasons,
        suggestedAction: /** @type {Record<string, string>} */ (ACTIONS)[state],
        availableFeatures, totalFeatures,
        baselineProgress: Math.min(1, baselineSamples / baselineTarget),
        baselineReady: baselineSamples >= baselineTarget,
        modelVersion: model.version,
        trainingDate: model.trainingDate,
        modelScope: model.trainingScope,
        isolationScore: Math.round(isolationScore),
        featureContributions: ranked.slice(0, 6),
        scoreHistory: history.map((item) => item.score),
        advisory: true,
        timestampMs
      };
      return latest;
    },

    getResult() { return latest; },

    reset() {
      previousTelemetry = null;
      previousTimestamp = 0;
      baselineSamples = 0;
      history = [];
      for (const name of Object.keys(learned)) delete learned[name];
      latest = unknownResult(model);
      return latest;
    }
  };
}
