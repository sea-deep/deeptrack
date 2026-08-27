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
      if (session) setTimeout(() => void this.verifyCurrentUser(), 0);
    });

    // 2. Read existing session from localStorage
    try {
      const { data: { session }, error } = await supabase.auth.getSession();
      if (error) {
        console.error('[auth] getSession error:', error.message);
      }
      this.session = session ?? null;
      this.user = session?.user ?? null;
      if (session) await this.verifyCurrentUser();
    } catch (err) {
      console.error('[auth] getSession exception:', err);
    } finally {
      this.isLoading = false;
    }
  }

  async verifyCurrentUser() {
    if (!this.session || navigator.onLine === false) return;
    try {
      const { data, error } = await supabase.auth.getUser();
      if (!error && data?.user) this.user = data.user;
    } catch {
      // A cached session still enables the local/offline console. RLS performs
      // authoritative authorization when queued data reaches Supabase.
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
