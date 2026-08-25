<script>
  import { fade } from 'svelte/transition';
  import { m3DialogScaleFade } from '$lib/utils/motion.js';

  /**
   * @typedef {Object} SensorData
   * @property {string} title
   * @property {string} sensorIc
   * @property {string} pinout
   * @property {string} sampling
   * @property {string} currentVal
   * @property {string} unit
   * @property {number[]} history
   * @property {string} status
   * @property {string} dgmsRule
   * @property {string} actionThreshold
   * @property {string} dangerThreshold
   */

  let {
    isOpen = false,
    sensor = /** @type {SensorData | null} */ (null),
    onClose = () => {}
  } = $props();

  /** @param {KeyboardEvent} e */
  function handleKeydown(e) {
    if (isOpen && e.key === 'Escape') {
      onClose();
    }
  }

  // Calculate SVG line path from history points
  let chartData = $derived.by(() => {
    if (!sensor || !sensor.history || sensor.history.length === 0) {
      return { path: '', area: '', min: 0, max: 100, lastX: 0, lastY: 0 };
    }

    const data = sensor.history;
    const w = 500;
    const h = 140;
    const pad = 12;

    const min = Math.min(...data) * 0.95;
    const max = Math.max(...data) * 1.05 || min + 10;
    const range = max - min || 1;

    const points = data.map((/** @type {number} */ val, /** @type {number} */ i) => {
      const x = pad + (i / (data.length - 1 || 1)) * (w - pad * 2);
      const y = h - pad - ((val - min) / range) * (h - pad * 2);
      return { x, y };
    });

    let path = `M ${points[0].x} ${points[0].y}`;
    for (let i = 1; i < points.length; i++) {
      const prev = points[i - 1];
      const curr = points[i];
      const cp1x = prev.x + (curr.x - prev.x) / 2;
      const cp1y = prev.y;
      const cp2x = prev.x + (curr.x - prev.x) / 2;
      const cp2y = curr.y;
      path += ` C ${cp1x} ${cp1y}, ${cp2x} ${cp2y}, ${curr.x} ${curr.y}`;
    }

    const last = points[points.length - 1];
    const area = `${path} L ${last.x} ${h - pad} L ${points[0].x} ${h - pad} Z`;

    return {
      path,
      area,
      min: Math.min(...data),
      max: Math.max(...data),
      lastX: last.x,
      lastY: last.y
    };
  });
</script>

<svelte:window onkeydown={handleKeydown} />

