import { offlineDatabase } from './offlineDb.js';

const CROCKFORD = '0123456789ABCDEFGHJKMNPQRSTVWXYZ';
const RECORD_TYPES = new Set([
  'telemetry', 'event', 'scan', 'map_snapshot', 'sentinel'
]);

/** @param {number} value @param {number} length */
function encodeBase32(value, length) {
  let remaining = value;
  let encoded = '';
  for (let index = 0; index < length; index += 1) {
    encoded = CROCKFORD[remaining % 32] + encoded;
    remaining = Math.floor(remaining / 32);
  }
  return encoded;
}

/** @param {number} [nowMs] @param {Uint8Array} [randomBytes] */
export function createSortableId(nowMs = Date.now(), randomBytes) {
  const bytes = randomBytes || crypto.getRandomValues(new Uint8Array(16));
  let random = '';
  for (let index = 0; index < 16; index += 1) {
    random += CROCKFORD[bytes[index] & 31];
  }
  return encodeBase32(Math.max(0, Math.floor(nowMs)), 10) + random;
}

/** @param {unknown} value */
function jsonObject(value) {
  if (!value || typeof value !== 'object' || Array.isArray(value)) return {};
  return JSON.parse(JSON.stringify(value, (_key, item) =>
    typeof item === 'number' && !Number.isFinite(item) ? null : item
  ));
}

/** @param {any} error */
function readableError(error) {
  if (!error) return 'Cloud sync failed';
  if (error.code === '42P01' || error.code === 'PGRST205') {
    return 'Cloud schema is not installed yet';
  }
  return error.message || 'Cloud sync failed';
}

/**
 * @param {{client:any,database?:typeof offlineDatabase,onState?:(state:any)=>void,isOnline?:()=>boolean}} options
 */
