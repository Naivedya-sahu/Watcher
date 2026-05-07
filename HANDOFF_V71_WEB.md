# Watcher v7.1 — Web-Driven Display Architecture
## New Session Handoff Prompt

---

## Project Identity

**The Watcher** — ESP32-S3 e-paper desktop productivity companion.
- Hardware: ESP32-S3-WROOM-1-**N8R8** (8MB flash, 8MB PSRAM)
- Display: GDEY042T81, SSD1683 controller, **400×300px, 1-bit B/W**
- partial_refresh ≈ 400ms, full_refresh ≈ 1200ms
- Repository root: `D:\Development\Personal\Watcher\`

---

## What Exists (Do Not Modify)

### v7.0 — Firmware (ESP-IDF 5.x, code-complete, pending hardware test)
**Location:** `D:\Development\Personal\Watcher\v7.0\`

All peripheral drivers are complete and stable. **Do not touch these.**

```
v7.0/components/
    epd/          — SSD1683 driver (SPI, 4MHz, full+partial refresh)
    fb/           — 1-bit framebuffer (400×300, 15000 bytes)
                    fb_set_pixel, fb_fill_rect, fb_draw_rect
                    fb_draw_str, fb_draw_str_centered
                    fb_draw_pill, fb_fill_pill
                    fb_draw_dot_ring (58-dot ring, clockwise fill)
                    fb_draw_7seg_digit, fb_draw_7seg_colon
                    fb_draw_play_tri, fb_draw_pause, fb_draw_stop_sq
    fonts/        — font_mono6.h (6×8 bitmap, 95 ASCII chars)
    button/       — ISR driver, short/long press, IO39/40/41
    buzzer/       — LEDC patterns: BOOT/TICK/SUCCESS/ERROR/ALERT/POMO_START/POMO_DONE
    time_svc/     — NTP-only (no RTC). Clock shows "--:--" until sync.
    rtc/          — DS3231 stub (kept, not wired — no RTC module)
```

**Hardware pins (FROZEN — Main PCB Rev 2.0):**
```
EPD SPI:  MOSI=IO11, CLK=IO12, CS=IO10, DC=IO15, RST=IO16, BUSY=IO17
Buttons:  BTN_1=IO39 (prev), BTN_2=IO40 (next), BTN_3=IO41 (pomo toggle)
Buzzer:   IO38 (LEDC timer0 ch0)
WiFi creds via env vars: WIFI_SSID, WIFI_PASS, DEVICE_TZ, NTP_SERVER
```

**Upload strategy (LOCKED):**
- UART via FTDI: all wired flashing → `idf.py -p COM<N> flash`
- OTA: wireless via `esp_https_ota`
- Native USB: monitor ONLY, never flash

### Design System
**Location:** `D:\Development\Personal\Watcher\v7.0\Watcher Design System\`

The design system contains HTML/CSS previews for every screen component.
The file `ui_kits/watcher-device/index.html` is the **ground-truth pixel reference** —
it has all screens implemented as 400×300 HTML with exact CSS geometry.
Read this file carefully before implementing anything.

Key design rules (from README.md):
- Palette: ink=#000000, paper=#FFFFFF, ghost=#E6E6E6
- Font: Share Tech Mono (web placeholder); Monocraft target (not yet decided)
- 8px base grid; no shadows, no gradients, no rounded cards
- ALL CAPS for action labels; no emoji; minimal copy

---

## What to Build: v7.1

**Location:** `D:\Development\Personal\Watcher\v7.1\` (create this directory)

### The Core Idea

Instead of hardcoded C++ rendering logic, **screens are 400×300 HTML files**.
The ESP32-S3 becomes a web server. Screens live in SPIFFS as HTML.
A headless HTML→bitmap renderer converts the HTML to a 1-bit image and pushes it to the EPD.

**The full loop:**
```
Developer edits screen HTML on PC
    → uploads to device via browser at http://watcher.local/
    → device re-renders to EPD within 2 seconds
    → no recompile, no reflash needed
