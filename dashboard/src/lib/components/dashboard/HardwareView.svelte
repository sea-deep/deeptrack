<script>
  // @ts-nocheck
  import { hardwareInventory, wiringAuthority } from '$lib/config/hardware.js';
  import { DIAGNOSTIC_ACTION } from '$lib/state/gatewayProtocol.js';

  let {
    isConnected = false, gatewayArmed = false, telemetry,
    diagnosticResults = [], onRunDiagnostic = () => {},
    onMotorStart = () => {}, onMotorStop = () => {}
  } = $props();

  let selectedComponent = $state(hardwareInventory[0].component);
  let servoAngle = $state(90);
  let motorTestSpeed = $state(35);
  let servoMoveTimer;

  const checks = [
    { label: 'System status', icon: 'health_and_safety', action: DIAGNOSTIC_ACTION.STATUS },
    { label: 'I2C scan', icon: 'hub', action: DIAGNOSTIC_ACTION.I2C_SCAN },
    { label: 'Climate', icon: 'device_thermostat', action: DIAGNOSTIC_ACTION.DHT },
    { label: 'IMU', icon: 'screen_rotation', action: DIAGNOSTIC_ACTION.IMU },
    { label: 'MQ-4 signal', icon: 'air', action: DIAGNOSTIC_ACTION.GAS },
    { label: 'Water probe', icon: 'water_drop', action: DIAGNOSTIC_ACTION.WATER },
    { label: 'Front range', icon: 'sensors', action: DIAGNOSTIC_ACTION.FRONT_RANGE },
    { label: 'VL53L0X', icon: 'radar', action: DIAGNOSTIC_ACTION.TOF },
    { label: 'Encoder snapshot', icon: 'speed', action: DIAGNOSTIC_ACTION.ENCODERS },
    { label: 'ESP-NOW link', icon: 'wifi_tethering', action: DIAGNOSTIC_ACTION.RADIO_LINK },
    { label: 'Motor state', icon: 'settings_input_component', action: DIAGNOSTIC_ACTION.MOTOR_STATE },
    { label: 'Sensor freshness', icon: 'schedule', action: DIAGNOSTIC_ACTION.SENSOR_FRESHNESS },
    { label: 'Calibration status', icon: 'rule', action: DIAGNOSTIC_ACTION.CALIBRATION_STATUS }
  ];

  function liveStatus(component) {
    if (!isConnected) return 'Offline';
    if (component === 'ESP32 Gateway') return 'Connected';
    if (component === 'ESP32 Rover') return telemetry?.source === 'LIVE' ? 'Live' : 'Waiting';
    if (component === 'MPU6050') return telemetry?.pitchDeg == null ? 'No data' : 'Live';
    if (component === 'VL53L0X ToF') return telemetry?.tofMm == null ? 'Standby' : 'Live';
    if (component === 'HC-SR04') return telemetry?.frontDistanceCm == null ? 'No data' : 'Live';
    if (component === 'MQ-4 module') return telemetry?.gasRaw == null ? 'No data' : 'Live';
    if (component === 'DHT22') return telemetry?.temperature == null ? 'No data' : 'Live';
    if (component === 'Water contact probe') return telemetry?.waterRaw == null ? 'No data' : 'Live';
    if (component.includes('encoder')) return telemetry?.encoderL == null ? 'No data' : 'Live';
    return 'Ready to test';
  }

  function resultColor(status) {
    if (status === 'PASS') return 'text-emerald-300';
    if (status === 'CHECK') return 'text-amber-300';
    return 'text-red-300';
  }

  async function copyLogs() {
    const text = diagnosticResults.map((item) =>
      `[${item.status}] ${item.label}: ${item.detail}`
    ).join('\n');
    await navigator.clipboard.writeText(text);
  }

  function startMotor(direction) {
    if (!gatewayArmed) return;
    const speed = Number(motorTestSpeed);
    const vectors = {
      forward: [speed, speed], reverse: [-speed, -speed],
      left: [-speed, speed], right: [speed, -speed]
    };
    onMotorStart(...vectors[direction]);
  }

  function moveServo(angle) {
    servoAngle = Number(angle);
    clearTimeout(servoMoveTimer);
    servoMoveTimer = setTimeout(() => {
      if (isConnected && !gatewayArmed)
        onRunDiagnostic(DIAGNOSTIC_ACTION.SERVO, Number(servoAngle));
    }, 120);
  }
