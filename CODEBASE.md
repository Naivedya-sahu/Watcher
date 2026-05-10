# Watcher Firmware — Codebase Summary

> **Target:** ESP32-S3-WROOM-1-N8R8 · **Display:** Waveshare GDEY042T81 4.2" e-paper (400×300, B/W)
> **Version:** v8.0.0 (branch: main) · **IDF:** ESP-IDF v6.0 · **CPU:** 240 MHz Xtensa LX7

---

## 1. Project Overview

Watcher is a wrist-worn / desk e-paper device running on ESP32-S3. It displays a clock, pomodoro timer, task list, alarms, calendar, and settings. All user interaction in v8 is via the **web console** (no physical buttons/encoder in production; v7.1 had them). The device connects to WiFi, syncs time via NTP, and serves an HTTP+WebSocket web console at `watcher.local`.

---

## 2. Directory Structure

```
/
├── main/                       # Application entry point + screen logic
│   ├── main.cpp                # app_main: boot, wifi, NTP, main loop
│   ├── board_config.h          # GPIO pin assignments + HARDWARE_ENABLED flags
│   ├── screen_mgr.{h,cpp}      # Screen registry, lifecycle, navigation
│   ├── ota_task.{h,cpp}        # Push-OTA state machine
│   └── screens/
│       ├── clock_screen.{h,cpp}
│       ├── pomo_screen.{h,cpp}
│       ├── alarm_screen.{h,cpp}
│       ├── tasks_screen.{h,cpp}
│       ├── settings_screen.{h,cpp}
│       └── cal_screen.{h,cpp}
│
├── components/                 # Custom ESP-IDF components
│   ├── button/                 # ISR debounced button driver (short/long press)
│   ├── buzzer/                 # LEDC-based buzzer with named patterns
│   ├── config_store/           # NVS-backed settings (watcher_cfg_t)
│   ├── encoder/                # EC11 quadrature encoder (ISR + poll)
│   ├── epd/                    # SSD1683 e-paper SPI driver (full/partial refresh)
│   ├── fb/                     # 1-bit 400×300 framebuffer + drawing API
│   ├── fonts/                  # Bitmap font data (Monocraft 6×8, mono6)
│   ├── gfx/                    # 7-segment digit renderer, primitive shapes
│   ├── time_svc/               # NTP time sync (no RTC hardware)
│   └── web_server/             # HTTP + WebSocket server (REST API + console)
│
├── spiffs_data/
│   ├── www/webserver.html      # Web console (single-page app, served from SPIFFS)
│   └── screens/                # HTML mockups: clock, pomodoro, tasks, settings
│
├── tests/test_periph/          # Standalone peripheral bring-up test firmware
├── design/                     # Design files (excluded from build)
├── CMakeLists.txt              # Top-level IDF project config
├── partitions.csv              # Custom 8MB partition table
├── sdkconfig.defaults          # Build defaults (flash, CPU freq, WiFi, OTA)
└── ota_push_upload.bat         # Windows helper to POST firmware binary
```

---

## 3. Hardware Configuration (`board_config.h`)

| Pin   | Signal         | Notes                                              |
|-------|----------------|----------------------------------------------------|
| IO11  | EPD MOSI       | SPI2_HOST (FSPI)                                   |
| IO12  | EPD CLK        |                                                    |
| IO10  | EPD CS         |                                                    |
| IO15  | EPD DC         |                                                    |
| IO16  | EPD RST        |                                                    |
| IO17  | EPD BUSY       |                                                    |
| IO18  | Buzzer         | LEDC PWM                                           |
| IO39  | BTN_1 (BTN A)  | Back / prev — active-low, 10K pull-up              |
| IO40  | BTN_2 (BTN B)  | Next / context — active-low                        |
| IO41  | BTN_3          | Pomodoro toggle / long=stop+reset                  |
| IO35  | Encoder A      | ISR ANYEDGE, 4.7K pull-up (conflicts with PSRAM)   |
| IO36  | Encoder B      |                                                    |
| IO37  | Encoder SW     | Routed through button driver                       |

