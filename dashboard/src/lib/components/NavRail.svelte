<script>
  import RoverLogo from './RoverLogo.svelte';
  import { getTheme, toggleTheme } from '$lib/utils/theme.js';
  import { sendShared, receiveShared } from '$lib/utils/motion.js';
  import { onMount } from 'svelte';
  import AccountPopover from './AccountPopover.svelte';

  let {
    activeView = 'console',
    onSelectView = (/** @type {string} */ _) => {},
    isConnected = true,
    isEstop = false
  } = $props();

  const navItems = [
    { id: 'console', icon: 'terminal', label: 'Overview' },
    { id: 'maps', icon: 'map', label: 'Maps' },
    { id: 'logs', icon: 'list_alt', label: 'Logs' },
    { id: 'hardware', icon: 'memory', label: 'Hardware' }
  ];

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

<aside
  class="w-20 shrink-0 h-full bg-[var(--md-sys-color-surface-container)] border-r border-[var(--md-sys-color-outline-variant)] flex flex-col items-center justify-between py-5 z-40 select-none transition-colors duration-200 overflow-visible"
  aria-label="Dashboard navigation"
>
  <!-- Top Brand Mark & Destinations -->
  <div class="flex flex-col items-center w-full">
    <!-- Brand Mark -->
    <a
      href="/"
      class="flex flex-col items-center justify-center gap-1 group no-underline focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--md-sys-color-primary)] rounded-xl mb-5 text-center"
      title="DeepTrack Home"
    >
      <div class="w-11 h-11 rounded-2xl flex items-center justify-center shadow-sm group-hover:scale-105 transition-transform duration-200">
        <RoverLogo size={36} />
      </div>
      <span class="text-[11px] font-bold tracking-tight text-[var(--md-sys-color-on-surface)] leading-tight">
        DeepTrack
      </span>
    </a>

    <!-- Destinations Group -->
    <nav class="flex flex-col items-center gap-3 w-full px-1" aria-label="Primary destinations">
      {#each navItems as item}
        <button
          type="button"
          class="w-full flex flex-col items-center justify-center py-1 group relative transition-colors duration-150 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--md-sys-color-primary)] rounded-xl {activeView === item.id ? 'text-[var(--md-sys-color-on-surface)]' : 'text-[var(--md-sys-color-on-surface-variant)] hover:text-[var(--md-sys-color-on-surface)]'}"
          onclick={() => onSelectView(item.id)}
          title={item.label}
          aria-current={activeView === item.id ? 'page' : undefined}
        >
          <!-- Active Pill / Icon Container (Fixed 56x32px M3 Rail Standard) -->
          <div class="w-14 h-8 rounded-full flex items-center justify-center relative transition-colors duration-150 {activeView === item.id ? 'text-[var(--md-sys-color-on-primary-container)] font-semibold' : 'group-hover:bg-[var(--md-sys-color-surface-container-highest)]'}">
            {#if activeView === item.id}
              <div
                class="absolute inset-0 rounded-full bg-[var(--md-sys-color-primary-container)] shadow-sm pointer-events-none"
                in:receiveShared={{ key: 'navrail-active' }}
                out:sendShared={{ key: 'navrail-active' }}
              ></div>
            {/if}
            <span class="material-symbols-rounded text-[24px] relative z-10 transition-transform duration-150 group-hover:scale-105 {activeView === item.id ? 'filled' : ''}">
              {item.icon}
            </span>
          </div>

          <!-- Label -->
          <span class="text-sm font-medium leading-tight mt-1 text-center truncate max-w-[72px] relative z-10 {activeView === item.id ? 'text-[var(--md-sys-color-on-surface)] font-semibold' : 'text-[var(--md-sys-color-on-surface-variant)]'}">
            {item.label}
          </span>

          <!-- Tooltip on hover -->
          <span class="absolute left-[76px] px-2.5 py-1 bg-[var(--md-sys-color-inverse-surface)] text-[var(--md-sys-color-inverse-on-surface)] text-xs rounded-lg shadow-xl pointer-events-none opacity-0 group-hover:opacity-100 transition-opacity duration-150 whitespace-nowrap z-50">
            {item.label}
          </span>
        </button>
      {/each}
    </nav>
  </div>

  <!-- Bottom Utilities: Status dot & Compact Circular Theme Toggle & Account Control -->
  <div class="flex flex-col items-center gap-3.5 w-full">
    <!-- Link Status Indicator -->
    <div class="flex items-center justify-center w-8 h-8">
      <div
        class="w-2.5 h-2.5 rounded-full shadow-sm transition-colors duration-300"
        class:bg-[var(--ui-color-success)]={isConnected && !isEstop}
        class:bg-[var(--md-sys-color-error)]={isEstop || !isConnected}
        class:animate-pulse={isConnected && !isEstop}
        title={isEstop ? 'Remote stop requested' : isConnected ? 'Dashboard connected' : 'Disconnected'}
      ></div>
    </div>

    <!-- Compact Theme Toggle Button -->
    <button
      type="button"
      class="w-10 h-10 rounded-full flex items-center justify-center text-[var(--md-sys-color-on-surface-variant)] hover:bg-[var(--md-sys-color-surface-container-highest)] hover:text-[var(--md-sys-color-on-surface)] border border-[var(--md-sys-color-outline-variant)] transition-all duration-200 active:scale-95 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[var(--md-sys-color-primary)]"
      onclick={handleThemeToggle}
      title="Toggle Theme"
      aria-label="Toggle theme"
    >
      <span class="material-symbols-rounded text-[24px]">{currentTheme === 'dark' ? 'light_mode' : 'dark_mode'}</span>
    </button>

    <!-- Account Control Popover -->
    <AccountPopover anchor="bottom-left" />
  </div>
</aside>
