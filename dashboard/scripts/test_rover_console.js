import assert from 'node:assert/strict';
import { test, describe } from 'node:test';
import { createUnknownTelemetry } from '../src/lib/state/telemetry.js';

describe('Rover Console Operational Logic & Test Suite', () => {

  describe('1. Connection Button State Logic', () => {
    function getConnectionButtonState(missionMode, transportMode, isConnected, isConnecting) {
      if (missionMode !== 'hardware') {
        return { visible: false, label: null, disabled: true, action: 'none' };
      }
      if (isConnecting) {
        return { visible: true, label: 'Connecting...', disabled: true, action: 'none' };
      }
      if (transportMode === 'SERIAL' && isConnected) {
        return { visible: true, label: 'Disconnect', disabled: false, action: 'disconnect' };
      }
      return { visible: true, label: 'Connect USB', disabled: false, action: 'connect' };
    }

    test('does not expose a connection control in demo mode', () => {
      const state = getConnectionButtonState('demo', 'SIMULATION', true, false);
      assert.equal(state.visible, false);
      assert.equal(state.label, null);
      assert.equal(state.action, 'none');
    });

    test('shows "Connecting..." only for a real hardware connection in progress', () => {
      const state = getConnectionButtonState('hardware', 'DISCONNECTED', false, true);
      assert.equal(state.visible, true);
      assert.equal(state.label, 'Connecting...');
      assert.equal(state.disabled, true);
      assert.equal(state.action, 'none');
    });

    test('shows "Disconnect" when live Web Serial is active', () => {
      const state = getConnectionButtonState('hardware', 'SERIAL', true, false);
      assert.equal(state.visible, true);
      assert.equal(state.label, 'Disconnect');
      assert.equal(state.disabled, false);
      assert.equal(state.action, 'disconnect');
    });

    test('shows "Connect USB" when real hardware mode is disconnected', () => {
      const stateDisc = getConnectionButtonState('hardware', 'DISCONNECTED', false, false);
      assert.equal(stateDisc.visible, true);
      assert.equal(stateDisc.label, 'Connect USB');
      assert.equal(stateDisc.action, 'connect');
    });
  });

  describe('2. Manual-Mode Control Gating & Safety Halt', () => {
    function calculateMotorOutput(leftReq, rightReq, { isEstop, isConnected, controlMode, isWindowBlurred }) {
      if (isEstop || !isConnected || controlMode !== 'MANUAL' || isWindowBlurred) {
        return { left: 0, right: 0, gated: true };
      }
      return { left: leftReq, right: rightReq, gated: false };
    }

    test('allows movement when in Manual mode, connected, and not remotely stopped', () => {
      const out = calculateMotorOutput(200, 200, {
        isEstop: false,
        isConnected: true,
        controlMode: 'MANUAL',
        isWindowBlurred: false
      });
      assert.equal(out.left, 200);
      assert.equal(out.right, 200);
      assert.equal(out.gated, false);
    });

    test('gates motor output to 0 when disconnected', () => {
      const out = calculateMotorOutput(200, 200, {
        isEstop: false,
        isConnected: false,
        controlMode: 'MANUAL',
        isWindowBlurred: false
      });
      assert.equal(out.left, 0);
      assert.equal(out.right, 0);
      assert.equal(out.gated, true);
    });

    test('gates motor output to 0 when in Autonomous mode', () => {
      const out = calculateMotorOutput(200, 200, {
        isEstop: false,
        isConnected: true,
        controlMode: 'AUTO_EXPLORE',
        isWindowBlurred: false
      });
      assert.equal(out.left, 0);
      assert.equal(out.right, 0);
      assert.equal(out.gated, true);
    });

    test('gates motor output to 0 immediately on browser window blur', () => {
      const out = calculateMotorOutput(200, 200, {
        isEstop: false,
        isConnected: true,
        controlMode: 'MANUAL',
        isWindowBlurred: true
      });
      assert.equal(out.left, 0);
      assert.equal(out.right, 0);
      assert.equal(out.gated, true);
    });
  });

  describe('3. Remote Software Stop Behavior & States', () => {
    function getEstopState(isEstop, isEstopPending, isConnected) {
      if (!isConnected) {
        return { label: 'Remote stop unavailable', disabled: true, status: 'unavailable' };
      }
      if (isEstopPending) {
        return { label: 'Stopping...', disabled: false, status: 'pending' };
      }
      if (isEstop) {
        return { label: 'Clear stop latch', disabled: false, status: 'active' };
      }
      return { label: 'Remote stop', disabled: false, status: 'nominal' };
    }

    test('disables the remote stop when disconnected', () => {
      const res = getEstopState(false, false, false);
      assert.equal(res.label, 'Remote stop unavailable');
      assert.equal(res.disabled, true);
    });

    test('shows active reset wording when the remote stop is engaged', () => {
      const res = getEstopState(true, false, true);
      assert.equal(res.label, 'Clear stop latch');
      assert.equal(res.status, 'active');
    });

    test('does not represent the remote command as a physical emergency brake', () => {
      const res = getEstopState(false, false, true);
      assert.equal(res.label, 'Remote stop');
      assert.equal(res.status, 'nominal');
    });
  });

  describe('4. Stale Orientation Behavior', () => {
    function evaluateOrientationVisuals(pitchDeg, rollDeg, { isStale, isCalibrating }) {
      if (isCalibrating) {
        return { status: 'calibrating', overlayText: 'Calibrating IMU...', isGrayscale: true };
      }
      if (isStale) {
        // Must freeze the last known pitch/roll and display stale overlay without zeroing
        return { status: 'stale', overlayText: 'Orientation stale / Frozen', isGrayscale: true, frozenPitch: pitchDeg, frozenRoll: rollDeg };
      }
      return { status: 'live', overlayText: null, isGrayscale: false, pitch: pitchDeg, roll: rollDeg };
    }

    test('freezes orientation and displays stale indicator on disconnect without zeroing angles', () => {
      const res = evaluateOrientationVisuals(-3.5, 2.6, { isStale: true, isCalibrating: false });
      assert.equal(res.status, 'stale');
      assert.equal(res.isGrayscale, true);
      assert.equal(res.frozenPitch, -3.5);
      assert.equal(res.frozenRoll, 2.6);
      assert.notEqual(res.frozenPitch, 0); // Must never falsely reset to level!
    });
  });

  describe('5. Artificial Horizon Pitch & Roll Transforms', () => {
    function computeHorizonTransforms(pitchDeg, rollDeg, radius) {
      const rollRad = (rollDeg * Math.PI) / 180;
      const pitchOffset = Math.max(-radius * 0.9, Math.min(radius * 0.9, pitchDeg * 2.4));
      return { rollRad, pitchOffset };
    }

    test('calculates correct rotational and translational offsets', () => {
      const radius = 100;
      const { rollRad, pitchOffset } = computeHorizonTransforms(10, 45, radius);
      assert.ok(Math.abs(rollRad - (45 * Math.PI / 180)) < 0.0001);
      assert.equal(pitchOffset, 24); // 10 * 2.4
    });

    test('clamps extreme pitch angles within visible reticle boundary', () => {
      const radius = 100;
      const { pitchOffset } = computeHorizonTransforms(80, 0, radius);
      assert.equal(pitchOffset, 90); // Clamped to radius * 0.9
    });
  });

  describe('6. Evidence-Bounded State Changes', () => {
    function evaluateStability(maxAbsAngle, cautionThreshold = 12.0, maxTiltThreshold = 25.0) {
      if (maxAbsAngle >= maxTiltThreshold) return 'critical';
      if (maxAbsAngle >= cautionThreshold) return 'caution';
      return 'level';
    }

    test('evaluates level, caution, and critical rollover stability thresholds', () => {
      assert.equal(evaluateStability(3.5), 'level');
      assert.equal(evaluateStability(14.0), 'caution');
      assert.equal(evaluateStability(25.5), 'critical');
    });

    function evaluateGasState(gasState) {
      if (gasState === 'SENSOR_FAULT') return 'fault';
      if (gasState === 'QUALITATIVE_ADVISORY') return 'advisory';
      if (gasState === 'RAW_ONLY' || gasState === 'WARMING') return 'uninterpreted';
      return 'unknown';
    }

    test('does not infer gas safety from an uncalibrated raw ADC value', () => {
      assert.equal(evaluateGasState('RAW_ONLY'), 'uninterpreted');
      assert.equal(evaluateGasState('WARMING'), 'uninterpreted');
      assert.equal(evaluateGasState('QUALITATIVE_ADVISORY'), 'advisory');
      assert.equal(evaluateGasState('UNKNOWN'), 'unknown');
    });
  });

  describe('7. Non-Wrapping Footer Layout Formatting', () => {
    function formatMapFooter(pointCount, zoomLevel, poseX, poseY, distanceTraversed) {
      return {
        points: `Points: ${pointCount}`,
        zoom: `Zoom: ${(zoomLevel * 100).toFixed(0)}%`,
        pose: `Pose: x: ${poseX.toFixed(1)} m · y: ${poseY.toFixed(1)} m`,
        traversed: `Traversed: ${distanceTraversed.toFixed(2)} m`
      };
    }

    test('formats coordinate string on a single consistent line', () => {
      const footer = formatMapFooter(30, 1.0, 33.2, -13.6, 4.52);
      assert.equal(footer.pose, 'Pose: x: 33.2 m · y: -13.6 m');
      assert.equal(footer.traversed, 'Traversed: 4.52 m');
    });
  });

  describe('8. Reduced-Motion Accessibility Behavior', () => {
    function updateDampedValue(target, current, prefersReducedMotion) {
      if (prefersReducedMotion) {
        return target; // Immediate update, bypass interpolation
      }
      return current + (target - current) * 0.18;
    }

    test('bypasses lerp damping when prefersReducedMotion is enabled', () => {
      const target = 15.0;
      const current = 0.0;
      const immediate = updateDampedValue(target, current, true);
      assert.equal(immediate, 15.0);

      const lerped = updateDampedValue(target, current, false);
      assert.ok(lerped > 0 && lerped < 15.0);
    });
  });

  describe('9. Real-Mode Unknown-State Boundary', () => {
    test('starts every observation as unknown and contains no fabricated engineering units', () => {
      const telemetry = createUnknownTelemetry();
      const observationKeys = [
        'temperature',
        'humidity',
        'gasRaw',
        'waterRaw',
        'pitchDeg',
        'rollDeg',
        'estimatedDistanceMeters',
        'frontDistanceCm',
        'rssi'
      ];

      assert.equal(telemetry.source, 'UNKNOWN');
      for (const key of observationKeys) assert.equal(telemetry[key], null);
      assert.equal(telemetry.gasState, 'UNKNOWN');
      assert.equal(telemetry.waterState, 'UNKNOWN');
      assert.equal(telemetry.alertState, 'UNKNOWN');
      assert.equal('gasPpm' in telemetry, false);
      assert.equal('lelPercent' in telemetry, false);
      assert.equal('waterDepth' in telemetry, false);
      assert.equal('batteryPercent' in telemetry, false);
    });
  });

  describe('10. Calibrated Mission Feature Gate', () => {
    function exploreControlReady(missionMode, hardwareControlReady,
                                 calibrationReady, dualFrontReady) {
      return missionMode === 'demo'
        ? hardwareControlReady
        : hardwareControlReady && calibrationReady && dualFrontReady;
    }

    test('keeps Explore unavailable on real hardware while odometry calibration is pending', () => {
      assert.equal(exploreControlReady('hardware', true, false, true), false);
      assert.equal(exploreControlReady('hardware', true, true, false), false);
      assert.equal(exploreControlReady('hardware', true, true, true), true);
      assert.equal(exploreControlReady('hardware', false, true, true), false);
    });

    test('preserves explicitly simulated exploration in demo mode', () => {
      assert.equal(exploreControlReady('demo', true, false, false), true);
      assert.equal(exploreControlReady('demo', false, true, true), false);
    });
  });
});
