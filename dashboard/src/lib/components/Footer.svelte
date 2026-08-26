<script>
  import RoverLogo from './RoverLogo.svelte';
</script>

<footer class="w-full bg-[var(--md-sys-color-surface-container-lowest)] border-t border-[var(--md-sys-color-outline-variant)] py-10 mt-auto">
  <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 flex flex-col md:flex-row items-center justify-between gap-6 text-sm text-[var(--md-sys-color-on-surface-variant)]">
    <div class="flex items-center gap-3">
      <RoverLogo size={28} />
      <div>
        <span class="font-bold text-[var(--md-sys-color-on-surface)]">DeepTrack</span> — Mine rescue rover
      </div>
    </div>

    <div class="flex flex-wrap items-center justify-center gap-6 text-xs">
      <span class="inline-flex items-center gap-1.5 font-medium">
        <span class="w-2 h-2 rounded-full bg-[var(--md-sys-color-outline)]"></span>
        Hackathon prototype · gateway not connected
      </span>
      <a href="#safety" class="hover:underline hover:text-[var(--md-sys-color-on-surface)] transition-colors">Safety</a>
      <a href="#specs" class="hover:underline hover:text-[var(--md-sys-color-on-surface)] transition-colors">Sensors</a>
      <span>Hardware testing in progress</span>
    </div>
  </div>
</footer>
