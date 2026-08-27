<script>
    import '../app.css';
    import { onMount } from 'svelte';
    import { afterNavigate } from '$app/navigation';
    import { auth } from '$lib/stores/auth.svelte.js';
    
    let { children } = $props();

    // auth.init() is called exactly once here — the single source of truth.
    // It resolves the existing session before setting isLoading = false,
    // so route guards never see a false-negative "no user" flash.
    onMount(() => {
        auth.init();
    });

    afterNavigate(() => {
        if ('serviceWorker' in navigator) {
            void navigator.serviceWorker.getRegistration()
                .then((registration) => registration?.update())
                .catch(() => {});
        }
    });
</script>

<div class="min-h-screen">
    {@render children()}
</div>
