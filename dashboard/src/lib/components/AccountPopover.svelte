<script>
  import { auth } from '$lib/stores/auth.svelte.js';
  import { onMount } from 'svelte';
  
  let { anchor = 'top-right' } = $props();
  
  let isOpen = $state(false);
  let isSigningOut = $state(false);
  let signOutError = $state('');
  let imageError = $state(false);
  
  /** @type {HTMLDivElement | null} */
  let menuRef = $state(null);
  /** @type {HTMLButtonElement | null} */
  let triggerRef = $state(null);

  let avatarUrl = $derived(auth.user?.user_metadata?.avatar_url ?? auth.user?.user_metadata?.picture ?? null);
  let displayName = $derived(
    auth.user?.user_metadata?.full_name ||
    auth.user?.user_metadata?.name ||
    auth.user?.email?.split('@')[0] ||
    'Operator'
  );
  let initials = $derived(
    displayName
      .trim()
      .split(/\s+/)
      .slice(0, 2)
      .map((/** @type {string} */ w) => w[0]?.toUpperCase() ?? '')
      .join('')
  );
  
  function handleClickOutside(/** @type {MouseEvent} */ e) {
    if (isOpen && menuRef && triggerRef) {
      const target = /** @type {Node} */ (e.target);
      if (!menuRef.contains(target) && !triggerRef.contains(target)) {
        isOpen = false;
        signOutError = '';
      }
    }
  }

  function handleKeydown(/** @type {KeyboardEvent} */ e) {
    if (e.key === 'Escape' && isOpen) {
      isOpen = false;
      signOutError = '';
      triggerRef?.focus();
    }
  }

  onMount(() => {
    window.addEventListener('click', handleClickOutside);
    window.addEventListener('keydown', handleKeydown);
    return () => {
      window.removeEventListener('click', handleClickOutside);
      window.removeEventListener('keydown', handleKeydown);
    };
  });

  async function handleSignOut() {
    isSigningOut = true;
    signOutError = '';
    try {
      await auth.signOut();
      // auth.signOut() navigates to /auth on success; isOpen stays true
      // momentarily but the component unmounts — no cleanup needed
    } catch (/** @type {any} */ err) {
      signOutError = err?.message || 'Sign out failed. Please try again.';
    } finally {
      isSigningOut = false;
    }
  }

  // Popover position classes based on anchor prop
  let menuPositionClass = $derived(
    anchor === 'top-right'
      ? 'top-[48px] right-0'
      : 'bottom-full mb-2 left-0'  // opens above; left-0 relative to the button container
  );
</script>

<!--
  Show a skeleton circle while auth is resolving to prevent layout shift.
  Once auth resolves, show the real avatar (if user) or nothing (no user, on non-dashboard pages).
-->
{#if auth.isLoading}
  <!-- Skeleton placeholder — same dimensions as the real button -->
  <div
    class="w-10 h-10 rounded-full bg-[var(--md-sys-color-surface-container-highest)] animate-pulse shrink-0"
    aria-hidden="true"
  ></div>
{:else if auth.user}
<div class="relative flex items-center justify-center">
  <button
    bind:this={triggerRef}
    onclick={() => { isOpen = !isOpen; signOutError = ''; }}
    class="w-10 h-10 rounded-full border border-[var(--md-sys-color-outline-variant)] overflow-hidden bg-[var(--md-sys-color-primary-container)] flex items-center justify-center shrink-0 hover:border-[var(--md-sys-color-outline)] hover:ring-2 hover:ring-[var(--md-sys-color-outline-variant)] transition-all focus-visible:ring-2 focus-visible:ring-[var(--md-sys-color-primary)] focus-visible:outline-none"
    aria-expanded={isOpen}
    aria-haspopup="menu"
    aria-label="Account menu for {displayName}"
    title="Account: {displayName}"
  >
    {#if avatarUrl && !imageError}
      <img
        src={avatarUrl}
        alt="Profile photo of {displayName}"
        class="w-full h-full object-cover"
        onerror={() => { imageError = true; }}
      />
    {:else}
      <span class="text-sm font-bold text-[var(--md-sys-color-on-primary-container)] select-none leading-none">
        {initials}
      </span>
    {/if}
  </button>

  {#if isOpen}
    <div
      bind:this={menuRef}
      class="absolute z-[200] w-60 bg-[var(--md-sys-color-surface-container)] border border-[var(--md-sys-color-outline-variant)] rounded-xl shadow-xl p-2 flex flex-col gap-1 {menuPositionClass}"
      role="menu"
      aria-label="Account menu"
    >
      <!-- Identity header -->
      <div class="px-3 py-2.5 border-b border-[var(--md-sys-color-outline-variant)] mb-1 flex items-center gap-2.5 min-w-0">
        <!-- Mini avatar in menu -->
        <div class="w-8 h-8 rounded-full border border-[var(--md-sys-color-outline-variant)] overflow-hidden bg-[var(--md-sys-color-primary-container)] flex items-center justify-center shrink-0">
          {#if avatarUrl && !imageError}
            <img src={avatarUrl} alt="" class="w-full h-full object-cover" aria-hidden="true" />
          {:else}
            <span class="text-xs font-bold text-[var(--md-sys-color-on-primary-container)]">{initials}</span>
          {/if}
        </div>
        <div class="min-w-0">
          <div class="text-sm font-bold text-[var(--md-sys-color-on-surface)] truncate" title={displayName}>{displayName}</div>
          <div class="text-xs text-[var(--md-sys-color-on-surface-variant)] truncate" title={auth.user?.email ?? ''}>{auth.user?.email ?? ''}</div>
        </div>
      </div>

      <!-- Sign-out error (non-destructive) -->
      {#if signOutError}
        <div class="mx-1 mb-1 px-3 py-2 rounded-lg bg-[var(--md-sys-color-error-container)] text-[var(--md-sys-color-on-error-container)] text-xs flex items-start gap-1.5" role="alert">
          <span class="material-symbols-rounded text-sm shrink-0 text-[var(--md-sys-color-error)]">error</span>
          <span>{signOutError}</span>
        </div>
      {/if}
      
      <button
        class="ui-button ui-button--text !justify-start w-full !h-10 !px-3 disabled:opacity-50 rounded-lg"
        onclick={handleSignOut}
        disabled={isSigningOut}
        role="menuitem"
        type="button"
      >
        <span class="material-symbols-rounded text-lg">{isSigningOut ? 'progress_activity' : 'logout'}</span>
        <span class={isSigningOut ? 'animate-pulse' : ''}>{isSigningOut ? 'Signing out...' : 'Sign out'}</span>
      </button>
    </div>
  {/if}
</div>
{/if}
