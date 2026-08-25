<script>
  import { onMount } from 'svelte';

  let currentTheme = $state('dark');

  /** @param {'system' | 'light' | 'dark'} theme */
  function setTheme(theme) {
    currentTheme = theme;
    try {
      window.localStorage.setItem('ui.theme', theme);
    } catch (e) {}

    const root = document.documentElement;
    const media = window.matchMedia('(prefers-color-scheme: dark)');
    const resolved = theme === 'system' ? (media.matches ? 'dark' : 'light') : theme;

    root.dataset.theme = resolved;
    root.dataset.themePreference = theme;
    root.style.colorScheme = resolved;

    /** @type {HTMLMetaElement | null} */
    const themeMeta = document.querySelector('meta[name="theme-color"]');
    if (themeMeta) {
      themeMeta.content = resolved === 'dark' ? '#111318' : '#f9f9fc';
    }

    window.dispatchEvent(
      new CustomEvent('ui:themechange', {
        detail: { preference: theme, resolved }
      })
    );
  }

  onMount(() => {
    try {
      const saved = /** @type {'system'|'light'|'dark'|null} */ (window.localStorage.getItem('ui.theme'));
      if (saved) currentTheme = saved;
    } catch (e) {}
  });
</script>

<div class="ui-segmented-control" role="radiogroup" aria-label="Theme selection">
  <button
    type="button"
    class:active={currentTheme === 'light'}
    onclick={() => setTheme('light')}
    title="Light theme"
    aria-checked={currentTheme === 'light'}
    role="radio"
  >
    <span class="material-symbols-rounded text-base">light_mode</span>
    <span class="hidden sm:inline">Light</span>
  </button>
  <button
    type="button"
    class:active={currentTheme === 'system'}
    onclick={() => setTheme('system')}
    title="System theme"
    aria-checked={currentTheme === 'system'}
    role="radio"
  >
    <span class="material-symbols-rounded text-base">devices</span>
    <span class="hidden sm:inline">System</span>
  </button>
  <button
    type="button"
    class:active={currentTheme === 'dark'}
    onclick={() => setTheme('dark')}
    title="Dark theme"
    aria-checked={currentTheme === 'dark'}
    role="radio"
  >
    <span class="material-symbols-rounded text-base">dark_mode</span>
    <span class="hidden sm:inline">Dark</span>
  </button>
</div>
