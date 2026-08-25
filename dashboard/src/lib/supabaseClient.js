import { createClient } from '@supabase/supabase-js';
import { PUBLIC_SUPABASE_URL, PUBLIC_SUPABASE_ANON_KEY } from '$env/static/public';

// detectSessionInUrl: true (default) — Supabase processes ?code= (PKCE) and
// #access_token= (implicit) from the URL automatically before firing auth events.
// persistSession: true (default) — sessions survive refresh and browser restart.
// No custom storageKey — use Supabase's default to avoid breaking existing sessions.
export const supabase = createClient(PUBLIC_SUPABASE_URL, PUBLIC_SUPABASE_ANON_KEY, {
  auth: {
    persistSession: true,
    autoRefreshToken: true,
    detectSessionInUrl: true
  }
});
