<script>
  import RoverLogo from './RoverLogo.svelte';
  import { getTheme, toggleTheme } from '$lib/utils/theme.js';
  import { onMount } from 'svelte';
  import AccountPopover from './AccountPopover.svelte';

  let { active = 'home' } = $props();
  let currentTheme = $state('dark');

  function handleThemeToggle() {
    currentTheme = toggleTheme();
  }

  onMount(() => {
    currentTheme = getTheme();
    const handleThemeChange = (/** @type {any} */ e) => {
      currentTheme = e.detail?.theme || getTheme();
    };
    window.addEventListener('ui:themechange', handleThemeChange);
    return () => window.removeEventListener('ui:themechange', handleThemeChange);
  });
</script>

<header class="sticky top-0 z-50 w-full bg-[var(--md-sys-color-surface)] border-b border-[var(--md-sys-color-outline-variant)] backdrop-blur-md bg-opacity-95 select-none transition-colors duration-300">
  <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 h-16 flex items-center justify-between gap-4">
    <!-- Brand Lockup -->
    <a href="/" class="flex items-center gap-3 no-underline text-[var(--md-sys-color-on-surface)] group">
      <RoverLogo size={40} class="transition-transform duration-300 group-hover:scale-105" />
      <span class="font-bold text-xl md:text-2xl tracking-tight leading-none text-[var(--md-sys-color-on-surface)]">
        DeepTrack
      </span>
    </a>

    <!-- Right: Account avatar (top-right, Google-style) + Theme Toggle + Nav Button -->
    <div class="flex items-center gap-3">
      <!-- Top-right account avatar — always occupies space to prevent layout shift -->
      <AccountPopover anchor="top-right" />

      <!-- GitHub Link -->
      <a
        href="https://github.com/sea-deep/deeptrack"
        target="_blank"
        rel="noopener noreferrer"
        class="w-10 h-10 rounded-full flex items-center justify-center text-[var(--md-sys-color-on-surface-variant)] hover:bg-[var(--md-sys-color-surface-container-highest)] hover:text-[var(--md-sys-color-on-surface)] border border-[var(--md-sys-color-outline-variant)] transition-all duration-200 active:scale-95 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--md-sys-color-primary)]"
        title="View on GitHub"
        aria-label="View on GitHub"
      >
        <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="currentColor" viewBox="0 0 24 24">
          <path d="M12 0c-6.626 0-12 5.373-12 12 0 5.302 3.438 9.8 8.207 11.387.599.111.793-.261.793-.577v-2.234c-3.338.726-4.033-1.416-4.033-1.416-.546-1.387-1.333-1.756-1.333-1.756-1.089-.745.083-.729.083-.729 1.205.084 1.839 1.237 1.839 1.237 1.07 1.834 2.807 1.304 3.492.997.107-.775.418-1.305.762-1.604-2.665-.305-5.467-1.334-5.467-5.931 0-1.311.469-2.381 1.236-3.221-.124-.303-.535-1.524.117-3.176 0 0 1.008-.322 3.301 1.23.957-.266 1.983-.399 3.003-.404 1.02.005 2.047.138 3.006.404 2.291-1.552 3.297-1.23 3.297-1.23.653 1.653.242 2.874.118 3.176.77.84 1.235 1.911 1.235 3.221 0 4.609-2.807 5.624-5.479 5.921.43.372.823 1.102.823 2.222v3.293c0 .319.192.694.801.576 4.765-1.589 8.199-6.086 8.199-11.386 0-6.627-5.373-12-12-12z"/>
        </svg>
      </a>

      <!-- Circular Theme Toggle -->
      <button
        type="button"
        class="w-10 h-10 rounded-full flex items-center justify-center text-[var(--md-sys-color-on-surface-variant)] hover:bg-[var(--md-sys-color-surface-container-highest)] hover:text-[var(--md-sys-color-on-surface)] border border-[var(--md-sys-color-outline-variant)] transition-all duration-200 active:scale-95 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--md-sys-color-primary)]"
        onclick={handleThemeToggle}
        title="Toggle Theme"
        aria-label="Toggle theme"
      >
        <span class="material-symbols-rounded text-xl">{currentTheme === 'dark' ? 'light_mode' : 'dark_mode'}</span>
      </button>

      <!-- Primary Navigation Button -->
      {#if active !== 'dashboard'}
        <a href="/dashboard" class="ui-button ui-button--filled !h-10 !px-4 text-xs font-bold whitespace-nowrap shadow-sm hover:shadow transition-all duration-200 active:scale-98">
          <span class="material-symbols-rounded text-base">terminal</span>
          Open dashboard
        </a>
      {:else}
        <a href="/auth" class="ui-button ui-button--outlined !h-10 !px-4 text-xs font-semibold whitespace-nowrap transition-all duration-200 active:scale-98">
          <span class="material-symbols-rounded text-base">account_circle</span>
          Sign in
        </a>
      {/if}
    </div>
  </div>
</header>
