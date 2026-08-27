import assert from 'node:assert/strict';
import { createDashboardSync, createSortableId } from '../src/lib/offline/dashboardSync.js';

function createMemoryDatabase() {
  const outbox = new Map();
  const snapshots = new Map();
  return {
    async putOutboxItem(item) { outbox.set(item.key, structuredClone(item)); },
    async listOutboxItems(userId, limit = 200) {
      return [...outbox.values()]
        .filter((item) => item.user_id === userId)
        .sort((a, b) => a.queued_at_ms - b.queued_at_ms)
        .slice(0, limit);
    },
    async deleteOutboxItems(keys) { keys.forEach((key) => outbox.delete(key)); },
    async countOutboxItems(userId) {
      return [...outbox.values()].filter((item) => item.user_id === userId).length;
    },
    async putDashboardSnapshot(userId, value) {
      snapshots.set(userId, { user_id: userId, value: structuredClone(value) });
    },
    async getDashboardSnapshot(userId) {
      return snapshots.has(userId) ? structuredClone(snapshots.get(userId)) : null;
    },
    outbox
  };
}

const fixedId = createSortableId(1_700_000_000_000, new Uint8Array(16).fill(7));
assert.match(fixedId, /^[0-9A-Z]{26}$/);
assert.equal(fixedId.length, 26);

const database = createMemoryDatabase();
const writes = [];
let online = false;
const client = {
  from(table) {
    return {
      async upsert(rows, options) {
        writes.push({ table, rows: structuredClone(rows), options });
        return { error: null };
      }
    };
  }
};
const states = [];
const sync = createDashboardSync({
  client,
  database,
  isOnline: () => online,
  onState: (state) => states.push(state)
});

await sync.init('11111111-1111-4111-8111-111111111111');
const sessionId = await sync.beginSession({
  metadata: { protocol: 2 }, startPose: { x_m: 0, y_m: 0 }
});
assert.match(sessionId, /^sess_[0-9A-Z]{26}$/);
await sync.record('telemetry', { temperature: 31.2 }, { throttleMs: 1_000 });
await sync.record('telemetry', { temperature: 31.3 }, { throttleMs: 1_000 });
await sync.record('event', { code: 'LINK_READY' });
assert.equal(await database.countOutboxItems('11111111-1111-4111-8111-111111111111'), 3);

await sync.cacheSnapshot({ telemetry: { source: 'LIVE' } });
const restored = await sync.restoreSnapshot();
assert.equal(restored.value.telemetry.source, 'LIVE');

online = true;
assert.equal(await sync.flush(), true);
assert.equal(writes[0].table, 'rover_sessions');
assert.equal(writes[1].table, 'mission_records');
assert.equal(writes[1].rows.length, 2);
assert.equal(writes[1].options.ignoreDuplicates, true);
assert.equal(await database.countOutboxItems('11111111-1111-4111-8111-111111111111'), 0);

online = false;
await sync.closeSession('interrupted', { x_m: 0.4, y_m: 0.2 });
assert.equal(await database.countOutboxItems('11111111-1111-4111-8111-111111111111'), 1);
assert.equal(states.at(-1).pending, 1);
sync.dispose();

console.log('Dashboard offline sync tests passed.');
