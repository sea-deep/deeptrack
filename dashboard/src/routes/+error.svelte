<script>
  import { page } from '$app/state';

  function retry() {
    window.location.reload();
  }
</script>

<svelte:head>
  <title>DeepTrack · Recovery</title>
</svelte:head>

<main class="min-h-screen bg-[var(--md-sys-color-surface)] px-5 py-12 text-[var(--md-sys-color-on-surface)] grid place-items-center">
  <section class="ui-card w-full max-w-lg text-center" aria-labelledby="recovery-title">
    <div class="mx-auto mb-4 grid h-14 w-14 place-items-center rounded-2xl bg-[var(--md-sys-color-error-container)] text-2xl font-bold text-[var(--md-sys-color-on-error-container)]">!</div>
    <p class="text-xs font-bold uppercase tracking-[0.16em] text-[var(--md-sys-color-on-surface-variant)]">Error {page.status}</p>
    <h1 id="recovery-title" class="mt-2 text-2xl font-bold">The console hit a recoverable problem</h1>
    <p class="mt-3 text-sm leading-relaxed text-[var(--md-sys-color-on-surface-variant)]">
      {page.error?.message || 'The page could not finish loading.'} Locally queued rover records remain in this browser and will sync after the console recovers.
    </p>
    <div class="mt-6 flex flex-col justify-center gap-3 sm:flex-row">
      <button type="button" class="ui-button ui-button--filled" onclick={retry}>Retry</button>
      <a class="ui-button ui-button--outlined" href="/dashboard/demo">Open offline demo</a>
      <a class="ui-button ui-button--text" href="/">Home</a>
    </div>
  </section>
</main>
