<script>
  let {
    label,
    value,
    unit = '',
    icon = '',
    status = 'normal', // 'normal', 'warning', 'critical', 'success'
    subtext = '',
    limit = '',
    progress = null, // 0 to 100 optional bar fill
    thresholdPos = null, // 0 to 100 threshold line position
    onclick = undefined
  } = $props();

  let statusColor = $derived(
    status === 'critical' ? 'var(--md-sys-color-error)' :
    status === 'warning' ? 'var(--ui-color-warning)' :
    status === 'success' ? 'var(--ui-color-success)' :
    'var(--md-sys-color-primary)'
  );

  let statusBg = $derived(
    status === 'critical' ? 'var(--md-sys-color-error-container)' :
    status === 'warning' ? 'var(--ui-color-warning-container)' :
    status === 'success' ? 'var(--ui-color-success-container)' :
    'var(--md-sys-color-surface-container-highest)'
  );
</script>

<!-- svelte-ignore a11y_interactive_supports_focus -->
<div
  role="button"
  tabindex="0"
  class="h-[74px] px-3.5 py-2 rounded-2xl bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] hover:border-[var(--md-sys-color-primary)] flex flex-col justify-between relative overflow-hidden group select-none transition-[transform,border-color,box-shadow,background-color] duration-200 ease-[cubic-bezier(0.1,0.7,0.1,1)] {onclick ? 'cursor-pointer active:scale-[0.98] hover:-translate-y-0.5 hover:shadow-md' : ''}"
  onclick={onclick}
  onkeydown={(e) => { if (onclick && (e.key === 'Enter' || e.key === ' ')) { e.preventDefault(); onclick(); } }}
>
  <div class="flex items-center justify-between gap-3 w-full">
    <!-- Left: Metric Label & Numeric Value -->
    <div class="flex flex-col justify-center min-w-0 flex-1">
      <div class="flex items-center gap-1.5 metric-label text-[var(--md-sys-color-on-surface-variant)] truncate">
        {#if icon}
          <span class="material-symbols-rounded text-sm filled transition-colors duration-150" style="color: {status !== 'normal' ? statusColor : 'var(--ui-brand-cyan)'}">{icon}</span>
        {/if}
        <span class="truncate">{label}</span>
      </div>

      <div class="flex items-baseline gap-1 mt-0.5">
        <span class="metric-value text-[var(--md-sys-color-on-surface)] transition-colors duration-150" style="color: {status !== 'normal' ? statusColor : 'inherit'}">
          {value}
        </span>
        {#if unit}
          <span class="metric-unit text-[var(--md-sys-color-on-surface-variant)]">
            {unit}
          </span>
        {/if}
      </div>
    </div>

    <!-- Right: Subtext & Limit Indicator -->
    <div class="flex flex-col items-end justify-center shrink-0 text-right">
      {#if subtext}
        <span class="px-2 py-0.5 rounded-md text-[11px] font-medium transition-colors duration-150" style="background-color: {statusBg}; color: {statusColor}">
          {subtext}
        </span>
      {/if}
      {#if limit}
        <span class="telemetry text-[10px] text-[var(--md-sys-color-on-surface-variant)] mt-1 truncate">
          {limit}
        </span>
      {/if}
    </div>
  </div>

  {#if progress !== null}
    <!-- Integrated Threshold Progress Track -->
    <div class="relative h-1 w-full rounded-full bg-[var(--md-sys-color-surface-container-highest)] overflow-hidden mt-1">
      <div
        class="h-full rounded-full transition-[width,background-color] duration-250 ease-[cubic-bezier(0,0,0,1)]"
        style="width: {Math.min(100, Math.max(0, progress))}%; background-color: {status !== 'normal' ? statusColor : 'var(--ui-brand-cyan)'}"
      ></div>
      {#if thresholdPos !== null}
        <div
          class="absolute top-0 bottom-0 w-0.5 bg-[var(--ui-color-warning)]"
          style="left: {thresholdPos}%"
        ></div>
      {/if}
    </div>
  {/if}
</div>
