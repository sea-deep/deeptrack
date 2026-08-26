<script>
  import Navbar from '$lib/components/Navbar.svelte';
  import Footer from '$lib/components/Footer.svelte';
  import RoverLogo from '$lib/components/RoverLogo.svelte';
  import { supabase } from '$lib/supabaseClient.js';
  import { auth } from '$lib/stores/auth.svelte.js';
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { sendShared, receiveShared, m3Expand, m3TopLevelFadeThrough } from '$lib/utils/motion.js';

  let mode = $state('login');

  $effect(() => {
    if (!auth.isLoading && auth.user) {
      goto('/dashboard/real');
    }
  }); // 'login' or 'register'
  let email = $state('');
  let password = $state('');
  let loading = $state(false);
  let errorMessage = $state('');
  let successMessage = $state('');
  let showPassword = $state(false);

  async function handleGoogleAuth() {
    loading = true;
    errorMessage = '';
    
    try {
      const { data, error } = await supabase.auth.signInWithOAuth({
        provider: 'google',
        options: {
          redirectTo: `${window.location.origin}/auth/callback`
        }
      });
      if (error) throw error;
    } catch (/** @type {any} */ err) {
      errorMessage = err?.message || 'An unexpected error occurred during Google authorization.';
      loading = false;
    }
  }

  async function handleGithubAuth() {
    loading = true;
    errorMessage = '';
    
    try {
      const { data, error } = await supabase.auth.signInWithOAuth({
        provider: 'github',
        options: {
          redirectTo: `${window.location.origin}/auth/callback`
        }
      });
      if (error) throw error;
    } catch (/** @type {any} */ err) {
      errorMessage = err?.message || 'An unexpected error occurred during GitHub authorization.';
      loading = false;
    }
  }

  /** @param {SubmitEvent} e */
  async function handleAuth(e) {
    e.preventDefault();
    loading = true;
    errorMessage = '';
    successMessage = '';

    try {
      if (mode === 'login') {
        const { data, error } = await supabase.auth.signInWithPassword({
          email,
          password
        });
        if (error) {
          errorMessage = error.message;
        } else {
          successMessage = 'Signed in. Opening the real dashboard...';
          setTimeout(() => {
            goto('/dashboard/real');
          }, 800);
        }
      } else {
        const { data, error } = await supabase.auth.signUp({
          email,
          password
        });
        if (error) {
          errorMessage = error.message;
        } else {
          successMessage = 'Account created. Check your email, then sign in.';
          mode = 'login';
        }
      }
    } catch (/** @type {any} */ err) {
      errorMessage = err?.message || 'Something went wrong while signing in.';
    } finally {
      loading = false;
    }
  }
</script>

<svelte:head>
  <title>DeepTrack · Sign In</title>
</svelte:head>