```

**The device also renders autonomously** — clock ticks, pomodoro counts down,
buttons navigate — all without a browser connected.

---

## Architecture

### Layer Stack

```
┌────────────────────────────────────────────────────────┐
│  BROWSER (any device on WiFi)                          │
│  http://watcher.local/ — live EPD mirror + controls    │
│  Upload new screen HTML, trigger buzzer, switch screen │
└──────────────────┬─────────────────────────────────────┘
                   │  HTTP REST + WebSocket
┌──────────────────▼─────────────────────────────────────┐
│  web_server component  (esp_http_server + WS)          │
│  Serves HTML files from SPIFFS                         │
│  REST API for state/control                            │
│  WebSocket pushes state JSON on every change           │
└──────────────────┬─────────────────────────────────────┘
                   │
┌──────────────────▼─────────────────────────────────────┐
│  renderer component                                    │
│  Loads screen HTML from SPIFFS                         │
│  Substitutes {{template_vars}} from state              │
│  Parses layout instructions → calls fb_* functions     │
│  fb_flush() to EPD                                     │
└──────────────────┬─────────────────────────────────────┘
                   │
┌──────────────────▼─────────────────────────────────────┐
│  state module                                          │
│  watcher_state_t — all template variable values        │
│  state_tick() — update time, pomo countdown            │
│  state_get_var(name) → string                          │
└──────────────────┬─────────────────────────────────────┘
                   │