**Build flags:**
- `HARDWARE_ENABLED 1` — enables EPD + buzzer init; `0` = simulation mode (fb_flush no-op)
- `ENABLE_INPUTS 1` — enables buttons + encoder; `0` = UART serial console commands only

**PSRAM:** Disabled (`CONFIG_SPIRAM=n`) — IO35/36/37 conflict with Octal PSRAM on N8R8 module.

---

## 4. Flash Partition Layout

| Partition | Type  | Offset    | Size    | Purpose                    |
|-----------|-------|-----------|---------|----------------------------|
| nvs       | data  | 0x9000    | 128 KB  | NVS config + WiFi creds    |
| otadata   | data  | 0x29000   | 8 KB    | OTA boot selection         |
| app0      | app   | 0x30000   | 3 MB    | Running firmware (ota_0)   |
| app1      | app   | 0x330000  | 3 MB    | OTA update target (ota_1)  |
| spiffs    | data  | 0x630000  | 1.9 MB  | Web console + HTML screens |

---

## 5. Architecture

### 5.1 Boot Sequence (`main.cpp`)
1. `nvs_flash_init()` → `cfg_load()` — load config from NVS
2. `time_svc_init()` — no-op (no RTC); system clock at epoch 0
3. Hardware init (if `HARDWARE_ENABLED`): EPD → FB → Buzzer → `BUZZ_BOOT` tone
4. Input init (if `ENABLE_INPUTS`): button driver + encoder ISR
5. WiFi connect (STA mode or AP mode per `g_cfg.ap_mode`)
6. On IP acquired → `time_svc_sync_ntp(tz, ntp_server)`
7. Register all screens → `screen_mgr_start()` → initial screen: **clock**
8. Web server start → register callbacks (screen, pomo, bitmap, input sim)
9. Main loop (50ms tick): `button_tick()` → `encoder_poll()` → `screen_mgr_tick()` → `web_server_poll()`

### 5.2 Screen Manager (`screen_mgr`)

Screens register via `screen_register(&screen_def)`. The manager maintains a flat list and a **cyclic prev/next** navigation (BTN_1 = prev, BTN_2 = next). Each screen is a `screen_def_t` vtable:

```c
typedef struct screen_def {
    const char *id;        // "clock", "pomo", "alarm", "calendar", "tasks", "settings"
    const char *label;     // "CLOCK", "POMODORO", ...
    const char *group;     // "time", "work", "sys"
    void (*enter)(void);
    void (*exit)(void);
    void (*tick)(void);           // 50ms
    void (*render)(fb_t *fb);
    void (*on_button)(btn_id_t, btn_evt_t);
    void (*on_encoder)(int delta);
    void (*on_enc_click)(void);
    bool needs_render;     // set by screen; cleared after render+flush
} screen_def_t;
```

**Navigation:** `screen_mgr_next()` / `screen_mgr_prev()` cycle through registered screens. `screen_mgr_goto(id)` jumps directly. Enter/exit called on transitions; full EPD refresh on screen change.

### 5.3 Input Routing

```
Physical:   BTN_1/2/3 ISR → button_tick() → screen_mgr_button()
            Encoder A/B ISR → encoder_poll() → screen_mgr_encoder()
            Encoder SW → button driver → screen_mgr_enc_click()

Virtual:    Web console WS → web_server_poll() → cb_btn()/cb_enc()
            UART serial (ENABLE_INPUTS=0): n/p/c/o/a/k/t/s/[/]/e/+/r/f/u/?
```

### 5.4 Display Pipeline

```
Screen render() writes to fb_t (15KB 1-bit buffer)
    ↓
fb_flush_auto(fb)  — picks partial (≤60 frames) or full refresh
    ↓
epd_flush(buf, PARTIAL|FULL)
    ↓
SPI → SSD1683 → GDEY042T81 e-paper panel
```

- **Full refresh:** ~1200ms, clears all ghosting
- **Partial refresh:** ~400ms, small region update via `epd_flush_region()`
- Auto-full after 60 partial frames (`FB_PARTIAL_LIMIT`)

---

