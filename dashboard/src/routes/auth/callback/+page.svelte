<script>
  import { onMount } from 'svelte';
  import { goto } from '$app/navigation';
  import { supabase } from '$lib/supabaseClient.js';
  import { auth } from '$lib/stores/auth.svelte.js';

  let statusText = $state('Finishing sign in...');
  let hasError = $state(false);

  onMount(async () => {
    try {
      // 1. Check for PKCE ?code= in URL query params
      const urlParams = new URLSearchParams(window.location.search);
      const code = urlParams.get('code');

      if (code) {
        statusText = 'Finishing sign in...';
        const { data, error } = await supabase.auth.exchangeCodeForSession(code);
        if (error) {
          console.error('[OAuth Callback] exchangeCodeForSession error:', error);
          // If exchange failed, check if we already have a session (e.g. Supabase auto-processed it)
          const { data: sessionData } = await supabase.auth.getSession();
          if (sessionData?.session) {
            auth.session = sessionData.session;
            auth.user = sessionData.session.user;
            auth.isLoading = false;
            goto('/dashboard/real', { replaceState: true });
            return;
          }
          throw error;
        }

        if (data?.session) {
          auth.session = data.session;
          auth.user = data.session.user;
          auth.isLoading = false;
          goto('/dashboard/real', { replaceState: true });
          return;
        }
      }

      // 2. Check for implicit #access_token= in URL hash
      if (window.location.hash && window.location.hash.includes('access_token')) {
        statusText = 'Checking sign in...';
        const hashParams = new URLSearchParams(window.location.hash.replace(/^#/, ''));
        const access_token = hashParams.get('access_token');
        const refresh_token = hashParams.get('refresh_token');

        if (access_token && refresh_token) {
          const { data, error } = await supabase.auth.setSession({ access_token, refresh_token });
          if (!error && data?.session) {
            auth.session = data.session;
            auth.user = data.session.user;
            auth.isLoading = false;
            goto('/dashboard/real', { replaceState: true });
            return;
          }
        }
      }

      // 3. Fallback: Check existing session in localStorage
      const { data: { session } } = await supabase.auth.getSession();
      if (session) {
        auth.session = session;
        auth.user = session.user;
        auth.isLoading = false;
        goto('/dashboard/real', { replaceState: true });
        return;
      }

      // If no code, no hash, and no session, wait briefly for any background event
      setTimeout(async () => {
        const { data: { session: delayedSession } } = await supabase.auth.getSession();
        if (delayedSession) {
          auth.session = delayedSession;
          auth.user = delayedSession.user;
          auth.isLoading = false;
          goto('/dashboard/real', { replaceState: true });
        } else {
          hasError = true;
          statusText = 'Sign in failed. Returning to the sign-in page...';
          setTimeout(() => {
            goto('/auth', { replaceState: true });
          }, 1500);
        }
      }, 800);

    } catch (err) {
      console.error('[OAuth Callback] Exception:', err);
      hasError = true;
      statusText = 'Sign in failed. Returning to the sign-in page...';
      setTimeout(() => {
        goto('/auth', { replaceState: true });
      }, 1500);
    }
  });
</script>

<svelte:head>
  <title>DeepTrack · Auth</title>
</svelte:head>

<div class="min-h-screen flex flex-col items-center justify-center bg-[var(--md-sys-color-surface)] text-[var(--md-sys-color-on-surface)] select-none">
  <div class="flex flex-col items-center gap-4 text-center px-4">
    {#if hasError}
      <span class="material-symbols-rounded text-4xl text-[var(--md-sys-color-error)]">error</span>
    {:else}
      <span class="material-symbols-rounded animate-spin text-4xl text-[var(--md-sys-color-primary)]">progress_activity</span>
    {/if}
    <p class="text-sm font-medium {hasError ? 'text-[var(--md-sys-color-error)]' : 'text-[var(--md-sys-color-on-surface-variant)]'}">
      {statusText}
    </p>
  </div>
</div>
