# Watcher · ESP32-S3 firmware

ESP-IDF (v5.2+) project that runs the Watcher EPD UI on an ESP32-S3-WROOM-1
(N8R8) and serves the React-based Web Console you've been designing.

## Layout

```
firmware/
├── CMakeLists.txt              ← project root
├── partitions.csv              ← 4 MB app + 1 MB littlefs (web assets)
├── sdkconfig.defaults          ← SPI flash + freertos + http server tuning
├── main/
│   ├── CMakeLists.txt
│   ├── main.c                  ← boot, wifi STA/AP, NVS, http start
│   ├── watcher_nvs.c / .h      ← NVS blob reader/writer (cfg, alarms, events, tasks)
│   ├── watcher_http.c / .h     ← HTTP + WS endpoints (mirrors watcher-api.js)
│   ├── watcher_epd.c / .h      ← SSD1683 EPD driver stubs (400×300 1-bit)
│   ├── watcher_buttons.c / .h  ← GPIO 39/40/41 hw button task (short/long press)
│   └── index_html_embed.S      ← embed the Web Console HTML into flash
└── data/
    └── index.html              ← drop the bundled Watcher Web Console here
```

## Build & flash

```bash
idf.py set-target esp32s3
idf.py menuconfig          # set wifi creds (or use AP-setup mode)
idf.py build flash monitor
```

First boot with no creds → device starts in **AP-SETUP** mode broadcasting
`WATCHER-SETUP` at `192.168.4.1`. Open the console, fill SSID/PASS in
**SETTINGS · WIFI**, save → `PATCH /api/cfg` writes NVS and reboots into STA.

## REST contract

Everything is documented in `../watcher-api.js`. Server side, each endpoint
maps to one NVS key under namespace `watcher`:

| Endpoint                  | NVS key   | Notes                              |
|---------------------------|-----------|------------------------------------|
| `GET/PATCH /api/cfg`      | `cfg`     | merged JSON blob                   |
| `GET/PUT  /api/alarms`    | `alarms`  | full-array replace                 |
| `GET/PUT  /api/events`    | `events`  | full-array replace                 |
| `GET/PUT  /api/tasks`     | `tasks`   | full-array replace                 |
| `GET      /api/device`    | —         | runtime telemetry (uptime, heap)   |
| `GET      /api/network`   | —         | wifi state                         |
| `POST     /api/refresh`   | —         | force EPD full refresh             |
| `POST     /api/ota`       | —         | toggle AP-setup mode               |
| `POST     /api/reboot`    | —         | `esp_restart()`                    |
| `POST     /api/screen`    | —         | switch active screen               |
| `POST     /api/hwbtn`     | —         | simulate BTN_1/2/3 short/long      |
| `WS       /ws`            | —         | `{type:'state'/'log',...}` push    |

On serve, `/` returns `data/index.html` with the `<script id="watcher-bootstrap">`
block rewritten to a single JSON blob containing `cfg / alarms / events / tasks`
+ device + network telemetry — so the UI hydrates without a second round-trip.

## Pinout (ESP32-S3-WROOM-1 N8R8)

| Function       | GPIO | Notes                             |
|----------------|------|-----------------------------------|
| EPD MOSI       |  11  | SPI                               |
| EPD SCLK       |  12  |                                   |
| EPD CS         |  10  |                                   |
| EPD DC         |   9  |                                   |
| EPD RST        |   8  |                                   |
| EPD BUSY       |   7  | input                             |
| BTN_1          |  39  | active-low, internal pull-up      |
| BTN_2          |  40  | active-low                        |
| BTN_3          |  41  | active-low                        |
| BUZZER         |  42  | LEDC PWM                          |

All pins are configurable in `menuconfig → Watcher Hardware`.
