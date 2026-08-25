<script>
  import RoverLogo from './RoverLogo.svelte';
</script>

<footer class="w-full bg-[var(--md-sys-color-surface-container-lowest)] border-t border-[var(--md-sys-color-outline-variant)] py-10 mt-auto">
  <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 flex flex-col md:flex-row items-center justify-between gap-6 text-sm text-[var(--md-sys-color-on-surface-variant)]">
    <div class="flex items-center gap-3">
      <RoverLogo size={28} />
      <div>
        <span class="font-bold text-[var(--md-sys-color-on-surface)]">DeepTrack System</span> — Subterranean Mine Search & Rescue
      </div>
    </div>

    <div class="flex flex-wrap items-center justify-center gap-6 text-xs">
      <span class="inline-flex items-center gap-1.5 text-[var(--ui-color-success)] font-medium">
        <span class="w-2 h-2 rounded-full bg-[var(--ui-color-success)]"></span>
        Telemetry Mesh Online (HiveMQ / Serial)
      </span>
      <a href="#safety" class="hover:underline hover:text-[var(--md-sys-color-on-surface)] transition-colors">Safety Protocols</a>
      <a href="#specs" class="hover:underline hover:text-[var(--md-sys-color-on-surface)] transition-colors">Sensor Architecture</a>
      <span>v2.4-PROD</span>
    </div>
  </div>
</footer>
