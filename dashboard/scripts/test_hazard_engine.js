import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import test from 'node:test';
import {
  createHazardEngine, createHazardFeatures, expectedPathLength,
  hazardState
} from '../src/lib/ai/hazardEngine.js';

const model = JSON.parse(readFileSync(
  new URL('../static/models/hazard-model.json', import.meta.url), 'utf8'));

function normalTelemetry(index = 0) {
  return {
    source: 'LIVE',
    gasRaw: 850 + Math.sin(index / 4) * 12,
    temperature: 29 + Math.sin(index / 10) * 0.2,
    humidity: 69 + Math.cos(index / 9) * 0.4,
    pitchDeg: 2,
    rollDeg: -1,
    waterRaw: 350 + Math.cos(index / 6) * 5,
    frontDistanceCm: 120 + Math.sin(index / 5) * 3,
    encoderL: index * 10,
    encoderR: index * 10,
    packetGaps: 0,
    rssi: -55
  };
}

test('Isolation Forest path normalization is finite and monotonic', () => {
  assert.equal(expectedPathLength(1), 0);
  assert.equal(expectedPathLength(2), 1);
  assert.ok(expectedPathLength(64) > expectedPathLength(16));
});

test('feature extraction preserves unknowns and computes encoder disagreement', () => {
  const previous = normalTelemetry(1);
  const current = { ...normalTelemetry(2), encoderL: 30, encoderR: 11 };
  const features = createHazardFeatures(current, previous, 1,
    { heartbeatAgeMs: 80 });
  assert.ok(features.encoderMismatch > 0.7);
  assert.equal(features.heartbeatAgeMs, 80);
  assert.equal(createHazardFeatures({}, null, 1).gasRaw, null);
});

test('stable normal telemetry remains normal and learns a baseline', () => {
  const engine = createHazardEngine(model);
  let result;
  for (let index = 0; index < 65; index += 1)
    result = engine.observe(normalTelemetry(index), index * 1000,
      { heartbeatAgeMs: 55 });
  assert.equal(result.state, 'NORMAL');
  assert.ok(result.score < 30);
  assert.equal(result.baselineReady, true);
  assert.ok(result.confidence > 60);
});

test('gradual demo anomaly becomes explainable HIGH or CRITICAL advice', () => {
  const engine = createHazardEngine(model);
  let result;
  for (let index = 0; index < 20; index += 1)
    engine.observe(normalTelemetry(index), index * 1000,
      { heartbeatAgeMs: 55 });
  for (let step = 1; step <= 18; step += 1) {
    const telemetry = normalTelemetry(20 + step);
    telemetry.gasRaw += step * 60;
    telemetry.temperature += step * 0.28;
    telemetry.humidity += step * 0.45;
    telemetry.waterRaw += step * 26;
    telemetry.frontDistanceCm -= step * 4;
    result = engine.observe(telemetry, (20 + step) * 1000,
      { heartbeatAgeMs: 55 });
  }
  assert.ok(['HIGH', 'CRITICAL'].includes(result.state));
  assert.ok(result.score >= 55);
  assert.equal(result.trend, 'RISING');
  assert.ok(result.reasons.some((reason) => reason.includes('MQ-4')));
  assert.ok(!result.reasons.join(' ').toLowerCase().includes('methane'));
  assert.equal(result.advisory, true);
  assert.equal('motorCommand' in result, false);
});

test('missing sensors reduce confidence without fabricating evidence', () => {
  const engine = createHazardEngine(model);
  const result = engine.observe({ source: 'LIVE', gasRaw: 850 }, 1000,
    { heartbeatAgeMs: null });
  assert.ok(result.confidence < 20);
  assert.ok(result.availableFeatures < result.totalFeatures);
});

test('hazard state thresholds are stable', () => {
  assert.equal(hazardState(0), 'NORMAL');
  assert.equal(hazardState(30), 'WATCH');
  assert.equal(hazardState(55), 'HIGH');
  assert.equal(hazardState(75), 'CRITICAL');
});