export function createDashboardSync(options) {
  const database = options.database || offlineDatabase;
  const isOnline = options.isOnline || (() =>
    typeof navigator === 'undefined' ? true : navigator.onLine !== false
  );
  let userId = null;
  let session = null;
  let sequence = 0;
  let flushPromise = null;
  let telemetryRecordedAt = 0;
  let snapshotSavedAt = 0;
  let periodicTimer = null;
  let disposed = false;
  let state = {
    online: isOnline(), status: 'idle', pending: 0,
    lastSyncedAt: null, lastError: ''
  };

  function publish(patch = {}) {
    state = { ...state, ...patch, online: isOnline() };
    options.onState?.({ ...state });
  }

  async function refreshPending() {
    if (!userId) return publish({ pending: 0 });
    publish({ pending: await database.countOutboxItems(userId) });
  }

  /** @param {'session'|'record'} target @param {Record<string, any>} row */
  async function enqueue(target, row) {
    if (!userId) return false;
    await database.putOutboxItem({
      key: `${userId}:${target}:${row.id}`,
      user_id: userId,
      target,
      row,
      queued_at_ms: Date.now(),
      attempts: 0,
      next_attempt_at_ms: 0
    });
    await refreshPending();
    if (isOnline()) void flush();
    return true;
  }

  async function flush() {
    if (flushPromise) return flushPromise;
    if (!userId || !isOnline()) {
      publish({ status: isOnline() ? 'idle' : 'offline' });
      return false;
    }
    flushPromise = (async () => {
      publish({ status: 'syncing', lastError: '' });
      const items = await database.listOutboxItems(userId, 200);
      if (!items.length) {
        publish({ status: 'synced', pending: 0 });
        return true;
      }

      const sessionItems = items.filter((item) => item.target === 'session');
      const recordItems = items.filter((item) => item.target === 'record');
      try {
        if (sessionItems.length) {
          const { error } = await options.client.from('rover_sessions').upsert(
            sessionItems.map((item) => item.row),
            { onConflict: 'user_id,id' }
          );
          if (error) throw error;
          await database.deleteOutboxItems(sessionItems.map((item) => item.key));
        }
        if (recordItems.length) {
          const { error } = await options.client.from('mission_records').upsert(
            recordItems.map((item) => item.row),
            { onConflict: 'user_id,id', ignoreDuplicates: true }
          );
          if (error) throw error;
          await database.deleteOutboxItems(recordItems.map((item) => item.key));
        }
        const pending = await database.countOutboxItems(userId);
        publish({
          status: pending ? 'syncing' : 'synced', pending,
          lastSyncedAt: new Date().toISOString(), lastError: ''
        });
        if (pending && !disposed) setTimeout(() => void flush(), 25);
        return true;
      } catch (error) {
        const failed = [...sessionItems, ...recordItems];
        const retryAt = Date.now() + Math.min(60_000,
          1_000 * 2 ** Math.min(6, Math.max(...failed.map((item) => item.attempts || 0))));
        await Promise.all(failed.map((item) => database.putOutboxItem({
          ...item,
          attempts: (item.attempts || 0) + 1,
          next_attempt_at_ms: retryAt
        })));
        publish({
          status: isOnline() ? 'error' : 'offline',
          pending: await database.countOutboxItems(userId),
          lastError: readableError(error)
        });
        return false;
      }
    })().finally(() => { flushPromise = null; });
    return flushPromise;
  }

  /** @param {string} nextUserId */
  async function init(nextUserId) {
    userId = nextUserId;
    disposed = false;
    await refreshPending();
    if (typeof window !== 'undefined') {
      window.addEventListener('online', handleConnectivity);
      window.addEventListener('offline', handleConnectivity);
      periodicTimer = setInterval(() => {
        if (!document.hidden && isOnline()) void flush();
      }, 15_000);
    }
    if (isOnline()) void flush();
  }

  function handleConnectivity() {
    publish({ status: isOnline() ? 'idle' : 'offline' });
    if (isOnline()) void flush();
  }

  /** @param {{roverId?:string,startPose?:Record<string,any>,metadata?:Record<string,any>}} [details] */
  async function beginSession(details = {}) {
    if (!userId) return null;
    const now = new Date().toISOString();
    session = {
      user_id: userId,
      id: `sess_${createSortableId()}`,
      rover_id: details.roverId || 'DT-ALPHA-01',
      mode: 'hardware',
      status: 'active',
      started_at: now,
      ended_at: null,
      start_pose: jsonObject(details.startPose),
      end_pose: null,
      metadata: jsonObject(details.metadata),
      updated_at: now
    };
    sequence = 0;
    telemetryRecordedAt = 0;
    await enqueue('session', session);
    return session.id;
  }

  /** @param {string} recordType @param {Record<string,any>} payload @param {{capturedAt?:string,throttleMs?:number}} [settings] */
  async function record(recordType, payload, settings = {}) {
    if (!session || !RECORD_TYPES.has(recordType)) return false;
    const nowMs = Date.now();
    const throttleMs = settings.throttleMs || 0;
    if (recordType === 'telemetry' && throttleMs > 0 &&
        nowMs - telemetryRecordedAt < throttleMs) return false;
    if (recordType === 'telemetry') telemetryRecordedAt = nowMs;
    sequence += 1;
    return enqueue('record', {
      user_id: userId,
      id: `rec_${createSortableId(nowMs)}`,
      session_id: session.id,
      record_type: recordType,
      captured_at: settings.capturedAt || new Date(nowMs).toISOString(),
      sequence,
      payload: jsonObject(payload)
    });
  }

  /** @param {'closed'|'interrupted'} [status] @param {Record<string,any>} [endPose] */
  async function closeSession(status = 'closed', endPose = {}) {
    if (!session) return;
    const now = new Date().toISOString();
    session = {
      ...session, status, ended_at: now,
      end_pose: jsonObject(endPose), updated_at: now
    };
    await enqueue('session', session);
    session = null;
  }

  /** @param {Record<string,any>} snapshot */
  async function cacheSnapshot(snapshot) {
    if (!userId || Date.now() - snapshotSavedAt < 1_500) return;
    snapshotSavedAt = Date.now();
    await database.putDashboardSnapshot(userId, jsonObject(snapshot));
  }

  async function restoreSnapshot() {
    if (!userId) return null;
    return database.getDashboardSnapshot(userId);
  }

  function dispose() {
    disposed = true;
    if (typeof window !== 'undefined') {
      window.removeEventListener('online', handleConnectivity);
      window.removeEventListener('offline', handleConnectivity);
    }
    if (periodicTimer) clearInterval(periodicTimer);
    periodicTimer = null;
  }

  return {
    init, beginSession, record, closeSession, cacheSnapshot,
    restoreSnapshot, flush, dispose,
    getState: () => ({ ...state }),
    getSessionId: () => session?.id || null
  };
}