</script>

<div class="h-full overflow-x-auto overflow-y-hidden bg-[var(--md-sys-color-surface-container-lowest)]">
  <div class="h-full min-w-[1240px] grid grid-cols-[300px_minmax(460px,0.9fr)_minmax(480px,1.15fr)]">
    <aside class="h-full min-h-0 overflow-y-auto border-r border-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface)]">
      <div class="px-5 py-4 border-b border-[var(--md-sys-color-outline-variant)] sticky top-0 bg-[var(--md-sys-color-surface)] z-10">
        <div class="flex items-center justify-between gap-3">
          <div><h2 class="text-lg font-bold">Hardware</h2><p class="text-xs text-[var(--md-sys-color-on-surface-variant)] mt-0.5">Live state + locked pin reference</p></div>
          <span class="material-symbols-rounded text-[var(--md-sys-color-primary)]">memory</span>
        </div>
      </div>
      <div class="divide-y divide-[var(--md-sys-color-outline-variant)]/60">
        {#each hardwareInventory as hw}
          <button type="button"
            class="w-full px-4 py-3 text-left flex items-center gap-3 hover:bg-[var(--md-sys-color-surface-container)] {selectedComponent === hw.component ? 'bg-[var(--md-sys-color-primary-container)] border-l-4 border-[var(--md-sys-color-primary)]' : 'border-l-4 border-transparent'}"
            onclick={() => selectedComponent = hw.component}>
            <div class="min-w-0 flex-1"><div class="text-sm font-semibold truncate">{hw.component}</div><div class="text-xs text-[var(--md-sys-color-on-surface-variant)] telemetry truncate">{hw.pin}</div></div>
            <span class="text-[11px] font-semibold whitespace-nowrap {liveStatus(hw.component) === 'Live' || liveStatus(hw.component) === 'Connected' ? 'text-[var(--ui-color-success)]' : 'text-[var(--md-sys-color-on-surface-variant)]'}">{liveStatus(hw.component)}</span>
          </button>
        {/each}
      </div>
      <div class="m-4 p-3 rounded-lg bg-[var(--md-sys-color-surface-container)] text-xs text-[var(--md-sys-color-on-surface-variant)]">
        Pin authority: <span class="font-semibold telemetry">{wiringAuthority.document}</span>
      </div>
    </aside>

    <section class="h-full min-h-0 overflow-y-auto p-5 border-r border-[var(--md-sys-color-outline-variant)] space-y-5">
      <div><h2 class="text-xl font-bold">Diagnostics</h2><p class="text-sm text-[var(--md-sys-color-on-surface-variant)] mt-1">Checks run on the real rover through the gateway. No demo values are used.</p></div>
      <div class="grid grid-cols-2 sm:grid-cols-3 gap-2">
        {#each checks as check}
          <button type="button" class="min-h-20 rounded-xl border border-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface)] px-3 py-3 text-left hover:border-[var(--md-sys-color-primary)] hover:bg-[var(--md-sys-color-primary-container)] disabled:opacity-45"
            disabled={!isConnected} onclick={() => onRunDiagnostic(check.action, 0)}>
            <span class="material-symbols-rounded text-[22px] text-[var(--md-sys-color-primary)]">{check.icon}</span><div class="text-sm font-semibold mt-1">{check.label}</div>
          </button>
        {/each}
      </div>

      <div class="rounded-xl border border-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface)] p-4 space-y-4">
        <div><h3 class="font-bold">Sensor head and indicators</h3><p class="text-xs text-[var(--md-sys-color-on-surface-variant)] mt-0.5">Disarm before moving the servo or testing outputs.</p></div>
        <label class="block"><div class="flex justify-between text-sm mb-2"><span>Servo angle — moves while sliding</span><strong class="telemetry">{servoAngle}°</strong></div><input class="w-full accent-[var(--md-sys-color-primary)]" type="range" min="30" max="150" step="5" value={servoAngle} oninput={(event) => moveServo(event.currentTarget.value)} disabled={!isConnected || gatewayArmed} /></label>
        <div class="grid grid-cols-3 gap-2">
          <button type="button" class="ui-button ui-button--tonal !h-9 text-xs" disabled={!isConnected || gatewayArmed} onclick={() => moveServo(35)}>Left 35°</button>
          <button type="button" class="ui-button ui-button--tonal !h-9 text-xs" disabled={!isConnected || gatewayArmed} onclick={() => moveServo(90)}>Center 90°</button>
          <button type="button" class="ui-button ui-button--tonal !h-9 text-xs" disabled={!isConnected || gatewayArmed} onclick={() => moveServo(145)}>Right 145°</button>
        </div>
        <div class="grid grid-cols-2 gap-2">
          <button type="button" class="ui-button ui-button--outlined !h-10 text-sm" disabled={!isConnected || gatewayArmed} onclick={() => onRunDiagnostic(DIAGNOSTIC_ACTION.SERVO, Number(servoAngle))}>Move servo</button>
          <button type="button" class="ui-button ui-button--outlined !h-10 text-sm" disabled={!isConnected || gatewayArmed} onclick={() => onRunDiagnostic(DIAGNOSTIC_ACTION.START_SCAN, 0)}>Run 7-angle scan</button>
          <button type="button" class="ui-button ui-button--outlined !h-10 text-sm" disabled={!isConnected || gatewayArmed} onclick={() => onRunDiagnostic(DIAGNOSTIC_ACTION.LEDS, 0)}>Flash LEDs</button>
          <button type="button" class="ui-button ui-button--outlined !h-10 text-sm" disabled={!isConnected || gatewayArmed} onclick={() => onRunDiagnostic(DIAGNOSTIC_ACTION.BUZZER, 1)}>Pulse buzzer</button>
        </div>
      </div>

      <div class="rounded-xl border border-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface)] p-4 space-y-4">
        <div class="flex items-start justify-between gap-3">
          <div><h3 class="font-bold">Motor response</h3><p class="text-xs text-[var(--md-sys-color-on-surface-variant)] mt-0.5">Hold a direction to run; releasing always sends stop.</p></div>
          <span class="text-xs font-bold px-2 py-1 rounded {gatewayArmed ? 'bg-[var(--ui-color-warning-container)] text-[var(--ui-color-on-warning-container)]' : 'bg-[var(--md-sys-color-surface-container-highest)] text-[var(--md-sys-color-on-surface-variant)]'}">{gatewayArmed ? 'ARMED' : 'ARM FIRST'}</span>
        </div>
        <label class="block"><div class="flex justify-between text-sm mb-2"><span>Test speed</span><strong class="telemetry">{motorTestSpeed}%</strong></div><input class="w-full accent-[var(--md-sys-color-primary)]" type="range" min="20" max="60" step="5" bind:value={motorTestSpeed} disabled={!gatewayArmed} /></label>
        <div class="grid grid-cols-3 gap-2 max-w-64 mx-auto select-none">
          <span></span><button type="button" class="ui-button ui-button--tonal !h-11" disabled={!gatewayArmed} onpointerdown={() => startMotor('forward')} onpointerup={onMotorStop} onpointerleave={onMotorStop} onpointercancel={onMotorStop}>↑</button><span></span>
          <button type="button" class="ui-button ui-button--tonal !h-11" disabled={!gatewayArmed} onpointerdown={() => startMotor('left')} onpointerup={onMotorStop} onpointerleave={onMotorStop} onpointercancel={onMotorStop}>←</button>
          <button type="button" class="ui-button !h-11 bg-[var(--md-sys-color-error-container)] text-[var(--md-sys-color-on-error-container)]" onclick={onMotorStop}>STOP</button>
          <button type="button" class="ui-button ui-button--tonal !h-11" disabled={!gatewayArmed} onpointerdown={() => startMotor('right')} onpointerup={onMotorStop} onpointerleave={onMotorStop} onpointercancel={onMotorStop}>→</button>
          <span></span><button type="button" class="ui-button ui-button--tonal !h-11" disabled={!gatewayArmed} onpointerdown={() => startMotor('reverse')} onpointerup={onMotorStop} onpointerleave={onMotorStop} onpointercancel={onMotorStop}>↓</button><span></span>
        </div>
        <div class="pt-3 border-t border-[var(--md-sys-color-outline-variant)]">
          <div class="text-xs font-bold uppercase tracking-wide text-[var(--md-sys-color-on-surface-variant)] mb-2">Individual wheel banks</div>
          <div class="grid grid-cols-2 gap-3">
            <div class="grid grid-cols-2 gap-2"><button type="button" class="ui-button ui-button--outlined !h-10 text-xs" disabled={!gatewayArmed} onpointerdown={() => onMotorStart(Number(motorTestSpeed), 0)} onpointerup={onMotorStop} onpointerleave={onMotorStop} onpointercancel={onMotorStop}>Left +</button><button type="button" class="ui-button ui-button--outlined !h-10 text-xs" disabled={!gatewayArmed} onpointerdown={() => onMotorStart(-Number(motorTestSpeed), 0)} onpointerup={onMotorStop} onpointerleave={onMotorStop} onpointercancel={onMotorStop}>Left −</button></div>
            <div class="grid grid-cols-2 gap-2"><button type="button" class="ui-button ui-button--outlined !h-10 text-xs" disabled={!gatewayArmed} onpointerdown={() => onMotorStart(0, Number(motorTestSpeed))} onpointerup={onMotorStop} onpointerleave={onMotorStop} onpointercancel={onMotorStop}>Right +</button><button type="button" class="ui-button ui-button--outlined !h-10 text-xs" disabled={!gatewayArmed} onpointerdown={() => onMotorStart(0, -Number(motorTestSpeed))} onpointerup={onMotorStop} onpointerleave={onMotorStop} onpointercancel={onMotorStop}>Right −</button></div>
          </div>
        </div>
      </div>
    </section>

    <section class="h-full min-h-0 overflow-hidden bg-black text-slate-200 flex flex-col">
      <div class="h-14 px-5 flex items-center justify-between border-b border-white/15 bg-slate-950 sticky top-0 z-10">
        <div class="flex items-center gap-2 text-sm font-semibold uppercase tracking-wider"><span class="material-symbols-rounded text-cyan-300">terminal</span>Live diagnostic results</div>
        <button type="button" class="text-sm text-slate-300 hover:text-white disabled:opacity-40" disabled={!diagnosticResults.length} onclick={copyLogs}>Copy</button>
      </div>
      <div class="flex-1 overflow-y-auto p-5 font-mono text-[13px] leading-relaxed">
        {#if diagnosticResults.length === 0}
          <div class="text-slate-400 max-w-xl"><div class="text-slate-200 font-semibold">Choose a diagnostic check to begin.</div><p class="mt-2">Results come from the rover over encrypted ESP-NOW. Encoder calibration controls are intentionally excluded while calibration work is in progress.</p></div>
        {:else}
          <div class="space-y-3">
            {#each diagnosticResults as result (result.id)}
              <div class="border-l-2 border-white/20 pl-3 py-1">
                <div class="flex items-center gap-2"><span class="font-bold {resultColor(result.status)}">[{result.status}]</span><span class="text-white font-semibold">{result.label}</span>{#if result.timestampMs !== null}<span class="text-slate-500 ml-auto">T+{(result.timestampMs / 1000).toFixed(3)}s</span>{/if}</div>
                <div class="text-cyan-200 mt-1 whitespace-pre-wrap">{result.detail}</div>
              </div>
            {/each}
          </div>
        {/if}
      </div>
    </section>
  </div>
</div>
