<script>
  import NavRail from '$lib/components/NavRail.svelte';
  import MetricTile from '$lib/components/MetricTile.svelte';
  import EstimatedRouteCanvas from '$lib/components/EstimatedRouteCanvas.svelte';
  import TiltHorizon from '$lib/components/TiltHorizon.svelte';
  import AiRiskPanel from '$lib/components/AiRiskPanel.svelte';
  import SensorDetailModal from '$lib/components/SensorDetailModal.svelte';
  import MapsView from '$lib/components/dashboard/MapsView.svelte';
  import LogsView from '$lib/components/dashboard/LogsView.svelte';
  import HardwareView from '$lib/components/dashboard/HardwareView.svelte';
  import Navbar from '$lib/components/Navbar.svelte';
  import Footer from '$lib/components/Footer.svelte';
  import { onMount } from 'svelte';
  import { goto } from '$app/navigation';
  import { auth } from '$lib/stores/auth.svelte.js';
  import { supabase } from '$lib/supabaseClient.js';
  import { createDashboardSync } from '$lib/offline/dashboardSync.js';
  import { initialTelemetry as demoTelemetry, initialScanPoints as demoScanPoints } from '$lib/mocks/telemetryMock.js';
  import { createUnknownTelemetry, createEmptyHistory, createDemoHistory } from '$lib/state/telemetry.js';
  import { roverCalibration } from '$lib/config/roverCalibration.js';
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
  import {
    createMissionControllerState, startMission, stepMission, stopMission
  } from '$lib/state/missionController.js';
  import {
    gateManualDrive, keyboardDriveVector, requestsForward
  } from '$lib/state/manualDrive.js';
  import { fitHeight } from '$lib/actions/fitHeight.js';
  import {
    createHazardEngine, createUnavailableHazardResult
  } from '$lib/ai/hazardEngine.js';
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
  let isArmPending = $state(false);
  let estimatedMapState = $state(/** @type {any} */ (createEstimatedMapState()));
  let realMapEvidence = $state(renderableEvidence(createEstimatedMapState()));
  let missionController = $state(/** @type {any} */ (createMissionControllerState()));
  let missionTickBusy = false;
  let exploreBootstrapScanIds = $state(/** @type {number[]} */ ([]));
  let exploreBootstrapStartedAt = 0;
  let exploreBootstrapRequestedAt = 0;
  /** @type {ReturnType<typeof createHazardEngine> | null} */
  let hazardEngine = null;
  let sentinelResult = $state(createUnavailableHazardResult());
  let sentinelDemoActive = $state(false);
  let sentinelDemoStartedAt = 0;
  let lastHazardZoneAt = 0;
  let hazardZones = $state(/** @type {Array<{x:number,y:number,score:number,state:string,simulated:boolean}>} */ ([]));
  let cloudSyncState = $state({
    online: true, status: 'idle', pending: 0,
    lastSyncedAt: null, lastError: ''
  });
  let cloudSyncUserId = null;
  let cloudGatewaySession = null;
  let lastCloudMapAt = 0;
  let lastSentinelRecordState = 'NORMAL';
  let dashboardSync = createDashboardSync({
    client: supabase,
    onState: (next) => { cloudSyncState = next; }
  });

  // --- Console Specific State ---
  /** @type {'MANUAL' | 'AUTO_EXPLORE'} */
  let controlMode = $state('MANUAL');
  let isRecordingMap = $state(false);
  let isAutoPaused = $state(false);
  let roverPose = $state(/** @type {{x:number,y:number,headingDeg:number}} */ ({
    x: 0, y: 0, headingDeg: roverCalibration.scanner.forwardAngleDeg
  }));
  let speedPercent = $state(55);
  /** @type {number | null} */
  let activeDrivePointerId = null;
  /** @type {'pointer'|'keyboard'|null} */
  let manualInputSource = null;

  // Real mode starts with no fixtures. Demo fixtures enter only through launchDemo().
  let telemetry = $state(/** @type {import('$lib/state/telemetry.js').TelemetryState} */ (createUnknownTelemetry()));
  let scanPoints = $state(/** @type {Array<{angle_deg: number, distance_mm: number | null, valid: boolean, seq?: number, scan_id?: number, range_status?: number, confidence_pct?: number, timestamp_ms?: number}>} */ ([]));
  let historyBuffers = $state(/** @type {import('$lib/state/telemetry.js').TelemetryHistory} */ (createEmptyHistory()));
  let roverLinkFresh = $derived(
    telemetry.source === 'LIVE' && heartbeatAgeMs !== null &&
    heartbeatAgeMs <= TELEMETRY_STALE_MS
  );
  const COMMAND_ARMED_FLAG = 1 << 11;
  let roverCommandArmed = $derived(
    missionMode === 'demo' ||
      (typeof telemetry.statusFlags === 'number' &&
       (telemetry.statusFlags & COMMAND_ARMED_FLAG) !== 0)
  );
  let hardwareControlReady = $derived(
    missionMode === 'demo'
      ? isConnected && !isEstop
      : isConnected && gatewayRadioReady && gatewayArmed &&
        roverCommandArmed && roverLinkFresh && !isEstop
  );
  let manualForwardAllowed = $derived(
    hardwareControlReady && telemetry.frontValid === true &&
      telemetry.frontFresh === true && telemetry.frontBlocked === false
  );
  let manualControlStatus = $derived.by(() => {
    if (missionMode === 'demo') return 'Demo controls are available.';
    if (!isConnected || transportMode !== 'SERIAL')
      return 'Connect the gateway to enable controls.';
    if (!gatewayRadioReady) return 'Gateway radio is unavailable.';
    if (isEstop) return 'Remote stop is latched. Clear it, then arm controls.';
    if (isArmPending || (gatewayArmed && !roverCommandArmed))
      return 'Waiting for the rover to confirm the armed command session…';
    if (!gatewayArmed) return 'Arm controls to enable WASD and the direction pad.';
    if (!roverLinkFresh) return 'Controls held: rover telemetry is stale.';
    if (!manualForwardAllowed)
      return 'Forward held by front safety. Reverse and pivot remain available.';
    return 'Manual control ready.';
  });
  let exploreControlReady = $derived(
    missionMode === 'demo'
      ? hardwareControlReady
      : hardwareControlReady && estimatedMapState.calibration !== null &&
        estimatedMapState.pose.known && telemetry.frontValid === true &&
        telemetry.frontFresh === true
  );
  let missionWaypoint = $derived(missionController.goal ? {
    x_m: (missionController.goal.x + 0.5) *
      (realMapEvidence.cellSizeM || 0.05),
    y_m: (missionController.goal.y + 0.5) *
      (realMapEvidence.cellSizeM || 0.05)
  } : null);
  let tofStatus = $derived(tofDisplayState(telemetry, scanPoints));
  let distanceFromDeploymentM = $derived(
    missionMode === 'demo'
      ? Math.hypot(roverPose.x, roverPose.y) /
        roverCalibration.map.pixelsPerMeter
      : estimatedMapState.pose.known
        ? Math.hypot(
            estimatedMapState.pose.x_m - roverCalibration.pose.startXM,
            estimatedMapState.pose.y_m - roverCalibration.pose.startYM)
        : null
  );
  let distanceFromDeploymentNote = $derived(
    missionMode === 'demo' ? 'Simulated deployment radius'
      : estimatedMapState.pose.known ? 'Estimated from session origin'
        : 'Calibrated pose required'
  );
  let cloudSyncLabel = $derived.by(() => {
    if (!cloudSyncState.online) {
      return cloudSyncState.pending
        ? `Offline · ${cloudSyncState.pending} queued` : 'Offline · saved locally';
    }
    if (cloudSyncState.status === 'syncing')
      return `Syncing${cloudSyncState.pending ? ` · ${cloudSyncState.pending}` : ''}`;
    if (cloudSyncState.status === 'error')
      return `Local only · ${cloudSyncState.pending} queued`;
    return cloudSyncState.pending
      ? `${cloudSyncState.pending} queued` : 'Cloud synced';
  });

  // Modal State
  let selectedSensorKey = $state(/** @type {string | null} */ (null));
  let isModalOpen = $state(false);

  // Drive Controls
  let keyState = $state({ w: false, a: false, s: false, d: false, space: false });
  let currentMotorL = $state(0);
  let currentMotorR = $state(0);

  function poseForStorage() {
    return estimatedMapState.pose.known ? {
      x_m: estimatedMapState.pose.x_m,
      y_m: estimatedMapState.pose.y_m,
      heading_rad: estimatedMapState.pose.heading_rad,
      confidence: estimatedMapState.pose.confidence
    } : {};
  }

  function cacheHardwareSnapshot() {
    if (missionMode !== 'hardware' || !cloudSyncUserId) return;
    void dashboardSync.cacheSnapshot({
      saved_at: new Date().toISOString(),
      telemetry,
      scan_points: scanPoints.slice(-64),
      history: historyBuffers,
      rover_pose: roverPose,
      hazard_zones: hazardZones.slice(-8),
      logs: realLogs.slice(0, 100),
      map_summary: {
        pose: poseForStorage(),
        trajectory: (realMapEvidence.trajectory || []).slice(-200),
        occupied_count: realMapEvidence.occupied?.length || 0,
        frontier_count: realMapEvidence.frontiers?.length || 0
      }
    });
  }

  async function restoreHardwareSnapshot() {
    if (!cloudSyncUserId || missionMode !== 'hardware' || isConnected) return;
    const cached = await dashboardSync.restoreSnapshot();
    const value = cached?.value;
    if (!value?.telemetry) return;
    telemetry = {
      ...createUnknownTelemetry(), ...value.telemetry,
      source: 'STALE', alertState: 'UNKNOWN'
    };
    scanPoints = Array.isArray(value.scan_points) ? value.scan_points : [];
    historyBuffers = value.history || createEmptyHistory();
    roverPose = value.rover_pose || roverPose;
    hazardZones = Array.isArray(value.hazard_zones) ? value.hazard_zones : [];
    realLogs = Array.isArray(value.logs) ? value.logs : [];
  }

  async function initializeCloudSync(userId) {
    if (!userId || cloudSyncUserId === userId) return;
    dashboardSync.dispose();
    dashboardSync = createDashboardSync({
      client: supabase,
      onState: (next) => { cloudSyncState = next; }
    });
    cloudSyncUserId = userId;
    await dashboardSync.init(userId);
    await restoreHardwareSnapshot();
  }

  async function ensureCloudSession(packet) {
    if (missionMode !== 'hardware' || !cloudSyncUserId ||
        !Number.isInteger(packet.session)) return;
    if (cloudGatewaySession === packet.session && dashboardSync.getSessionId()) return;
    if (dashboardSync.getSessionId()) {
      await dashboardSync.closeSession('interrupted', poseForStorage());
    }
    cloudGatewaySession = packet.session;
    await dashboardSync.beginSession({
      roverId: 'DT-ALPHA-01',
      startPose: poseForStorage(),
      metadata: {
        protocol: packet.protocol,
        gateway_session: packet.session,
        radio_ready: packet.radio_ready === true
      }
    });
  }

  function resetInteractionState() {
    activeNavView = 'console';
    controlMode = 'MANUAL';
    isRecordingMap = false;
    isAutoPaused = false;
    isEstop = false;
    isEstopPending = false;
    currentMotorL = 0;
    currentMotorR = 0;
    roverPose = { x: 0, y: 0,
      headingDeg: roverCalibration.scanner.forwardAngleDeg };
    heartbeatAgeMs = null;
    gatewaySession = null;
    gatewayRadioReady = false;
    gatewayArmed = false;
    commandSequence = 0;
    lastTelemetryReceivedAt = 0;
    realLogs = [];
    diagnosticResults = [];
    isArmPending = false;
    estimatedMapState = createEstimatedMapState();
    missionController = createMissionControllerState();
    exploreBootstrapScanIds = [];
    exploreBootstrapStartedAt = 0;
    exploreBootstrapRequestedAt = 0;
    sentinelDemoActive = false;
    sentinelDemoStartedAt = 0;
    lastHazardZoneAt = 0;
    hazardZones = [];
    cloudGatewaySession = null;
    lastCloudMapAt = 0;
    lastSentinelRecordState = 'NORMAL';
    sentinelResult = hazardEngine?.reset() || createUnavailableHazardResult();
    realMapEvidence = renderableEvidence(estimatedMapState);
    activeDrivePointerId = null;
    manualInputSource = null;
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
    if (auth.user?.id) {
      void initializeCloudSync(auth.user.id).then(restoreHardwareSnapshot);
    }
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

  $effect(() => {
    if (auth.user?.id) void initializeCloudSync(auth.user.id);
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
    if (missionMode === 'hardware') {
      void dashboardSync.record('event', { ...log, code: packet.code });
      cacheHardwareSnapshot();
    }
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
      void ensureCloudSession(packet);
      gatewayRadioReady = packet.radio_ready === true;
      gatewayArmed = packet.armed === true;
      if (!gatewayArmed) isArmPending = false;
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
      if (gatewayArmed && roverCommandArmed) isArmPending = false;
      estimatedMapState = updateEstimatedPose(estimatedMapState, packet);
      realMapEvidence = renderableEvidence(estimatedMapState);
      if (estimatedMapState.pose.known) {
        roverPose = {
          x: estimatedMapState.pose.x_m * roverCalibration.map.pixelsPerMeter,
          y: estimatedMapState.pose.y_m * roverCalibration.map.pixelsPerMeter,
          headingDeg: estimatedMapState.pose.heading_rad * 180 / Math.PI +
            roverCalibration.scanner.forwardAngleDeg
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
      void dashboardSync.record('telemetry', {
        ...telemetry,
        pose: poseForStorage(),
        gateway_session: gatewaySession,
        control_mode: controlMode
      }, { capturedAt: telemetry.timestamp, throttleMs: 1_000 });
      const cloudNow = Date.now();
      if (cloudNow - lastCloudMapAt >= 10_000) {
        lastCloudMapAt = cloudNow;
        void dashboardSync.record('map_snapshot', {
          pose: poseForStorage(),
          trajectory: (realMapEvidence.trajectory || []).slice(-200),
          occupied_count: realMapEvidence.occupied?.length || 0,
          inflated_count: realMapEvidence.inflated?.length || 0,
          frontier_count: realMapEvidence.frontiers?.length || 0,
          hazard_zones: hazardZones.slice(-8)
        });
      }
      cacheHardwareSnapshot();
    } else if (packet.type === 'scan') {
      scanPoints = upsertScanPoint(scanPoints, packet);
      if (missionController.mode === 'EXPLORE' &&
          missionController.status === 'SCANNING' &&
          Number.isInteger(packet.scan_id) &&
          !exploreBootstrapScanIds.includes(packet.scan_id)) {
        exploreBootstrapScanIds = [...exploreBootstrapScanIds, packet.scan_id];
      }
      estimatedMapState = addScanEvidence(
        estimatedMapState, [packet],
        Number.isFinite(packet.timestamp_ms) ? packet.timestamp_ms : performance.now()
      );
      realMapEvidence = renderableEvidence(estimatedMapState);
      void dashboardSync.record('scan', {
        angle_deg: packet.angle_deg,
        distance_mm: packet.distance_mm,
        valid: packet.valid === true,
        scan_id: packet.scan_id,
        range_status: packet.range_status,
        confidence_pct: packet.confidence_pct,
        pose: poseForStorage()
      });
      cacheHardwareSnapshot();
    } else if (packet.type === 'event') {
      appendRealLog(packet);
      if (packet.code === 'ARMED') {
        gatewayArmed = true;
        isArmPending = true;
      } else if (packet.code === 'GATEWAY_STOP' ||
                 packet.code === 'ARM_REJECTED') {
        gatewayArmed = false;
        isArmPending = false;
      }
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
    const endingPose = poseForStorage();
    cacheHardwareSnapshot();
    if (dashboardSync.getSessionId()) {
      void dashboardSync.closeSession('interrupted', endingPose);
    }
    cloudGatewaySession = null;
    stopDriveRefresh();
    isConnected = false;
    transportMode = 'DISCONNECTED';
    gatewaySession = null;
    gatewayRadioReady = false;
    gatewayArmed = false;
    isArmPending = false;
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
      if (dashboardSync.getSessionId()) {
        await dashboardSync.closeSession('closed', poseForStorage());
        cloudGatewaySession = null;
      }
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
    const gated = gateManualDrive(left, right, {
      controlReady: hardwareControlReady,
      manualMode: controlMode === 'MANUAL',
      forwardAllowed: manualForwardAllowed
    });
    currentMotorL = gated.left;
    currentMotorR = gated.right;
    if (gated.reason === 'CONTROL_LOCKED') return;
    // Mirror the rover's fail-closed directional gate in the console. The
    // firmware remains authoritative; this avoids presenting or repeatedly
    // transmitting a forward request that is already known to be blocked.
    if (gated.reason === 'FORWARD_HELD') {
      if (missionMode === 'hardware')
        await sendSessionCommand('drive', {
          left: 0, right: 0, ttl_ms: COMMAND_TTL_MS
        });
      return;
    }
    if (missionMode === 'hardware')
      await sendSessionCommand('drive', {
        left: gated.left, right: gated.right, ttl_ms: COMMAND_TTL_MS
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

  /** @param {PointerEvent} event @param {number} left @param {number} right */
  function beginPointerDrive(event, left, right) {
    if (!hardwareControlReady || controlMode !== 'MANUAL' ||
        activeDrivePointerId !== null) return;
    if (requestsForward(left, right) && !manualForwardAllowed) return;
    activeDrivePointerId = event.pointerId;
    manualInputSource = 'pointer';
    const target = /** @type {HTMLElement | null} */ (event.currentTarget);
    try { target?.setPointerCapture(event.pointerId); } catch {}
    startHoldDrive(left, right);
  }

  /** @param {PointerEvent} event */
  function endPointerDrive(event) {
    if (activeDrivePointerId !== event.pointerId) return;
    activeDrivePointerId = null;
    manualInputSource = null;
    const target = /** @type {HTMLElement | null} */ (event.currentTarget);
    try {
      if (target?.hasPointerCapture(event.pointerId))
        target.releasePointerCapture(event.pointerId);
    } catch {}
    void releaseManualDrive();
  }

  async function stopDriveMotion() {
    stopDriveRefresh();
    currentMotorL = 0;
    currentMotorR = 0;
    if (missionMode === 'hardware' && transportMode === 'SERIAL')
      await writeGatewayRecord({ type: 'stop' });
  }

  // Manual release is neutral, not disarm. The armed session and watchdogs
  // stay alive, while the rover receives an explicit zero command.
  async function releaseManualDrive() {
    stopDriveRefresh();
    activeDrivePointerId = null;
    manualInputSource = null;
    currentMotorL = 0;
    currentMotorR = 0;
    if (missionMode === 'hardware' && transportMode === 'SERIAL' &&
        gatewayArmed && gatewaySession) {
      await sendSessionCommand('drive', { left: 0, right: 0,
        ttl_ms: COMMAND_TTL_MS });
    }
  }

  /** @param {'EXPLORE'|'NAVIGATE'|'RETURN_HOME'} mode @param {{x:number,y:number}|null} goal */
  async function beginMission(mode, goal = null) {
    if (missionMode !== 'hardware' || !exploreControlReady) return false;
    const next = startMission(missionController, mode, estimatedMapState,
      goal, performance.now());
    missionController = next;
    if (mode === 'EXPLORE' && next.reason === 'NO_REACHABLE_FRONTIER') {
      await beginExploreObservation('BUILDING_INITIAL_MAP');
      return true;
    }
    if (next.status !== 'RUNNING') {
      appendRealLog({ source: 'dashboard', severity: 'warning',
        code: next.reason, message: `Mission rejected: ${next.reason}.` });
      return false;
    }
    controlMode = 'AUTO_EXPLORE';
    isAutoPaused = false;
    await sendSessionCommand('drive', { left: 0, right: 0,
      ttl_ms: COMMAND_TTL_MS });
    return true;
  }

  async function beginExploreObservation(reason = 'UPDATING_FRONTIER_MAP') {
    if (missionMode !== 'hardware' || !exploreControlReady) return false;
    stopDriveRefresh();
    currentMotorL = 0;
    currentMotorR = 0;
    controlMode = 'AUTO_EXPLORE';
    isAutoPaused = false;
    exploreBootstrapScanIds = [];
    exploreBootstrapStartedAt = performance.now();
    exploreBootstrapRequestedAt = exploreBootstrapStartedAt;
    missionController = {
      ...missionController, mode: 'EXPLORE', status: 'SCANNING', reason,
      goal: null, path: [], pathIndex: 0, targetKey: null
    };
    await sendSessionCommand('drive', { left: 0, right: 0,
      ttl_ms: COMMAND_TTL_MS });
    const sent = await runDiagnostic(DIAGNOSTIC_ACTION.START_SCAN, 0);
    if (!sent) {
      await haltMission('STATIONARY_SCAN_NOT_SENT');
      return false;
    }
    return true;
  }

  /** @param {{x:number,y:number}} goal */
  function setNavigationWaypoint(goal) {
    if (missionMode === 'demo' || controlMode !== 'AUTO_EXPLORE' ||
        missionController.mode !== 'NAVIGATE') return;
    void beginMission('NAVIGATE', goal);
  }

  /** @param {'MANUAL'|'EXPLORE'|'NAVIGATE'|'RETURN_HOME'} mode */
  async function selectAutonomyMode(mode) {
    if (mode === 'MANUAL') {
      await selectControlMode('MANUAL');
      return;
    }
    if (!exploreControlReady) return;
    if (missionMode === 'demo') {
      controlMode = 'AUTO_EXPLORE';
      isAutoPaused = false;
      missionController = {
        ...createMissionControllerState(), mode,
        status: mode === 'NAVIGATE' ? 'AWAITING_GOAL' : 'RUNNING',
        reason: mode === 'NAVIGATE' ? 'CLICK_KNOWN_FREE_CELL' : 'SIMULATED_DEMO'
      };
      return;
    }
    stopDriveRefresh();
    currentMotorL = 0;
    currentMotorR = 0;
    await sendSessionCommand('drive', { left: 0, right: 0,
      ttl_ms: COMMAND_TTL_MS });
    controlMode = 'AUTO_EXPLORE';
    isAutoPaused = false;
    if (mode === 'NAVIGATE') {
      missionController = {
        ...stopMission(missionController, 'CLICK_KNOWN_FREE_CELL'),
        mode: 'NAVIGATE', status: 'AWAITING_GOAL',
        reason: 'CLICK_KNOWN_FREE_CELL'
      };
      return;
    }
    await beginMission(mode);
  }

  async function haltMission(reason = 'OPERATOR_STOP') {
    missionController = stopMission(missionController, reason);
    controlMode = 'MANUAL';
    isAutoPaused = false;
    await releaseManualDrive();
    if (missionMode === 'hardware' && gatewayArmed)
      await sendSessionCommand('manual');
  }

  async function armHardware() {
    if (missionMode !== 'hardware' || !isConnected ||
        !gatewayRadioReady || !gatewaySession || !roverLinkFresh ||
        isEstop || isArmPending) return;
    isArmPending = true;
    await sendSessionCommand('heartbeat');
    const sent = await sendSessionCommand('arm');
    if (!sent) isArmPending = false;
    setTimeout(() => {
      if (!gatewayArmed || !roverCommandArmed) isArmPending = false;
    }, 1800);
  }

  async function engageRemoteStop() {
    if (!isConnected || isEstopPending) return;
    isEstopPending = true;
    isEstop = true;
    if (missionMode === 'hardware') await safetyDisarm();
    else await stopDriveMotion();
    setTimeout(() => { isEstopPending = false; }, 150);
  }

  function clearRemoteStop() {
    // Clearing the UI latch never rearms motion. Arming remains a separate,
    // deliberate action with rover-side confirmation.
    isEstop = false;
    isEstopPending = false;
  }

  function updateKeyboardDrive() {
    if (!keyState.w && !keyState.a && !keyState.s && !keyState.d) {
      void releaseManualDrive();
      return;
    }
    manualInputSource = 'keyboard';
    const vector = keyboardDriveVector(
      keyState, speedPercent, manualForwardAllowed
    );
    startHoldDrive(vector.left, vector.right);
  }

  /** @param {KeyboardEvent} e */
  function handleKeydown(e) {
    if (e.target instanceof HTMLInputElement || e.target instanceof HTMLTextAreaElement) return;
    const movementKey = ['KeyW', 'KeyA', 'KeyS', 'KeyD', 'ArrowUp',
      'ArrowDown', 'ArrowLeft', 'ArrowRight'].includes(e.code);
    if (movementKey && (!hardwareControlReady || controlMode !== 'MANUAL')) {
      e.preventDefault();
      return;
    }
    if (movementKey && manualInputSource === 'pointer') {
      e.preventDefault();
      return;
    }
    if (e.code === 'KeyW' || e.code === 'ArrowUp') {
      e.preventDefault();
      if (!manualForwardAllowed) return;
      if (!keyState.w) { keyState.w = true; updateKeyboardDrive(); }
    } else if (e.code === 'KeyS' || e.code === 'ArrowDown') {
      e.preventDefault(); if (!keyState.s) { keyState.s = true; updateKeyboardDrive(); }
    } else if (e.code === 'KeyA' || e.code === 'ArrowLeft') {
      e.preventDefault(); if (!keyState.a) { keyState.a = true; updateKeyboardDrive(); }
    } else if (e.code === 'KeyD' || e.code === 'ArrowRight') {
      e.preventDefault(); if (!keyState.d) { keyState.d = true; updateKeyboardDrive(); }
    } else if (e.code === 'Space') {
      e.preventDefault();
      if (!keyState.space) { keyState.space = true; void engageRemoteStop(); }
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
    if (keyState.w || keyState.a || keyState.s || keyState.d)
      updateKeyboardDrive();
    else void releaseManualDrive();
  }

  function handleWindowBlur() {
    keyState = { w: false, a: false, s: false, d: false, space: false };
    activeDrivePointerId = null;
    manualInputSource = null;
    if (missionMode === 'hardware') void safetyDisarm();
    else void stopDriveMotion();
  }

  function handleVisibilityChange() {
    if (document.visibilityState === 'hidden') handleWindowBlur();
  }

  /** @param {'MANUAL' | 'AUTO_EXPLORE'} mode */
  async function selectControlMode(mode) {
    if (mode === 'AUTO_EXPLORE' && missionMode === 'hardware') {
      // Preserve the already explicit armed session while transitioning from
      // manual to the TTL-refreshed planner. A raw STOP would disarm it.
      stopDriveRefresh();
      currentMotorL = 0;
      currentMotorR = 0;
      await sendSessionCommand('drive', { left: 0, right: 0,
        ttl_ms: COMMAND_TTL_MS });
      await beginMission('EXPLORE');
      return;
    }
    if (controlMode !== mode) await releaseManualDrive();
    if (mode === 'MANUAL' && missionMode === 'hardware')
      missionController = stopMission(missionController);
    controlMode = mode;
    if (mode === 'AUTO_EXPLORE') isAutoPaused = false;
    if (missionMode !== 'hardware' || !gatewayArmed) return;
    await sendSessionCommand(mode === 'MANUAL' ? 'manual' : 'auto');
  }

  async function toggleAutoPause() {
    isAutoPaused = !isAutoPaused;
    if (missionMode === 'hardware' && gatewayArmed && isAutoPaused)
      await sendSessionCommand('drive', { left: 0, right: 0,
        ttl_ms: COMMAND_TTL_MS });
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
          title: 'Fail-closed Fused Front Clearance',
          sensorIc: 'HC-SR04 + centered VL53L0X',
          pinout: 'Ultrasonic GPIO 19/18 · top ToF I²C GPIO 21/22',
          sampling: telemetry.source === 'SIMULATED' ? 'Simulated' : 'Both channels required within 300 ms',
          currentVal: `${telemetry.frontDistanceCm ?? '—'} cm`,
          unit: 'cm',
          history: historyBuffers.ultrasonic,
          status: telemetry.frontDistanceCm === null ? 'unknown' : 'normal',
          safetyNote: 'The nearer valid reading is used. If either ultrasonic or centered top ToF is invalid or stale, forward motion is blocked.',
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

  function toggleSentinelDemo() {
    if (missionMode !== 'demo') return;
    sentinelDemoActive = !sentinelDemoActive;
    sentinelDemoStartedAt = sentinelDemoActive ? Date.now() : 0;
  }

  /** @param {number} now */
  function updateSentinel(now) {
    if (!hazardEngine || missionMode === null) return;
    sentinelResult = hazardEngine.observe(telemetry, now, { heartbeatAgeMs });
    if (missionMode === 'hardware' && sentinelResult.state !== lastSentinelRecordState) {
      lastSentinelRecordState = sentinelResult.state;
      void dashboardSync.record('sentinel', {
        score: sentinelResult.score,
        state: sentinelResult.state,
        confidence: sentinelResult.confidence,
        trend: sentinelResult.trend,
        reasons: sentinelResult.reasons,
        pose: poseForStorage()
      });
    }
    if (sentinelResult.score < 55 ||
        now - lastHazardZoneAt < 8000 ||
        !(missionMode === 'demo' || estimatedMapState.pose.known)) return;

    const previous = hazardZones[hazardZones.length - 1];
    if (previous && Math.hypot(previous.x - roverPose.x,
        previous.y - roverPose.y) < 42) return;
    hazardZones = [...hazardZones, {
      x: roverPose.x,
      y: roverPose.y,
      score: sentinelResult.score,
      state: sentinelResult.state,
      simulated: missionMode === 'demo'
    }].slice(-8);
    lastHazardZoneAt = now;
  }


  onMount(() => {
    window.addEventListener('keydown', handleKeydown);
    window.addEventListener('keyup', handleKeyup);
    window.addEventListener('blur', handleWindowBlur);
    document.addEventListener('visibilitychange', handleVisibilityChange);
    window.addEventListener('pagehide', handleWindowBlur);
    window.addEventListener('pagehide', cacheHardwareSnapshot);

    let disposed = false;
    void fetch('/models/hazard-model.json')
      .then((response) => {
        if (!response.ok) throw new Error(`Model HTTP ${response.status}`);
        return response.json();
      })
      .then((model) => {
        if (disposed) return;
        hazardEngine = createHazardEngine(model);
        updateSentinel(Date.now());
      })
      .catch(() => {
        if (!disposed) sentinelResult = createUnavailableHazardResult();
      });

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
    }, 250);

    const sentinelInterval = setInterval(() => {
      if (!document.hidden) updateSentinel(Date.now());
    }, 1000);

    const missionInterval = setInterval(async () => {
      if (missionTickBusy || missionMode !== 'hardware' ||
          controlMode !== 'AUTO_EXPLORE' || isAutoPaused) return;
      missionTickBusy = true;
      try {
        if (missionController.status === 'SCANNING') {
          const now = performance.now();
          const scanComplete = telemetry.driveState === 'MANUAL';
          if (scanComplete && exploreBootstrapScanIds.length >= 2) {
            const next = startMission(missionController, 'EXPLORE',
              estimatedMapState, null, now);
            missionController = next;
            if (next.status !== 'RUNNING')
              await haltMission(next.reason);
          } else if (scanComplete && exploreBootstrapScanIds.length < 2 &&
              now - exploreBootstrapRequestedAt > 900) {
            exploreBootstrapRequestedAt = now;
            await runDiagnostic(DIAGNOSTIC_ACTION.START_SCAN, 0);
          } else if (now - exploreBootstrapStartedAt > 25000) {
            await haltMission('STATIONARY_SCAN_TIMEOUT');
          }
          return;
        }
        if (missionController.status !== 'RUNNING') return;
        const tick = stepMission(missionController, estimatedMapState, {
          controlReady: hardwareControlReady,
          frontValid: telemetry.frontValid === true,
          frontFresh: telemetry.frontFresh === true,
          frontBlocked: telemetry.frontBlocked !== false,
          speedPercent: Math.min(speedPercent, 45)
        }, performance.now());
        missionController = tick.state;
        currentMotorL = tick.drive.left;
        currentMotorR = tick.drive.right;
        if (tick.state.status === 'RUNNING') {
          await sendSessionCommand('drive', { ...tick.drive,
            ttl_ms: COMMAND_TTL_MS });
        } else if (missionController.mode === 'EXPLORE' &&
            (tick.state.status === 'COMPLETE' ||
             tick.state.reason === 'FRONT_CLEARANCE_BLOCKED' ||
             tick.state.reason === 'ROUTE_BLOCKED')) {
          await beginExploreObservation(
            tick.state.status === 'COMPLETE' ? 'SCANNING_AT_FRONTIER' :
              'SCANNING_BLOCKED_ROUTE');
        } else {
          appendRealLog({ source: 'dashboard', severity:
            tick.state.status === 'COMPLETE' ? 'info' : 'warning',
            code: tick.state.reason,
            message: `Mission stopped: ${tick.state.reason}.` });
          await haltMission(tick.state.reason);
        }
      } finally { missionTickBusy = false; }
    }, DRIVE_REFRESH_MS);

    const simInterval = setInterval(() => {
      if (missionMode === 'demo' && transportMode === 'SIMULATION' && isConnected) {
        const now = Date.now();
        const scenarioProgress = sentinelDemoActive
          ? Math.min(1, Math.max(0, (now - sentinelDemoStartedAt) / 18000))
          : 0;
        telemetry.source = 'SIMULATED';
        heartbeatAgeMs = 45 + Math.floor(Math.random() * 20);
        telemetry.temperature = +(28.2 + Math.sin(now / 4000) * 0.8 +
          scenarioProgress * 4.8).toFixed(1);
        telemetry.humidity = +(69.7 + Math.cos(now / 5000) * 1.5 +
          scenarioProgress * 8).toFixed(1);
        telemetry.gasRaw = Math.max(600, Math.round(
          862 + Math.sin(now / 3000) * 40 + scenarioProgress * 980));
        telemetry.gasState = 'SIMULATED';
        telemetry.waterRaw = Math.round(350 + Math.cos(now / 7000) * 20 +
          Math.max(0, scenarioProgress - 0.25) * 520);
        telemetry.waterState = 'SIMULATED';
        telemetry.pitchDeg = +(-3.5 + Math.sin(now / 2500) * 1.5 +
          scenarioProgress * 9).toFixed(1);
        telemetry.rollDeg = +(2.6 + Math.cos(now / 2800) * 1.2 +
          scenarioProgress * 6).toFixed(1);
        telemetry.frontDistanceCm = +(119 + Math.sin(now / 2000) * 15 -
          scenarioProgress * 68).toFixed(0);
        telemetry.ultrasonicDistanceCm = telemetry.frontDistanceCm + 4;
        telemetry.tofMm = (telemetry.frontDistanceCm + 7) * 10;
        telemetry.frontValid = true;
        telemetry.frontFresh = true;
        telemetry.frontBlocked = telemetry.frontDistanceCm < 25;
        historyBuffers.gas = [...historyBuffers.gas.slice(-29), telemetry.gasRaw];
        historyBuffers.temperature = [...historyBuffers.temperature.slice(-29), telemetry.temperature];
        historyBuffers.water = [...historyBuffers.water.slice(-29), telemetry.waterRaw];
        historyBuffers.ultrasonic = [...historyBuffers.ultrasonic.slice(-29), telemetry.frontDistanceCm];
        historyBuffers.pitch = [...historyBuffers.pitch.slice(-29), telemetry.pitchDeg];
        historyBuffers.roll = [...historyBuffers.roll.slice(-29), telemetry.rollDeg];

        if (!isEstop) {
          if (controlMode === 'AUTO_EXPLORE' && !isAutoPaused) {
            const t = Date.now() / 1500;
            roverPose.x = +(33.2 + Math.sin(t * 0.4) * 8).toFixed(1);
            roverPose.y = +(-13.6 + Math.cos(t * 0.3) * 6).toFixed(1);
            roverPose.headingDeg = +(
              roverCalibration.scanner.forwardAngleDeg +
              Math.sin(t * 0.5) * 30).toFixed(0);
            const leftTicks = (telemetry.encoderL ?? 0) + 1;
            const rightTicks = (telemetry.encoderR ?? 0) + 1;
            telemetry.encoderL = leftTicks;
            telemetry.encoderR = rightTicks;
            telemetry.estimatedDistanceMeters = +(4.52 + leftTicks * 0.005).toFixed(2);
          } else if (currentMotorL !== 0 || currentMotorR !== 0) {
            const speed = (currentMotorL + currentMotorR) / 100;
            const turn = (currentMotorR - currentMotorL) / 100;
            roverPose.headingDeg += turn * 3;
            const rad = (roverPose.headingDeg -
              roverCalibration.scanner.forwardAngleDeg) * (Math.PI / 180);
            roverPose.x += Math.cos(rad) * speed * 2;
            roverPose.y += Math.sin(rad) * speed * 2;
            const leftTicks = (telemetry.encoderL ?? 0) + 2;
            telemetry.encoderL = leftTicks;
            telemetry.encoderR = (telemetry.encoderR ?? 0) + 2;
            telemetry.estimatedDistanceMeters = +(leftTicks * (Math.PI * 0.065 / 20)).toFixed(2);
          }
        }
      }
    }, 500);

    return () => {
      disposed = true;
      window.removeEventListener('keydown', handleKeydown);
      window.removeEventListener('keyup', handleKeyup);
      window.removeEventListener('blur', handleWindowBlur);
      document.removeEventListener('visibilitychange', handleVisibilityChange);
      window.removeEventListener('pagehide', handleWindowBlur);
      window.removeEventListener('pagehide', cacheHardwareSnapshot);
      clearInterval(simInterval);
      clearInterval(heartbeatInterval);
      clearInterval(freshnessInterval);
      clearInterval(sentinelInterval);
      clearInterval(missionInterval);
      stopDriveRefresh();
      cacheHardwareSnapshot();
      if (dashboardSync.getSessionId())
        void dashboardSync.closeSession('interrupted', poseForStorage());
      dashboardSync.dispose();
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
          {missionMode === 'demo' ? 'DEMO · SIMULATED' : transportMode !== 'SERIAL' ? 'REAL · UNKNOWN' : !gatewaySession ? 'REAL · USB / WAITING' : !gatewayRadioReady ? 'REAL · RADIO DISABLED' : isArmPending || (gatewayArmed && !roverCommandArmed) ? 'REAL · ARMING' : gatewayArmed ? 'REAL · ARMED' : 'REAL · DISARMED'}
        </span>
        {#if missionMode === 'hardware' && auth.user}
          <span
            class="hidden xl:inline-flex items-center gap-1.5 rounded-full border px-2.5 py-1 text-xs font-semibold {cloudSyncState.status === 'error' ? 'border-[var(--ui-color-warning)] bg-[var(--ui-color-warning-container)] text-[var(--ui-color-on-warning-container)]' : cloudSyncState.online ? 'border-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface-container-low)] text-[var(--md-sys-color-on-surface-variant)]' : 'border-[var(--ui-color-warning)] bg-[var(--ui-color-warning-container)] text-[var(--ui-color-on-warning-container)]'}"
            title={cloudSyncState.lastError || 'Real rover records are saved locally first, then synced to your Supabase account.'}
            aria-live="polite"
          >
            <span class="h-2 w-2 rounded-full {cloudSyncState.status === 'error' || !cloudSyncState.online ? 'bg-[var(--ui-color-warning)]' : cloudSyncState.status === 'syncing' ? 'bg-[var(--md-sys-color-primary)] animate-pulse' : 'bg-[var(--ui-color-success)]'}"></span>
            {cloudSyncLabel}
          </span>
        {/if}
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
            <button type="button" class="ui-button ui-button--tonal !h-10 !px-4 text-sm" onclick={gatewayArmed ? safetyDisarm : armHardware} disabled={!gatewayRadioReady || isArmPending || isEstop || (!gatewayArmed && !roverLinkFresh)}>
              {isArmPending ? 'Arming…' : gatewayArmed ? 'Disarm' : 'Arm controls'}
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
          onclick={isEstop ? clearRemoteStop : engageRemoteStop}
          disabled={!isConnected}
        >
          <span class="material-symbols-rounded text-[20px]">
            {isEstop ? 'play_arrow' : 'stop_circle'}
          </span>
          <span>
            {#if !isConnected}Remote stop unavailable
            {:else if isEstopPending}Stopping...
            {:else if isEstop}Clear stop latch
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
        <div class="absolute inset-0 flex flex-col p-4 md:p-5 gap-4 overflow-y-auto overflow-x-hidden lg:overflow-hidden min-h-0" in:m3TopLevelFadeThrough={{ duration: 300 }}>
          
          <!-- TELEMETRY RIBBON -->
          <section class="grid grid-cols-2 lg:grid-cols-6 divide-y lg:divide-y-0 lg:divide-x divide-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface-container-low)] rounded-xl border border-[var(--md-sys-color-outline-variant)] shrink-0 overflow-hidden">
            
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
              <span class="text-sm text-[var(--md-sys-color-on-surface-variant)]">
                Fused nearest · ultrasonic {telemetry.ultrasonicDistanceCm ?? '—'} cm · top ToF {telemetry.tofMm === null ? '—' : (telemetry.tofMm / 10).toFixed(1)} cm
              </span>
            </button>

            <div class="text-left flex flex-col gap-1 px-5 py-4" aria-label="Distance from deployment point">
              <div class="flex items-center gap-1.5 text-sm font-medium text-[var(--md-sys-color-on-surface-variant)]">
                <span class="material-symbols-rounded text-[20px] text-[var(--md-sys-color-primary)]">route</span> Distance from start
              </div>
              <div class="flex items-baseline gap-2">
                <span class="text-3xl font-bold telemetry text-[var(--md-sys-color-on-surface)]">{distanceFromDeploymentM === null ? '—' : distanceFromDeploymentM.toFixed(1)} <span class="text-base font-normal">{distanceFromDeploymentM === null ? '' : 'm'}</span></span>
              </div>
              <span class="text-sm text-[var(--md-sys-color-on-surface-variant)]">{distanceFromDeploymentNote}</span>
            </div>

            <AiRiskPanel
              result={sentinelResult}
              isDemo={missionMode === 'demo'}
              demoActive={sentinelDemoActive}
              onToggleDemo={toggleSentinelDemo}
            />
          </section>

          <!-- CONSOLE PANES (Left, Center, Right) -->
          <div class="flex-none lg:flex-1 flex flex-col lg:flex-row gap-6 min-h-[400px] lg:min-h-0">
            
            <!-- LEFT: DRIVE CONTROLS -->
            <aside class="w-full lg:w-[260px] shrink-0 lg:min-h-0 overflow-hidden" data-testid="drive-panel" use:fitHeight={`${controlMode}:${manualControlStatus}`}>
              <div data-fit-content class="flex flex-col gap-6 origin-top-left">
              <div data-fit-mode-group class="flex flex-col gap-3">
                <h3 class="text-lg font-semibold flex items-center gap-2">
                  <span class="material-symbols-rounded text-[22px] text-[var(--md-sys-color-on-surface-variant)]">gamepad</span> Control mode
                </h3>
                <div data-fit-mode-grid class="grid grid-cols-2 gap-1.5 p-1.5 bg-[var(--md-sys-color-surface-container)] rounded-xl mt-1 w-full">
                  <button type="button" class="min-h-12 px-2 py-2 flex items-center justify-center gap-1.5 text-sm font-semibold transition-colors rounded-lg {controlMode === 'MANUAL' ? 'bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)] shadow-sm' : 'text-[var(--md-sys-color-on-surface-variant)] hover:bg-[var(--md-sys-color-surface-container-highest)]'}" onclick={() => selectAutonomyMode('MANUAL')}>
                    <span class="material-symbols-rounded text-[18px]">sports_esports</span> Manual
                  </button>
                  <button type="button" class="min-h-12 px-2 py-2 flex items-center justify-center gap-1.5 text-sm font-semibold transition-colors rounded-lg disabled:opacity-40 disabled:cursor-not-allowed {controlMode === 'AUTO_EXPLORE' && missionController.mode === 'EXPLORE' ? 'bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)] shadow-sm' : 'text-[var(--md-sys-color-on-surface-variant)] hover:bg-[var(--md-sys-color-surface-container-highest)]'}" onclick={() => selectAutonomyMode('EXPLORE')} disabled={!exploreControlReady} aria-label={exploreControlReady ? 'Start frontier exploration' : 'Explore requires an armed fresh link, pose, ultrasonic, and top ToF'}>
                    <span class="material-symbols-rounded text-[18px]">travel_explore</span> Explore
                  </button>
                  <button type="button" class="min-h-12 px-2 py-2 flex items-center justify-center gap-1.5 text-sm font-semibold transition-colors rounded-lg disabled:opacity-40 disabled:cursor-not-allowed {controlMode === 'AUTO_EXPLORE' && missionController.mode === 'NAVIGATE' ? 'bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)] shadow-sm' : 'text-[var(--md-sys-color-on-surface-variant)] hover:bg-[var(--md-sys-color-surface-container-highest)]'}" onclick={() => selectAutonomyMode('NAVIGATE')} disabled={!exploreControlReady}>
                    <span class="material-symbols-rounded text-[18px]">near_me</span> Navigate
                  </button>
                  <button type="button" class="min-h-12 px-2 py-2 flex items-center justify-center gap-1.5 text-sm font-semibold transition-colors rounded-lg disabled:opacity-40 disabled:cursor-not-allowed {controlMode === 'AUTO_EXPLORE' && missionController.mode === 'RETURN_HOME' ? 'bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)] shadow-sm' : 'text-[var(--md-sys-color-on-surface-variant)] hover:bg-[var(--md-sys-color-surface-container-highest)]'}" onclick={() => selectAutonomyMode('RETURN_HOME')} disabled={!exploreControlReady}>
                    <span class="material-symbols-rounded text-[18px]">home</span> Return
                  </button>
                </div>
              </div>

              {#if controlMode === 'MANUAL'}
                <div data-fit-dpad-section class="flex flex-col items-center">
                  <div data-fit-dpad class="grid grid-cols-3 gap-2 w-52">
                    <div></div>
                    <button type="button" aria-label="Drive forward" class="touch-none h-14 rounded-lg bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center hover:bg-[var(--md-sys-color-primary-container)] hover:text-[var(--md-sys-color-on-primary-container)] active:bg-[var(--md-sys-color-primary)] active:text-[var(--md-sys-color-on-primary)] active:scale-95 transition-all duration-150 disabled:opacity-40 disabled:cursor-not-allowed {keyState.w ? '!bg-[var(--md-sys-color-primary)] !text-[var(--md-sys-color-on-primary)] scale-95' : ''}" disabled={!manualForwardAllowed} onpointerdown={(event) => beginPointerDrive(event, speedPercent, speedPercent)} onpointerup={endPointerDrive} onpointercancel={endPointerDrive} onlostpointercapture={endPointerDrive}><span class="material-symbols-rounded text-[26px]">arrow_upward</span></button>
                    <div></div>
                    <button type="button" aria-label="Pivot left" class="touch-none h-14 rounded-lg bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center hover:bg-[var(--md-sys-color-primary-container)] hover:text-[var(--md-sys-color-on-primary-container)] active:bg-[var(--md-sys-color-primary)] active:text-[var(--md-sys-color-on-primary)] active:scale-95 transition-all duration-150 disabled:opacity-40 disabled:cursor-not-allowed {keyState.a ? '!bg-[var(--md-sys-color-primary)] !text-[var(--md-sys-color-on-primary)] scale-95' : ''}" disabled={!hardwareControlReady} onpointerdown={(event) => beginPointerDrive(event, -speedPercent, speedPercent)} onpointerup={endPointerDrive} onpointercancel={endPointerDrive} onlostpointercapture={endPointerDrive}><span class="material-symbols-rounded text-[26px]">arrow_back</span></button>
                    <button type="button" class="h-14 rounded-lg bg-[var(--md-sys-color-error-container)] text-[var(--md-sys-color-on-error-container)] text-sm font-bold active:scale-95 border border-[var(--md-sys-color-outline-variant)] transition-all opacity-80 hover:opacity-100 {keyState.space ? '!bg-[var(--md-sys-color-error)] !text-[var(--md-sys-color-on-error)] scale-95 !opacity-100' : ''}" onclick={engageRemoteStop}>STOP</button>
                    <button type="button" aria-label="Pivot right" class="touch-none h-14 rounded-lg bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center hover:bg-[var(--md-sys-color-primary-container)] hover:text-[var(--md-sys-color-on-primary-container)] active:bg-[var(--md-sys-color-primary)] active:text-[var(--md-sys-color-on-primary)] active:scale-95 transition-all duration-150 disabled:opacity-40 disabled:cursor-not-allowed {keyState.d ? '!bg-[var(--md-sys-color-primary)] !text-[var(--md-sys-color-on-primary)] scale-95' : ''}" disabled={!hardwareControlReady} onpointerdown={(event) => beginPointerDrive(event, speedPercent, -speedPercent)} onpointerup={endPointerDrive} onpointercancel={endPointerDrive} onlostpointercapture={endPointerDrive}><span class="material-symbols-rounded text-[26px]">arrow_forward</span></button>
                    <div></div>
                    <button type="button" aria-label="Drive reverse" class="touch-none h-14 rounded-lg bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center hover:bg-[var(--md-sys-color-primary-container)] hover:text-[var(--md-sys-color-on-primary-container)] active:bg-[var(--md-sys-color-primary)] active:text-[var(--md-sys-color-on-primary)] active:scale-95 transition-all duration-150 disabled:opacity-40 disabled:cursor-not-allowed {keyState.s ? '!bg-[var(--md-sys-color-primary)] !text-[var(--md-sys-color-on-primary)] scale-95' : ''}" disabled={!hardwareControlReady} onpointerdown={(event) => beginPointerDrive(event, -speedPercent, -speedPercent)} onpointerup={endPointerDrive} onpointercancel={endPointerDrive} onlostpointercapture={endPointerDrive}><span class="material-symbols-rounded text-[26px]">arrow_downward</span></button>
                    <div></div>
                  </div>
                  <div data-fit-status class="mt-3 w-full text-center text-sm leading-snug font-semibold {manualForwardAllowed || !hardwareControlReady ? 'text-[var(--md-sys-color-on-surface-variant)]' : 'text-[var(--ui-color-on-warning-container)]'}">{manualControlStatus}</div>
                  <div data-fit-shortcuts class="mt-3 text-sm font-medium text-[var(--md-sys-color-on-surface-variant)] flex gap-3 telemetry"><span class={!manualForwardAllowed ? 'opacity-40' : ''}>W</span><span>A</span><span>S</span><span>D</span><span>Space = stop</span></div>
                </div>

                <div data-fit-speed class="flex flex-col gap-3">
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

                <div data-fit-diagnostics class="flex flex-col gap-3 pt-5 border-t border-[var(--md-sys-color-outline-variant)] text-sm text-[var(--md-sys-color-on-surface-variant)]">
                  <div class="flex justify-between items-center"><span class="font-medium">Requested output</span><span class="telemetry font-bold text-[var(--md-sys-color-on-surface)]">L: {currentMotorL}% &nbsp; R: {currentMotorR}%</span></div>
                  <div class="flex justify-between items-center"><span class="font-medium">Accepted ticks</span><span class="telemetry font-bold text-[var(--md-sys-color-on-surface)]">{telemetry.encoderL ?? '—'} L &nbsp; {telemetry.encoderR ?? '—'} R</span></div>
                  <div class="flex justify-between items-center"><span class="font-medium">Raw edges</span><span class="telemetry font-bold text-[var(--md-sys-color-on-surface)]">{telemetry.encoderRawL ?? '—'} L &nbsp; {telemetry.encoderRawR ?? '—'} R</span></div>
                  <div class="flex justify-between items-center"><span class="font-medium">Rejected D/S</span><span class="telemetry font-bold text-[var(--md-sys-color-on-surface)]">{telemetry.encoderRejectedDebounceL ?? '—'}/{telemetry.encoderRejectedStateL ?? '—'} L &nbsp; {telemetry.encoderRejectedDebounceR ?? '—'}/{telemetry.encoderRejectedStateR ?? '—'} R</span></div>
                </div>
              {:else}
                <div class="flex flex-col gap-4">
                  <div class="flex justify-between items-center text-sm">
                    <span class="font-semibold text-[var(--md-sys-color-on-surface)]">Navigation logic</span>
                    <span class="px-2.5 py-0.5 rounded font-bold text-[13px] {isAutoPaused || missionController.status === 'SCANNING' || missionController.status === 'AWAITING_GOAL' ? 'bg-[var(--ui-color-warning-container)] text-[var(--ui-color-on-warning-container)]' : missionController.status === 'RUNNING' ? 'bg-[var(--ui-color-success-container)] text-[var(--ui-color-on-success-container)]' : 'bg-[var(--md-sys-color-surface-container-highest)] text-[var(--md-sys-color-on-surface-variant)]'}">{isAutoPaused ? 'PAUSED' : missionController.status}</span>
                  </div>
                  <div class="grid grid-cols-2 gap-2 text-sm">
                    <div class="rounded-lg bg-[var(--md-sys-color-surface-container)] p-2"><span class="block text-[var(--md-sys-color-on-surface-variant)]">Mission</span><strong>{missionController.mode}</strong></div>
                    <div class="rounded-lg bg-[var(--md-sys-color-surface-container)] p-2"><span class="block text-[var(--md-sys-color-on-surface-variant)]">Planner</span><strong>{missionController.status}</strong></div>
                    <div class="rounded-lg bg-[var(--md-sys-color-surface-container)] p-2"><span class="block text-[var(--md-sys-color-on-surface-variant)]">Route</span><strong>{missionController.path.length} cells</strong></div>
                    <div class="rounded-lg bg-[var(--md-sys-color-surface-container)] p-2"><span class="block text-[var(--md-sys-color-on-surface-variant)]">Goal</span><strong>{missionController.goal ? `${missionController.goal.x}, ${missionController.goal.y}` : '—'}</strong></div>
                  </div>
                  <p class="text-sm text-[var(--md-sys-color-on-surface-variant)] leading-relaxed">{missionController.reason}</p>
                  {#if missionMode === 'hardware'}
                    <p class="text-sm leading-snug text-[var(--md-sys-color-on-surface-variant)]">
                      {missionController.mode === 'NAVIGATE' ? 'Click a known-free map cell to set the destination. Unknown and inflated cells are rejected.' : missionController.status === 'SCANNING' ? 'Rover is stopped while the top ToF builds or refreshes local occupancy evidence.' : 'Explore selects a reachable frontier and replans only through known-free cells.'}
                    </p>
                  {/if}
                  <div class="flex gap-3 mt-2">
                    <button type="button" class="ui-button ui-button--tonal flex-1 !h-10" onclick={toggleAutoPause} disabled={missionController.status !== 'RUNNING'}>
                      <span class="material-symbols-rounded text-[20px]">{isAutoPaused ? 'play_arrow' : 'pause'}</span>
                      {isAutoPaused ? 'Resume' : 'Pause'}
                    </button>
                    <button type="button" class="ui-button ui-button--outlined flex-1 !h-10" onclick={() => haltMission()}>
                      <span class="material-symbols-rounded text-[20px]">stop</span>
                      Halt
                    </button>
                  </div>
                </div>
              {/if}
              </div>
            </aside>

            <!-- CENTER: explicitly simulated/estimated route viewport -->
            <main class="flex-1 min-w-0 h-[450px] lg:h-auto lg:min-h-0 border border-[var(--md-sys-color-outline-variant)] bg-[#0c0e13] relative flex flex-col">
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
                {hazardZones}
                waypointEnabled={missionMode === 'hardware' && exploreControlReady && controlMode === 'AUTO_EXPLORE' && missionController.mode === 'NAVIGATE'}
                {missionWaypoint}
                onSetWaypoint={setNavigationWaypoint}
              />
            </main>

            <!-- RIGHT: SAFETY & TILT -->
            <aside class="w-full lg:w-[280px] xl:w-[310px] shrink-0 lg:min-h-0 overflow-hidden" data-testid="safety-panel" use:fitHeight={`${telemetry.alertState}:${tofStatus}:${telemetry.frontBlocked}`}>
              <div data-fit-content class="flex flex-col gap-6 origin-top-left">
              <div data-fit-safety-summary class="flex flex-col gap-5">
                <h3 class="text-lg font-semibold flex items-center gap-2">
                  <span class="material-symbols-rounded text-[22px] text-[var(--md-sys-color-on-surface-variant)]">health_and_safety</span> Safety state
                </h3>
                
                <div class="flex items-start gap-3 p-3 rounded-lg border {isEstop || telemetry.alertState === 'STOPPED' ? 'bg-[var(--md-sys-color-error-container)] border-[var(--md-sys-color-error)] text-[var(--md-sys-color-on-error-container)]' : telemetry.frontBlocked || telemetry.alertState === 'ADVISORY' ? 'bg-[var(--ui-color-warning-container)] border-[var(--ui-color-warning)] text-[var(--ui-color-on-warning-container)]' : telemetry.alertState === 'UNKNOWN' ? 'bg-[var(--md-sys-color-surface-container-highest)] border-[var(--md-sys-color-outline-variant)] text-[var(--md-sys-color-on-surface-variant)]' : 'bg-[var(--ui-color-success-container)] border-transparent text-[var(--ui-color-on-success-container)]'}">
                  <span class="material-symbols-rounded text-[28px]">
                    {isEstop ? 'front_hand' : telemetry.frontBlocked || telemetry.alertState === 'ADVISORY' || telemetry.alertState === 'STOPPED' ? 'warning' : 'info'}
                  </span>
                  <div>
                    <div class="text-base font-bold">{isEstop ? 'Remote stop latched' : telemetry.alertState === 'STOPPED' ? 'Rover safety stop' : telemetry.frontBlocked ? 'Forward motion held' : telemetry.alertState === 'ADVISORY' ? 'Sensor advisory' : `${telemetry.source} telemetry`}</div>
                    <div class="text-sm leading-snug mt-0.5 opacity-90">{transportMode === 'SIMULATION' ? 'No hardware watchdog in demo mode' : isEstop ? 'Clear the latch, then explicitly arm again.' : telemetry.alertState === 'STOPPED' ? 'A safety fault or stuck state requires operator review.' : telemetry.frontBlocked ? 'Front gate is blocked; reverse and in-place pivot remain available.' : gatewayRadioReady ? 'Gateway 450 ms heartbeat watchdog · rover command TTL 300 ms' : 'Hardware link is not ready'}</div>
                  </div>
                </div>

                <div class="flex flex-col gap-3 text-sm">
                  <div class="flex items-center justify-between">
                    <span class="text-[var(--md-sys-color-on-surface-variant)] flex items-center gap-2"><span class="w-2.5 h-2.5 rounded-full bg-[var(--md-sys-color-outline)]"></span> MQ-4 signal</span>
                    <span class="font-bold text-[var(--md-sys-color-on-surface)]">{telemetry.gasState}</span>
                  </div>
                  <div class="flex items-center justify-between">
                    <span class="text-[var(--md-sys-color-on-surface-variant)] flex items-center gap-2"><span class="w-2.5 h-2.5 rounded-full {telemetry.tofMm === null ? 'bg-[var(--ui-color-warning)]' : 'bg-[var(--ui-color-success)]'}"></span> VL53L0X</span>
                    <span class="font-bold text-[var(--md-sys-color-on-surface)]">{tofStatus}</span>
                  </div>
                  <div class="flex items-center justify-between">
                    <span class="text-[var(--md-sys-color-on-surface-variant)] flex items-center gap-2"><span class="w-2.5 h-2.5 rounded-full {telemetry.ultrasonicDistanceCm === null ? 'bg-[var(--ui-color-warning)]' : 'bg-[var(--ui-color-success)]'}"></span> HC-SR04</span>
                    <span class="font-bold text-[var(--md-sys-color-on-surface)]">{telemetry.ultrasonicDistanceCm === null ? 'NO RETURN' : `${telemetry.ultrasonicDistanceCm.toFixed(1)} cm`}</span>
                  </div>
                  <div class="flex items-center justify-between">
                    <span class="text-[var(--md-sys-color-on-surface-variant)] flex items-center gap-2"><span class="w-2.5 h-2.5 rounded-full {telemetry.frontBlocked ? 'bg-[var(--ui-color-warning)]' : telemetry.frontValid && telemetry.frontFresh ? 'bg-[var(--ui-color-success)]' : 'bg-[var(--md-sys-color-outline)]'}"></span> Fused front gate</span>
                    <span class="font-bold text-[var(--md-sys-color-on-surface)]">{telemetry.frontValid && telemetry.frontFresh ? telemetry.frontBlocked ? 'FORWARD HELD' : 'CLEAR' : 'FAIL-CLOSED'}</span>
                  </div>
                  <div class="flex items-center justify-between">
                    <span class="text-[var(--md-sys-color-on-surface-variant)] flex items-center gap-2"><span class="w-2.5 h-2.5 rounded-full bg-[var(--ui-color-success)]"></span> MPU6050</span> 
                    <span class="font-bold text-[var(--md-sys-color-on-surface)]">{telemetry.pitchDeg === null ? 'UNKNOWN' : 'LIVE 6-DOF'}</span>
                  </div>
                </div>
              </div>

              <div data-fit-orientation class="flex-1 min-h-0 flex flex-col border-t border-[var(--md-sys-color-outline-variant)] pt-3">
                <div class="mb-3 flex items-center justify-between gap-3">
                  <h3 class="text-lg font-semibold flex items-center gap-2">
                    <span class="material-symbols-rounded text-[22px] text-[var(--md-sys-color-on-surface-variant)]">explore</span> Orientation
                  </h3>
                  <button type="button" class="ui-button ui-button--outlined !h-9 px-3 text-sm" disabled={orientationCalibrating || gatewayArmed || (missionMode !== 'demo' && (!isConnected || !gatewayRadioReady))} onclick={calibrateOrientation}>
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
