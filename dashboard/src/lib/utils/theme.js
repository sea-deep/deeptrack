/**
 * Global Theme Helper for DeepTrack
 */

export function getTheme() {
  if (typeof document === 'undefined') return 'dark';
  return document.documentElement.dataset.theme === 'light' ? 'light' : 'dark';
}

/**
 * @param {'dark' | 'light' | 'system'} theme
 */
export function setTheme(theme) {
  if (typeof document === 'undefined') return;
  const isDark = theme === 'dark' || (theme === 'system' && window.matchMedia('(prefers-color-scheme: dark)').matches);
  const resolved = isDark ? 'dark' : 'light';

  document.documentElement.dataset.theme = resolved;
  document.documentElement.classList.toggle('dark', isDark);
  document.documentElement.classList.toggle('light', !isDark);
  document.documentElement.style.colorScheme = resolved;

  try {
    localStorage.setItem('ui.theme', resolved);
  } catch (e) {}

  window.dispatchEvent(new CustomEvent('ui:themechange', { detail: { theme: resolved } }));
}

export function toggleTheme() {
  const current = getTheme();
  const next = current === 'dark' ? 'light' : 'dark';
  setTheme(next);
  return next;
}
