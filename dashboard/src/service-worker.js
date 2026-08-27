/// <reference no-default-lib="true"/>
/// <reference lib="esnext" />
/// <reference lib="webworker" />
/// <reference types="@sveltejs/kit" />

import { build, files, version } from '$service-worker';

const worker = /** @type {ServiceWorkerGlobalScope} */ (
  /** @type {unknown} */ (globalThis)
);
const CACHE = `deeptrack-shell-${version}`;
// Large landing-page photos are runtime-cached when visited. Keeping them out
// of the install transaction makes the offline shell available immediately
// and avoids one missing photo preventing service-worker activation.
const PRECACHE_FILES = files.filter((path) => !path.startsWith('/images/'));
const APP_ASSETS = [...new Set([...build, ...PRECACHE_FILES])];

worker.addEventListener('install', (event) => {
  event.waitUntil((async () => {
    const cache = await caches.open(CACHE);
    await cache.addAll(APP_ASSETS);
    await worker.skipWaiting();
  })());
});

worker.addEventListener('activate', (event) => {
  event.waitUntil((async () => {
    for (const key of await caches.keys()) {
      if (key.startsWith('deeptrack-shell-') && key !== CACHE) {
        await caches.delete(key);
      }
    }
    await worker.clients.claim();
  })());
});

/** @param {Request} request */
function isCacheableRequest(request) {
  if (request.method !== 'GET') return false;
  const url = new URL(request.url);
  if (url.origin !== worker.location.origin) return false;
  if (url.pathname === '/auth/callback') return false;
  return !request.headers.has('authorization');
}

worker.addEventListener('fetch', (event) => {
  const request = event.request;
  if (!isCacheableRequest(request)) return;

  event.respondWith((async () => {
    const url = new URL(request.url);
    const cache = await caches.open(CACHE);

    if (APP_ASSETS.includes(url.pathname)) {
      const cachedAsset = await cache.match(url.pathname);
      if (cachedAsset) return cachedAsset;
    }

    try {
      const response = await fetch(request);
      if (response.ok &&
          !response.headers.get('cache-control')?.includes('no-store')) {
        await cache.put(request, response.clone());
      }
      return response;
    } catch (error) {
      const cached = await cache.match(request);
      if (cached) return cached;

      if (request.mode === 'navigate') {
        const routeFallback = await cache.match(url.pathname) ||
          await cache.match('/dashboard/demo') || await cache.match('/');
        if (routeFallback) return routeFallback;
      }
      throw error;
    }
  })());
});
