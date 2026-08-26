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