{#if auth.isLoading || auth.user}
  <div class="min-h-screen flex items-center justify-center bg-[var(--md-sys-color-surface)]">
    <span class="material-symbols-rounded animate-spin text-4xl text-[var(--md-sys-color-primary)]">progress_activity</span>
  </div>
{:else}
<div class="min-h-screen flex flex-col bg-[var(--md-sys-color-surface)] text-[var(--md-sys-color-on-surface)] select-none">
  <!-- Shared Top Navigation -->
  <Navbar active="auth" />

  <!-- Main Auth Canvas -->
  <main class="flex-1 flex items-center justify-center px-4 py-12 sm:px-6 lg:px-8">
    <div class="w-full max-w-md" in:m3TopLevelFadeThrough={{ duration: 400 }}>
      <!-- Auth Card Container -->
      <div class="ui-card">
        <!-- Brand / Identity Header -->
        <div class="text-center mb-8">
          <div class="inline-flex items-center justify-center mb-3">
            <RoverLogo size={52} />
          </div>
          <h1 class="text-2xl font-bold font-headline tracking-tight text-[var(--md-sys-color-on-surface)]">
            {mode === 'login' ? 'Sign in' : 'Create an account'}
          </h1>
          <p class="text-xs text-[var(--md-sys-color-on-surface-variant)] mt-1">
            Sign in to connect the real gateway
          </p>
        </div>

        <!-- Mode Toggle Tabs with Shared Element Motion -->
        <div class="grid grid-cols-2 p-1 rounded-full bg-[var(--md-sys-color-surface-container-highest)] mb-6 border border-[var(--md-sys-color-outline-variant)] relative">
          <button
            type="button"
            class="py-2 text-xs font-medium rounded-full relative transition-colors duration-150 flex items-center justify-center gap-1.5 {mode === 'login' ? 'text-[var(--md-sys-color-on-primary-container)]' : 'text-[var(--md-sys-color-on-surface-variant)] hover:text-[var(--md-sys-color-on-surface)]'}"
            onclick={() => { mode = 'login'; errorMessage = ''; successMessage = ''; }}
          >
            {#if mode === 'login'}
              <div
                class="absolute inset-0 rounded-full bg-[var(--md-sys-color-primary-container)] shadow-sm pointer-events-none"
                in:receiveShared={{ key: 'auth-tab-pill' }}
                out:sendShared={{ key: 'auth-tab-pill' }}
              ></div>
            {/if}
            <span class="material-symbols-rounded text-base relative z-10">login</span>
            <span class="relative z-10">Sign in</span>
          </button>
          <button
            type="button"
            class="py-2 text-xs font-medium rounded-full relative transition-colors duration-150 flex items-center justify-center gap-1.5 {mode === 'register' ? 'text-[var(--md-sys-color-on-primary-container)]' : 'text-[var(--md-sys-color-on-surface-variant)] hover:text-[var(--md-sys-color-on-surface)]'}"
            onclick={() => { mode = 'register'; errorMessage = ''; successMessage = ''; }}
          >
            {#if mode === 'register'}
              <div
                class="absolute inset-0 rounded-full bg-[var(--md-sys-color-primary-container)] shadow-sm pointer-events-none"
                in:receiveShared={{ key: 'auth-tab-pill' }}
                out:sendShared={{ key: 'auth-tab-pill' }}
              ></div>
            {/if}
            <span class="material-symbols-rounded text-base relative z-10">person_add</span>
            <span class="relative z-10">Create account</span>
          </button>
        </div>

        <!-- Error / Success Banners -->
        {#if errorMessage}
          <div transition:m3Expand class="mb-5 overflow-hidden"><div id="auth-error" class="p-3.5 rounded-xl bg-[var(--md-sys-color-error-container)] text-[var(--md-sys-color-on-error-container)] text-xs flex items-center gap-2.5" role="alert" aria-live="polite">
            <span class="material-symbols-rounded text-base filled text-[var(--md-sys-color-error)]">error</span>
            <span class="font-medium">{errorMessage}</span>
          </div></div>
        {/if}

        {#if successMessage}
          <div transition:m3Expand class="mb-5 overflow-hidden"><div class="p-3.5 rounded-xl bg-[var(--ui-color-success-container)] text-[var(--ui-color-on-success-container)] text-xs flex items-center gap-2.5">
            <span class="material-symbols-rounded text-base filled text-[var(--ui-color-success)]">check_circle</span>
            <span class="font-medium">{successMessage}</span>
          </div></div>
        {/if}

        <!-- Interactive Form -->
        <form onsubmit={handleAuth} class="space-y-4">
          <!-- Email -->
          <div class="ui-field">
            <label for="auth-email">Email address</label>
            <div class="relative">
              <input
                id="auth-email"
                type="email"
                bind:value={email}
                required
                placeholder="you@example.com"
                autocomplete="email"
                aria-invalid={errorMessage ? 'true' : undefined}
                aria-describedby={errorMessage ? 'auth-error' : undefined}
                class="w-full text-sm"
              />
            </div>
          </div>

          <!-- Password with Reveal Toggle -->
          <div class="ui-field">
            <div class="flex justify-between items-center mb-1">
              <label for="auth-password">Password</label>
              {#if mode === 'login'}
                <a href="#forgot" class="text-xs text-[var(--md-sys-color-primary)] hover:underline">Forgot password?</a>
              {/if}
            </div>
            <div class="relative">
              <input
                id="auth-password"
                type={showPassword ? 'text' : 'password'}
                bind:value={password}
                required
                minlength="6"
                placeholder="••••••••••••"
                autocomplete={mode === 'login' ? 'current-password' : 'new-password'}
                aria-invalid={errorMessage ? 'true' : undefined}
                aria-describedby={errorMessage ? 'auth-error' : undefined}
                class="w-full text-sm pr-12 telemetry"
              />
              <button
                type="button"
                class="absolute right-3 top-1/2 -translate-y-1/2 text-[var(--md-sys-color-on-surface-variant)] hover:text-[var(--md-sys-color-on-surface)] transition-colors duration-150"
                onclick={() => showPassword = !showPassword}
                aria-label={showPassword ? 'Hide password' : 'Show password'}
              >
                <span class="material-symbols-rounded text-lg">
                  {showPassword ? 'visibility_off' : 'visibility'}
                </span>
              </button>
            </div>
          </div>

          <!-- Submit Button -->
          <div class="pt-2">
            <button
              type="submit"
              disabled={loading}
              class="ui-button ui-button--filled w-full font-medium transition-transform duration-150 active:scale-95"
            >
              {#if loading}
                <span class="material-symbols-rounded text-lg animate-spin">progress_activity</span>
                <span>Signing in...</span>
              {:else if mode === 'login'}
                <span class="material-symbols-rounded text-lg">login</span>
                <span>Sign in and continue</span>
              {:else}
                <span class="material-symbols-rounded text-lg">person_add</span>
                <span>Create account</span>
              {/if}
            </button>
          </div>
        </form>

        <!-- Divider -->
        <div class="flex items-center my-6">
          <div class="flex-grow border-t border-[var(--md-sys-color-outline-variant)]"></div>
          <span class="mx-4 text-[11px] font-medium text-[var(--md-sys-color-outline)] uppercase tracking-widest">Or</span>
          <div class="flex-grow border-t border-[var(--md-sys-color-outline-variant)]"></div>
        </div>

        <!-- Google OAuth Button -->
        <button
          type="button"
          onclick={handleGoogleAuth}
          disabled={loading}
          class="ui-button ui-button--outlined w-full font-medium transition-transform duration-150 active:scale-95 flex items-center justify-center gap-2"
        >
          <svg viewBox="0 0 24 24" width="20" height="20" xmlns="http://www.w3.org/2000/svg">
            <g transform="matrix(1, 0, 0, 1, 27.009001, -39.238998)">
              <path fill="#4285F4" d="M -3.264 51.509 C -3.264 50.719 -3.334 49.969 -3.454 49.239 L -14.754 49.239 L -14.754 53.749 L -8.284 53.749 C -8.574 55.229 -9.424 56.479 -10.684 57.329 L -10.684 60.329 L -6.824 60.329 C -4.564 58.239 -3.264 55.159 -3.264 51.509 Z"/>
              <path fill="#34A853" d="M -14.754 63.239 C -11.514 63.239 -8.804 62.159 -6.824 60.329 L -10.684 57.329 C -11.764 58.049 -13.134 58.489 -14.754 58.489 C -17.884 58.489 -20.534 56.379 -21.484 53.529 L -25.464 53.529 L -25.464 56.619 C -23.494 60.539 -19.444 63.239 -14.754 63.239 Z"/>
              <path fill="#FBBC05" d="M -21.484 53.529 C -21.734 52.809 -21.864 52.039 -21.864 51.239 C -21.864 50.439 -21.724 49.669 -21.484 48.949 L -21.484 45.859 L -25.464 45.859 C -26.284 47.479 -26.754 49.299 -26.754 51.239 C -26.754 53.179 -26.284 54.999 -25.464 56.619 L -21.484 53.529 Z"/>
              <path fill="#EA4335" d="M -14.754 43.989 C -12.984 43.989 -11.404 44.599 -10.154 45.789 L -6.734 42.369 C -8.804 40.429 -11.514 39.239 -14.754 39.239 C -19.444 39.239 -23.494 41.939 -25.464 45.859 L -21.484 48.949 C -20.534 46.099 -17.884 43.989 -14.754 43.989 Z"/>
            </g>
          </svg>
          <span>Continue with Google</span>
        </button>


        <!-- GitHub OAuth Button -->
        <button
          type="button"
          onclick={handleGithubAuth}
          disabled={loading}
          class="ui-button ui-button--outlined w-full font-medium transition-transform duration-150 active:scale-95 flex items-center justify-center gap-2 mt-3"
        >
          <svg viewBox="0 0 24 24" width="20" height="20" xmlns="http://www.w3.org/2000/svg" fill="currentColor">
            <path d="M12 .297c-6.63 0-12 5.373-12 12 0 5.303 3.438 9.8 8.205 11.385.6.113.82-.258.82-.577 0-.285-.01-1.04-.015-2.04-3.338.724-4.042-1.61-4.042-1.61C4.422 18.07 3.633 17.7 3.633 17.7c-1.087-.744.084-.729.084-.729 1.205.084 1.838 1.236 1.838 1.236 1.07 1.835 2.809 1.305 3.495.998.108-.776.417-1.305.76-1.605-2.665-.3-5.466-1.332-5.466-5.93 0-1.31.465-2.38 1.235-3.22-.135-.303-.54-1.523.105-3.176 0 0 1.005-.322 3.3 1.23.96-.267 1.98-.399 3-.405 1.02.006 2.04.138 3 .405 2.28-1.552 3.285-1.23 3.285-1.23.645 1.653.24 2.873.12 3.176.765.84 1.23 1.91 1.23 3.22 0 4.61-2.805 5.625-5.475 5.92.42.36.81 1.096.81 2.22 0 1.606-.015 2.896-.015 3.286 0 .315.21.69.825.57C20.565 22.092 24 17.592 24 12.297c0-6.627-5.373-12-12-12"/>
          </svg>
          <span>Continue with GitHub</span>
        </button>

        <!-- Current access boundary -->
        <div class="mt-6 pt-4 border-t border-[var(--md-sys-color-outline-variant)] text-center">
          <p class="text-[11px] text-[var(--md-sys-color-on-surface-variant)]">
            Sign in is only required for the real gateway. The demo stays open to everyone.
          </p>
          <a href="/" class="text-xs font-medium text-[var(--md-sys-color-primary)] hover:underline inline-flex items-center gap-1 mt-1">
            <span>Back to home</span>
            <span class="material-symbols-rounded text-sm">arrow_forward</span>
          </a>
        </div>
      </div>
    </div>
  </main>

  <!-- Shared Footer -->
  <Footer />
</div>

{/if}
