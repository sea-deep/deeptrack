<script>
  import RoverLogo from './RoverLogo.svelte';
</script>

<footer class="w-full bg-[var(--md-sys-color-surface-container-lowest)] border-t border-[var(--md-sys-color-outline-variant)] mt-auto">
  <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-12">
    <div class="grid grid-cols-1 md:grid-cols-[1.4fr_1fr] gap-10 md:gap-16 items-end">
      <div>
        <div class="flex items-center gap-3">
          <RoverLogo size={34} />
          <span class="font-bold text-xl tracking-tight text-[var(--md-sys-color-on-surface)]">DeepTrack</span>
        </div>
        <p class="mt-5 max-w-lg text-sm leading-6 text-[var(--md-sys-color-on-surface-variant)]">
          A student-built rover experiment for offline sensing, safer remote exploration and learning by making.
        </p>
      </div>

      <nav class="flex md:justify-end flex-wrap gap-x-6 gap-y-3 text-sm" aria-label="Footer navigation">
        <a href="/dashboard" class="text-[var(--md-sys-color-on-surface)] hover:text-[var(--md-sys-color-primary)] transition-colors">Dashboard</a>
        <a href="/#hardware" class="text-[var(--md-sys-color-on-surface)] hover:text-[var(--md-sys-color-primary)] transition-colors">Hardware</a>
        <a href="/images/components/SOURCES.md" class="text-[var(--md-sys-color-on-surface)] hover:text-[var(--md-sys-color-primary)] transition-colors">Image credits</a>
        <a href="/auth" class="text-[var(--md-sys-color-on-surface)] hover:text-[var(--md-sys-color-primary)] transition-colors">Sign in</a>
      </nav>
    </div>

    <div class="mt-10 pt-5 border-t border-[var(--md-sys-color-outline-variant)] flex flex-col sm:flex-row sm:items-center justify-between gap-3 text-xs text-[var(--md-sys-color-on-surface-variant)]">
      <span>V0 · Built and tested on a workbench.</span>

    </div>
  </div>
</footer>
