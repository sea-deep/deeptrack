const DATABASE_NAME = 'deeptrack-offline';
const DATABASE_VERSION = 1;
const OUTBOX_STORE = 'outbox';
const SNAPSHOT_STORE = 'snapshots';

/** @type {Promise<IDBDatabase | null> | null} */
let databasePromise = null;
const memoryOutbox = new Map();
const memorySnapshots = new Map();

/** @template T @param {IDBRequest<T>} request @returns {Promise<T>} */
function requestResult(request) {
  return new Promise((resolve, reject) => {
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => reject(request.error || new Error('IndexedDB request failed'));
  });
}

/** @returns {Promise<IDBDatabase | null>} */
function openDatabase() {
  if (databasePromise) return databasePromise;
  if (typeof indexedDB === 'undefined') return Promise.resolve(null);

  databasePromise = new Promise((resolve) => {
    const request = indexedDB.open(DATABASE_NAME, DATABASE_VERSION);
    request.onupgradeneeded = () => {
      const database = request.result;
      if (!database.objectStoreNames.contains(OUTBOX_STORE)) {
        const outbox = database.createObjectStore(OUTBOX_STORE, { keyPath: 'key' });
        outbox.createIndex('user_id', 'user_id', { unique: false });
        outbox.createIndex('queued_at_ms', 'queued_at_ms', { unique: false });
      }
      if (!database.objectStoreNames.contains(SNAPSHOT_STORE)) {
        database.createObjectStore(SNAPSHOT_STORE, { keyPath: 'key' });
      }
    };
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => resolve(null);
    request.onblocked = () => resolve(null);
  });
  return databasePromise;
}

/** @param {string} storeName @param {IDBTransactionMode} mode */
async function transaction(storeName, mode) {
  const database = await openDatabase();
  if (!database) return null;
  return database.transaction(storeName, mode).objectStore(storeName);
}

/** @param {Record<string, any>} item */
export async function putOutboxItem(item) {
  const store = await transaction(OUTBOX_STORE, 'readwrite');
  if (!store) {
    memoryOutbox.set(item.key, structuredClone(item));
    return;
  }
  await requestResult(store.put(item));
}

/** @param {string} userId @param {number} [limit] */
export async function listOutboxItems(userId, limit = 200) {
  const store = await transaction(OUTBOX_STORE, 'readonly');
  const items = store
    ? await requestResult(store.getAll())
    : [...memoryOutbox.values()].map((item) => structuredClone(item));
  const now = Date.now();
  return items
    .filter((item) => item.user_id === userId &&
      (item.next_attempt_at_ms || 0) <= now)
    .sort((a, b) => a.queued_at_ms - b.queued_at_ms)
    .slice(0, limit);
}

/** @param {string[]} keys */
export async function deleteOutboxItems(keys) {
  if (!keys.length) return;
  const store = await transaction(OUTBOX_STORE, 'readwrite');
  if (!store) {
    keys.forEach((key) => memoryOutbox.delete(key));
    return;
  }
  await Promise.all(keys.map((key) => requestResult(store.delete(key))));
}

/** @param {string} userId */
export async function countOutboxItems(userId) {
  const store = await transaction(OUTBOX_STORE, 'readonly');
  const items = store
    ? await requestResult(store.getAll())
    : [...memoryOutbox.values()];
  return items.filter((item) => item.user_id === userId).length;
}

/** @param {string} userId @param {Record<string, any>} value */
export async function putDashboardSnapshot(userId, value) {
  const item = {
    key: `dashboard:${userId}`,
    user_id: userId,
    saved_at: new Date().toISOString(),
    value
  };
  const store = await transaction(SNAPSHOT_STORE, 'readwrite');
  if (!store) {
    memorySnapshots.set(item.key, structuredClone(item));
    return;
  }
  await requestResult(store.put(item));
}

/** @param {string} userId */
export async function getDashboardSnapshot(userId) {
  const key = `dashboard:${userId}`;
  const store = await transaction(SNAPSHOT_STORE, 'readonly');
  const item = store
    ? await requestResult(store.get(key))
    : memorySnapshots.get(key);
  return item ? structuredClone(item) : null;
}

export const offlineDatabase = {
  putOutboxItem,
  listOutboxItems,
  deleteOutboxItems,
  countOutboxItems,
  putDashboardSnapshot,
  getDashboardSnapshot
};
