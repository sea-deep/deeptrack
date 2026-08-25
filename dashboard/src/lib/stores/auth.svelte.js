import { supabase } from '$lib/supabaseClient.js';
import { goto } from '$app/navigation';

class AuthStore {
  /** @type {any} */
  session = $state(null);
  /** @type {any} */
  user = $state(null);
  isLoading = $state(true);
  initialized = false;

  async init() {
    if (this.initialized) return;
    this.initialized = true;

    if (typeof window === 'undefined') {
      this.isLoading = false;
      return;
    }

    // 1. Subscribe to auth state changes (token refresh, cross-tab sync, sign-in/out)
    supabase.auth.onAuthStateChange((_event, session) => {
      this.session = session ?? null;
      this.user = session?.user ?? null;
      this.isLoading = false;
    });

    // 2. Read existing session from localStorage
    try {
      const { data: { session }, error } = await supabase.auth.getSession();
      if (error) {
        console.error('[auth] getSession error:', error.message);
      }
      this.session = session ?? null;
      this.user = session?.user ?? null;
    } catch (err) {
      console.error('[auth] getSession exception:', err);
    } finally {
      this.isLoading = false;
    }
  }

  async signOut() {
    const previousSession = this.session;
    const previousUser = this.user;

    try {
      const { error } = await supabase.auth.signOut();
      if (error) throw error;

      this.session = null;
      this.user = null;
      goto('/auth');
    } catch (/** @type {any} */ err) {
      console.error('[auth] Sign out failed:', err);
      this.session = previousSession;
      this.user = previousUser;
      throw err;
    }
  }
}

export const auth = new AuthStore();

if (typeof window !== 'undefined') {
  auth.init();
}
