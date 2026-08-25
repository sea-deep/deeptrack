<script>
  import RoverLogo from './RoverLogo.svelte';
  import { getTheme, toggleTheme } from '$lib/utils/theme.js';
  import { onMount } from 'svelte';
  import { supabase } from '$lib/supabaseClient.js';

  let { active = 'home' } = $props();
  let currentTheme = $state('dark');
  let userSession = $state(null);

  function handleThemeToggle() {
    currentTheme = toggleTheme();
  }

  onMount(() => {
    supabase.auth.getSession().then(({ data: { session } }) => {
      userSession = session;
    });
    supabase.auth.onAuthStateChange((_event, session) => {
      userSession = session;
    });

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
      <RoverLogo size={36} class="transition-transform duration-300 group-hover:scale-105" />
      <div>
        <div class="font-bold text-lg tracking-tight leading-none flex items-center gap-2">
          DeepTrack
          <span class="text-[10px] font-mono px-1.5 py-0.5 rounded bg-[var(--md-sys-color-primary-container)] text-[var(--md-sys-color-on-primary-container)] font-semibold">
            v2.4
          </span>
        </div>
        <div class="text-[11px] text-[var(--md-sys-color-on-surface-variant)] font-medium mt-0.5">
          Subterranean Mine Rescue Rover
        </div>
      </div>
    </a>

    <!-- Clean Actions: Compact Theme Toggle + Single Primary Button -->
    <div class="flex items-center gap-3">
      <!-- PFP (If Logged In) -->
      {#if userSession?.user}
        <div class="w-10 h-10 rounded-full border border-[var(--md-sys-color-outline-variant)] overflow-hidden bg-[var(--md-sys-color-surface-container-highest)] flex items-center justify-center shrink-0">
          {#if userSession.user.user_metadata?.avatar_url}
            <img src={userSession.user.user_metadata.avatar_url} alt="Profile" class="w-full h-full object-cover" />
          {:else}
            <span class="material-symbols-rounded text-[var(--md-sys-color-on-surface-variant)] text-xl">person</span>
          {/if}
        </div>
      {/if}

      <!-- Circular Theme Toggle -->
      <button
        type="button"
        class="w-10 h-10 rounded-full flex items-center justify-center text-[var(--md-sys-color-on-surface-variant)] hover:bg-[var(--md-sys-color-surface-container-highest)] hover:text-[var(--md-sys-color-on-surface)] border border-[var(--md-sys-color-outline-variant)] transition-all duration-200 active:scale-95"
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
          Launch Console
        </a>
      {:else}
        <a href="/auth" class="ui-button ui-button--outlined !h-10 !px-4 text-xs font-semibold whitespace-nowrap transition-all duration-200 active:scale-98">
          <span class="material-symbols-rounded text-base">account_circle</span>
          Operator Portal
        </a>
      {/if}
    </div>
  </div>
</header>