┌──────────────────▼─────────────────────────────────────┐
│  UNCHANGED from v7.0                                   │
│  epd │ fb │ fonts │ button │ buzzer │ time_svc          │
└────────────────────────────────────────────────────────┘
```

### Screen HTML Format

Each screen is a **400×300 HTML file** stored in SPIFFS at `/screens/`.
It uses a restricted but real HTML/CSS subset designed for 1-bit rendering.

**Design philosophy:**
- HTML is written exactly like the design system HTML (it IS the design system HTML, adapted)
- The renderer does NOT run JavaScript on-device
- Dynamic values injected via `{{template_var}}` substitution before parse
- CSS `position:absolute` with pixel coordinates maps directly to fb calls
- Color: anything that resolves to black (#000, #111, rgb(0,0,0)) → FB_BLACK. White → FB_WHITE.

**Example: `/screens/clock.html`**
```html
<!DOCTYPE html>
<html>
<head>
<style>
body { margin:0; background:#fff; width:400px; height:300px; }
.digit { position:absolute; width:62px; height:110px; }
.seg { position:absolute; background:#111; }
.sh { left:8px; width:46px; height:9px; }   /* horizontal segment */
.sv { width:9px; height:41px; }             /* vertical segment */
.seg-top { top:0px; }   .seg-mid { top:50px; }  .seg-bot { top:101px; }
.seg-tl  { left:0px; top:9px; }   .seg-tr { left:53px; top:9px; }
.seg-bl  { left:0px; top:60px; }  .seg-br { left:53px; top:60px; }
.colon-wrap { position:absolute; width:18px; }
.colon-dot  { position:absolute; width:9px; height:9px; background:#111; }
.date { position:absolute; font-family:monospace; font-size:11px; color:#000; }
</style>
</head>
<body>
<!-- Hour tens (skip if zero) -->
{{#if h1_nonzero}}
<div class="digit" style="left:61px;top:85px;">
  {{#7seg value="{{h1}}"}}
</div>
{{/if}}
<!-- Hour units -->
<div class="digit" style="left:126px;top:85px;">
  {{#7seg value="{{h2}}"}}
</div>
<!-- Colon -->
{{#if colon_on}}
<div class="colon-wrap" style="left:191px;top:85px;">
  <div class="colon-dot" style="top:35px;left:4px;"></div>
  <div class="colon-dot" style="top:60px;left:4px;"></div>
</div>
{{/if}}
<!-- Minute tens -->
<div class="digit" style="left:212px;top:85px;">
  {{#7seg value="{{m1}}"}}
</div>
<!-- Minute units -->
<div class="digit" style="left:277px;top:85px;">
  {{#7seg value="{{m2}}"}}
</div>
<!-- Date -->
<div class="date" style="left:{{date_x}}px;top:205px;">
  {{weekday}}, {{mday}}{{ordinal}} {{month}}. {{year}}
</div>
</body>
</html>
```

### Template Variables (full set)

| Variable | Value | Example |
|----------|-------|---------|
| `{{h1}}` | Hour tens | `1` |
| `{{h2}}` | Hour units | `4` |
| `{{m1}}` | Minute tens | `3` |
| `{{m2}}` | Minute units | `0` |
| `{{h1_nonzero}}` | `1` if h1≠0, `0` if h1=0 | For `{{#if}}` |
| `{{colon_on}}` | `1` / `0` | Blink state |
| `{{weekday}}` | `Saturday` | |
| `{{mday}}` | `26` | |
| `{{ordinal}}` | `th` | |
| `{{month}}` | `Apr` | |
| `{{year}}` | `2026` | |
| `{{date_x}}` | Centered x for date string | `auto-calculated` |
| `{{ntp_synced}}` | `1` / `0` | |
| `{{pomo_mm}}` | `24` | Zero-padded |
| `{{pomo_ss}}` | `17` | Zero-padded |
| `{{pomo_filled}}` | `38` | 0-58 dot count |
| `{{pomo_running}}` | `1` / `0` | |
| `{{pomo_paused}}` | `1` / `0` | |
| `{{pomo_stopped}}` | `1` / `0` | |
| `{{pomo_mode}}` | `FOCUS` / `BREAK` / `LONG` | |
| `{{mode_is_focus}}` | `1` / `0` | |
| `{{mode_is_break}}` | `1` / `0` | |
| `{{mode_is_long}}` | `1` / `0` | |
| `{{screen}}` | `clock` | Current screen |
| `{{fw_ver}}` | `7.1.0` | |

### HTML Custom Tags (device renderer handles these)

```html
{{#7seg value="3"}}        <!-- Expands to 7 .seg divs for digit 3 -->
{{#dot_ring filled="38"}}  <!-- Expands to 58 positioned squares -->
{{#if condition}}...{{/if}} <!-- Conditional block -->
```

These are expanded BEFORE the HTML is rendered. The renderer replaces them
with plain HTML divs that the CSS layout engine then positions.

---

## REST API

```
GET  /api/state              → JSON: all template vars as key-value
POST /api/render             → force re-render current screen to EPD
GET  /api/screen             → {"screen": "clock"}
POST /api/screen/{name}      → switch to screen (clock/pomodoro/calendar/tasks)
POST /api/buzzer/{tone}      → fire buzzer (boot/tick/success/error/alert/pomo_start/pomo_done)
POST /api/pomo/toggle        → start / pause / resume
POST /api/pomo/reset         → stop + reset to FOCUS
GET  /api/bitmap             → raw 15000-byte 1-bit framebuffer (for browser preview)
GET  /api/screens            → list JSON of available screen files in SPIFFS
POST /api/upload             → multipart upload: saves file to SPIFFS path, triggers re-render
WS   /ws                     → WebSocket: server pushes state JSON on every change
```

---

## Browser UI (`/www/index.html` served from SPIFFS)

A single-page app served from the device at `http://watcher.local/`:

```
┌──────────────────────────────────────────────┐
│ WATCHER  ●Connected   v7.1.0                 │
│ ┌────────────────────────────────────────┐   │
│ │  400×300 Canvas — live EPD mirror      │   │
│ │  (redraws from WebSocket state)        │   │
│ │                                        │   │
│ │         9:41                           │   │
│ │   Sunday, 27th Apr. 2026               │   │
│ └────────────────────────────────────────┘   │
│  [Clock] [Pomodoro] [Calendar] [Tasks]       │
│  [▶ Start] [⏹ Reset] [🔔 Test Buzzer]       │
│  [Upload Screen HTML]                        │
└──────────────────────────────────────────────┘
```

- Canvas redraws using WebSocket state pushes — mirrors EPD in real-time
- Canvas uses SAME CSS as `Watcher Design System/ui_kits/watcher-device/index.html`
- Screen switcher calls `POST /api/screen/{name}`
- Upload button: file picker → `POST /api/upload` → device re-renders → canvas updates

---

## File Structure to Create

```
v7.1/
├── CMakeLists.txt
├── partitions.csv           ← copy from v7.0 (SPIFFS already allocated)
├── sdkconfig.defaults       ← copy from v7.0
│
├── components/
│   ├── epd/                 ← COPY from v7.0, unchanged
│   ├── fb/                  ← COPY from v7.0, unchanged
│   ├── fonts/               ← COPY from v7.0, unchanged
│   ├── button/              ← COPY from v7.0, unchanged
│   ├── buzzer/              ← COPY from v7.0, unchanged
│   ├── time_svc/            ← COPY from v7.0, unchanged
│   │
│   ├── renderer/            ← NEW
│   │   ├── CMakeLists.txt
│   │   ├── renderer.h
│   │   └── renderer.cpp     ← HTML template → fb_* calls
│   │
│   └── web_server/          ← NEW
│       ├── CMakeLists.txt
│       ├── web_server.h
│       └── web_server.cpp   ← esp_http_server + WebSocket + REST
│
├── main/
│   ├── CMakeLists.txt
│   ├── board_config.h       ← COPY from v7.0, unchanged
│   ├── state.h              ← NEW: watcher_state_t + all template vars
│   ├── state.cpp            ← NEW: tick, pomo logic, var resolver
│   └── main.cpp             ← NEW: init + main loop (no AppManager)
│
└── spiffs_data/             ← Flashed to SPIFFS partition
    ├── screens/
    │   ├── clock.html       ← 400×300 screen HTML with {{vars}}
    │   ├── pomodoro.html
    │   ├── calendar.html    ← stub (empty screen + "Coming Soon")
    │   └── tasks.html       ← stub
    └── www/
        └── index.html       ← Browser compositor + controls
```

---

## Renderer Implementation Notes

The renderer does NOT run a real browser. It implements a **restricted CSS layout parser**:

### What to support (in priority order):

1. **`position:absolute` + `left`/`top`/`width`/`height` in px**
   Maps to: `fb_fill_rect(x, y, w, h, color)` for any colored element

2. **`color` / `background` / `background-color`**
   Maps to: FB_BLACK if resolves to dark (#000..#333, rgb<50,50,50), FB_WHITE otherwise

3. **Font rendering via `font-family:monospace` + `font-size`**
   Maps to: `fb_draw_str(x, y, text, color)` using font_mono6
   Scale = round(font-size / 8) — font_mono6 is 8px tall at scale 1

4. **`text-align:center` with known container width**
   Maps to: `fb_draw_str_centered(cx, y, text, color)`

5. **Custom tag `{{#7seg value="N"}}`** — pre-expanded before parse
   Expands to 7 `<div>` elements with `.seg` class and correct positions

6. **Custom tag `{{#dot_ring filled="N"}}`** — pre-expanded before parse
   Expands to 58 `<div>` elements at exact dot ring positions
   (positions: top y=49 x=59..331 step16, right x=331 y=65..225 step16,
    bottom y=241 x=331..59 step-16, left x=59 y=225..65 step-16)

7. **`border-radius: 999px` on a pill shape**
   Maps to: `fb_draw_pill` or `fb_fill_pill` based on fill color

8. **`{{#if var}}...{{/if}}`** — evaluated before parse, block stripped if var="0"

### What to explicitly NOT support:
- JavaScript (no JS engine on device)
- Flexbox / Grid layout
- `%` units (pixels only)
- External fonts or images
- Animations / transitions
- Media queries

### Parse approach:
Use a simple recursive descent HTML parser or a line-by-line `style=` attribute scanner.
No need for a full DOM — just walk elements, extract position+color+text, call fb functions.
cJSON is available (ESP-IDF built-in) but HTML needs a lightweight custom parser.
Alternatively: use a tiny XML parser (TinyXML2 or similar as a component).

---

## State Module

### `state.h` — watcher_state_t

```c
typedef struct {
    // Clock
    int  h1, h2, m1, m2;
    bool h1_nonzero;
    bool colon_on;
    char weekday[12];   // "Saturday"
    int  mday;
    char ordinal[4];    // "th"
    char month[5];      // "Apr"
    int  year;
    bool ntp_synced;

    // Pomodoro
    uint32_t pomo_remaining;  // seconds
    uint32_t pomo_total;
    int      pomo_filled;     // 0-58
    bool     pomo_running;
    bool     pomo_paused;
    char     pomo_mode[10];   // "FOCUS" / "BREAK" / "LONG"
    bool     mode_is_focus, mode_is_break, mode_is_long;
    int      session_idx;

    // Navigation
    char current_screen[16];

    // System
    char fw_ver[10];
    uint32_t last_change_ms;  // for dirty detection
} watcher_state_t;

extern watcher_state_t g_state;

void state_init(void);
void state_tick(void);           // call every 100ms from main loop
void pomo_start_stop(void);
void pomo_reset(void);
const char *state_get_var(const char *name);  // "h1" → "1"
```

---

## Pomodoro Logic (move from v7.0 pomodoro_app.cpp)

Session sequence: FOCUS(25m) → BREAK(5m) → FOCUS → BREAK → FOCUS → BREAK → LONG(15m), repeat.
- `pomo_start_stop()`: if stopped→start, if running→pause, if paused→resume
- `pomo_reset()`: stop, session_idx=0, mode=FOCUS, remaining=total=25*60
- Countdown: 1s tick using `esp_timer_get_time()`, drift-free: `last_tick_ms += 1000`
- On session complete: `buzzer_tone(BUZZ_POMO_DONE)`, advance session_idx, set new mode+total

---

## Main Loop (no AppManager needed)

```c
void main_task(void *arg) {
    // init: buzzer → time_svc → epd → fb → button → wifi/event_loop
    // web_server_start() — starts HTTP server + WebSocket
    // renderer_load_screen("clock") — initial render

    while (true) {
        button_tick();
        state_tick();       // updates time, pomo countdown, dirty flag
        if (state_needs_render()) {
            renderer_render(&g_state, g_state.current_screen);
            web_server_push_state(&g_state);  // WebSocket broadcast
        }
        vTaskDelay(pdMS_TO_TICKS(50));  // 20Hz — EPD doesn't need faster
    }
}
```

---

## Button Handling

```
BTN_1 short → app_prev() → switch to prev screen
BTN_2 short → app_next() → switch to next screen
BTN_3 short → pomo_start_stop()
BTN_3 long  → pomo_reset()
```

Screen order: clock → pomodoro → calendar → tasks → (wrap)

---

## SPIFFS Flash Workflow

Add to CMakeLists.txt:
```cmake
spiffs_create_partition_image(spiffs spiffs_data FLASH_IN_PROJECT)
```

This auto-flashes `spiffs_data/` to the SPIFFS partition during `idf.py flash`.
To update only SPIFFS (faster than full reflash):
```bash
idf.py -p COM<N> spiffs_flash
```

---

## What v7.0 Had That v7.1 Drops

| v7.0 | v7.1 replacement |
|------|-----------------|
| AppManager vtable | Simple screen name + function table in main.cpp |
| clock_app.cpp | clock.html in SPIFFS |
| pomodoro_app.cpp | pomodoro.html + state.cpp |
| ota_app.cpp | REST endpoint, no dedicated screen yet |
| calendar_app.cpp (stub) | calendar.html stub in SPIFFS |
| tasks_app.cpp (stub) | tasks.html stub in SPIFFS |
| Hardcoded pixel coordinates in C | Coordinates in HTML style= attributes |

---

## Phase Plan for This Session

### Phase 1 — Core (must complete)
1. Create v7.1 directory, copy unchanged components from v7.0
2. Write `state.h` / `state.cpp` — state struct + tick + pomo logic + var resolver
3. Write `renderer.h` / `renderer.cpp` — HTML template parser → fb calls
4. Write `screens/clock.html` and `screens/pomodoro.html`
5. Write `main.cpp` — init + loop, no AppManager
6. Verify build compiles

### Phase 2 — Web Server
1. Write `web_server.h` / `web_server.cpp`
2. All REST endpoints
3. WebSocket state broadcast
4. `/api/upload` for screen file upload

### Phase 3 — Browser UI
1. Write `www/index.html` — canvas mirror + controls
2. WebSocket client in HTML
3. Screen switcher, buzzer test, pomo controls
4. Upload button for screen HTML files

### Phase 4 — Screen stubs
1. `screens/calendar.html` — shows "Calendar — Phase 4" placeholder
2. `screens/tasks.html` — shows "Tasks — Phase 4" placeholder

---

## Critical Rules

1. **Never touch v7.0** — it's the fallback if v7.1 has problems
2. **Component drivers (epd, fb, fonts, button, buzzer, time_svc) are copied verbatim** — do not modify
3. **Main PCB is FROZEN** — no pin changes
4. **No RTC** — `time_svc` is NTP-only; clock shows "--:--" until NTP syncs
5. **HTML screens are 400×300 pixels exactly** — match design system geometry
6. **No JS execution on device** — renderer is layout-only; all logic is in C state module
7. **Renderer uses fb_* primitives only** — no new drawing code; if a shape can't be expressed as fb calls, it can't be in a screen HTML
8. **cJSON is built into ESP-IDF** — use it for REST JSON responses; HTML needs a custom parser
9. **LEDC API is ESP-IDF native** — `ledc_timer_config` + `ledc_channel_config`, not Arduino

---

## Reference Files to Read First

```
v7.0/components/fb/fb.h                              — all draw functions (copy these)
v7.0/components/fb/fb.cpp                            — pixel addressing, dot ring positions
v7.0/components/buzzer/buzzer.h                      — tone enum
v7.0/main/board_config.h                             — all GPIO pins
v7.0/main/apps/clock_app.cpp                         — clock logic to port to state.cpp
v7.0/main/apps/pomodoro_app.cpp                      — pomo logic to port to state.cpp
v7.0/partitions.csv                                  — copy for v7.1
v7.0/sdkconfig.defaults                              — copy for v7.1
v7.0/Watcher Design System/ui_kits/watcher-device/index.html  — ALL screen HTML reference
v7.0/Watcher Design System/README.md                 — design rules and palette
v7.0/DESIGN_PLAN.md                                  — dot ring positions, 7-seg geometry
```

---

## Quick Design Geometry Reference

**7-segment digit — 62×110px bounding box:**
```
Segment    dx    dy    w    h
top        8     0    46    9
tl         0     9     9   41
tr        53     9     9   41
mid        8    50    46    9
bl         0    60     9   41
br        53    60     9   41
bot        8   101    46    9
```
Digit bitmask [top,tl,tr,mid,bl,br,bot]:
0=1110111, 1=0010010, 2=1011101, 3=1011011, 4=0111010,
5=1101011, 6=1101111, 7=1010010, 8=1111111, 9=1111011

**Colon — 18×110px:**
Two 9×9 dots at (x+4, y+35) and (x+4, y+60)

**Clock digit X positions (centered on 400px):**
D1=61, D2=126, Colon=191, D3=212, D4=277

**Dot ring — 58 squares of 10×10px clockwise:**
Top 18:    y=49,  x = 59,75,91...331  (step +16)
Right 11:  x=331, y = 65,81...225     (step +16)
Bottom 18: y=241, x = 331,315...59    (step -16)
Left 11:   x=59,  y = 225,209...65    (step -16)
Content area inside ring: left=59, top=49, width=282, height=202

**Pomodoro inner content center:** (200, 150)
MM:SS text at y=128 (scale 4 = 24×32px per char)
Mode label at y=172
Button row at y=254 (pill h=24px)
Pill positions: FOCUS x=125 w=62, BREAK x=193 w=60, LONG x=259 w=58
Play/Stop icon centers: (36, 266) and (375, 266)
