// watcher-api.js
// ─────────────────────────────────────────────────────────────────────────────
// Bridge between the Web Console UI and the ESP32-S3 firmware.
//
// On the device, the firmware:
//   1. Serves the Web Console HTML at GET /
//   2. Injects current NVS state into <script id="watcher-bootstrap" type="application/json">
//      so the UI starts with the live device state (no extra round-trip).
//   3. Implements the REST endpoints below — each maps to a single NVS namespace key.
//   4. Pushes live updates over /ws (optional — UI also polls if WS missing).
//
// In the design preview (no firmware), WatcherAPI runs in OFFLINE mode and just
// keeps state in memory + localStorage.
// ─────────────────────────────────────────────────────────────────────────────

(function (root) {
  'use strict';

  // ── REST endpoints (firmware should implement) ────────────────────────────
  // All return JSON. PUT/POST bodies are JSON. NVS namespace = "watcher".
  const ENDPOINTS = {
    // --- read-only telemetry ---
    device:   { method: 'GET',  path: '/api/device'   }, // { uptime_s, heap_free, psram, cpu_pct, fw, epd_ctrl }
    network:  { method: 'GET',  path: '/api/network'  }, // { mode, ssid, ip, rssi, mdns, ntp_synced, ws_open }

    // --- config (NVS) ---
    cfg:      { method: 'GET',  path: '/api/cfg'      }, // full cfg blob (see CFG_DEFAULTS below)
    cfgPatch: { method: 'PATCH',path: '/api/cfg'      }, // partial update; firmware merges + writes NVS

    // --- collections (each persisted as a single NVS blob key) ---
    alarms:   { method: 'GET',  path: '/api/alarms'   }, putAlarms: { method: 'PUT', path: '/api/alarms' },
    events:   { method: 'GET',  path: '/api/events'   }, putEvents: { method: 'PUT', path: '/api/events' },
    tasks:    { method: 'GET',  path: '/api/tasks'    }, putTasks:  { method: 'PUT', path: '/api/tasks'  },

    // --- system actions ---
    refresh:  { method: 'POST', path: '/api/refresh'  }, // force EPD full refresh
    otaMode:  { method: 'POST', path: '/api/ota'      }, // body: { enable: bool }  → toggles AP-setup mode
    reboot:   { method: 'POST', path: '/api/reboot'   }, // soft reboot via esp_restart()
    screen:   { method: 'POST', path: '/api/screen'   }, // body: { id: 'clock'|'alarm'|... } — switch active screen
    hwBtn:    { method: 'POST', path: '/api/hwbtn'    }, // body: { btn: 1|2|3, long: bool } — simulate hw button

    // --- live stream ---
    ws:       { path: '/ws' }, // pushes { type: 'state', ... } / { type: 'log', ... }
  };

  // ── NVS keys (suggested ESP-IDF layout) ───────────────────────────────────
  // Inside namespace "watcher", store each as a JSON-encoded blob:
  //   nvs_set_blob(h, "cfg",    json, len);
  //   nvs_set_blob(h, "alarms", json, len);
  //   nvs_set_blob(h, "events", json, len);
  //   nvs_set_blob(h, "tasks",  json, len);
  // Firmware reloads these at boot and re-applies on PUT/PATCH.
  const NVS = {
    namespace: 'watcher',
    keys: { cfg: 'cfg', alarms: 'alarms', events: 'events', tasks: 'tasks' },
  };

  // ── Default config (matches NVS schema) ───────────────────────────────────
  const CFG_DEFAULTS = {
    theme:       'light',           // device EPD theme: light | dark
    apMode:      false,
    wifiSsid:    'WATCHER-NET-5G',
    wifiPass:    '••••••••••',
    deviceName:  'watcher',
    timezone:    'Asia/Kolkata',
    ntpServer:   'pool.ntp.org',
    buzzer:      true,
    timeFormat:  '24h',
    dateFormat:  'long',
    focusMins:   25,
    breakMins:   5,
    longMins:    15,
    sessionsBeforeLong: 4,
  };

  // ── Bootstrap blob ────────────────────────────────────────────────────────
  // Firmware should replace the contents of <script id="watcher-bootstrap"> at
  // serve time with the current NVS state. Schema:
  //   { cfg: {...}, alarms: [...], events: [...], tasks: [...],
  //     device: {...}, network: {...}, fwVer: "7.1.0" }
  function readBootstrap() {
    const el = document.getElementById('watcher-bootstrap');
    if (!el) return null;
    try { const t = el.textContent.trim(); return t ? JSON.parse(t) : null; }
    catch (e) { console.warn('[watcher] bootstrap parse failed:', e); return null; }
  }

  // ── Fetch wrappers ────────────────────────────────────────────────────────
  const ONLINE = typeof window !== 'undefined' && /^https?:/.test(location.protocol)
    && !!readBootstrap(); // only treat as online if firmware injected bootstrap

  async function call(ep, body) {
    if (!ONLINE) return null;
    const opts = { method: ep.method, headers: { 'Content-Type': 'application/json' } };
    if (body !== undefined) opts.body = JSON.stringify(body);
    try {
      const r = await fetch(ep.path, opts);
      if (!r.ok) throw new Error(`${ep.method} ${ep.path} → ${r.status}`);
      const ct = r.headers.get('content-type') || '';
      return ct.includes('json') ? r.json() : r.text();
    } catch (e) { console.warn('[watcher]', e.message); return null; }
  }

  // Debounced PUT — collapse rapid edits into one write.
  function debounced(fn, ms = 400) {
    let id; return (...a) => { clearTimeout(id); id = setTimeout(() => fn(...a), ms); };
  }

  // ── Public API ────────────────────────────────────────────────────────────
  const WatcherAPI = {
    ONLINE,
    ENDPOINTS, NVS, CFG_DEFAULTS,
    bootstrap: readBootstrap(),

    // collection getters
    getAlarms: () => call(ENDPOINTS.alarms),
    getEvents: () => call(ENDPOINTS.events),
    getTasks:  () => call(ENDPOINTS.tasks),
    getCfg:    () => call(ENDPOINTS.cfg),

    // collection setters (debounced)
    putAlarms: debounced((alarms) => call(ENDPOINTS.putAlarms, alarms)),
    putEvents: debounced((events) => call(ENDPOINTS.putEvents, events)),
    putTasks:  debounced((tasks)  => call(ENDPOINTS.putTasks,  tasks)),
    patchCfg:  debounced((patch)  => call(ENDPOINTS.cfgPatch,  patch)),

    // actions
    refresh:   ()      => call(ENDPOINTS.refresh),
    setOtaMode:(on)    => call(ENDPOINTS.otaMode, { enable: !!on }),
    reboot:    ()      => call(ENDPOINTS.reboot),
    setScreen: (id)    => call(ENDPOINTS.screen, { id }),
    pressBtn:  (b, lp) => call(ENDPOINTS.hwBtn,  { btn: b, long: !!lp }),

    // ── WebSocket live link ────────────────────────────────────────────────
    // Connect from app: WatcherAPI.connectWs({ onState, onLog, onOpen, onClose })
    connectWs(handlers = {}) {
      if (!ONLINE || typeof WebSocket === 'undefined') return null;
      const url = `${location.protocol === 'https:' ? 'wss' : 'ws'}://${location.host}${ENDPOINTS.ws.path}`;
      const ws = new WebSocket(url);
      ws.onopen  = () => handlers.onOpen && handlers.onOpen();
      ws.onclose = () => handlers.onClose && handlers.onClose();
      ws.onmessage = (e) => {
        try {
          const m = JSON.parse(e.data);
          if (m.type === 'state' && handlers.onState) handlers.onState(m);
          if (m.type === 'log'   && handlers.onLog)   handlers.onLog(m);
        } catch (_) { /* ignore */ }
      };
      return ws;
    },
  };

  root.WatcherAPI = WatcherAPI;
})(typeof window !== 'undefined' ? window : globalThis);