## 6. Components

### 6.1 `button` — ISR Button Driver
- Up to 4 buttons (3 tactile + encoder click)
- Active-low, external 10K pull-ups
- Short/long press detection (`long_ms` threshold, default configurable)
- `button_tick()` called from main loop at ~10–20ms interval
- Callback: `void cb(button_id_t id, button_evt_t evt)`

### 6.2 `encoder` — EC11 Rotary Encoder
- Quadrature decoding via ISR on A+B channels (ANYEDGE)
- `ENC_PULSES_PER_STEP = 2` (configurable in NVS via `enc_pulses_per_step`)
- Callback: `enc_step_cb_t(int delta)` — `+1` = CW, `-1` = CCW
- `encoder_poll()` dispatches accumulated steps from main loop

### 6.3 `buzzer` — LEDC Buzzer
Named patterns:

| Pattern         | Sequence                         |
|-----------------|----------------------------------|
| `BUZZ_BOOT`     | 800Hz 80ms → 1200Hz 80ms         |
| `BUZZ_TICK`     | 2000Hz 1ms                       |
| `BUZZ_SUCCESS`  | 1000Hz 100ms → 1500Hz 150ms      |
| `BUZZ_ERROR`    | 300Hz 400ms                      |
| `BUZZ_ALERT`    | 1800Hz 80ms × 3                  |
| `BUZZ_POMO_START` | same as SUCCESS                |
| `BUZZ_POMO_DONE`  | 1000→1200→1500Hz ascending     |

Gated by `g_cfg.buzzer_on`.

### 6.4 `epd` — E-Paper Driver (GDEY042T81 / SSD1683)
- 400×300 pixels, 1-bit B/W
- Full waveform: clears ghosting (~1200ms); Partial: fast update (~400ms)
- `epd_flush_region()` for sub-region partial refresh (x/w must be byte-aligned)
- `epd_sleep()` → deep sleep mode (low power); `epd_hw_reset()` to wake

### 6.5 `fb` — Framebuffer
- 15000-byte 1-bit buffer; bit=1 → white, bit=0 → black
- Drawing API: pixels, filled/outline rects, hlines, vlines
- Text: Monocraft 6×8 bitmap font (built-in) + swappable `font_desc_t`
- `fb_draw_str_centered()`, `fb_draw_pill()`, `fb_fill_pill()`
- Day-ring: `fb_draw_day_ring()` — 60 squares clockwise (matches JS design source)
- `fb_flush_auto()` auto-selects partial/full based on `partial_count`

### 6.6 `fonts`
- `font_mono6.h` — 6×8 monospace (primary UI font)
- `font_monocraft.h` — Monocraft display font (large digits)

### 6.7 `gfx`
- `gfx_7seg` — 7-segment digit renderer for clock/pomo large displays
- `gfx_primitives` — additional shape primitives

### 6.8 `config_store` — NVS Config (`watcher_cfg_t`)

| Category | Fields                                                      |
|----------|-------------------------------------------------------------|
| WiFi     | `wifi_ssid`, `wifi_pass`, `device_name`, `ap_mode`         |
| Clock    | `timezone` (POSIX TZ), `ntp_server`, `time_24h`, `date_format` |
| Pomodoro | `pomo_focus_mins`, `pomo_break_mins`, `pomo_long_mins`, `pomo_cycles` |
| System   | `buzzer_on`, `theme_dark`, `sleep_timeout_min`             |
| Encoder  | `enc_pulses_per_step`                                       |
| OTA      | `ota_url`                                                   |

`cfg_load()` at boot, `cfg_save()` persists all, individual setters for WiFi/time/pomo.

### 6.9 `time_svc` — NTP Time Service
- No RTC hardware — system clock at epoch 0 until NTP sync
- `time_svc_sync_ntp(tz, ntp_server)` — call after WiFi IP acquired
- `time_svc_is_synced()` — true after first successful sync
- `localtime_r()` with POSIX TZ string for local display

