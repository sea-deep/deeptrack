<script>
  import NavRail from '$lib/components/NavRail.svelte';
  import MetricTile from '$lib/components/MetricTile.svelte';
  import EstimatedRouteCanvas from '$lib/components/EstimatedRouteCanvas.svelte';
  import TiltHorizon from '$lib/components/TiltHorizon.svelte';
  import SensorDetailModal from '$lib/components/SensorDetailModal.svelte';
  import MapsView from '$lib/components/dashboard/MapsView.svelte';
  import LogsView from '$lib/components/dashboard/LogsView.svelte';
  import HardwareView from '$lib/components/dashboard/HardwareView.svelte';
  import Navbar from '$lib/components/Navbar.svelte';
  import Footer from '$lib/components/Footer.svelte';
  import { onMount } from 'svelte';
  import { goto } from '$app/navigation';
  import { auth } from '$lib/stores/auth.svelte.js';
  import { initialTelemetry as demoTelemetry, initialScanPoints as demoScanPoints } from '$lib/mocks/telemetryMock.js';
  import { createUnknownTelemetry, createEmptyHistory, createDemoHistory } from '$lib/state/telemetry.js';
  import {
    applyGatewayTelemetry, COMMAND_TTL_MS, DIAGNOSTIC_ACTION, DRIVE_REFRESH_MS,
    formatDiagnosticResult,
    gatewayEventToLog, HEARTBEAT_INTERVAL_MS, makeGatewayCommand,
    parseGatewayLine, PROTOCOL_VERSION, shouldAcceptGatewayRecord,
    TELEMETRY_STALE_MS, tofDisplayState,
    upsertScanPoint
  } from '$lib/state/gatewayProtocol.js';
  import {
    addScanEvidence, createEstimatedMapState, renderableEvidence,
    updateEstimatedPose
  } from '$lib/state/estimatedMap.js';
  import { sendShared, receiveShared, m3TopLevelFadeThrough } from '$lib/utils/motion.js';

  /** @type {{initialMode?: 'demo' | 'hardware' | null}} */
  let { initialMode = null } = $props();
  let initialModeHandled = false;

  /** @type {'hardware' | 'demo' | null} */
  let missionMode = $state(null);
  let activeNavView = $state('console'); // 'console', 'maps', 'logs', 'hardware'
  let transportMode = $state('DISCONNECTED'); // 'DISCONNECTED', 'SIMULATION', 'SERIAL'
  let isConnected = $state(false);
  let isEstop = $state(false);
  let isEstopPending = $state(false);
  let heartbeatAgeMs = $state(/** @type {number | null} */ (null));

  /** @type {any} */
  let serialPort = $state(null);
  /** @type {WritableStreamDefaultWriter<string> | null} */
  let serialWriter = $state(null);
  /** @type {ReadableStreamDefaultReader<string> | null} */
  let serialReader = $state(null);
  let isConnectingSerial = $state(false);
  let serialConnectionError = $state('');
  let gatewaySession = $state(/** @type {number | null} */ (null));
  let gatewayRadioReady = $state(false);
  let gatewayArmed = $state(false);
  let commandSequence = 0;
  let lastTelemetryReceivedAt = 0;
  /** @type {ReturnType<typeof setInterval> | null} */
  let driveRefreshTimer = null;
  let eventCounter = 0;
  let realLogs = $state(/** @type {Array<any>} */ ([]));
  let diagnosticResults = $state(/** @type {Array<any>} */ ([]));
  let orientationCalibrating = $state(false);
  let estimatedMapState = $state(/** @type {any} */ (createEstimatedMapState()));
  let realMapEvidence = $state(renderableEvidence(createEstimatedMapState()));

  // --- Console Specific State ---
  /** @type {'MANUAL' | 'AUTO_EXPLORE'} */
  let controlMode = $state('MANUAL');
  let isRecordingMap = $state(false);
  let isAutoPaused = $state(false);
  let roverPose = $state({ x: 0, y: 0, headingDeg: 90 });
  let speedPercent = $state(55);

  // Real mode starts with no fixtures. Demo fixtures enter only through launchDemo().
  let telemetry = $state(/** @type {import('$lib/state/telemetry.js').TelemetryState} */ (createUnknownTelemetry()));
  let scanPoints = $state(/** @type {Array<{angle_deg: number, distance_mm: number | null, valid: boolean, seq?: number, scan_id?: number, range_status?: number, confidence_pct?: number, timestamp_ms?: number}>} */ ([]));
  let historyBuffers = $state(/** @type {import('$lib/state/telemetry.js').TelemetryHistory} */ (createEmptyHistory()));
  let roverLinkFresh = $derived(
    telemetry.source === 'LIVE' && heartbeatAgeMs !== null &&
    heartbeatAgeMs <= TELEMETRY_STALE_MS
  );
  let hardwareControlReady = $derived(
    missionMode === 'demo'
      ? isConnected && !isEstop
      : isConnected && gatewayRadioReady && gatewayArmed && roverLinkFresh && !isEstop
  );
  let tofStatus = $derived(tofDisplayState(telemetry, scanPoints));

  // Modal State
  let selectedSensorKey = $state(/** @type {string | null} */ (null));
  let isModalOpen = $state(false);

  // Drive Controls
  let keyState = $state({ w: false, a: false, s: false, d: false, space: false });
  let currentMotorL = $state(0);
  let currentMotorR = $state(0);

  function resetInteractionState() {
    activeNavView = 'console';
    controlMode = 'MANUAL';
    isRecordingMap = false;
    isAutoPaused = false;
    isEstop = false;
    isEstopPending = false;
    currentMotorL = 0;
    currentMotorR = 0;
    roverPose = { x: 0, y: 0, headingDeg: 90 };
    heartbeatAgeMs = null;
    gatewaySession = null;
    gatewayRadioReady = false;
    gatewayArmed = false;
    commandSequence = 0;
    lastTelemetryReceivedAt = 0;
    realLogs = [];
    diagnosticResults = [];
    estimatedMapState = createEstimatedMapState();
    realMapEvidence = renderableEvidence(estimatedMapState);
    stopDriveRefresh();
  }

  function launchDemo() {
    resetInteractionState();
    missionMode = 'demo';
    transportMode = 'SIMULATION';
    isConnected = true;
    telemetry = { ...demoTelemetry, timestamp: new Date().toISOString() };
    scanPoints = demoScanPoints.map((point) => ({ ...point }));
    historyBuffers = createDemoHistory();
  }

  function launchHardware() {
    resetInteractionState();
    missionMode = 'hardware';
    transportMode = 'DISCONNECTED';
    isConnected = false;
    telemetry = createUnknownTelemetry();
    scanPoints = [];
    historyBuffers = createEmptyHistory();
    realLogs = [];
  }

  $effect(() => {
    if (initialModeHandled || !initialMode) return;
    if (initialMode === 'demo') {
      initialModeHandled = true;
      launchDemo();
    } else if (!auth.isLoading) {
      initialModeHandled = true;
      if (auth.user) launchHardware();
      else goto('/auth');
    }
  });

  // --- Web Serial API ---
  /** @param {Record<string, any>} record */
  async function writeGatewayRecord(record) {
    if (!serialWriter || transportMode !== 'SERIAL') return false;
    try {
      await serialWriter.write(JSON.stringify(record) + '\n');
      return true;
    } catch {
      markSerialDisconnected();
      return false;
    }
  }

  /** @param {string} type @param {Record<string, unknown>} payload */
  async function sendSessionCommand(type, payload = {}) {
    if (!gatewaySession) return false;
    const record = makeGatewayCommand(
      type, gatewaySession, ++commandSequence, payload
    );
    return writeGatewayRecord(record);
  }

  /** @param {Record<string, any>} packet */
  function appendRealLog(packet) {
    const log = gatewayEventToLog(packet, ++eventCounter);
    realLogs = [log, ...realLogs.filter((item) => item.id !== log.id)].slice(0, 250);
  }

  /** @param {Record<string, any>} packet */
  function handleGatewayRecord(packet) {
    if (packet.type === 'hello') {
      if (packet.protocol !== PROTOCOL_VERSION || packet.board !== 'gateway' ||
          !Number.isInteger(packet.session) || packet.session <= 0) {
        appendRealLog({
          source: 'gateway', severity: 'error', code: 'PROTOCOL_MISMATCH',
          message: 'Gateway hello rejected: protocol or identity mismatch.'
        });
        gatewaySession = null;
        gatewayRadioReady = false;
        gatewayArmed = false;
        return;
      }
      if (gatewaySession !== packet.session) {
        gatewaySession = packet.session;
        commandSequence = 0;
        scanPoints = [];
        estimatedMapState = createEstimatedMapState();
        realMapEvidence = renderableEvidence(estimatedMapState);
      }
      gatewayRadioReady = packet.radio_ready === true;
      gatewayArmed = packet.armed === true;
      serialConnectionError = '';
      if (!gatewayArmed) {
        currentMotorL = 0;
        currentMotorR = 0;
        stopDriveRefresh();
      }
      void sendSessionCommand('heartbeat');
      return;
    }
    if (!shouldAcceptGatewayRecord(packet, gatewaySession)) return;
    if (packet.type === 'telemetry') {
      telemetry = applyGatewayTelemetry(telemetry, packet);
      estimatedMapState = updateEstimatedPose(estimatedMapState, packet);
      realMapEvidence = renderableEvidence(estimatedMapState);
      if (estimatedMapState.pose.known) {
        roverPose = {
          x: estimatedMapState.pose.x_m * 40,
          y: estimatedMapState.pose.y_m * 40,
          headingDeg: estimatedMapState.pose.heading_rad * 180 / Math.PI + 90
        };
      }
      lastTelemetryReceivedAt = performance.now();
      heartbeatAgeMs = 0;
      historyBuffers.gas = [...historyBuffers.gas.slice(-29), ...(telemetry.gasRaw === null ? [] : [telemetry.gasRaw])];
      historyBuffers.temperature = [...historyBuffers.temperature.slice(-29), ...(telemetry.temperature === null ? [] : [telemetry.temperature])];
      historyBuffers.water = [...historyBuffers.water.slice(-29), ...(telemetry.waterRaw === null ? [] : [telemetry.waterRaw])];
      historyBuffers.ultrasonic = [...historyBuffers.ultrasonic.slice(-29), ...(telemetry.frontDistanceCm === null ? [] : [telemetry.frontDistanceCm])];
      historyBuffers.pitch = [...historyBuffers.pitch.slice(-29), ...(telemetry.pitchDeg === null ? [] : [telemetry.pitchDeg])];
      historyBuffers.roll = [...historyBuffers.roll.slice(-29), ...(telemetry.rollDeg === null ? [] : [telemetry.rollDeg])];
    } else if (packet.type === 'scan') {
      scanPoints = upsertScanPoint(scanPoints, packet);
      estimatedMapState = addScanEvidence(
        estimatedMapState, [packet], performance.now()
      );
      realMapEvidence = renderableEvidence(estimatedMapState);
    } else if (packet.type === 'event') {
      appendRealLog(packet);
      if (packet.code === 'GATEWAY_STOP') gatewayArmed = false;
    } else if (packet.type === 'diagnostic') {
      try {
        const result = formatDiagnosticResult(packet);
        diagnosticResults = [{
          ...result,
          id: `${packet.session || 0}-${packet.seq || performance.now()}`
        }, ...diagnosticResults].slice(0, 120);
      } catch {
        appendRealLog({
          source: 'gateway', severity: 'warning',
          code: 'BAD_DIAGNOSTIC_RESULT',
          message: 'An invalid diagnostic result was ignored.'
        });
      }
    } else if (packet.type === 'ack' && packet.status === 1) {
      appendRealLog({
        source: 'rover', session: packet.session, seq: packet.seq,
        severity: 1, code: 'COMMAND_REJECTED',
        value: packet.reason, timestamp_ms: null
      });
    }
  }

  function markSerialDisconnected() {
    stopDriveRefresh();
    isConnected = false;
    transportMode = 'DISCONNECTED';
    gatewaySession = null;
    gatewayRadioReady = false;
    gatewayArmed = false;
    heartbeatAgeMs = null;
    currentMotorL = 0;
    currentMotorR = 0;
    if (telemetry.source === 'LIVE') telemetry.source = 'STALE';
  }

  async function connectWebSerial() {
    if (missionMode !== 'hardware') return;
    if (!('serial' in navigator)) {
      alert('Web Serial API is only supported in Chromium-based browsers (Chrome, Edge).');
      return;
    }
    try {
      isConnectingSerial = true;
      serialConnectionError = '';
      // @ts-ignore
      const webSerial = /** @type {any} */ (navigator.serial);
      const approvedPorts = await webSerial.getPorts();
      // Reuse the one previously approved gateway. If none or more than one
      // are approved, require an explicit chooser so the rover is never opened
      // accidentally when both ESP32 boards share the same USB VID/PID.
      serialPort = approvedPorts.length === 1
        ? approvedPorts[0]
        : await webSerial.requestPort();
      await serialPort.open({ baudRate: 115200 });

      // Match the reset sequence used by the verified gateway bench reader.
      // Leaving RTS asserted can keep an ESP32 behind a CH34x bridge in reset.
      if (typeof serialPort.setSignals === 'function') {
        await serialPort.setSignals({
          dataTerminalReady: false,
          requestToSend: true
        });
        await new Promise((resolve) => setTimeout(resolve, 100));
        await serialPort.setSignals({
          dataTerminalReady: false,
          requestToSend: false
        });
        await new Promise((resolve) => setTimeout(resolve, 800));
      }

      const textDecoder = new TextDecoderStream();
      // @ts-ignore
      serialPort.readable.pipeTo(textDecoder.writable);
      serialReader = textDecoder.readable.getReader();

      const textEncoder = new TextEncoderStream();
      // @ts-ignore
      textEncoder.readable.pipeTo(serialPort.writable);
      serialWriter = textEncoder.writable.getWriter();

      transportMode = 'SERIAL';
      isConnected = true;

      setTimeout(() => {
        if (transportMode === 'SERIAL' && gatewaySession === null) {
          serialConnectionError =
            'USB opened, but the gateway sent no data. Disconnect and reconnect the gateway.';
        }
      }, 5000);

      (async () => {
        let buffer = '';
        try {
          while (serialReader) {
            const { value, done } = await serialReader.read();
            if (done) break;
            if (value) {
              buffer += value;
              const lines = buffer.split('\n');
              buffer = lines.pop() || '';
              for (const line of lines) {
                const trimmed = line.trim();
                if (!trimmed) continue;
                try {
                  handleGatewayRecord(parseGatewayLine(trimmed));
                } catch {
                  appendRealLog({
                    source: 'gateway', severity: 'warning',
                    code: 'UNPARSEABLE_LINE',
                    message: 'A non-NDJSON serial line was ignored.'
                  });
                }
              }
            }
          }
        } finally {
          try { serialReader?.releaseLock(); } catch {}
          serialReader = null;
          if (transportMode === 'SERIAL') markSerialDisconnected();
        }
      })();
    } catch (err) {
      console.error(err);
      isConnected = false;
      transportMode = 'DISCONNECTED';
      const errorName = err instanceof DOMException ? err.name : '';
      serialConnectionError = errorName === 'NotFoundError'
        ? 'No USB device was selected.'
        : errorName === 'NetworkError'
          ? 'The gateway is already open in another app or could not be opened.'
          : 'Could not connect to the gateway. Check the USB cable and try again.';
      serialPort = null;
    } finally {
      isConnectingSerial = false;
    }
  }

  async function safetyDisarm() {
    stopDriveRefresh();
    if (transportMode !== 'SERIAL') return;
    await writeGatewayRecord({ type: 'stop' });
    if (gatewaySession) await sendSessionCommand('disarm');
    gatewayArmed = false;
    currentMotorL = 0;
    currentMotorR = 0;
  }

  async function disconnectWebSerial() {
    try {
      await safetyDisarm();
      if (serialReader) {
        await serialReader.cancel();
        try { serialReader.releaseLock(); } catch {}
        serialReader = null;
      }
      if (serialWriter) { await serialWriter.close(); serialWriter = null; }
      if (serialPort) { await serialPort.close(); serialPort = null; }
    } catch {}
    markSerialDisconnected();
  }

  async function returnToModeSelection() {
    if (transportMode === 'SERIAL') await disconnectWebSerial();
    missionMode = null;
    transportMode = 'DISCONNECTED';
    isConnected = false;
    telemetry = createUnknownTelemetry();
    scanPoints = [];
    historyBuffers = createEmptyHistory();
    resetInteractionState();
    if (initialMode) await goto('/dashboard');
  }

  // --- Motor Commands ---
  /** @param {number} left @param {number} right */
  async function sendDrive(left, right) {
    if (!hardwareControlReady || controlMode !== 'MANUAL') {
      currentMotorL = 0;
      currentMotorR = 0;
      return;
    }
    currentMotorL = left;
    currentMotorR = right;
    if (missionMode === 'hardware')
      await sendSessionCommand('drive', {
        left, right, ttl_ms: COMMAND_TTL_MS
      });
  }

  /** @param {number} action @param {number} argument */
  async function runDiagnostic(action, argument = 0) {
    if (missionMode !== 'hardware' || transportMode !== 'SERIAL' ||
        !gatewaySession || !gatewayRadioReady) return false;
    return sendSessionCommand('diagnostic', { action, argument });
  }

  async function calibrateOrientation() {
    if (orientationCalibrating || gatewayArmed) return;
    orientationCalibrating = true;
    if (missionMode === 'demo') {
      telemetry.pitchDeg = 0;
      telemetry.rollDeg = 0;
      telemetry.headingDeg = 0;
      setTimeout(() => { orientationCalibrating = false; }, 900);
      return;
    }
    const sent = await runDiagnostic(DIAGNOSTIC_ACTION.CALIBRATE_IMU, 0);
    if (!sent) {
      orientationCalibrating = false;
      return;
    }
    setTimeout(() => { orientationCalibrating = false; }, 3300);
  }

  function stopDriveRefresh() {
    if (driveRefreshTimer) clearInterval(driveRefreshTimer);
    driveRefreshTimer = null;
  }

  /** @param {number} left @param {number} right */
  function startHoldDrive(left, right) {
    stopDriveRefresh();
    void sendDrive(left, right);
    if (missionMode === 'hardware') {
      driveRefreshTimer = setInterval(
        () => void sendDrive(left, right), DRIVE_REFRESH_MS
      );
    }
  }

  async function stopDriveMotion() {
    stopDriveRefresh();
    currentMotorL = 0;
    currentMotorR = 0;
    if (missionMode === 'hardware' && transportMode === 'SERIAL')
      await writeGatewayRecord({ type: 'stop' });
  }

  async function armHardware() {
    if (missionMode !== 'hardware' || !isConnected ||
        !gatewayRadioReady || !gatewaySession || !roverLinkFresh) return;
    isEstop = false;
    await sendSessionCommand('heartbeat');
    await sendSessionCommand('arm');
  }

  async function toggleEstop() {
    if (!isConnected) return;
    if (!isEstop) {
      isEstopPending = true;
      isEstop = true;
      if (missionMode === 'hardware') await safetyDisarm();
      else await stopDriveMotion();
      setTimeout(() => { isEstopPending = false; }, 150);
    } else {
      isEstop = false;
      if (missionMode === 'hardware') await armHardware();
    }
  }

  function updateKeyboardDrive() {
    if (!keyState.w && !keyState.a && !keyState.s && !keyState.d) {
      void stopDriveMotion();
      return;
    }
    const linear = (keyState.w ? speedPercent : 0) -
                   (keyState.s ? speedPercent : 0);
    const turn = (keyState.d ? speedPercent : 0) -
                 (keyState.a ? speedPercent : 0);
    startHoldDrive(
      Math.max(-100, Math.min(100, linear + turn)),
      Math.max(-100, Math.min(100, linear - turn))
    );
  }

  /** @param {KeyboardEvent} e */
  function handleKeydown(e) {
    if (e.target instanceof HTMLInputElement || e.target instanceof HTMLTextAreaElement) return;
    if (e.code === 'KeyW' || e.code === 'ArrowUp') {
      e.preventDefault(); if (!keyState.w) { keyState.w = true; updateKeyboardDrive(); }
    } else if (e.code === 'KeyS' || e.code === 'ArrowDown') {
      e.preventDefault(); if (!keyState.s) { keyState.s = true; updateKeyboardDrive(); }
    } else if (e.code === 'KeyA' || e.code === 'ArrowLeft') {
      e.preventDefault(); if (!keyState.a) { keyState.a = true; updateKeyboardDrive(); }
    } else if (e.code === 'KeyD' || e.code === 'ArrowRight') {
      e.preventDefault(); if (!keyState.d) { keyState.d = true; updateKeyboardDrive(); }
    } else if (e.code === 'Space') {
      e.preventDefault();
      if (!keyState.space) { keyState.space = true; void toggleEstop(); }
    }
  }

  /** @param {KeyboardEvent} e */
  function handleKeyup(e) {
    if (e.code === 'KeyW' || e.code === 'ArrowUp') {
      keyState.w = false;
    } else if (e.code === 'KeyS' || e.code === 'ArrowDown') {
      keyState.s = false;
    } else if (e.code === 'KeyA' || e.code === 'ArrowLeft') {
      keyState.a = false;
    } else if (e.code === 'KeyD' || e.code === 'ArrowRight') {
      keyState.d = false;
    } else if (e.code === 'Space') {
      keyState.space = false;
      return;
    } else {
      return;
    }
    void stopDriveMotion();
    if (keyState.w || keyState.a || keyState.s || keyState.d)
      updateKeyboardDrive();
  }

  function handleWindowBlur() {
    keyState = { w: false, a: false, s: false, d: false, space: false };
    if (missionMode === 'hardware') void safetyDisarm();
    else void stopDriveMotion();
  }

  function handleVisibilityChange() {
    if (document.visibilityState === 'hidden') handleWindowBlur();
  }

  /** @param {'MANUAL' | 'AUTO_EXPLORE'} mode */
  async function selectControlMode(mode) {
    await stopDriveMotion();
    controlMode = mode;
    if (mode === 'AUTO_EXPLORE') isAutoPaused = false;
    if (missionMode !== 'hardware' || !gatewayArmed) return;
    await sendSessionCommand(mode === 'MANUAL' ? 'manual' : 'auto');
  }

  async function toggleAutoPause() {
    isAutoPaused = !isAutoPaused;
    if (missionMode === 'hardware' && gatewayArmed)
      await sendSessionCommand(isAutoPaused ? 'manual' : 'auto');
  }

  // --- Modal Config ---
  let activeModalData = $derived.by(() => {
    switch (selectedSensorKey) {
      case 'gas':
        return {
          title: 'MQ-4 Semiconductor Gas Activity',
          sensorIc: 'MQ-4 (SnO₂ Heating Element)',
          pinout: 'GPIO 36 / VP via 10k/15k divider',
          sampling: telemetry.source === 'SIMULATED' ? 'Simulated' : 'Raw ADC cadence',
          currentVal: `${telemetry.gasRaw ?? '—'} ADC`,
          unit: 'ADC',
          history: historyBuffers.gas,
          status: telemetry.source === 'UNKNOWN' ? 'unknown' : telemetry.gasState === 'QUALITATIVE_ADVISORY' ? 'warning' : 'normal',
          safetyNote: 'Raw ADC is not methane ppm, %LEL, or proof that air is safe. Establish warm-up and a recorded clean-air baseline before qualitative trend use.',
          actionThreshold: 'Not calibrated',
          dangerThreshold: 'No certified cutoff'
        };
      case 'temp':
        return {
          title: 'DHT22 Temperature and Humidity',
          sensorIc: 'DHT22',
          pinout: 'GPIO 23',
          sampling: '0.5 Hz',
          currentVal: `${telemetry.temperature ?? '—'} °C`,
          unit: '°C',
          history: historyBuffers.temperature,
          status: telemetry.temperature === null ? 'unknown' : 'normal',
          safetyNote: 'Temperature and humidity provide environmental context only; placement, self-heating, and stale samples must be recorded.',
          actionThreshold: 'Bench baseline required',
          dangerThreshold: 'No certified cutoff'
        };
      case 'water':
        return {
          title: 'Resistive Water-Contact Probe',
          sensorIc: 'Contact probe',
          pinout: 'GPIO 39 / VN',
          sampling: telemetry.source === 'SIMULATED' ? 'Simulated' : 'Raw ADC cadence',
          currentVal: `${telemetry.waterRaw ?? '—'} ADC`,
          unit: 'ADC',
          history: historyBuffers.water,
          status: telemetry.source === 'UNKNOWN' ? 'unknown' : telemetry.waterState === 'CONTACT' ? 'warning' : 'normal',
          safetyNote: 'The probe can indicate contact after calibration; it does not measure water depth or guarantee a dry path.',
          actionThreshold: 'Dry baseline required',
          dangerThreshold: 'Contact threshold unmeasured'
        };
      case 'ultrasonic':
        return {
          title: 'HC-SR04 Forward Ultrasonic',
          sensorIc: 'HC-SR04',
          pinout: 'TRIG GPIO 19 / ECHO GPIO 18 via divider',
          sampling: telemetry.source === 'SIMULATED' ? 'Simulated' : 'Bounded front sample',
          currentVal: `${telemetry.frontDistanceCm ?? '—'} cm`,
          unit: 'cm',
          history: historyBuffers.ultrasonic,
          status: telemetry.frontDistanceCm === null ? 'unknown' : 'normal',
          safetyNote: 'A fresh, valid front reading is required for forward motion. Final stop distances must be measured on the actual surface and battery state.',
          actionThreshold: 'Bench-calibrate slowdown',
          dangerThreshold: 'Bench-calibrate stop'
        };
      default: return null;
    }
  });

  /** @param {string} key */
  function inspectSensor(key) {
    selectedSensorKey = key;
    isModalOpen = true;
  }


  onMount(() => {
    window.addEventListener('keydown', handleKeydown);
    window.addEventListener('keyup', handleKeyup);
    window.addEventListener('blur', handleWindowBlur);
    document.addEventListener('visibilitychange', handleVisibilityChange);
    window.addEventListener('pagehide', handleWindowBlur);

    const heartbeatInterval = setInterval(() => {
      if (missionMode === 'hardware' && transportMode === 'SERIAL' && gatewaySession)
        void sendSessionCommand('heartbeat');
    }, HEARTBEAT_INTERVAL_MS);

    const freshnessInterval = setInterval(() => {
      if (missionMode !== 'hardware' || !lastTelemetryReceivedAt) return;
      heartbeatAgeMs = Math.round(performance.now() - lastTelemetryReceivedAt);
      if (heartbeatAgeMs > TELEMETRY_STALE_MS && telemetry.source === 'LIVE') {
        telemetry.source = 'STALE';
        telemetry.alertState = 'UNKNOWN';
      }
    }, 100);

    const simInterval = setInterval(() => {
      if (missionMode === 'demo' && transportMode === 'SIMULATION' && isConnected) {
        telemetry.source = 'SIMULATED';
        heartbeatAgeMs = 45 + Math.floor(Math.random() * 20);
        telemetry.temperature = +(28.2 + Math.sin(Date.now() / 4000) * 0.8).toFixed(1);
        telemetry.humidity = +(69.7 + Math.cos(Date.now() / 5000) * 1.5).toFixed(1);
        telemetry.gasRaw = Math.max(600, Math.round(862 + Math.sin(Date.now() / 3000) * 40));
        telemetry.gasState = 'SIMULATED';
        telemetry.waterRaw = Math.round(350 + Math.cos(Date.now() / 7000) * 20);
        telemetry.waterState = 'SIMULATED';
        telemetry.pitchDeg = +(-3.5 + Math.sin(Date.now() / 2500) * 1.5).toFixed(1);
        telemetry.rollDeg = +(2.6 + Math.cos(Date.now() / 2800) * 1.2).toFixed(1);
        telemetry.frontDistanceCm = +(119.0 + Math.sin(Date.now() / 2000) * 15.0).toFixed(0);

        if (!isEstop) {
          if (controlMode === 'AUTO_EXPLORE' && !isAutoPaused) {
            const t = Date.now() / 1500;
            roverPose.x = +(33.2 + Math.sin(t * 0.4) * 8).toFixed(1);
            roverPose.y = +(-13.6 + Math.cos(t * 0.3) * 6).toFixed(1);
            roverPose.headingDeg = +(90 + Math.sin(t * 0.5) * 30).toFixed(0);
            const leftTicks = (telemetry.encoderL ?? 0) + 1;
            const rightTicks = (telemetry.encoderR ?? 0) + 1;
            telemetry.encoderL = leftTicks;
            telemetry.encoderR = rightTicks;
            telemetry.estimatedDistanceMeters = +(4.52 + leftTicks * 0.005).toFixed(2);
          } else if (currentMotorL !== 0 || currentMotorR !== 0) {
            const speed = (currentMotorL + currentMotorR) / 100;
            const turn = (currentMotorR - currentMotorL) / 100;
            roverPose.headingDeg += turn * 3;
            const rad = (roverPose.headingDeg - 90) * (Math.PI / 180);
            roverPose.x += Math.cos(rad) * speed * 2;
            roverPose.y += Math.sin(rad) * speed * 2;
            const leftTicks = (telemetry.encoderL ?? 0) + 2;
            telemetry.encoderL = leftTicks;
            telemetry.encoderR = (telemetry.encoderR ?? 0) + 2;
            telemetry.estimatedDistanceMeters = +(leftTicks * (Math.PI * 0.065 / 20)).toFixed(2);
          }
        }
      }
    }, 200);

    return () => {
      window.removeEventListener('keydown', handleKeydown);
      window.removeEventListener('keyup', handleKeyup);
      window.removeEventListener('blur', handleWindowBlur);
      document.removeEventListener('visibilitychange', handleVisibilityChange);
      window.removeEventListener('pagehide', handleWindowBlur);
      clearInterval(simInterval);
      clearInterval(heartbeatInterval);
      clearInterval(freshnessInterval);
      stopDriveRefresh();
      if (missionMode === 'hardware' && transportMode === 'SERIAL')
        void writeGatewayRecord({ type: 'stop' });
    };
  });
