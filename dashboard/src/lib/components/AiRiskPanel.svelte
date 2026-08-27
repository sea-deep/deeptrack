<script>
  /** @type {{result:any,isDemo?:boolean,demoActive?:boolean,onToggleDemo?:()=>void}} */
  let {
    result,
    isDemo = false,
    demoActive = false,
    onToggleDemo = () => {}
  } = $props();

  let isOpen = $state(false);
  /** @type {Record<string, {accent:string,soft:string,onSoft:string,icon:string}>} */
  const TONES = {
    NORMAL: {
      accent: 'var(--ui-color-success)',
      soft: 'var(--ui-color-success-container)',
      onSoft: 'var(--ui-color-on-success-container)',
      icon: 'verified_user'
    },
    WATCH: {
      accent: 'var(--ui-color-warning)',
      soft: 'var(--ui-color-warning-container)',
      onSoft: 'var(--ui-color-on-warning-container)',
      icon: 'visibility'
    },
    HIGH: {
      accent: '#d97706',
      soft: '#ffedd5',
      onSoft: '#7c2d12',
      icon: 'warning'
    },
    CRITICAL: {
      accent: 'var(--md-sys-color-error)',
      soft: 'var(--md-sys-color-error-container)',
      onSoft: 'var(--md-sys-color-on-error-container)',
      icon: 'crisis_alert'
    }
  };
  let tone = $derived(TONES[result?.state] || {
    accent: 'var(--md-sys-color-outline)',
    soft: 'var(--md-sys-color-surface-container)',
    onSoft: 'var(--md-sys-color-on-surface-variant)',
    icon: 'psychology'
  });

  let sparklinePoints = $derived.by(() => {
    const values = result?.scoreHistory || [];
    if (values.length < 2) return '0,30 120,30';
    return values.map((/** @type {number} */ value, /** @type {number} */ index) => {
      const x = index / Math.max(1, values.length - 1) * 120;
      const y = 38 - Math.min(100, Math.max(0, value)) / 100 * 34;
      return `${x.toFixed(1)},${y.toFixed(1)}`;
    }).join(' ');
  });
</script>

<button
  type="button"
  class="text-left min-w-0 flex flex-col gap-1 px-4 py-4 cursor-pointer hover:bg-[var(--md-sys-color-surface-container-highest)] transition-colors duration-150 active:bg-[var(--md-sys-color-surface-variant)]"
  onclick={() => isOpen = true}
  aria-label={`Open DeepTrack Sentinel details: ${result?.state || 'NORMAL'}, ${result?.score || 0} out of 100`}
>
  <div class="flex items-center justify-between gap-2 text-sm font-semibold text-[var(--md-sys-color-on-surface-variant)]">
    <span class="flex min-w-0 items-center gap-1.5">
      <span class="material-symbols-rounded text-[20px]" style={`color:${tone.accent}`}>psychology</span>
      <span class="truncate">Sentinel AI</span>
    </span>
    <span class="rounded-full px-2 py-0.5 text-[10px] font-bold tracking-wide" style={`background:${tone.soft};color:${tone.onSoft}`}>{demoActive ? 'SIM ANOMALY' : 'ADVISORY'}</span>
  </div>

  <div class="flex items-end justify-between gap-2">
    <div class="flex items-baseline gap-1">
      <span class="telemetry text-3xl font-bold" style={`color:${tone.accent}`}>{result?.score ?? 0}</span>
      <span class="text-sm font-medium text-[var(--md-sys-color-on-surface-variant)]">/100</span>
    </div>
    <span class="mb-1 rounded-md px-2 py-0.5 text-xs font-bold" style={`background:${tone.soft};color:${tone.onSoft}`}>{result?.state || 'NORMAL'}</span>
  </div>

  <div class="flex items-center gap-1.5 text-xs font-semibold text-[var(--md-sys-color-on-surface-variant)]">
    <span class="material-symbols-rounded text-sm" style={`color:${tone.accent}`}>
      {result?.trend === 'RISING' ? 'trending_up' : result?.trend === 'FALLING' ? 'trending_down' : 'trending_flat'}
    </span>
    {result?.trend || 'STABLE'} · {result?.confidence ?? 0}% confidence
  </div>
  <span class="line-clamp-2 text-xs leading-snug text-[var(--md-sys-color-on-surface-variant)]">
    {result?.reasons?.[0] || 'Waiting for sensor windows.'}
  </span>
</button>

