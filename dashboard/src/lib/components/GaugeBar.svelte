<script>
  let {
    label,
    value,
    max = 100,
    unit = '',
    threshold = null,
    icon = ''
  } = $props();

  let percent = $derived(Math.min(100, Math.max(0, (value / max) * 100)));
  let isExceeded = $derived(threshold !== null && value >= threshold);
</script>

<div class="p-3.5 rounded-xl bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] transition-colors duration-150">
  <div class="flex items-center justify-between gap-2 mb-2 text-xs">
    <div class="flex items-center gap-1.5 metric-label text-[var(--md-sys-color-on-surface-variant)]">
      {#if icon}
        <span class="material-symbols-rounded text-sm">{icon}</span>
      {/if}
      <span>{label}</span>
    </div>
    <div class="telemetry text-xs font-semibold" class:text-[var(--md-sys-color-error)]={isExceeded} class:text-[var(--md-sys-color-on-surface)]={!isExceeded}>
      {typeof value === 'number' ? value.toFixed(1) : value} <span class="metric-unit text-xs text-[var(--md-sys-color-on-surface-variant)]">{unit}</span>
    </div>
  </div>

  <!-- Progress Bar Container (ANIMATION.md Section 7 & 29: Monotonic progress fill, no overshoot) -->
  <div class="relative h-2 w-full rounded-full bg-[var(--md-sys-color-surface-container-highest)] overflow-hidden">
    <div
      class="h-full rounded-full transition-[width,background-color] duration-250 ease-[cubic-bezier(0,0,0,1)]"
      class:bg-[var(--md-sys-color-error)]={isExceeded}
      class:bg-[var(--ui-brand-cyan)]={!isExceeded}
      style="width: {percent}%"
    ></div>

    {#if threshold !== null}
      <!-- Threshold Marker -->
      <div
        class="absolute top-0 bottom-0 w-0.5 bg-[var(--ui-color-warning)] shadow-sm transition-[left] duration-200"
        style="left: {(threshold / max) * 100}%"
        title="Warning threshold: {threshold}{unit}"
      ></div>
    {/if}
  </div>
</div>