</script>

<svelte:head>
  <title>DeepTrack · Dashboard</title>
</svelte:head>

{#if missionMode === null}
  <div class="min-h-screen flex flex-col bg-[var(--md-sys-color-surface)] text-[var(--md-sys-color-on-surface)] select-none" in:m3TopLevelFadeThrough={{ duration: 400 }}>
    <Navbar active="dashboard" />
    
    <main class="flex-1 flex items-center justify-center p-6">
      <div class="ui-card w-full max-w-md">
        <div class="text-center mb-8">
          <h1 class="text-2xl font-bold font-headline tracking-tight text-[var(--md-sys-color-on-surface)] mb-2">Choose a dashboard</h1>
          <p class="text-sm text-[var(--md-sys-color-on-surface-variant)] leading-relaxed">
            Try the demo, or sign in to connect the real gateway.
          </p>
        </div>
        
        <div class="flex flex-col gap-4">
          <a
            href="/dashboard/demo"
            class="ui-button ui-button--filled w-full flex items-center justify-center gap-2"
          >
            <span class="material-symbols-rounded text-lg">science</span>
            <span>Open demo dashboard</span>
          </a>

          <a
            href={auth.user ? '/dashboard/real' : '/auth'}
            class="ui-button ui-button--outlined w-full flex items-center justify-center gap-2"
          >
            <span class="material-symbols-rounded text-lg">memory</span>
            <span>{auth.user ? 'Connect real gateway' : 'Sign in to connect gateway'}</span>
          </a>
        </div>
        
        <div class="mt-8 pt-5 border-t border-[var(--md-sys-color-outline-variant)] text-[11px] text-[var(--md-sys-color-on-surface-variant)] flex items-start gap-1.5 font-medium">
          <span class="material-symbols-rounded text-sm">verified_user</span>
          <span>Demo data never appears in the real dashboard.</span>
        </div>
      </div>
    </main>
    
    <Footer />
  </div>
{:else}
<!-- ONE SHARED APPLICATION SHELL -->
<div class="h-screen max-h-screen w-screen overflow-hidden flex bg-[var(--md-sys-color-surface)] text-[var(--md-sys-color-on-surface)] select-none">
  
  <!-- Left Navigation Rail (Persistent 80px) -->
  <NavRail
    activeView={activeNavView}
    onSelectView={(/** @type {string} */ v) => activeNavView = v}
    {isConnected}
    {isEstop}
  />

  <div class="flex-1 flex flex-col h-full overflow-hidden">
    <!-- PERSISTENT TOP APP BAR (64px, Flat, No Cards) -->
    <header class="h-16 shrink-0 px-4 md:px-6 bg-[var(--md-sys-color-surface)] border-b border-[var(--md-sys-color-outline-variant)] flex items-center justify-between gap-4">
      
      <!-- Left: Rover Identity & Environment -->
      <div class="flex items-center gap-3">
        <span class="w-3 h-3 rounded-full shadow-sm"
          class:bg-[var(--ui-color-success)]={hardwareControlReady}
          class:bg-[var(--md-sys-color-error)]={!hardwareControlReady}
          class:animate-pulse={hardwareControlReady}>
        </span>
        <span class="telemetry font-bold text-base md:text-lg tracking-wide text-[var(--md-sys-color-on-surface)]">
          DT-ALPHA-01
        </span>
        <span class="text-sm font-medium px-2 py-0.5 rounded-md flex items-center gap-1.5 {missionMode === 'demo' ? 'bg-[var(--ui-color-warning-container)] text-[var(--ui-color-on-warning-container)]' : transportMode === 'SERIAL' ? 'bg-[var(--ui-color-success-container)] text-[var(--ui-color-on-success-container)]' : 'bg-[var(--md-sys-color-surface-container-highest)] text-[var(--md-sys-color-on-surface-variant)]'}">
          <span class="material-symbols-rounded text-[18px]">{missionMode === 'demo' ? 'science' : transportMode === 'SERIAL' ? 'usb' : 'memory'}</span>
          {missionMode === 'demo' ? 'DEMO · SIMULATED' : transportMode !== 'SERIAL' ? 'REAL · UNKNOWN' : !gatewaySession ? 'REAL · USB / WAITING' : !gatewayRadioReady ? 'REAL · RADIO DISABLED' : gatewayArmed ? 'REAL · ARMED' : 'REAL · DISARMED'}
        </span>
      </div>

      <!-- Center: Connection State -->
      <div class="hidden lg:grid grid-cols-[132px_164px_142px] items-center text-sm font-medium text-[var(--md-sys-color-on-surface-variant)] tabular-nums">
        {#if missionMode === 'hardware' && transportMode === 'SERIAL'}
          <span class="whitespace-nowrap border-r border-[var(--md-sys-color-outline-variant)] pr-4">RSSI: <strong class="telemetry inline-block min-w-[64px] text-[var(--md-sys-color-on-surface)]">{telemetry.rssi === null ? 'UNKNOWN' : `${telemetry.rssi} dBm`}</strong></span>
          <span class="whitespace-nowrap px-4 border-r border-[var(--md-sys-color-outline-variant)]">Heartbeat: <strong class="telemetry inline-block min-w-[64px] text-[var(--ui-brand-cyan)]">{heartbeatAgeMs === null ? 'UNKNOWN' : `${heartbeatAgeMs} ms`}</strong></span>
          <span class="whitespace-nowrap pl-4">Radio: <strong class="inline-block min-w-[88px]">{gatewayRadioReady ? 'READY' : 'UNAVAILABLE'}</strong></span>
        {/if}
      </div>

      <!-- Right: mode action, real-only connection, and remote stop -->
      <div class="flex items-center gap-3 shrink-0">
        <button type="button" class="ui-button ui-button--outlined !h-10 !px-4 text-sm" onclick={returnToModeSelection}>
          Change mode
        </button>

        {#if missionMode === 'hardware'}
          {#if transportMode === 'SERIAL'}
            <button type="button" class="ui-button ui-button--tonal !h-10 !px-4 text-sm" onclick={gatewayArmed ? safetyDisarm : armHardware} disabled={!gatewayRadioReady || (!gatewayArmed && !roverLinkFresh)}>
              {gatewayArmed ? 'Disarm' : 'Arm controls'}
            </button>
            <button type="button" class="ui-button ui-button--outlined !h-10 !px-4 text-sm" onclick={disconnectWebSerial}>
              Disconnect
            </button>
          {:else}
            <button type="button" class="ui-button ui-button--filled !bg-[var(--md-sys-color-primary)] !text-[var(--md-sys-color-on-primary)] !h-10 !px-5 text-sm font-semibold" onclick={connectWebSerial} disabled={isConnectingSerial}>
              <span class="material-symbols-rounded text-[20px]">usb</span>
              {#if isConnectingSerial}Connecting...{:else}Connect USB{/if}
            </button>
          {/if}
        {/if}

        <button
          type="button"
          class="ui-button !h-10 !px-6 text-sm font-bold transition-all duration-150 {isEstop ? 'bg-[var(--ui-color-success)] text-[var(--ui-color-on-success)] animate-pulse' : isEstopPending ? 'bg-[var(--ui-color-warning)] text-[var(--ui-color-on-warning)]' : 'ui-button--filled !bg-[var(--md-sys-color-error)] !text-[var(--md-sys-color-on-error)] focus-visible:ring-2 focus-visible:ring-[var(--md-sys-color-error)]'} {!isConnected ? 'opacity-50 cursor-not-allowed' : 'active:scale-95'}"
          onclick={toggleEstop}
          disabled={!isConnected}
        >
          <span class="material-symbols-rounded text-[20px]">
            {isEstop ? 'play_arrow' : 'stop_circle'}
          </span>
          <span>
            {#if !isConnected}Remote stop unavailable
            {:else if isEstopPending}Stopping...
            {:else if isEstop}Reset remote stop
            {:else}Remote stop{/if}
          </span>
          </button>
      </div>
    </header>

    {#if missionMode === 'hardware' && serialConnectionError}
      <div role="alert" class="mx-4 mt-3 rounded-xl border border-[var(--md-sys-color-error)] px-4 py-3 text-sm text-[var(--md-sys-color-error)]">
        {serialConnectionError}
      </div>
    {/if}

    <!-- CONTENT AREA -->
    <main class="flex-1 overflow-hidden relative">
      {#if activeNavView === 'console'}
        <div class="absolute inset-0 flex flex-col p-4 md:p-5 gap-4 overflow-hidden min-h-0" in:m3TopLevelFadeThrough={{ duration: 300 }}>
          
          <!-- TELEMETRY RIBBON -->
          <section class="grid grid-cols-2 lg:grid-cols-4 divide-y lg:divide-y-0 lg:divide-x divide-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface-container-low)] rounded-xl border border-[var(--md-sys-color-outline-variant)] shrink-0 overflow-hidden">
            
            <button type="button" class="text-left flex flex-col gap-1 px-5 py-4 cursor-pointer hover:bg-[var(--md-sys-color-surface-container-highest)] transition-colors duration-150 active:bg-[var(--md-sys-color-surface-variant)]" onclick={() => inspectSensor('gas')}>
              <div class="flex items-center gap-1.5 text-sm font-medium text-[var(--md-sys-color-on-surface-variant)]">
                <span class="material-symbols-rounded text-[20px] text-[var(--ui-brand-cyan)]">air</span> MQ-4 activity
              </div>
              <div class="flex items-baseline gap-2">
                <span class="text-3xl font-bold telemetry text-[var(--md-sys-color-on-surface)]">{telemetry.gasRaw ?? '—'} <span class="text-base font-normal">{telemetry.gasRaw === null ? '' : 'ADC'}</span></span>
              </div>
              <span class="text-sm text-[var(--md-sys-color-on-surface-variant)]">Uncalibrated raw signal · {telemetry.source}</span>
            </button>

            <button type="button" class="text-left flex flex-col gap-1 px-5 py-4 cursor-pointer hover:bg-[var(--md-sys-color-surface-container-highest)] transition-colors duration-150 active:bg-[var(--md-sys-color-surface-variant)]" onclick={() => inspectSensor('temp')}>
              <div class="flex items-center gap-1.5 text-sm font-medium text-[var(--md-sys-color-on-surface-variant)]">
                <span class="material-symbols-rounded text-[20px] text-[var(--ui-color-warning)]">device_thermostat</span> Climate
              </div>
              <div class="flex items-baseline gap-2">
                <span class="text-3xl font-bold telemetry text-[var(--md-sys-color-on-surface)]">{telemetry.temperature ?? '—'} <span class="text-base font-normal">{telemetry.temperature === null ? '' : '°C'}</span></span>
              </div>
              <span class="text-sm text-[var(--md-sys-color-on-surface-variant)]">{telemetry.humidity === null ? 'UNKNOWN' : `${telemetry.humidity}% RH`}</span>
            </button>

            <button type="button" class="text-left flex flex-col gap-1 px-5 py-4 cursor-pointer hover:bg-[var(--md-sys-color-surface-container-highest)] transition-colors duration-150 active:bg-[var(--md-sys-color-surface-variant)]" onclick={() => inspectSensor('water')}>
              <div class="flex items-center gap-1.5 text-sm font-medium text-[var(--md-sys-color-on-surface-variant)]">
                <span class="material-symbols-rounded text-[20px] text-blue-400">water_drop</span> Water
              </div>
              <div class="flex items-baseline gap-2">
                <span class="text-3xl font-bold telemetry text-[var(--md-sys-color-on-surface)]">{telemetry.waterRaw ?? '—'} <span class="text-base font-normal">{telemetry.waterRaw === null ? '' : 'ADC'}</span></span>
              </div>
              <span class="text-sm text-[var(--md-sys-color-on-surface-variant)]">Contact signal only · {telemetry.waterState}</span>
            </button>

            <button type="button" class="text-left flex flex-col gap-1 px-5 py-4 cursor-pointer hover:bg-[var(--md-sys-color-surface-container-highest)] transition-colors duration-150 active:bg-[var(--md-sys-color-surface-variant)]" onclick={() => inspectSensor('ultrasonic')}>
              <div class="flex items-center gap-1.5 text-sm font-medium text-[var(--md-sys-color-on-surface-variant)]">
                <span class="material-symbols-rounded text-[20px] text-[var(--ui-color-success)]">sensors</span> Front clearance
              </div>
              <div class="flex items-baseline gap-2">
                <span class="text-3xl font-bold telemetry text-[var(--md-sys-color-on-surface)]">{telemetry.frontDistanceCm === null ? '—' : telemetry.frontDistanceCm > 300 ? 'Clear' : telemetry.frontDistanceCm < 2 ? '---' : telemetry.frontDistanceCm} <span class="text-base font-normal">{telemetry.frontDistanceCm !== null && telemetry.frontDistanceCm <= 300 && telemetry.frontDistanceCm >= 2 ? 'cm' : ''}</span></span>
              </div>
              <span class="text-sm text-[var(--md-sys-color-on-surface-variant)]">Stop threshold pending bench calibration</span>
            </button>
          </section>

          <!-- CONSOLE PANES (Left, Center, Right) -->
          <div class="flex-1 flex flex-col lg:flex-row gap-6 min-h-[400px]">
            
            <!-- LEFT: DRIVE CONTROLS -->
            <aside class="w-full lg:w-[260px] flex flex-col gap-6 shrink-0">
              <div class="flex flex-col gap-3">
                <h3 class="text-lg font-semibold flex items-center gap-2">
                  <span class="material-symbols-rounded text-[22px] text-[var(--md-sys-color-on-surface-variant)]">gamepad</span> Control mode
                </h3>
                <div class="flex p-1 bg-[var(--md-sys-color-surface-container)] rounded-xl mt-1 w-full relative z-0">
                  <button type="button" class="flex-1 py-1.5 flex items-center justify-center gap-1.5 text-sm font-semibold transition-colors rounded-lg relative z-10 {controlMode === 'MANUAL' ? 'text-[var(--md-sys-color-on-primary-container)]' : 'text-[var(--md-sys-color-on-surface-variant)] hover:text-[var(--md-sys-color-on-surface)]'}" onclick={() => selectControlMode('MANUAL')}>
                    {#if controlMode === 'MANUAL'}
                      <div class="absolute inset-0 bg-[var(--md-sys-color-primary-container)] rounded-lg shadow-sm -z-10" in:receiveShared={{key: 'control-mode'}} out:sendShared={{key: 'control-mode'}}></div>
                    {/if}
                    <span class="material-symbols-rounded text-[18px]">sports_esports</span> Manual
                  </button>
                  <button type="button" class="flex-1 py-1.5 flex items-center justify-center gap-1.5 text-sm font-semibold transition-colors rounded-lg relative z-10 {controlMode === 'AUTO_EXPLORE' ? 'text-[var(--md-sys-color-on-primary-container)]' : 'text-[var(--md-sys-color-on-surface-variant)] hover:text-[var(--md-sys-color-on-surface)]'}" onclick={() => selectControlMode('AUTO_EXPLORE')} disabled={!hardwareControlReady}>
                    {#if controlMode === 'AUTO_EXPLORE'}
                      <div class="absolute inset-0 bg-[var(--md-sys-color-primary-container)] rounded-lg shadow-sm -z-10" in:receiveShared={{key: 'control-mode'}} out:sendShared={{key: 'control-mode'}}></div>
                    {/if}
                    <span class="material-symbols-rounded text-[18px]">smart_toy</span> Autonomous
                  </button>
                </div>
              </div>

              {#if controlMode === 'MANUAL'}
                <div class="flex flex-col items-center">
                  <div class="grid grid-cols-3 gap-2 w-52">
                    <div></div>
                    <button type="button" class="h-14 rounded-lg bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center hover:bg-[var(--md-sys-color-primary-container)] hover:text-[var(--md-sys-color-on-primary-container)] active:bg-[var(--md-sys-color-primary)] active:text-[var(--md-sys-color-on-primary)] active:scale-95 transition-all duration-150 {keyState.w ? '!bg-[var(--md-sys-color-primary)] !text-[var(--md-sys-color-on-primary)] scale-95' : ''}" disabled={!hardwareControlReady} onpointerdown={() => startHoldDrive(speedPercent, speedPercent)} onpointerup={stopDriveMotion} onpointercancel={stopDriveMotion} onpointerleave={stopDriveMotion}><span class="material-symbols-rounded text-[26px]">arrow_upward</span></button>
                    <div></div>
                    <button type="button" class="h-14 rounded-lg bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center hover:bg-[var(--md-sys-color-primary-container)] hover:text-[var(--md-sys-color-on-primary-container)] active:bg-[var(--md-sys-color-primary)] active:text-[var(--md-sys-color-on-primary)] active:scale-95 transition-all duration-150 {keyState.a ? '!bg-[var(--md-sys-color-primary)] !text-[var(--md-sys-color-on-primary)] scale-95' : ''}" disabled={!hardwareControlReady} onpointerdown={() => startHoldDrive(-speedPercent, speedPercent)} onpointerup={stopDriveMotion} onpointercancel={stopDriveMotion} onpointerleave={stopDriveMotion}><span class="material-symbols-rounded text-[26px]">arrow_back</span></button>
                    <button type="button" class="h-14 rounded-lg bg-[var(--md-sys-color-error-container)] text-[var(--md-sys-color-on-error-container)] text-sm font-bold active:scale-95 border border-[var(--md-sys-color-outline-variant)] transition-all opacity-80 hover:opacity-100 {keyState.space ? '!bg-[var(--md-sys-color-error)] !text-[var(--md-sys-color-on-error)] scale-95 !opacity-100' : ''}" onclick={stopDriveMotion}>STOP</button>
                    <button type="button" class="h-14 rounded-lg bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center hover:bg-[var(--md-sys-color-primary-container)] hover:text-[var(--md-sys-color-on-primary-container)] active:bg-[var(--md-sys-color-primary)] active:text-[var(--md-sys-color-on-primary)] active:scale-95 transition-all duration-150 {keyState.d ? '!bg-[var(--md-sys-color-primary)] !text-[var(--md-sys-color-on-primary)] scale-95' : ''}" disabled={!hardwareControlReady} onpointerdown={() => startHoldDrive(speedPercent, -speedPercent)} onpointerup={stopDriveMotion} onpointercancel={stopDriveMotion} onpointerleave={stopDriveMotion}><span class="material-symbols-rounded text-[26px]">arrow_forward</span></button>
                    <div></div>
                    <button type="button" class="h-14 rounded-lg bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center hover:bg-[var(--md-sys-color-primary-container)] hover:text-[var(--md-sys-color-on-primary-container)] active:bg-[var(--md-sys-color-primary)] active:text-[var(--md-sys-color-on-primary)] active:scale-95 transition-all duration-150 {keyState.s ? '!bg-[var(--md-sys-color-primary)] !text-[var(--md-sys-color-on-primary)] scale-95' : ''}" disabled={!hardwareControlReady} onpointerdown={() => startHoldDrive(-speedPercent, -speedPercent)} onpointerup={stopDriveMotion} onpointercancel={stopDriveMotion} onpointerleave={stopDriveMotion}><span class="material-symbols-rounded text-[26px]">arrow_downward</span></button>
                    <div></div>
                  </div>
                  <div class="mt-3 text-sm font-medium text-[var(--md-sys-color-on-surface-variant)] flex gap-3 telemetry"><span>W</span><span>A</span><span>S</span><span>D</span><span>Space</span></div>
                </div>

                <div class="flex flex-col gap-3">
                  <div class="flex justify-between items-end text-sm">
                    <span class="font-semibold flex items-center gap-1.5"><span class="material-symbols-rounded text-[18px]">speed</span> Speed limit</span>
                    <span class="telemetry font-bold text-[var(--md-sys-color-primary)]">{speedPercent}%</span>
                  </div>
                  <input type="range" min="30" max="80" step="5" bind:value={speedPercent} aria-label="Manual motor command percent" class="w-full h-2 rounded-lg appearance-none bg-[var(--md-sys-color-surface-container-highest)] accent-[var(--md-sys-color-primary)] cursor-pointer">
                  <div class="flex gap-1 p-1 bg-[var(--md-sys-color-surface-container)] rounded-xl text-sm font-medium mt-1 relative z-0 border border-[var(--md-sys-color-outline-variant)]">
                    <button type="button" class="flex-1 py-1.5 rounded-lg transition-colors relative z-10 {speedPercent === 40 ? 'text-[var(--md-sys-color-on-primary-container)]' : 'text-[var(--md-sys-color-on-surface-variant)] hover:text-[var(--md-sys-color-on-surface)]'}" onclick={() => speedPercent=40}>
                      {#if speedPercent === 40}
                        <div class="absolute inset-0 bg-[var(--md-sys-color-primary-container)] rounded-lg shadow-sm -z-10" in:receiveShared={{key: 'speed-limit'}} out:sendShared={{key: 'speed-limit'}}></div>
                      {/if}
                      Low
                    </button>
                    <button type="button" class="flex-1 py-1.5 rounded-lg transition-colors relative z-10 {speedPercent === 55 ? 'text-[var(--md-sys-color-on-primary-container)]' : 'text-[var(--md-sys-color-on-surface-variant)] hover:text-[var(--md-sys-color-on-surface)]'}" onclick={() => speedPercent=55}>
                      {#if speedPercent === 55}
                        <div class="absolute inset-0 bg-[var(--md-sys-color-primary-container)] rounded-lg shadow-sm -z-10" in:receiveShared={{key: 'speed-limit'}} out:sendShared={{key: 'speed-limit'}}></div>
                      {/if}
                      Med
                    </button>
                    <button type="button" class="flex-1 py-1.5 rounded-lg transition-colors relative z-10 {speedPercent === 70 ? 'text-[var(--md-sys-color-on-primary-container)]' : 'text-[var(--md-sys-color-on-surface-variant)] hover:text-[var(--md-sys-color-on-surface)]'}" onclick={() => speedPercent=70}>
                      {#if speedPercent === 70}
                        <div class="absolute inset-0 bg-[var(--md-sys-color-primary-container)] rounded-lg shadow-sm -z-10" in:receiveShared={{key: 'speed-limit'}} out:sendShared={{key: 'speed-limit'}}></div>
                      {/if}
                      Max
                    </button>
                  </div>
                </div>

                <div class="flex flex-col gap-3 pt-5 border-t border-[var(--md-sys-color-outline-variant)] text-sm text-[var(--md-sys-color-on-surface-variant)]">
                  <div class="flex justify-between items-center"><span class="font-medium">Command</span><span class="telemetry font-bold text-[var(--md-sys-color-on-surface)]">L: {currentMotorL}% &nbsp; R: {currentMotorR}%</span></div>
                  <div class="flex justify-between items-center"><span class="font-medium">Encoder count</span><span class="telemetry font-bold text-[var(--md-sys-color-on-surface)]">{telemetry.encoderL ?? '—'} L &nbsp; {telemetry.encoderR ?? '—'} R</span></div>
                </div>
              {:else}
                <div class="flex flex-col gap-4">
                  <div class="flex justify-between items-center text-sm">
                    <span class="font-semibold text-[var(--md-sys-color-on-surface)]">Navigation logic</span>
                    <span class="px-2.5 py-0.5 rounded font-bold text-[13px] {isAutoPaused ? 'bg-[var(--ui-color-warning-container)] text-[var(--ui-color-on-warning-container)]' : 'bg-[var(--ui-color-success-container)] text-[var(--ui-color-on-success-container)]'}">{isAutoPaused ? 'Paused' : 'Active'}</span>
                  </div>
                  <p class="text-sm text-[var(--md-sys-color-on-surface-variant)] leading-relaxed">Sweep navigation driven by SG90 servo sweeps and VL53L0X distance sensing.</p>
                  <div class="flex gap-3 mt-2">
                    <button type="button" class="ui-button ui-button--tonal flex-1 !h-10" onclick={toggleAutoPause}>
                      <span class="material-symbols-rounded text-[20px]">{isAutoPaused ? 'play_arrow' : 'pause'}</span>
                      {isAutoPaused ? 'Resume' : 'Pause'}
                    </button>
                    <button type="button" class="ui-button ui-button--outlined flex-1 !h-10" onclick={() => selectControlMode('MANUAL')}>
                      <span class="material-symbols-rounded text-[20px]">stop</span>
                      Halt
                    </button>
                  </div>
                </div>
              {/if}
            </aside>

            <!-- CENTER: explicitly simulated/estimated route viewport -->
            <main class="flex-1 min-w-0 h-[450px] lg:h-auto border border-[var(--md-sys-color-outline-variant)] bg-[#0c0e13] relative flex flex-col">
              <EstimatedRouteCanvas
                mode={controlMode}
                isRecording={isRecordingMap}
                onToggleRecord={() => isRecordingMap = !isRecordingMap}
                {scanPoints}
                {roverPose}
                {isConnected}
                isDemo={missionMode === 'demo'}
                dataSource={telemetry.source}
                hasEstimatedPose={missionMode === 'demo' || estimatedMapState.pose.known}
                mapEvidence={missionMode === 'demo' ? null : realMapEvidence}
              />
            </main>

            <!-- RIGHT: SAFETY & TILT -->
            <aside class="w-full lg:w-[280px] xl:w-[310px] flex flex-col gap-6 shrink-0">
              <div class="flex flex-col gap-5">
                <h3 class="text-lg font-semibold flex items-center gap-2">
                  <span class="material-symbols-rounded text-[22px] text-[var(--md-sys-color-on-surface-variant)]">health_and_safety</span> Safety state
                </h3>
                
                <div class="flex items-start gap-3 p-3 rounded-lg border {isEstop || telemetry.alertState === 'STOPPED' ? 'bg-[var(--md-sys-color-error-container)] border-[var(--md-sys-color-error)] text-[var(--md-sys-color-on-error-container)]' : telemetry.alertState === 'ADVISORY' ? 'bg-[var(--ui-color-warning-container)] border-[var(--ui-color-warning)] text-[var(--ui-color-on-warning-container)]' : telemetry.alertState === 'UNKNOWN' ? 'bg-[var(--md-sys-color-surface-container-highest)] border-[var(--md-sys-color-outline-variant)] text-[var(--md-sys-color-on-surface-variant)]' : 'bg-[var(--ui-color-success-container)] border-transparent text-[var(--ui-color-on-success-container)]'}">
                  <span class="material-symbols-rounded text-[28px]">
                    {isEstop ? 'front_hand' : telemetry.alertState === 'ADVISORY' || telemetry.alertState === 'STOPPED' ? 'warning' : 'info'}
                  </span>
                  <div>
                    <div class="text-base font-bold">{isEstop ? 'Remote stop requested' : telemetry.alertState === 'ADVISORY' ? 'Prototype advisory' : telemetry.alertState === 'STOPPED' ? 'Stopped' : `${telemetry.source} telemetry`}</div>
                    <div class="text-[13px] mt-0.5 opacity-90">{transportMode === 'SIMULATION' ? 'No hardware watchdog in demo mode' : gatewayRadioReady ? 'Gateway 450 ms heartbeat watchdog · rover command TTL 300 ms' : 'Hardware link is not ready'}</div>
                  </div>
                </div>

                <div class="flex flex-col gap-3 text-sm">
                  <div class="flex items-center justify-between">
                    <span class="text-[var(--md-sys-color-on-surface-variant)] flex items-center gap-2"><span class="w-2.5 h-2.5 rounded-full bg-[var(--md-sys-color-outline)]"></span> MQ-4 signal</span>
                    <span class="font-bold text-[var(--md-sys-color-on-surface)]">{telemetry.gasState}</span>
                  </div>
                  <div class="flex items-center justify-between">
                    <span class="text-[var(--md-sys-color-on-surface-variant)] flex items-center gap-2"><span class="w-2.5 h-2.5 rounded-full bg-[var(--ui-color-success)]"></span> VL53L0X</span> 
                    <span class="font-bold text-[var(--md-sys-color-on-surface)]">{tofStatus}</span>
                  </div>
                  <div class="flex items-center justify-between">
                    <span class="text-[var(--md-sys-color-on-surface-variant)] flex items-center gap-2"><span class="w-2.5 h-2.5 rounded-full bg-[var(--ui-color-success)]"></span> HC-SR04</span> 
                    <span class="font-bold text-[var(--md-sys-color-on-surface)]">{telemetry.frontFresh === true ? telemetry.frontBlocked ? 'BLOCKED' : 'FRESH' : 'UNKNOWN'}</span>
                  </div>
                  <div class="flex items-center justify-between">
                    <span class="text-[var(--md-sys-color-on-surface-variant)] flex items-center gap-2"><span class="w-2.5 h-2.5 rounded-full bg-[var(--ui-color-success)]"></span> MPU6050</span> 
                    <span class="font-bold text-[var(--md-sys-color-on-surface)]">{telemetry.pitchDeg === null ? 'UNKNOWN' : 'LIVE 6-DOF'}</span>
                  </div>
                </div>
              </div>

              <div class="flex-1 min-h-0 flex flex-col border-t border-[var(--md-sys-color-outline-variant)] pt-3">
                <div class="mb-3 flex items-center justify-between gap-3">
                  <h3 class="text-lg font-semibold flex items-center gap-2">
                    <span class="material-symbols-rounded text-[22px] text-[var(--md-sys-color-on-surface-variant)]">explore</span> Orientation
                  </h3>
                  <button type="button" class="ui-button ui-button--outlined !h-9 px-3 text-xs" disabled={orientationCalibrating || gatewayArmed || (missionMode !== 'demo' && (!isConnected || !gatewayRadioReady))} onclick={calibrateOrientation}>
                    <span class="material-symbols-rounded text-base">{orientationCalibrating ? 'progress_activity' : 'my_location'}</span>
                    {orientationCalibrating ? 'Keep still…' : 'Calibrate'}
                  </button>
                </div>
                <div class="flex-1">
                  <TiltHorizon
                    pitchDeg={telemetry.pitchDeg}
                    rollDeg={telemetry.rollDeg}
                    headingDeg={telemetry.headingDeg}
                    isStale={missionMode !== 'demo' && telemetry.source !== 'LIVE'}
                    isCalibrating={orientationCalibrating}
                    maxTiltThreshold={25.0}
                    cautionThreshold={12.0}
                  />
                </div>
              </div>
            </aside>
          </div>
        </div>
      {:else if activeNavView === 'maps'}
        <div class="absolute inset-0" in:m3TopLevelFadeThrough={{ duration: 300 }}>
          <MapsView isDemo={missionMode === 'demo'} />
        </div>
      {:else if activeNavView === 'logs'}
        <div class="absolute inset-0" in:m3TopLevelFadeThrough={{ duration: 300 }}>
          <LogsView isDemo={missionMode === 'demo'} {realLogs} />
        </div>
      {:else if activeNavView === 'hardware'}
        <div class="absolute inset-0" in:m3TopLevelFadeThrough={{ duration: 300 }}>
          <HardwareView
            {isConnected}
            {gatewayArmed}
            {telemetry}
            {diagnosticResults}
            onRunDiagnostic={runDiagnostic}
            onMotorStart={startHoldDrive}
            onMotorStop={stopDriveMotion}
          />
        </div>
      {/if}
    </main>
  </div>
</div>

<SensorDetailModal isOpen={isModalOpen} sensor={activeModalData} onClose={() => { isModalOpen = false; selectedSensorKey = null; }} />

{/if}
