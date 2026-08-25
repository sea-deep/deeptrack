<script>
  import NavRail from '$lib/components/NavRail.svelte';
  import MetricTile from '$lib/components/MetricTile.svelte';
  import SlamMapCanvas from '$lib/components/SlamMapCanvas.svelte';
  import TiltHorizon from '$lib/components/TiltHorizon.svelte';
  import SensorDetailModal from '$lib/components/SensorDetailModal.svelte';
  import MapsView from '$lib/components/dashboard/MapsView.svelte';
  import LogsView from '$lib/components/dashboard/LogsView.svelte';
  import HardwareView from '$lib/components/dashboard/HardwareView.svelte';
  import Navbar from '$lib/components/Navbar.svelte';
  import Footer from '$lib/components/Footer.svelte';
  import { onMount } from 'svelte';
  import { supabase } from '$lib/supabaseClient.js';
  import { goto } from '$app/navigation';
  import { initialTelemetry, initialScanPoints } from '$lib/mocks/telemetryMock.js';
  import { sendShared, receiveShared, m3TopLevelFadeThrough } from '$lib/utils/motion.js';

  // --- Shared App State ---
  /** @type {'hardware' | 'demo' | null} */
  let missionMode = $state(null);
  let activeNavView = $state('console'); // 'console', 'maps', 'logs', 'hardware'
  let transportMode = $state('SIMULATION'); // 'SIMULATION', 'SERIAL'
  let isConnected = $state(true);
  let isEstop = $state(false);
  let isEstopPending = $state(false);
  let heartbeatAgeMs = $state(48);

  /** @type {any} */
  let serialPort = $state(null);
  /** @type {WritableStreamDefaultWriter<string> | null} */
  let serialWriter = $state(null);
  let isConnectingSerial = $state(false);

  // --- Console Specific State ---
  /** @type {'MANUAL' | 'AUTO_EXPLORE'} */
  let controlMode = $state('MANUAL');
  let isRecordingMap = $state(false);
  let isAutoPaused = $state(false);
  let roverPose = $state({ x: 0, y: 0, headingDeg: 90 });
  let speedPwm = $state(200);

  // Live Telemetry
  let telemetry = $state({ ...initialTelemetry });
  let scanPoints = $state([...initialScanPoints]);

  let historyBuffers = $state({
    methane: [800, 810, 815, 820, 825, 830, 820, 815, 820, 825, 820, 830, 825, 820],
    temperature: [28.8, 28.9, 29.0, 29.1, 29.2, 29.2, 29.3, 29.2, 29.1, 29.2, 29.2],
    water: [340, 345, 350, 350, 352, 348, 350, 355, 350, 350],
    ultrasonic: [145, 144, 143, 142, 142, 141, 142, 142, 143, 142],
    distance: [0.0, 0.4, 0.8, 1.2, 1.8, 2.4, 3.1, 3.8, 4.28],
    pitch: [0.5, 0.8, 1.2, 1.5, 1.8, 2.1, 2.0, 2.1, 2.1],
    roll: [-0.4, -0.6, -0.9, -1.2, -1.4, -1.3, -1.4, -1.4]
  });

  // Modal State
  let selectedSensorKey = $state(/** @type {string | null} */ (null));
  let isModalOpen = $state(false);

  // Drive Controls
  let keyState = $state({ w: false, a: false, s: false, d: false, space: false });
  let currentMotorL = $state(0);
  let currentMotorR = $state(0);

  // --- Web Serial API ---
  async function connectWebSerial() {
    if (!('serial' in navigator)) {
      alert('Web Serial API is only supported in Chromium-based browsers (Chrome, Edge).');
      return;
    }
    try {
      isConnectingSerial = true;
      // @ts-ignore
      serialPort = await navigator.serial.requestPort();
      await serialPort.open({ baudRate: 115200 });

      const textDecoder = new TextDecoderStream();
      // @ts-ignore
      serialPort.readable.pipeTo(textDecoder.writable);
      const reader = textDecoder.readable.getReader();

      const textEncoder = new TextEncoderStream();
      // @ts-ignore
      textEncoder.readable.pipeTo(serialPort.writable);
      serialWriter = textEncoder.writable.getWriter();

      transportMode = 'SERIAL';
      isConnected = true;

      (async () => {
        let buffer = '';
        while (true) {
          const { value, done } = await reader.read();
          if (done) break;
          if (value) {
            buffer += value;
            const lines = buffer.split('\n');
            buffer = lines.pop() || '';
            for (const line of lines) {
              const trimmed = line.trim();
              if (!trimmed) continue;
              if (trimmed.startsWith('TELEMETRY:')) {
                try { handleRawTelemetryPacket(JSON.parse(trimmed.substring(10))); } catch (e) {}
              } else if (trimmed.startsWith('SCAN:')) {
                try { handleScanPacket(JSON.parse(trimmed.substring(5))); } catch (e) {}
              }
            }
          }
        }
      })();
    } catch (err) {
      console.error(err);
    } finally {
      isConnectingSerial = false;
    }
  }

  async function disconnectWebSerial() {
    try {
      if (serialWriter) { await serialWriter.close(); serialWriter = null; }
      if (serialPort) { await serialPort.close(); serialPort = null; }
    } catch (e) {}
    transportMode = 'SIMULATION';
  }

  /** @param {any} pkt */
  function handleRawTelemetryPacket(pkt) {
    const pitch = Math.atan2(-pkt.ax, Math.sqrt(pkt.ay * pkt.ay + pkt.az * pkt.az)) * (180.0 / Math.PI);
    const roll = Math.atan2(pkt.ay, pkt.az) * (180.0 / Math.PI);
    const methaneEst = Math.max(0, Math.round((pkt.gas - 400) * 1.8));

    telemetry.temperature = pkt.t;
    telemetry.humidity = pkt.h;
    telemetry.methaneRaw = pkt.gas;
    telemetry.methanePpm = methaneEst;
    telemetry.waterRaw = pkt.water;
    telemetry.pitchDeg = +pitch.toFixed(1);
    telemetry.rollDeg = +roll.toFixed(1);
    telemetry.dangerState = pkt.danger === 1 || pkt.gas > 1000 || Math.abs(pitch) > 25 || Math.abs(roll) > 25;
    heartbeatAgeMs = 32;

    historyBuffers.methane = [...historyBuffers.methane.slice(-29), telemetry.methaneRaw];
    historyBuffers.temperature = [...historyBuffers.temperature.slice(-29), telemetry.temperature];
    historyBuffers.water = [...historyBuffers.water.slice(-29), telemetry.waterRaw];
    historyBuffers.pitch = [...historyBuffers.pitch.slice(-29), telemetry.pitchDeg];
    historyBuffers.roll = [...historyBuffers.roll.slice(-29), telemetry.rollDeg];
  }

  /** @param {any} pkt */
  function handleScanPacket(pkt) {
    const { angle_deg, distance_mm, valid } = pkt;
    scanPoints = scanPoints.map(p => 
      Math.abs(p.angle_deg - angle_deg) < 5 ? { angle_deg, distance_mm, valid: valid === true || valid === 1 } : p
    );
  }

  // --- Motor Commands ---
  /** @param {number} left @param {number} right */
  async function sendDrive(left, right) {
    if (isEstop || !isConnected || controlMode !== 'MANUAL') {
      currentMotorL = 0;
      currentMotorR = 0;
      return;
    }
    currentMotorL = left;
    currentMotorR = right;
    const cmdStr = `L:${left} R:${right}\n`;
    if (serialWriter && transportMode === 'SERIAL') {
      try { await serialWriter.write(cmdStr); } catch (e) {}
    }
  }

  function toggleEstop() {
    if (!isConnected) return;
    if (!isEstop) {
      isEstopPending = true;
      isEstop = true;
      sendDrive(0, 0);
      setTimeout(() => { isEstopPending = false; }, 150);
    } else {
      isEstop = false;
    }
  }

  /** @param {KeyboardEvent} e */
  function handleKeydown(e) {
    if (e.target instanceof HTMLInputElement || e.target instanceof HTMLTextAreaElement) return;
    if (e.code === 'KeyW' || e.code === 'ArrowUp') {
      e.preventDefault(); keyState.w = true;
      if (!isEstop && controlMode === 'MANUAL') sendDrive(speedPwm, speedPwm);
    } else if (e.code === 'KeyS' || e.code === 'ArrowDown') {
      e.preventDefault(); keyState.s = true;
      if (!isEstop && controlMode === 'MANUAL') sendDrive(-speedPwm, -speedPwm);
    } else if (e.code === 'KeyA' || e.code === 'ArrowLeft') {
      e.preventDefault(); keyState.a = true;
      if (!isEstop && controlMode === 'MANUAL') sendDrive(-Math.round(speedPwm * 0.9), Math.round(speedPwm * 0.9));
    } else if (e.code === 'KeyD' || e.code === 'ArrowRight') {
      e.preventDefault(); keyState.d = true;
      if (!isEstop && controlMode === 'MANUAL') sendDrive(Math.round(speedPwm * 0.9), -Math.round(speedPwm * 0.9));
    } else if (e.code === 'Space') {
      e.preventDefault(); keyState.space = true; toggleEstop();
    }
  }

  /** @param {KeyboardEvent} e */
  function handleKeyup(e) {
    if (e.code === 'KeyW' || e.code === 'ArrowUp') {
      keyState.w = false;
      if (!isEstop && controlMode === 'MANUAL' && !keyState.s && !keyState.a && !keyState.d) sendDrive(0, 0);
    } else if (e.code === 'KeyS' || e.code === 'ArrowDown') {
      keyState.s = false;
      if (!isEstop && controlMode === 'MANUAL' && !keyState.w && !keyState.a && !keyState.d) sendDrive(0, 0);
    } else if (e.code === 'KeyA' || e.code === 'ArrowLeft') {
      keyState.a = false;
      if (!isEstop && controlMode === 'MANUAL' && !keyState.w && !keyState.s && !keyState.d) sendDrive(0, 0);
    } else if (e.code === 'KeyD' || e.code === 'ArrowRight') {
      keyState.d = false;
      if (!isEstop && controlMode === 'MANUAL' && !keyState.w && !keyState.s && !keyState.a) sendDrive(0, 0);
    } else if (e.code === 'Space') {
      keyState.space = false;
    }
  }

  function handleWindowBlur() {
    keyState = { w: false, a: false, s: false, d: false, space: false };
    if (controlMode === 'MANUAL') sendDrive(0, 0);
  }

  // --- Modal Config ---
  let activeModalData = $derived.by(() => {
    switch (selectedSensorKey) {
      case 'methane':
        return {
          title: 'MQ-4 Semiconductor Gas Sensor',
          sensorIc: 'MQ-4 (SnO₂ Heating Element)',
          pinout: 'GPIO 34 (ADC1_CH6 via 10k/15k Divider)',
          sampling: '10 Hz continuous ADC',
          currentVal: `${telemetry.methaneRaw} ADC`,
          unit: 'ADC',
          history: historyBuffers.methane,
          status: telemetry.methaneRaw >= 1500 ? 'critical' : telemetry.methaneRaw >= 1000 ? 'warning' : 'normal',
          dgmsRule: 'Action required at threshold.',
          actionThreshold: '1000 ADC — Advisory',
          dangerThreshold: '1500 ADC — Cutoff'
        };
      case 'temp':
        return {
          title: 'DHT22 Underground Climate Monitor',
          sensorIc: 'DHT22',
          pinout: 'GPIO 23',
          sampling: '0.5 Hz',
          currentVal: `${telemetry.temperature} °C`,
          unit: '°C',
          history: historyBuffers.temperature,
          status: telemetry.temperature >= 33.5 ? 'warning' : 'normal',
          dgmsRule: 'Temperature monitoring.',
          actionThreshold: '30.5 °C',
          dangerThreshold: '33.5 °C'
        };
      case 'water':
        return {
          title: 'Resistive Water Sensor',
          sensorIc: 'Immersion Probe',
          pinout: 'GPIO 36',
          sampling: '5 Hz',
          currentVal: `${telemetry.waterRaw} ADC`,
          unit: 'ADC',
          history: historyBuffers.water,
          status: telemetry.waterRaw >= 1500 ? 'warning' : 'normal',
          dgmsRule: 'Inundation prevention.',
          actionThreshold: '800 ADC',
          dangerThreshold: '1500 ADC'
        };
      case 'ultrasonic':
        return {
          title: 'HC-SR04 Forward Ultrasonic',
          sensorIc: 'HC-SR04',
          pinout: 'TRIG GPIO 5 / ECHO GPIO 18',
          sampling: '10 Hz',
          currentVal: `${telemetry.ultrasonicCm} cm`,
          unit: 'cm',
          history: historyBuffers.ultrasonic,
          status: telemetry.ultrasonicCm < 15 ? 'critical' : telemetry.ultrasonicCm < 25 ? 'warning' : 'normal',
          dgmsRule: 'Obstacle avoidance failsafe.',
          actionThreshold: '25 cm',
          dangerThreshold: '15 cm'
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
    // Auth Guard: Prevent kicking if OAuth is returning with a token in the hash
    const isOAuthReturn = window.location.hash.includes('access_token');
    
    supabase.auth.getSession().then(({ data: { session } }) => {
      if (!session && !isOAuthReturn) {
        goto('/auth');
      }
    });

    const { data: { subscription } } = supabase.auth.onAuthStateChange((_event, session) => {
      if (!session && !window.location.hash.includes('access_token')) {
        goto('/auth');
      }
    });

    window.addEventListener('keydown', handleKeydown);
    window.addEventListener('keyup', handleKeyup);
    window.addEventListener('blur', handleWindowBlur);

    const simInterval = setInterval(() => {
      if (transportMode === 'SIMULATION' && isConnected) {
        heartbeatAgeMs = 45 + Math.floor(Math.random() * 20);
        telemetry.temperature = +(28.2 + Math.sin(Date.now() / 4000) * 0.8).toFixed(1);
        telemetry.humidity = +(69.7 + Math.cos(Date.now() / 5000) * 1.5).toFixed(1);
        telemetry.methaneRaw = Math.max(600, Math.round(862 + Math.sin(Date.now() / 3000) * 40));
        telemetry.methanePpm = Math.max(100, Math.round((telemetry.methaneRaw - 400) * 1.8));
        telemetry.waterRaw = Math.round(350 + Math.cos(Date.now() / 7000) * 20);
        telemetry.pitchDeg = +(-3.5 + Math.sin(Date.now() / 2500) * 1.5).toFixed(1);
        telemetry.rollDeg = +(2.6 + Math.cos(Date.now() / 2800) * 1.2).toFixed(1);
        telemetry.ultrasonicCm = +(119.0 + Math.sin(Date.now() / 2000) * 15.0).toFixed(0);

        if (!isEstop) {
          if (controlMode === 'AUTO_EXPLORE' && !isAutoPaused) {
            const t = Date.now() / 1500;
            roverPose.x = +(33.2 + Math.sin(t * 0.4) * 8).toFixed(1);
            roverPose.y = +(-13.6 + Math.cos(t * 0.3) * 6).toFixed(1);
            roverPose.headingDeg = +(90 + Math.sin(t * 0.5) * 30).toFixed(0);
            telemetry.encoderL += 1;
            telemetry.encoderR += 1;
            telemetry.distanceMeters = +(4.52 + telemetry.encoderL * 0.005).toFixed(2);
          } else if (currentMotorL !== 0 || currentMotorR !== 0) {
            const speed = (currentMotorL + currentMotorR) / 400;
            const turn = (currentMotorR - currentMotorL) / 200;
            roverPose.headingDeg += turn * 3;
            const rad = (roverPose.headingDeg - 90) * (Math.PI / 180);
            roverPose.x += Math.cos(rad) * speed * 2;
            roverPose.y += Math.sin(rad) * speed * 2;
            telemetry.encoderL += 2;
            telemetry.encoderR += 2;
            telemetry.distanceMeters = +(telemetry.encoderL * (Math.PI * 0.065 / 20)).toFixed(2);
          }
        }
      }
    }, 200);

    return () => {
      subscription.unsubscribe();
      window.removeEventListener('keydown', handleKeydown);
      window.removeEventListener('keyup', handleKeyup);
      window.removeEventListener('blur', handleWindowBlur);
      clearInterval(simInterval);
    };
  });
</script>

<svelte:head>
  <title>DeepTrack Dashboard</title>
</svelte:head>


{#if missionMode === null}
  <div class="min-h-screen flex flex-col bg-[var(--md-sys-color-surface)] text-[var(--md-sys-color-on-surface)] select-none" in:m3TopLevelFadeThrough={{ duration: 400 }}>
    <Navbar active="dashboard" />
    
    <main class="flex-1 flex items-center justify-center p-6">
      <div class="ui-card w-full max-w-md">
        <div class="text-center mb-8">
          <h1 class="text-2xl font-bold font-headline tracking-tight text-[var(--md-sys-color-on-surface)] mb-2">Select Mission Target</h1>
          <p class="text-sm text-[var(--md-sys-color-on-surface-variant)] leading-relaxed">
            Connect to a physical DeepTrack gateway board via serial, or load the simulated telemetry sandbox.
          </p>
        </div>
        
        <div class="flex flex-col gap-4">
          <button
            type="button"
            class="ui-button ui-button--filled w-full flex items-center justify-center gap-2"
            onclick={() => { transportMode = 'SIMULATION'; missionMode = 'demo'; }}
          >
            <span class="material-symbols-rounded text-lg">science</span>
            <span>Launch Demo Sandbox</span>
          </button>

          <button
            type="button"
            class="ui-button ui-button--outlined w-full flex items-center justify-center gap-2"
            disabled={true}
          >
            <span class="material-symbols-rounded text-lg">memory</span>
            <span>Physical Board (Unavailable)</span>
          </button>
        </div>
        
        <div class="mt-8 pt-5 border-t border-[var(--md-sys-color-outline-variant)] flex items-center justify-between">
          <div class="text-[11px] text-[var(--md-sys-color-on-surface-variant)] flex items-center gap-1.5 font-medium">
            <span class="material-symbols-rounded text-sm">security</span>
            Verified Operator Session
          </div>
          <button
            type="button"
            class="text-[11px] font-semibold text-[var(--md-sys-color-error)] hover:underline flex items-center gap-1"
            onclick={() => supabase.auth.signOut()}
          >
            End Session
          </button>
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
          class:bg-[var(--ui-color-success)]={isConnected && !isEstop}
          class:bg-[var(--md-sys-color-error)]={isEstop || !isConnected}
          class:animate-pulse={isConnected && !isEstop}>
        </span>
        <span class="telemetry font-bold text-base md:text-lg tracking-wide text-[var(--md-sys-color-on-surface)]">
          DT-ALPHA-01
        </span>
        <span class="text-sm font-medium px-2 py-0.5 rounded-md flex items-center gap-1.5 {transportMode === 'SERIAL' ? 'bg-[var(--ui-color-success-container)] text-[var(--ui-color-on-success-container)]' : 'bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)]'}">
          <span class="material-symbols-rounded text-[18px]">{transportMode === 'SERIAL' ? 'usb' : 'hub'}</span>
          {transportMode === 'SERIAL' ? 'Live USB' : 'Simulated'}
        </span>
      </div>

      <!-- Center: Connection State -->
      <div class="hidden md:flex items-center gap-4 text-sm font-medium text-[var(--md-sys-color-on-surface-variant)]">
        {#if transportMode === 'SERIAL'}
          <span>RSSI: <strong class="telemetry text-[var(--md-sys-color-on-surface)]">{telemetry.rssi} dBm</strong></span>
          <span class="opacity-40">·</span>
          <span>Heartbeat: <strong class="telemetry text-[var(--ui-brand-cyan)]">{heartbeatAgeMs} ms</strong></span>
        {/if}
      </div>

      <!-- Right: Connection Action & Emergency Stop -->
      <div class="flex items-center gap-3 shrink-0">
        {#if transportMode === 'SERIAL'}
          <button type="button" class="ui-button ui-button--outlined !h-10 !px-4 text-sm" onclick={disconnectWebSerial}>
            Disconnect
          </button>
        {:else}
          <button type="button" class="ui-button ui-button--filled !bg-[var(--md-sys-color-primary)] !text-[var(--md-sys-color-on-primary)] !h-10 !px-5 text-sm font-semibold" onclick={connectWebSerial} disabled={isConnectingSerial}>
            <span class="material-symbols-rounded text-[20px]">usb</span>
            {#if isConnectingSerial}Connecting...{:else}Connect USB{/if}
          </button>
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
            {#if !isConnected}E-Stop offline
            {:else if isEstopPending}Stopping...
            {:else if isEstop}Reset E-stop
            {:else}Emergency brake{/if}
          </span>
        </button>
      </div>
    </header>

    <!-- CONTENT AREA -->
    <main class="flex-1 overflow-hidden relative">
      {#if activeNavView === 'console'}
        <div class="absolute inset-0 flex flex-col p-4 md:p-5 gap-4 overflow-hidden min-h-0" in:m3TopLevelFadeThrough={{ duration: 300 }}>
          
          <!-- TELEMETRY RIBBON -->
          <section class="grid grid-cols-2 lg:grid-cols-4 divide-y lg:divide-y-0 lg:divide-x divide-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface-container-low)] rounded-xl border border-[var(--md-sys-color-outline-variant)] shrink-0 overflow-hidden">
            
            <button type="button" class="text-left flex flex-col gap-1 px-5 py-4 cursor-pointer hover:bg-[var(--md-sys-color-surface-container-highest)] transition-colors duration-150 active:bg-[var(--md-sys-color-surface-variant)]" onclick={() => inspectSensor('methane')}>
              <div class="flex items-center gap-1.5 text-sm font-medium text-[var(--md-sys-color-on-surface-variant)]">
                <span class="material-symbols-rounded text-[20px] text-[var(--ui-brand-cyan)]">air</span> Methane
              </div>
              <div class="flex items-baseline gap-2">
                <span class="text-3xl font-bold telemetry text-[var(--md-sys-color-on-surface)]">{telemetry.methaneRaw} <span class="text-base font-normal">ADC</span></span>
                {#if telemetry.methaneRaw >= 1000}
                  <span class="text-sm px-2 py-0.5 rounded font-bold bg-[var(--ui-color-warning-container)] text-[var(--ui-color-on-warning-container)]">Advisory</span>
                {/if}
              </div>
              <span class="text-sm text-[var(--md-sys-color-on-surface-variant)]">Raw MQ-4 gas activity</span>
            </button>

            <button type="button" class="text-left flex flex-col gap-1 px-5 py-4 cursor-pointer hover:bg-[var(--md-sys-color-surface-container-highest)] transition-colors duration-150 active:bg-[var(--md-sys-color-surface-variant)]" onclick={() => inspectSensor('temp')}>
              <div class="flex items-center gap-1.5 text-sm font-medium text-[var(--md-sys-color-on-surface-variant)]">
                <span class="material-symbols-rounded text-[20px] text-[var(--ui-color-warning)]">device_thermostat</span> Climate
              </div>
              <div class="flex items-baseline gap-2">
                <span class="text-3xl font-bold telemetry text-[var(--md-sys-color-on-surface)]">{telemetry.temperature} <span class="text-base font-normal">°C</span></span>
                {#if telemetry.temperature >= 33.5}
                  <span class="text-sm px-2 py-0.5 rounded font-bold bg-[var(--md-sys-color-error-container)] text-[var(--md-sys-color-on-error-container)]">Heat</span>
                {/if}
              </div>
              <span class="text-sm text-[var(--md-sys-color-on-surface-variant)]">{telemetry.humidity}% RH</span>
            </button>

            <button type="button" class="text-left flex flex-col gap-1 px-5 py-4 cursor-pointer hover:bg-[var(--md-sys-color-surface-container-highest)] transition-colors duration-150 active:bg-[var(--md-sys-color-surface-variant)]" onclick={() => inspectSensor('water')}>
              <div class="flex items-center gap-1.5 text-sm font-medium text-[var(--md-sys-color-on-surface-variant)]">
                <span class="material-symbols-rounded text-[20px] text-blue-400">water_drop</span> Water
              </div>
              <div class="flex items-baseline gap-2">
                <span class="text-3xl font-bold text-[var(--md-sys-color-on-surface)]">{telemetry.waterRaw < 800 ? 'Dry' : 'Wet'}</span>
                {#if telemetry.waterRaw >= 1500}
                  <span class="text-sm px-2 py-0.5 rounded font-bold bg-[var(--md-sys-color-error-container)] text-[var(--md-sys-color-on-error-container)]">Risk</span>
                {/if}
              </div>
              <span class="text-sm text-[var(--md-sys-color-on-surface-variant)]">{telemetry.waterRaw < 800 ? 'No standing water' : 'Immersion detected'}</span>
            </button>

            <button type="button" class="text-left flex flex-col gap-1 px-5 py-4 cursor-pointer hover:bg-[var(--md-sys-color-surface-container-highest)] transition-colors duration-150 active:bg-[var(--md-sys-color-surface-variant)]" onclick={() => inspectSensor('ultrasonic')}>
              <div class="flex items-center gap-1.5 text-sm font-medium text-[var(--md-sys-color-on-surface-variant)]">
                <span class="material-symbols-rounded text-[20px] text-[var(--ui-color-success)]">sensors</span> Front clearance
              </div>
              <div class="flex items-baseline gap-2">
                <span class="text-3xl font-bold telemetry text-[var(--md-sys-color-on-surface)]">{telemetry.ultrasonicCm > 300 ? 'Clear' : telemetry.ultrasonicCm < 2 ? '---' : telemetry.ultrasonicCm} <span class="text-base font-normal">{telemetry.ultrasonicCm <= 300 && telemetry.ultrasonicCm >= 2 ? 'cm' : ''}</span></span>
                {#if telemetry.ultrasonicCm < 15}
                  <span class="text-sm px-2 py-0.5 rounded font-bold bg-[var(--md-sys-color-error-container)] text-[var(--md-sys-color-on-error-container)]">Brake</span>
                {/if}
              </div>
              <span class="text-sm text-[var(--md-sys-color-on-surface-variant)]">{telemetry.ultrasonicCm < 15 ? 'Hardware cutoff' : 'Trajectory clear'}</span>
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
                  <button type="button" class="flex-1 py-1.5 flex items-center justify-center gap-1.5 text-sm font-semibold transition-colors rounded-lg relative z-10 {controlMode === 'MANUAL' ? 'text-[var(--md-sys-color-on-primary-container)]' : 'text-[var(--md-sys-color-on-surface-variant)] hover:text-[var(--md-sys-color-on-surface)]'}" onclick={() => { controlMode = 'MANUAL'; sendDrive(0,0); }}>
                    {#if controlMode === 'MANUAL'}
                      <div class="absolute inset-0 bg-[var(--md-sys-color-primary-container)] rounded-lg shadow-sm -z-10" in:receiveShared={{key: 'control-mode'}} out:sendShared={{key: 'control-mode'}}></div>
                    {/if}
                    <span class="material-symbols-rounded text-[18px]">sports_esports</span> Manual
                  </button>
                  <button type="button" class="flex-1 py-1.5 flex items-center justify-center gap-1.5 text-sm font-semibold transition-colors rounded-lg relative z-10 {controlMode === 'AUTO_EXPLORE' ? 'text-[var(--md-sys-color-on-primary-container)]' : 'text-[var(--md-sys-color-on-surface-variant)] hover:text-[var(--md-sys-color-on-surface)]'}" onclick={() => controlMode = 'AUTO_EXPLORE'}>
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
                    <button type="button" class="h-14 rounded-lg bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center hover:bg-[var(--md-sys-color-primary-container)] hover:text-[var(--md-sys-color-on-primary-container)] active:bg-[var(--md-sys-color-primary)] active:text-[var(--md-sys-color-on-primary)] active:scale-95 transition-all duration-150 {keyState.w ? '!bg-[var(--md-sys-color-primary)] !text-[var(--md-sys-color-on-primary)] scale-95' : ''}" disabled={isEstop || !isConnected} onpointerdown={() => { if(!isEstop) sendDrive(speedPwm, speedPwm); }} onpointerup={() => sendDrive(0,0)} onpointercancel={() => sendDrive(0,0)}><span class="material-symbols-rounded text-[26px]">arrow_upward</span></button>
                    <div></div>
                    <button type="button" class="h-14 rounded-lg bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center hover:bg-[var(--md-sys-color-primary-container)] hover:text-[var(--md-sys-color-on-primary-container)] active:bg-[var(--md-sys-color-primary)] active:text-[var(--md-sys-color-on-primary)] active:scale-95 transition-all duration-150 {keyState.a ? '!bg-[var(--md-sys-color-primary)] !text-[var(--md-sys-color-on-primary)] scale-95' : ''}" disabled={isEstop || !isConnected} onpointerdown={() => { if(!isEstop) sendDrive(-Math.round(speedPwm*0.9), Math.round(speedPwm*0.9)); }} onpointerup={() => sendDrive(0,0)} onpointercancel={() => sendDrive(0,0)}><span class="material-symbols-rounded text-[26px]">arrow_back</span></button>
                    <button type="button" class="h-14 rounded-lg bg-[var(--md-sys-color-error-container)] text-[var(--md-sys-color-on-error-container)] text-sm font-bold active:scale-95 border border-[var(--md-sys-color-outline-variant)] transition-all opacity-80 hover:opacity-100 {keyState.space ? '!bg-[var(--md-sys-color-error)] !text-[var(--md-sys-color-on-error)] scale-95 !opacity-100' : ''}" onclick={() => sendDrive(0,0)}>STOP</button>
                    <button type="button" class="h-14 rounded-lg bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center hover:bg-[var(--md-sys-color-primary-container)] hover:text-[var(--md-sys-color-on-primary-container)] active:bg-[var(--md-sys-color-primary)] active:text-[var(--md-sys-color-on-primary)] active:scale-95 transition-all duration-150 {keyState.d ? '!bg-[var(--md-sys-color-primary)] !text-[var(--md-sys-color-on-primary)] scale-95' : ''}" disabled={isEstop || !isConnected} onpointerdown={() => { if(!isEstop) sendDrive(Math.round(speedPwm*0.9), -Math.round(speedPwm*0.9)); }} onpointerup={() => sendDrive(0,0)} onpointercancel={() => sendDrive(0,0)}><span class="material-symbols-rounded text-[26px]">arrow_forward</span></button>
                    <div></div>
                    <button type="button" class="h-14 rounded-lg bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center hover:bg-[var(--md-sys-color-primary-container)] hover:text-[var(--md-sys-color-on-primary-container)] active:bg-[var(--md-sys-color-primary)] active:text-[var(--md-sys-color-on-primary)] active:scale-95 transition-all duration-150 {keyState.s ? '!bg-[var(--md-sys-color-primary)] !text-[var(--md-sys-color-on-primary)] scale-95' : ''}" disabled={isEstop || !isConnected} onpointerdown={() => { if(!isEstop) sendDrive(-speedPwm, -speedPwm); }} onpointerup={() => sendDrive(0,0)} onpointercancel={() => sendDrive(0,0)}><span class="material-symbols-rounded text-[26px]">arrow_downward</span></button>
                    <div></div>
                  </div>
                  <div class="mt-3 text-sm font-medium text-[var(--md-sys-color-on-surface-variant)] flex gap-3 telemetry"><span>W</span><span>A</span><span>S</span><span>D</span><span>Space</span></div>
                </div>

                <div class="flex flex-col gap-3">
                  <div class="flex justify-between items-end text-sm">
                    <span class="font-semibold flex items-center gap-1.5"><span class="material-symbols-rounded text-[18px]">speed</span> Speed limit</span>
                    <span class="telemetry font-bold text-[var(--md-sys-color-primary)]">{speedPwm} PWM</span>
                  </div>
                  <input type="range" min="100" max="255" step="5" bind:value={speedPwm} class="w-full h-2 rounded-lg appearance-none bg-[var(--md-sys-color-surface-container-highest)] accent-[var(--md-sys-color-primary)] cursor-pointer">
                  <div class="flex gap-1 p-1 bg-[var(--md-sys-color-surface-container)] rounded-xl text-sm font-medium mt-1 relative z-0 border border-[var(--md-sys-color-outline-variant)]">
                    <button type="button" class="flex-1 py-1.5 rounded-lg transition-colors relative z-10 {speedPwm === 140 ? 'text-[var(--md-sys-color-on-primary-container)]' : 'text-[var(--md-sys-color-on-surface-variant)] hover:text-[var(--md-sys-color-on-surface)]'}" onclick={() => speedPwm=140}>
                      {#if speedPwm === 140}
                        <div class="absolute inset-0 bg-[var(--md-sys-color-primary-container)] rounded-lg shadow-sm -z-10" in:receiveShared={{key: 'speed-limit'}} out:sendShared={{key: 'speed-limit'}}></div>
                      {/if}
                      Low
                    </button>
                    <button type="button" class="flex-1 py-1.5 rounded-lg transition-colors relative z-10 {speedPwm === 200 ? 'text-[var(--md-sys-color-on-primary-container)]' : 'text-[var(--md-sys-color-on-surface-variant)] hover:text-[var(--md-sys-color-on-surface)]'}" onclick={() => speedPwm=200}>
                      {#if speedPwm === 200}
                        <div class="absolute inset-0 bg-[var(--md-sys-color-primary-container)] rounded-lg shadow-sm -z-10" in:receiveShared={{key: 'speed-limit'}} out:sendShared={{key: 'speed-limit'}}></div>
                      {/if}
                      Med
                    </button>
                    <button type="button" class="flex-1 py-1.5 rounded-lg transition-colors relative z-10 {speedPwm === 255 ? 'text-[var(--md-sys-color-on-primary-container)]' : 'text-[var(--md-sys-color-on-surface-variant)] hover:text-[var(--md-sys-color-on-surface)]'}" onclick={() => speedPwm=255}>
                      {#if speedPwm === 255}
                        <div class="absolute inset-0 bg-[var(--md-sys-color-primary-container)] rounded-lg shadow-sm -z-10" in:receiveShared={{key: 'speed-limit'}} out:sendShared={{key: 'speed-limit'}}></div>
                      {/if}
                      Max
                    </button>
                  </div>
                </div>

                <div class="flex flex-col gap-3 pt-5 border-t border-[var(--md-sys-color-outline-variant)] text-sm text-[var(--md-sys-color-on-surface-variant)]">
                  <div class="flex justify-between items-center"><span class="font-medium">Motor PWM</span><span class="telemetry font-bold text-[var(--md-sys-color-on-surface)]">L: {currentMotorL} &nbsp; R: {currentMotorR}</span></div>
                  <div class="flex justify-between items-center"><span class="font-medium">Encoder count</span><span class="telemetry font-bold text-[var(--md-sys-color-on-surface)]">{telemetry.encoderL} L &nbsp; {telemetry.encoderR} R</span></div>
                </div>
              {:else}
                <div class="flex flex-col gap-4">
                  <div class="flex justify-between items-center text-sm">
                    <span class="font-semibold text-[var(--md-sys-color-on-surface)]">Navigation logic</span>
                    <span class="px-2.5 py-0.5 rounded font-bold text-[13px] {isAutoPaused ? 'bg-[var(--ui-color-warning-container)] text-[var(--ui-color-on-warning-container)]' : 'bg-[var(--ui-color-success-container)] text-[var(--ui-color-on-success-container)]'}">{isAutoPaused ? 'Paused' : 'Active'}</span>
                  </div>
                  <p class="text-sm text-[var(--md-sys-color-on-surface-variant)] leading-relaxed">Sweep navigation driven by SG90 servo sweeps and VL53L0X distance sensing.</p>
                  <div class="flex gap-3 mt-2">
                    <button type="button" class="ui-button ui-button--tonal flex-1 !h-10" onclick={() => isAutoPaused = !isAutoPaused}>
                      <span class="material-symbols-rounded text-[20px]">{isAutoPaused ? 'play_arrow' : 'pause'}</span>
                      {isAutoPaused ? 'Resume' : 'Pause'}
                    </button>
                    <button type="button" class="ui-button ui-button--outlined flex-1 !h-10" onclick={() => { controlMode = 'MANUAL'; sendDrive(0,0); }}>
                      <span class="material-symbols-rounded text-[20px]">stop</span>
                      Halt
                    </button>
                  </div>
                </div>
              {/if}
            </aside>

            <!-- CENTER: SLAM VIEWPORT -->
            <main class="flex-1 min-w-0 h-[450px] lg:h-auto border border-[var(--md-sys-color-outline-variant)] bg-[#0c0e13] relative flex flex-col">
              <SlamMapCanvas
                mode={controlMode}
                isRecording={isRecordingMap}
                onToggleRecord={() => isRecordingMap = !isRecordingMap}
                {scanPoints}
                {roverPose}
                distanceMeters={telemetry.distanceMeters}
                {isConnected}
              />
            </main>

            <!-- RIGHT: SAFETY & TILT -->
            <aside class="w-full lg:w-[280px] xl:w-[310px] flex flex-col gap-6 shrink-0">
              <div class="flex flex-col gap-5">
                <h3 class="text-lg font-semibold flex items-center gap-2">
                  <span class="material-symbols-rounded text-[22px] text-[var(--md-sys-color-on-surface-variant)]">health_and_safety</span> Safety state
                </h3>
                
                <div class="flex items-start gap-3 p-3 rounded-lg border {telemetry.dangerState || isEstop ? 'bg-[var(--md-sys-color-error-container)] border-[var(--md-sys-color-error)] text-[var(--md-sys-color-on-error-container)]' : 'bg-[var(--ui-color-success-container)] border-transparent text-[var(--ui-color-on-success-container)]'}">
                  <span class="material-symbols-rounded text-[28px]">
                    {isEstop ? 'front_hand' : telemetry.dangerState ? 'warning' : 'check_circle'}
                  </span>
                  <div>
                    <div class="text-base font-bold">{isEstop ? 'Hardware E-Stop' : telemetry.dangerState ? 'Hazard alert' : 'Nominal bounds'}</div>
                    <div class="text-[13px] mt-0.5 opacity-90">500 ms loss-of-signal failsafe active</div>
                  </div>
                </div>

                <div class="flex flex-col gap-3 text-sm">
                  <div class="flex items-center justify-between">
                    <span class="text-[var(--md-sys-color-on-surface-variant)] flex items-center gap-2"><span class="w-2.5 h-2.5 rounded-full bg-[var(--ui-color-success)]"></span> MQ-4 Gas</span> 
                    <span class="font-bold text-[var(--md-sys-color-on-surface)]">Ready</span>
                  </div>
                  <div class="flex items-center justify-between">
                    <span class="text-[var(--md-sys-color-on-surface-variant)] flex items-center gap-2"><span class="w-2.5 h-2.5 rounded-full bg-[var(--ui-color-success)]"></span> VL53L0X</span> 
                    <span class="font-bold text-[var(--md-sys-color-on-surface)]">10 Hz</span>
                  </div>
                  <div class="flex items-center justify-between">
                    <span class="text-[var(--md-sys-color-on-surface-variant)] flex items-center gap-2"><span class="w-2.5 h-2.5 rounded-full bg-[var(--ui-color-success)]"></span> HC-SR04</span> 
                    <span class="font-bold text-[var(--md-sys-color-on-surface)]">Valid</span>
                  </div>
                  <div class="flex items-center justify-between">
                    <span class="text-[var(--md-sys-color-on-surface-variant)] flex items-center gap-2"><span class="w-2.5 h-2.5 rounded-full bg-[var(--ui-color-success)]"></span> MPU6050</span> 
                    <span class="font-bold text-[var(--md-sys-color-on-surface)]">6-DOF</span>
                  </div>
                </div>
              </div>

              <div class="flex-1 min-h-0 flex flex-col border-t border-[var(--md-sys-color-outline-variant)] pt-3">
                <h3 class="text-lg font-semibold flex items-center gap-2 mb-4">
                  <span class="material-symbols-rounded text-[22px] text-[var(--md-sys-color-on-surface-variant)]">explore</span> Orientation
                </h3>
                <div class="flex-1">
                  <TiltHorizon
                    pitchDeg={telemetry.pitchDeg}
                    rollDeg={telemetry.rollDeg}
                    isStale={!isConnected}
                    isCalibrating={false}
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
          <MapsView />
        </div>
      {:else if activeNavView === 'logs'}
        <div class="absolute inset-0" in:m3TopLevelFadeThrough={{ duration: 300 }}>
          <LogsView />
        </div>
      {:else if activeNavView === 'hardware'}
        <div class="absolute inset-0" in:m3TopLevelFadeThrough={{ duration: 300 }}>
          <HardwareView />
        </div>
      {/if}
    </main>
  </div>
</div>

<SensorDetailModal isOpen={isModalOpen} sensor={activeModalData} onClose={() => { isModalOpen = false; selectedSensorKey = null; }} />

{/if}