{#if isOpen}
  <div class="fixed inset-0 z-[100] grid place-items-center p-4 sm:p-6">
    <button class="absolute inset-0 bg-black/55 backdrop-blur-sm" aria-label="Close Sentinel details" onclick={() => isOpen = false}></button>
    <div
      role="dialog"
      aria-modal="true"
      aria-labelledby="sentinel-title"
      class="relative z-10 max-h-[90vh] w-full max-w-2xl overflow-y-auto rounded-2xl border border-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface)] p-5 shadow-2xl sm:p-6"
    >
      <div class="flex items-start justify-between gap-4">
        <div>
          <div class="mb-1 flex items-center gap-2">
            <span class="material-symbols-rounded text-2xl" style={`color:${tone.accent}`}>psychology</span>
            <h2 id="sentinel-title" class="text-xl font-bold">DeepTrack Sentinel</h2>
            <span class="rounded-full px-2 py-0.5 text-xs font-bold" style={`background:${tone.soft};color:${tone.onSoft}`}>OFFLINE AI</span>
          </div>
          <p class="text-sm text-[var(--md-sys-color-on-surface-variant)]">Explainable multi-sensor anomaly advisory</p>
        </div>
        <button type="button" class="grid h-9 w-9 place-items-center rounded-full hover:bg-[var(--md-sys-color-surface-container)]" aria-label="Close" onclick={() => isOpen = false}>
          <span class="material-symbols-rounded">close</span>
        </button>
      </div>

      <div class="mt-5 grid gap-4 sm:grid-cols-[180px_1fr]">
        <div class="rounded-xl p-4 text-center" style={`background:${tone.soft};color:${tone.onSoft}`}>
          <div class="mx-auto grid h-28 w-28 place-items-center rounded-full" style={`background:conic-gradient(${tone.accent} ${(result?.score || 0) * 3.6}deg, color-mix(in srgb, ${tone.accent} 18%, transparent) 0)`}>
            <div class="grid h-20 w-20 place-items-center rounded-full bg-[var(--md-sys-color-surface)]">
              <div>
                <div class="telemetry text-3xl font-bold" style={`color:${tone.accent}`}>{result?.score ?? 0}</div>
                <div class="text-[10px] font-bold uppercase tracking-wide text-[var(--md-sys-color-on-surface-variant)]">Risk score</div>
              </div>
            </div>
          </div>
          <div class="mt-3 text-lg font-bold">{result?.state || 'NORMAL'}</div>
          <div class="text-xs font-medium">{result?.confidence ?? 0}% confidence</div>
        </div>

        <div class="rounded-xl border border-[var(--md-sys-color-outline-variant)] p-4">
          <div class="flex items-center justify-between gap-3">
            <div>
              <div class="text-xs font-bold uppercase tracking-wide text-[var(--md-sys-color-on-surface-variant)]">30-second trend</div>
              <div class="mt-0.5 font-bold">{result?.trend || 'STABLE'}</div>
            </div>
            <div class="text-right text-xs text-[var(--md-sys-color-on-surface-variant)]">
              <div>{result?.availableFeatures ?? 0}/{result?.totalFeatures ?? 0} features</div>
              <div>{Math.round((result?.baselineProgress || 0) * 100)}% baseline</div>
            </div>
          </div>
          <svg class="mt-3 h-14 w-full" viewBox="0 0 120 42" preserveAspectRatio="none" aria-label="Sentinel score trend">
            <path d="M0 29 H120" stroke="var(--md-sys-color-outline-variant)" stroke-width="1" stroke-dasharray="3 3" />
            <polyline points={sparklinePoints} fill="none" stroke={tone.accent} stroke-width="2.5" vector-effect="non-scaling-stroke" />
          </svg>
        </div>
      </div>

      <div class="mt-4 grid gap-4 sm:grid-cols-2">
        <div class="rounded-xl bg-[var(--md-sys-color-surface-container-low)] p-4">
          <h3 class="font-bold">Why Sentinel flagged this</h3>
          <ul class="mt-2 space-y-2 text-sm text-[var(--md-sys-color-on-surface-variant)]">
            {#each result?.reasons || [] as reason}
              <li class="flex gap-2"><span class="mt-1 h-2 w-2 shrink-0 rounded-full" style={`background:${tone.accent}`}></span><span>{reason}</span></li>
            {/each}
          </ul>
        </div>

        <div class="rounded-xl bg-[var(--md-sys-color-surface-container-low)] p-4">
          <h3 class="font-bold">Contributing signals</h3>
          <div class="mt-2 space-y-2">
            {#each (result?.featureContributions || []).slice(0, 4) as item}
              <div>
                <div class="mb-1 flex justify-between gap-2 text-xs"><span>{item.label}</span><strong>{Math.round(item.contribution)}%</strong></div>
                <div class="h-1.5 overflow-hidden rounded-full bg-[var(--md-sys-color-surface-container-highest)]"><div class="h-full rounded-full" style={`width:${Math.round(item.contribution)}%;background:${tone.accent}`}></div></div>
              </div>
            {/each}
          </div>
        </div>
      </div>

      <div class="mt-4 rounded-xl border p-4" style={`border-color:${tone.accent};background:${tone.soft};color:${tone.onSoft}`}>
        <div class="flex gap-3">
          <span class="material-symbols-rounded">assistant_direction</span>
          <div><div class="font-bold">Suggested operator action</div><p class="mt-1 text-sm">{result?.suggestedAction}</p></div>
        </div>
      </div>

      {#if isDemo}
        <div class="mt-4 flex flex-wrap items-center justify-between gap-3 rounded-xl border border-dashed border-[var(--md-sys-color-outline)] p-3">
          <div><div class="text-sm font-bold">Simulated environmental anomaly</div><div class="text-xs text-[var(--md-sys-color-on-surface-variant)]">Gradually changes several demo-only sensor streams.</div></div>
          <button type="button" class="ui-button ui-button--tonal !h-9 text-sm" onclick={onToggleDemo}>
            <span class="material-symbols-rounded text-lg">{demoActive ? 'stop' : 'science'}</span>
            {demoActive ? 'End scenario' : 'Run scenario'}
          </button>
        </div>
      {/if}

      <div class="mt-4 flex flex-col gap-1 border-t border-[var(--md-sys-color-outline-variant)] pt-3 text-xs text-[var(--md-sys-color-on-surface-variant)] sm:flex-row sm:items-center sm:justify-between">
        <span>Model {result?.modelVersion || 'loading'} · {result?.trainingDate || 'date unavailable'}</span>
        <strong>Advisory only · no motor authority</strong>
      </div>
      <p class="mt-2 text-xs text-[var(--md-sys-color-on-surface-variant)]">MQ-4 results are environmental anomalies from an uncalibrated raw signal—not methane concentration or proof of a gas leak.</p>
    </div>
  </div>
{/if}
