<script>
  import { m3SlideFade, sendShared, receiveShared } from '$lib/utils/motion.js';

  let filter = $state('ALL'); // ALL, WARNING, ERROR

  const logs = [
    { id: 1, ts: '10:42:15.204', type: 'INFO', source: 'Connection', icon: 'usb', msg: 'Web Serial port opened at 115200 baud.' },
    { id: 2, ts: '10:42:15.520', type: 'INFO', source: 'System', icon: 'memory', msg: 'DT-ALPHA-01 firmware v1.2.0 initialized.' },
    { id: 3, ts: '10:43:02.100', type: 'INFO', source: 'Drive', icon: 'sports_esports', msg: 'Control mode transitioned to MANUAL.' },
    { id: 4, ts: '10:45:12.800', type: 'WARNING', source: 'Gas', icon: 'air', msg: 'MQ-4 raw reading elevated (1024 ADC).' },
    { id: 5, ts: '10:46:00.000', type: 'ERROR', source: 'Clearance', icon: 'sensors', msg: 'Failsafe brake: Front distance < 15cm.' },
    { id: 6, ts: '10:46:01.200', type: 'INFO', source: 'System', icon: 'front_hand', msg: 'Hardware E-Stop activated by user.' },
    { id: 7, ts: '10:46:15.000', type: 'INFO', source: 'System', icon: 'check_circle', msg: 'E-Stop reset. Resuming nominal operation.' }
  ].reverse();

  let filteredLogs = $derived(
    logs.filter(l => filter === 'ALL' ? true : filter === 'ERROR' ? l.type === 'ERROR' : (l.type === 'WARNING' || l.type === 'ERROR'))
  );

  /** @param {string} type */ function getSeverityClass(type) {
    if (type === 'ERROR') return 'text-[var(--md-sys-color-error)] font-semibold';
    if (type === 'WARNING') return 'text-[var(--ui-color-warning)] font-semibold';
    return 'text-[var(--md-sys-color-on-surface)]';
  }
  
  /** @param {string} type */ function getSeverityBg(type) {
    if (type === 'ERROR') return 'bg-[var(--md-sys-color-error-container)]/20';
    if (type === 'WARNING') return 'bg-[var(--ui-color-warning-container)]/30';
    return 'hover:bg-[var(--md-sys-color-surface-container)]';
  }
</script>

<div class="h-full flex flex-col bg-[var(--md-sys-color-surface)]">
  <!-- Top Toolbar -->
  <div class="flex items-center justify-between px-6 py-4 border-b border-[var(--md-sys-color-outline-variant)] shrink-0">
    <h2 class="text-2xl font-bold text-[var(--md-sys-color-on-surface)]">
      Event log
    </h2>

    <div class="flex items-center gap-2">
      <div class="flex items-center gap-1 bg-[var(--md-sys-color-surface-container)] p-1 rounded-lg">
        {#each [{id: 'ALL', label: 'All'}, {id: 'WARNING', label: 'Warnings'}, {id: 'ERROR', label: 'Errors'}] as tab}
          <button
            type="button"
            class="relative px-3 py-1.5 rounded text-sm font-medium transition-colors z-0 {filter === tab.id ? (tab.id==='WARNING' ? 'text-[var(--ui-color-on-warning-container)]' : tab.id==='ERROR' ? 'text-[var(--md-sys-color-on-error-container)]' : 'text-[var(--md-sys-color-on-surface)]') : 'text-[var(--md-sys-color-on-surface-variant)] hover:bg-[var(--md-sys-color-surface-container-highest)]'}"
            onclick={() => filter = tab.id}
          >
            {#if filter === tab.id}
              <div
                class="absolute inset-0 rounded -z-10 shadow-sm {tab.id==='WARNING' ? 'bg-[var(--ui-color-warning-container)]' : tab.id==='ERROR' ? 'bg-[var(--md-sys-color-error-container)]' : 'bg-[var(--md-sys-color-surface-container-highest)]'}"
                in:receiveShared={{key: 'logs-tab'}}
                out:sendShared={{key: 'logs-tab'}}
              ></div>
            {/if}
            <span class="relative z-10">{tab.label}</span>
          </button>
        {/each}
      </div>
      <div class="w-px h-4 bg-[var(--md-sys-color-outline-variant)] mx-2"></div>
      <button type="button" class="ui-button ui-button--outlined !h-9 !px-4 text-sm"><span class="material-symbols-rounded text-[18px]">download</span> Export</button>
      <button type="button" class="ui-button ui-button--outlined !h-9 !px-4 text-sm"><span class="material-symbols-rounded text-[18px]">delete</span> Clear</button>
    </div>
  </div>

  <div class="flex-1 overflow-y-auto p-6">
    <div class="text-sm font-semibold text-[var(--md-sys-color-on-surface-variant)] uppercase tracking-wider mb-4 border-b border-[var(--md-sys-color-outline-variant)] pb-2 flex items-center justify-between">
      <span>Timestamp</span>
      <span class="w-[120px] ml-4 text-center">Source</span>
      <span class="flex-1 ml-6">Message</span>
    </div>
    
    <div class="flex flex-col">
      {#each filteredLogs as log (log.id)}
        <div class="group flex items-center py-2.5 px-2 border-b border-[var(--md-sys-color-outline-variant)]/50 transition-colors rounded-sm {getSeverityBg(log.type)}" in:m3SlideFade={{ y: 5, duration: 150 }}>
          <div class="w-28 shrink-0 text-sm text-[var(--md-sys-color-on-surface-variant)] telemetry">{log.ts}</div>
          <div class="w-[120px] ml-4 shrink-0 flex items-center gap-1.5 px-2.5 py-1 rounded bg-[var(--md-sys-color-surface-container-highest)] text-[var(--md-sys-color-on-surface)]">
            <span class="material-symbols-rounded text-[18px] text-[var(--md-sys-color-on-surface-variant)]">{log.icon}</span>
            <span class="text-sm font-medium truncate">{log.source}</span>
          </div>
          <div class="flex-1 ml-6 text-sm {getSeverityClass(log.type)}">{log.msg}</div>
        </div>
      {/each}
      {#if filteredLogs.length === 0}
         <div class="text-sm text-[var(--md-sys-color-on-surface-variant)] py-8 text-center">No logs match the current filter.</div>
      {/if}
    </div>
  </div>
</div>