### 6.10 `web_server` — HTTP + WebSocket Console
- Serves `webserver.html` from SPIFFS at root `/`
- REST endpoints:
  - `GET /api/bitmap` — current framebuffer as raw bytes
  - `POST /api/alarms` — set alarm list (JSON)
  - `POST /api/tasks` — set task list (JSON)
  - `POST /api/ota/upload` — OTA firmware binary upload
- WebSocket `/ws`:
  - Server → client: state JSON push (screen, pomo, time sync status)
  - Client → server: `{"cmd":"goto","id":"clock"}`, `{"cmd":"btn","id":0,"evt":0}`, `{"cmd":"enc","delta":1}`, `{"cmd":"restart"}`
- Virtual input simulation: `cb_btn(btn_id, evt)` + `cb_enc(delta)` wired to same handler as physical hardware

---

## 7. Screens

### 7.1 Clock Screen (`clock`)
- **Group:** `time`
- **Display:** 7-segment HH:MM digits (116px tall), date string, 60-square perimeter ring
- **Ring behavior:** 120s wave — first 60s empties clockwise, next 60s refills
- **Layout:** digits centered at 150,150; date at y=228
- **Digit positions:** H1=45, H2=118, colon=191, M1=218, M2=291
- **Input:** BTN_1 short = prev screen, BTN_2 short = next screen
- **Refresh:** partial every second; full on screen enter

### 7.2 Pomodoro Screen (`pomo`)
- **Group:** `work`
- **Display:** large countdown timer (7-seg), mode label (FOCUS/SHORT BREAK/LONG BREAK), session counter, dot-ring progress
- **Sequence:** `[FOCUS, BREAK] × pomo_cycles + [LONG_BREAK]` (max 21 steps)
- **Input:**
  - BTN_3 short = start/stop toggle
  - BTN_3 long = stop + reset to beginning
  - BTN_1 short = prev screen, BTN_2 short = next screen
  - Web: `pomo_start_stop()`, `pomo_reset()`
- **Buzzer:** `BUZZ_POMO_START` on start, `BUZZ_POMO_DONE` on interval complete
- **State queries (web server):** `pomo_is_running()`, `pomo_get_remaining_s()`, `pomo_get_mode_str()`, `pomo_get_session()`

### 7.3 Alarm Screen (`alarm`)
- **Group:** `time`
- **Display:** list of alarms with time + day-mask + enabled state; focused row inverted
- **Navigation:** encoder rotate = move focus, BTN_1/BTN_2 = prev/next item
- **Toggle:** BTN_3 short or encoder click = toggle alarm enabled
- **Back:** BTN_1 long = return to clock screen
- **Data:** JSON pushed from web console via `POST /api/alarms`
  - Fields per alarm: `hour`, `min`, `label`, `days` (bitmask: bit0=Mon…bit6=Sun), `enabled`
- **Runtime check:** `alarm_check_now()` called every second from main loop; fires buzzer if alarm matches current time

### 7.4 Tasks Screen (`tasks`)
- **Group:** `work`
- **Display:** scrollable task list; focused task highlighted; complete tasks shown with strikethrough indicator
- **Navigation:** encoder rotate = scroll, BTN_1/BTN_2 = prev/next
- **Actions:** encoder click = toggle complete; BTN_3 = context action
- **Data:** JSON via `POST /api/tasks` (`tasks_set_json()`); `tasks_get_json()` for read
- **Back:** BTN_1 long = return to prev screen

### 7.5 Settings Screen (`settings`)
- **Group:** `sys`
- **Display:** menu list of settings rows; focused row inverted
- **Items include:** WiFi SSID/status, timezone, NTP server, date format, 24h mode, pomo timings, buzzer on/off, OTA update trigger
- **Navigation:** encoder rotate = move cursor, encoder click = select/edit
- **OTA row:** shows IDLE / ARMED / UPDATING... state; footer updated to show `watcher.local / WEB CONSOLE` (v8)
- **Back:** BTN_1 or BTN_1 long

