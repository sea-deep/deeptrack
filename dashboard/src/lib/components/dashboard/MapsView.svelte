<script>
  import { m3SlideFade, sendShared, receiveShared } from '$lib/utils/motion.js';

  let searchQuery = $state('');

  const savedMaps = [
    {
      id: 'MAP-DHN-01',
      name: 'North Gallery Seam 4 (Jharia)',
      date: '2026-08-24 21:40',
      areaSqM: 342,
      pointsCount: 1240,
      preview: 'M 10 30 L 40 30 L 40 10 L 80 10 L 80 50 L 10 50 Z'
    },
    {
      id: 'MAP-DHN-02',
      name: 'East Incline Shaft B (Bokaro)',
      date: '2026-08-23 18:15',
      areaSqM: 520,
      pointsCount: 2180,
      preview: 'M 10 20 L 50 20 L 70 50 L 30 50 Z'
    }
  ];

  let selectedMap = $state(savedMaps[0]);

  let filteredMaps = $derived(
    savedMaps.filter(m => {
      return m.name.toLowerCase().includes(searchQuery.toLowerCase()) || m.id.toLowerCase().includes(searchQuery.toLowerCase());
    })
  );

</script>

<div class="h-full flex flex-col overflow-hidden bg-[var(--md-sys-color-surface)]">
  <!-- Top Toolbar -->
  <div class="flex items-center justify-between px-6 py-4 border-b border-[var(--md-sys-color-outline-variant)] shrink-0">
    <h2 class="text-xl font-semibold text-[var(--md-sys-color-on-surface)]">
      Saved scans
    </h2>

    <div class="relative flex items-center w-64">
      <span class="material-symbols-rounded absolute left-3 text-lg text-[var(--md-sys-color-on-surface-variant)]">search</span>
      <input
        type="text"
        bind:value={searchQuery}
        placeholder="Search..."
        class="w-full h-9 pl-10 pr-4 rounded-md bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] text-sm focus:border-[var(--md-sys-color-primary)] focus:outline-none"
      />
    </div>
  </div>

  <div class="flex-1 flex overflow-hidden">
    <!-- Left List Pane -->
    <div class="w-full md:w-[360px] flex flex-col border-r border-[var(--md-sys-color-outline-variant)] bg-[var(--md-sys-color-surface-container-lowest)] overflow-y-auto">
      {#if filteredMaps.length === 0}
        <div class="flex-1 flex flex-col items-center justify-center text-[var(--md-sys-color-on-surface-variant)] p-8 text-center">
          <span class="material-symbols-rounded text-4xl mb-2 opacity-50">map</span>
          <p class="text-sm font-medium">No saved scans yet</p>
          <button type="button" class="mt-4 ui-button ui-button--tonal !h-9 text-sm" onclick={() => { /* route to console */ }}>Start a scan</button>
        </div>
      {:else}
        {#each filteredMaps as map, i}
          <button
            type="button"
            class="flex items-start gap-4 p-4 border-b border-[var(--md-sys-color-outline-variant)] text-left hover:bg-[var(--md-sys-color-surface-container)] transition-colors relative z-0"
            onclick={() => selectedMap = map}
          >
            {#if selectedMap?.id === map.id}
              <div class="absolute inset-0 bg-[var(--md-sys-color-primary-container)] border-l-4 border-l-[var(--md-sys-color-primary)] -z-10" in:receiveShared={{key: 'active-map'}} out:sendShared={{key: 'active-map'}}></div>
            {:else}
              <div class="absolute inset-0 border-l-4 border-l-transparent -z-10"></div>
            {/if}
            <div class="w-16 h-12 shrink-0 rounded bg-[var(--md-sys-color-surface)] border border-[var(--md-sys-color-outline-variant)] flex items-center justify-center">
              <svg viewBox="0 0 100 60" class="w-12 h-8 stroke-[var(--md-sys-color-primary)] fill-none stroke-2">
                <path d={map.preview} />
              </svg>
            </div>
            <div class="min-w-0">
              <h3 class="text-sm font-semibold text-[var(--md-sys-color-on-surface)] truncate">{map.name}</h3>
              <div class="text-sm text-[var(--md-sys-color-on-surface-variant)] mt-1 telemetry">{map.date}</div>
            </div>
          </button>
        {/each}
      {/if}
    </div>

    <!-- Right Preview Pane -->
    <div class="hidden md:flex flex-1 flex-col bg-[var(--md-sys-color-surface)]">
      {#if selectedMap}
        <div class="flex-1 flex flex-col p-6 gap-6 overflow-y-auto" in:m3SlideFade={{ y: 12, duration: 200 }}>
          
          <div class="flex items-start justify-between">
            <div>
              <h2 class="text-2xl font-bold">{selectedMap.name}</h2>
              <p class="text-sm text-[var(--md-sys-color-on-surface-variant)] mt-1 telemetry">ID: {selectedMap.id} · Recorded: {selectedMap.date}</p>
            </div>
            <div class="flex gap-2">
              <button type="button" class="ui-button ui-button--outlined !h-9 !px-4 text-sm"><span class="material-symbols-rounded text-[18px]">open_in_new</span> Open in Console</button>
              <button type="button" class="ui-button ui-button--outlined !h-9 w-9 p-0 flex items-center justify-center text-[var(--md-sys-color-error)] border-[var(--md-sys-color-outline-variant)] hover:bg-[var(--md-sys-color-error-container)] hover:text-[var(--md-sys-color-on-error-container)] hover:border-[var(--md-sys-color-error-container)]"><span class="material-symbols-rounded text-[18px]">delete</span></button>
            </div>
          </div>

          <div class="flex gap-6 text-sm border-y border-[var(--md-sys-color-outline-variant)] py-4">
            <div>
              <div class="text-sm text-[var(--md-sys-color-on-surface-variant)] mb-1">Estimated Area</div>
              <div class="font-semibold telemetry">{selectedMap.areaSqM} m²</div>
            </div>
            <div>
              <div class="text-sm text-[var(--md-sys-color-on-surface-variant)] mb-1">Points Recorded</div>
              <div class="font-semibold telemetry">{selectedMap.pointsCount}</div>
            </div>
          </div>

          <div class="flex-1 border border-[var(--md-sys-color-outline-variant)] rounded-xl bg-[var(--md-sys-color-surface-container-lowest)] relative overflow-hidden flex items-center justify-center min-h-[400px]">
             <svg viewBox="0 0 100 60" class="w-full h-full p-12 stroke-[var(--ui-brand-cyan)] fill-none stroke-[0.5]">
                <path d={selectedMap.preview} />
             </svg>
          </div>

        </div>
      {:else}
        <div class="flex-1 flex items-center justify-center text-[var(--md-sys-color-on-surface-variant)] text-sm">
          Select a scan to preview
        </div>
      {/if}
    </div>
  </div>
</div>