{#if isOpen && sensor}
  <!-- Backdrop Scrim with Material 3 Fade -->
  <!-- svelte-ignore a11y_click_events_have_key_events, a11y_no_noninteractive_element_interactions, a11y_interactive_supports_focus -->
  <div
    class="fixed inset-0 z-[500] bg-black/60 backdrop-blur-sm flex items-center justify-center p-4"
    role="dialog"
    tabindex="-1"
    aria-modal="true"
    aria-labelledby="modal-sensor-title"
    transition:fade={{ duration: 150 }}
    onclick={(e) => { if (e.target === e.currentTarget) onClose(); }}
  >
    <!-- Modal Card (28px radius, GPU-accelerated m3DialogScaleFade) -->
    <div
      class="relative w-full max-w-xl bg-[var(--md-sys-color-surface)] pointer-events-auto text-[var(--md-sys-color-on-surface)] rounded-[28px] border border-[var(--md-sys-color-outline-variant)] shadow-2xl p-6 flex flex-col gap-5 overflow-hidden"
      transition:m3DialogScaleFade={{ duration: 250 }}
    >
      
      <!-- Top Title & Dismiss -->
      <div class="flex items-start justify-between border-b border-[var(--md-sys-color-outline-variant)] pb-4">
        <div>
          <div class="flex items-center gap-2">
            <h2 id="modal-sensor-title" class="text-xl font-bold font-headline">
              {sensor.title}
            </h2>
            <span
              class="text-sm px-2.5 py-0.5 rounded-full font-medium transition-colors duration-200 capitalize"
              class:bg-[var(--md-sys-color-error-container)]={sensor.status === 'critical'}
              class:text-[var(--md-sys-color-on-error-container)]={sensor.status === 'critical'}
              class:bg-[var(--ui-color-warning-container)]={sensor.status === 'warning'}
              class:text-[var(--ui-color-on-warning-container)]={sensor.status === 'warning'}
              class:bg-[var(--ui-color-success-container)]={sensor.status === 'normal' || sensor.status === 'success'}
              class:text-[var(--ui-color-on-success-container)]={sensor.status === 'normal' || sensor.status === 'success'}
            >
              {sensor.status}
            </span>
          </div>
          <p class="text-sm text-[var(--md-sys-color-on-surface-variant)] mt-1 font-mono">
            IC: {sensor.sensorIc} • Pin: {sensor.pinout} • Polling: {sensor.sampling}
          </p>
        </div>

        <button
          type="button"
          class="w-9 h-9 rounded-full flex items-center justify-center text-[var(--md-sys-color-on-surface-variant)] hover:bg-[var(--md-sys-color-surface-container-highest)] hover:text-[var(--md-sys-color-on-surface)] transition-colors duration-150 active:scale-95"
          onclick={() => onClose()}
          aria-label="Close dialog"
        >
          <span class="material-symbols-rounded text-xl">close</span>
        </button>
      </div>

      <!-- Real-Time Rolling Trend Waveform -->
      <div class="p-4 rounded-2xl bg-[var(--md-sys-color-surface-container-lowest)] border border-[var(--md-sys-color-outline-variant)]">
        <div class="flex items-baseline justify-between mb-2">
          <div class="section-title text-[var(--md-sys-color-on-surface)]">
            Live 30-sample rolling trend
          </div>
          <div class="text-sm text-[var(--md-sys-color-on-surface-variant)]">
            Current: <strong class="telemetry text-lg text-[var(--md-sys-color-primary)] font-semibold">{sensor.currentVal.toString().replace(sensor.unit, '').trim()}</strong> <span class="metric-unit">{sensor.unit}</span>
          </div>
        </div>

        <!-- SVG Line Chart -->
        <div class="relative w-full h-[140px]">
          <svg viewBox="0 0 500 140" class="w-full h-full overflow-visible">
            <defs>
              <linearGradient id="areaGradient" x1="0" y1="0" x2="0" y2="1">
                <stop offset="0%" stop-color="var(--ui-brand-cyan)" stop-opacity="0.35" />
                <stop offset="100%" stop-color="var(--ui-brand-cyan)" stop-opacity="0.0" />
              </linearGradient>
            </defs>

            <!-- Guide grid lines -->
            <line x1="12" y1="20" x2="488" y2="20" stroke="var(--md-sys-color-outline-variant)" stroke-dasharray="3,3" stroke-width="1" />
            <line x1="12" y1="70" x2="488" y2="70" stroke="var(--md-sys-color-outline-variant)" stroke-dasharray="3,3" stroke-width="1" />
            <line x1="12" y1="120" x2="488" y2="120" stroke="var(--md-sys-color-outline-variant)" stroke-dasharray="3,3" stroke-width="1" />

            <!-- Gradient Area Fill -->
            <path d={chartData.area} fill="url(#areaGradient)" class="transition-all duration-200" />

            <!-- Waveform Stroke -->
            <path d={chartData.path} fill="none" stroke="var(--ui-brand-cyan)" stroke-width="2.5" stroke-linecap="round" class="transition-all duration-200" />

            <!-- Current Head Dot -->
            {#if chartData.lastX > 0}
              <circle cx={chartData.lastX} cy={chartData.lastY} r="5" fill="var(--ui-brand-cyan)" class="transition-all duration-150" />
              <circle cx={chartData.lastX} cy={chartData.lastY} r="9" fill="var(--ui-brand-cyan)" opacity="0.3" class="animate-ping" />
            {/if}
          </svg>
        </div>

        <!-- Axis Range Labels -->
        <div class="flex justify-between text-sm text-[var(--md-sys-color-on-surface-variant)] mt-1.5 pt-1 border-t border-[var(--md-sys-color-outline-variant)]">
          <span>Min: <strong class="telemetry">{chartData.min.toFixed(1)}</strong> {sensor.unit}</span>
          <span class="text-sm">Window: ~15 seconds</span>
          <span>Max: <strong class="telemetry">{chartData.max.toFixed(1)}</strong> {sensor.unit}</span>
        </div>
      </div>

      <!-- Real Operational Safety Rules & Limits -->
      <div class="grid grid-cols-1 sm:grid-cols-2 gap-3 text-sm">
        <div class="p-3.5 rounded-xl bg-[var(--md-sys-color-surface-container-high)] border border-[var(--md-sys-color-outline-variant)] flex flex-col justify-between">
          <div class="font-semibold text-[var(--md-sys-color-on-surface)] mb-1 flex items-center gap-1.5">
            <span class="material-symbols-rounded text-sm filled text-[var(--ui-brand-cyan)]">policy</span>
            Hazard mitigation policy
          </div>
          <p class="text-sm text-[var(--md-sys-color-on-surface-variant)] leading-relaxed">
            {sensor.dgmsRule}
          </p>
        </div>

        <div class="p-3.5 rounded-xl bg-[var(--md-sys-color-surface-container-high)] border border-[var(--md-sys-color-outline-variant)] flex flex-col justify-between">
          <div class="font-semibold text-[var(--md-sys-color-on-surface)] mb-1 flex items-center gap-1.5">
            <span class="material-symbols-rounded text-sm filled text-[var(--ui-color-warning)]">alarm</span>
            Operational thresholds
          </div>
          <div class="text-sm space-y-1 text-[var(--md-sys-color-on-surface-variant)]">
            <div>• Advisory: <strong class="telemetry text-[var(--ui-color-warning)]">{sensor.actionThreshold}</strong></div>
            <div>• Cutoff trigger: <strong class="telemetry text-[var(--md-sys-color-error)]">{sensor.dangerThreshold}</strong></div>
          </div>
        </div>
      </div>

      <!-- Close Action -->
      <div class="flex justify-end pt-1">
        <button
          type="button"
          class="ui-button ui-button--tonal text-sm !h-9 !px-5 transition-all duration-200 active:scale-95"
          onclick={() => onClose()}
        >
          Dismiss inspection
        </button>
      </div>
    </div>
  </div>
{/if}
