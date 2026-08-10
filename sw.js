/* Service worker: приложение целиком кладётся в кэш и работает без сети.
   При выпуске новой версии достаточно поднять CACHE — старый кэш удалится сам. */
const CACHE = 'utilization-knt-v1';

const SHELL = [
  './',
  './index.html',
  './app.css',
  './app.js',
  './vendor/xlsx.full.min.js',
  './manifest.webmanifest',
  './icons/icon-192.png',
  './icons/icon-512.png',
  './icons/icon-maskable-512.png'
];

self.addEventListener('install', (e) => {
  e.waitUntil((async () => {
    const cache = await caches.open(CACHE);
    // addAll падает целиком, если хоть один файл недоступен, — кладём по одному
    await Promise.all(SHELL.map((url) => cache.add(url).catch(() => {})));
    await self.skipWaiting();
  })());
});

self.addEventListener('activate', (e) => {
  e.waitUntil((async () => {
    const keys = await caches.keys();
    await Promise.all(keys.filter((k) => k !== CACHE).map((k) => caches.delete(k)));
    await self.clients.claim();
  })());
});

self.addEventListener('fetch', (e) => {
  const req = e.request;
  if (req.method !== 'GET') return;
  if (new URL(req.url).origin !== self.location.origin) return;

  // stale-while-revalidate: из кэша отвечаем сразу (работает без сети),
  // а свежую версию подтягиваем фоном — обновление доедет со следующим открытием
  e.respondWith((async () => {
    const cache = await caches.open(CACHE);
    const cached = await cache.match(req, { ignoreSearch: true });

    const network = fetch(req).then((res) => {
      if (res && res.ok && res.type === 'basic') cache.put(req, res.clone());
      return res;
    }).catch(() => null);

    if (cached) {
      e.waitUntil(network);
      return cached;
    }

    const res = await network;
    if (res) return res;

    // навигация без сети и без кэша — отдаём стартовую страницу
    if (req.mode === 'navigate') {
      const fallback = await cache.match('./index.html');
      if (fallback) return fallback;
    }
    return Response.error();
  })());
});