### 7.6 Calendar Screen (`cal`)
- **Group:** `time`
- **Display:** full-bleed month grid (400×300); header (30px) = "MONTH YEAR"; day-name strip (14px); grid (256px); 7 columns × 57px wide
- **Navigation:** BTN_1 = prev month, BTN_2 = next month (or encoder rotate)
- **Today:** current date cell highlighted/inverted
- **Back:** BTN_1 long

---

## 8. OTA Update

**Workflow:**
1. `ota_trigger()` — arm OTA mode (web console button or UART `u`)
2. `POST /api/ota/upload` with raw firmware binary
3. `ota_push_begin(size)` → `ota_push_write(data, len)` × N → `ota_push_end()`
4. On success: writes to inactive OTA partition, `esp_restart()`
5. Bootloader selects new partition via `otadata`

`ota_push_upload.bat` — Windows helper to POST `.bin` file to device.

---

## 9. Web Console (`webserver.html`)

Single-page app served from SPIFFS. Three-pane layout:

| Pane   | Content                                                              |
|--------|----------------------------------------------------------------------|
| Left   | Screen navigator (list of screens, click to switch)                 |
| Center | Live EPD bitmap preview (fetched via `/api/bitmap`)                 |
| Right  | Button simulator, encoder simulator, pomo controls, status          |

- WebSocket auto-reconnect; connection dot shows online/offline/reconnecting
- Dark/light theme toggle
- Virtual buttons: BTN_1, BTN_2, BTN_3 with SHORT/LONG events
- Virtual encoder: delta +1 / -1 / click
- Pomo controls: start/stop, reset, remaining time display
- Restart button: sends `{"cmd":"restart"}` → device reboots after 250ms

---

## 10. Development Environment (VS Code)

### Settings (`.vscode/settings.json`)
| Key                        | Value                                    |
|----------------------------|------------------------------------------|
| `idf.currentSetup`         | `C:\esp\v6.0\esp-idf`                   |
| `idf.portWin`              | `COM3`                                   |
| `idf.flashType`            | `UART`                                   |
| `idf.openOcdConfigs`       | `esp_ftdi.cfg` + `esp32s3.cfg`          |
| `idf.customExtraVars`      | `IDF_TARGET=esp32s3`                    |
| `clangd.path`              | `...\esp-clang\bin\clangd.exe`          |
| `clangd.arguments`         | `--background-index --query-driver=**`  |
| `clangd compile-commands-dir` | `d:\Development\...\v7.1\build`      |

### IntelliSense (`.vscode/c_cpp_properties.json`)
- **Primary:** `${workspaceFolder}/build/compile_commands.json` (post-build, exact -I flags)
- **Fallback (pre-build):** explicit IDF v6.0 include paths listed for all split peripheral driver components (`esp_driver_uart`, `esp_driver_gpio`, `esp_driver_spi`, etc.) plus FreeRTOS, SoC, HAL headers
- **intelliSenseMode:** `gcc-x86` (no native Xtensa mode; accepted IDF VS Code extension workaround)
- **Defines:** `ESP_PLATFORM`, `IDF_VER="v6.0"`, `CONFIG_IDF_TARGET_ESP32S3=1`

### Build Commands
```
idf.py build          # full build
idf.py flash monitor  # flash via UART + open serial monitor (COM3, 115200)
idf.py build flash    # build + flash
```

---

## 11. Tests (`tests/test_periph/`)

Standalone ESP-IDF project for peripheral bring-up testing. Separate `sdkconfig`, `CMakeLists.txt`, and `.vscode/` settings (same IDF v6.0, COM3). Used to validate EPD, button, encoder, buzzer individually before integrating into main firmware.

---

## 12. Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| No RTC hardware | Simplified BOM; NTP sync sufficient for intended use |
| PSRAM disabled | IO35/36/37 pin conflict on N8R8 module; 520KB SRAM adequate |
| 1-bit framebuffer | Matches e-paper native format; no color conversion needed |
| Extensible screen registry | No central enum — screens self-register; easy to add/remove |
| v8: no physical buttons | BOM reduction; web console + serial as primary dev input |
| cJSON for alarm/task data | IDF bundled library; avoids external dependency |
| SPIFFS for web console | OTA-updatable without reflashing entire firmware |
