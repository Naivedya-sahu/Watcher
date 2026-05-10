# Watcher Firmware — UI State Machine

> All transitions apply equally to **physical inputs** (buttons/encoder) and **virtual inputs** (web console button/encoder simulation via WebSocket).
>
> **Input key used in diagrams:**
> - `B1s` = BTN_1 short press (back/prev)
> - `B1l` = BTN_1 long press
> - `B2s` = BTN_2 short press (next/context)
> - `B3s` = BTN_3 short press
> - `B3l` = BTN_3 long press
> - `ENC+` = encoder rotate CW (+1)
> - `ENC-` = encoder rotate CCW (−1)
> - `ENCk` = encoder click

---

## 1. Top-Level Screen Navigation

```mermaid
stateDiagram-v2
    direction LR

    [*] --> BOOT

    BOOT --> CLOCK : app_main complete\n(screen_mgr_start)

    CLOCK --> POMO      : B2s (next)
    POMO  --> ALARM     : B2s (next)
    ALARM --> CALENDAR  : B2s (next)
    CALENDAR --> TASKS  : B2s (next)
    TASKS --> SETTINGS  : B2s (next)
    SETTINGS --> CLOCK  : B2s (next, wraps)

    CLOCK    --> SETTINGS : B1s (prev, wraps)
    POMO     --> CLOCK    : B1s (prev)
    ALARM    --> POMO     : B1s (prev)
    CALENDAR --> ALARM    : B1s (prev)
    TASKS    --> CALENDAR : B1s (prev)
    SETTINGS --> TASKS    : B1s (prev)

    note right of BOOT
        1. NVS init → cfg_load
        2. EPD + buzzer init
        3. WiFi connect
        4. NTP sync (async)
        5. Register screens
        6. Web server start
    end note
```

> **Direct jump** (web console / UART): `goto(id)` switches to any screen immediately.
> **Full EPD refresh** occurs on every screen transition.

---

## 2. Boot Sequence State

```mermaid
stateDiagram-v2
    [*] --> NVS_INIT
    NVS_INIT --> CFG_LOAD       : nvs_flash_init OK
    CFG_LOAD --> HW_INIT        : cfg_load (NVS or defaults)
    HW_INIT  --> WIFI_CONNECT   : EPD init, buzzer BOOT tone
    WIFI_CONNECT --> NTP_SYNC   : IP acquired (STA mode)
    WIFI_CONNECT --> AP_MODE    : ap_mode=true → broadcast WATCHER-SETUP
    NTP_SYNC --> RUNNING        : NTP synced (time_svc_is_synced = true)
    WIFI_CONNECT --> RUNNING    : start screens without NTP (clock shows -- --)
    AP_MODE  --> RUNNING        : start screens (no time sync)
    RUNNING  --> [*]            : esp_restart (OTA or web cmd)
```

---

## 3. Clock Screen

```mermaid
stateDiagram-v2
    [*] --> IDLE

    IDLE --> IDLE : tick (every 1s)\nre-render 7-seg HH:MM\nring fills/empties on 120s wave

    IDLE --> NEXT_SCREEN : B2s → screen_mgr_next()
    IDLE --> PREV_SCREEN : B1s → screen_mgr_prev()

    note left of IDLE
        Display elements:
        · 7-seg HH:MM (116px tall)
        · 60-square perimeter ring
          (120s wave: empties then refills)
        · Date string (y=228)
        · Shows "--:--" if NTP not synced
    end note
```

---

## 4. Pomodoro Screen

```mermaid
stateDiagram-v2
    [*] --> IDLE

    IDLE --> FOCUS_RUNNING  : B3s or pomo_start_stop()\n[mode=FOCUS, seq_idx=0]\nbuzzer POMO_START
    IDLE --> NEXT_SCREEN    : B2s
    IDLE --> PREV_SCREEN    : B1s

    FOCUS_RUNNING --> FOCUS_PAUSED  : B3s
    FOCUS_RUNNING --> BREAK_RUNNING : timer expires\nbuzzer POMO_DONE\nauto-advance seq_idx

    FOCUS_PAUSED --> FOCUS_RUNNING  : B3s (resume)
    FOCUS_PAUSED --> IDLE           : B3l (stop + reset)\nreset seq_idx=0

    BREAK_RUNNING --> BREAK_PAUSED  : B3s
    BREAK_RUNNING --> FOCUS_RUNNING : timer expires\n[if next in seq is FOCUS]\nbuzzer POMO_DONE
    BREAK_RUNNING --> LONG_RUNNING  : timer expires\n[if next in seq is LONG]

    BREAK_PAUSED --> BREAK_RUNNING  : B3s
    BREAK_PAUSED --> IDLE           : B3l

    LONG_RUNNING --> LONG_PAUSED    : B3s
    LONG_RUNNING --> IDLE           : timer expires\nbuzzer POMO_DONE\nsequence complete

    LONG_PAUSED --> LONG_RUNNING    : B3s
    LONG_PAUSED --> IDLE            : B3l

    note right of FOCUS_RUNNING
        Sequence built at enter():
        [FOCUS, BREAK] × pomo_cycles
        + [LONG_BREAK]
        Default: 3 cycles = 7 steps
    end note

    note left of IDLE
        Display:
        · 7-seg countdown MM:SS
        · Mode label: FOCUS /
          SHORT BREAK / LONG BREAK
        · Session counter (1-based)
        · Dot-ring progress
    end note
```

---

## 5. Alarm Screen

```mermaid
stateDiagram-v2
    [*] --> LIST_VIEW

    LIST_VIEW --> LIST_VIEW     : ENC+ or B2s → move focus down
    LIST_VIEW --> LIST_VIEW     : ENC- or B1s → move focus up
    LIST_VIEW --> LIST_VIEW     : B3s or ENCk → toggle alarm enabled\n(focused row)
    LIST_VIEW --> CLOCK_SCREEN  : B1l → goto clock

    state LIST_VIEW {
        [*] --> NORMAL
        NORMAL --> FOCUSED  : focus index changes\nrow inversion renders
    }

    LIST_VIEW --> RINGING       : alarm_check_now() match\n[hour+min+day+enabled]
    RINGING --> LIST_VIEW       : any button press\nbuzzer stop

    note right of LIST_VIEW
        Data source: POST /api/alarms (JSON)
        Fields: hour, min, label,
                days (bitmask Mon=0..Sun=6),
                enabled
        Max display: fits screen height
    end note

    note right of RINGING
        buzzer BUZZ_ALERT repeating
        Display: flashing alarm time
    end note
```

---

## 6. Tasks Screen

```mermaid
stateDiagram-v2
    [*] --> LIST_VIEW

    LIST_VIEW --> LIST_VIEW : ENC+ or B2s → scroll down
    LIST_VIEW --> LIST_VIEW : ENC- or B1s → scroll up
    LIST_VIEW --> LIST_VIEW : ENCk → toggle task complete\n(focused item)
    LIST_VIEW --> PREV_SCREEN : B1l

    state LIST_VIEW {
        [*] --> NORMAL
        NORMAL --> FOCUSED : cursor moves\nrow inverted
        FOCUSED --> COMPLETED : ENCk\nstrike-through rendered
        COMPLETED --> FOCUSED : ENCk (toggle back)
    }

    note right of LIST_VIEW
        Data source: POST /api/tasks (JSON)
        tasks_get_json() for read-back
        Completed items shown with
        strike-through indicator
    end note
```

---

## 7. Settings Screen

```mermaid
stateDiagram-v2
    [*] --> MENU

    MENU --> MENU       : ENC+ → move cursor down
    MENU --> MENU       : ENC- → move cursor up
    MENU --> EDIT_VALUE : ENCk → enter edit for focused row\n[editable rows only]
    MENU --> PREV_SCREEN : B1s or B1l

    state EDIT_VALUE {
        [*] --> EDITING
        EDITING --> EDITING : ENC+/ENC- → increment/decrement value
        EDITING --> MENU    : ENCk → confirm + cfg_save()
        EDITING --> MENU    : B1s → cancel (discard change)
    }

    MENU --> OTA_STATE  : ENCk on UPDATE row

    state OTA_STATE {
        [*] --> OTA_IDLE
        OTA_IDLE --> OTA_ARMED    : ENCk → ota_trigger()
        OTA_ARMED --> OTA_RUNNING : POST /api/ota/upload received
        OTA_RUNNING --> [*]       : ota_push_end() → esp_restart()
    }

    note right of MENU
        Menu rows (examples):
        · WiFi SSID / status
        · Timezone (POSIX TZ string)
        · NTP server
        · 24h / 12h toggle
        · Date format (long/short/iso/numeric)
        · Pomo focus / break / long mins
        · Pomo cycles count
        · Buzzer on/off
        · UPDATE (OTA trigger)
        Footer: "watcher.local / WEB CONSOLE"
    end note
```

---

## 8. Calendar Screen

```mermaid
stateDiagram-v2
    [*] --> MONTH_VIEW

    MONTH_VIEW --> MONTH_VIEW : B1s or ENC- → prev month\ns_month--
    MONTH_VIEW --> MONTH_VIEW : B2s or ENC+ → next month\ns_month++
    MONTH_VIEW --> PREV_SCREEN : B1l

    note left of MONTH_VIEW
        Display (400×300 full bleed):
        · Header 30px: "MONTH YEAR"
        · Day-name strip 14px: SUN–SAT
        · Grid 256px: 7 cols × 57px
        · Today's date: cell inverted
        · Navigation wraps month/year
    end note
```

---

## 9. OTA Update State Machine

```mermaid
stateDiagram-v2
    [*] --> IDLE

    IDLE --> ARMED      : ota_trigger() called\n(settings screen ENCk, or UART 'u',\nor web console OTA button)
    ARMED --> WRITING   : POST /api/ota/upload\nota_push_begin(size)
    WRITING --> WRITING : ota_push_write(chunk) × N
    WRITING --> VERIFY  : ota_push_end()
    VERIFY --> REBOOT   : verify OK\nesp_ota_set_boot_partition()
    REBOOT --> [*]      : esp_restart()

    WRITING --> IDLE    : ota_push_abort() / error
    ARMED --> IDLE      : timeout / abort

    note right of ARMED
        Display: settings row shows "ARMED"
        Waits for binary upload
    end note

    note right of WRITING
        Display: settings row shows "UPDATING..."
        Writes to inactive OTA partition
        (app0 ↔ app1)
    end note
```

---

## 10. WiFi / NTP Lifecycle

```mermaid
stateDiagram-v2
    [*] --> WIFI_INIT

    WIFI_INIT --> STA_CONNECTING : ap_mode = false\nwifi_ssid set in NVS
    WIFI_INIT --> AP_BROADCASTING : ap_mode = true\nbroadcast "WATCHER-SETUP" AP

    STA_CONNECTING --> STA_CONNECTED : WIFI_EVENT_STA_GOT_IP
    STA_CONNECTING --> STA_CONNECTING : retry (auto reconnect)
    STA_CONNECTED --> NTP_SYNCING   : time_svc_sync_ntp(tz, server)
    NTP_SYNCING --> NTP_SYNCED      : SNTP callback fires\ntime_svc_is_synced() = true
    NTP_SYNCED --> NTP_SYNCED       : periodic resync (lwIP SNTP)

    AP_BROADCASTING --> STA_CONNECTING : credentials set via web portal\ncfg_set_wifi() + cfg_set_ap_mode(false)\nesp_restart()

    STA_CONNECTED --> [*]  : wifi disconnect → reconnect loop
```

---

## 11. Web Console Input Simulation

```mermaid
stateDiagram-v2
    direction LR

    [*] --> WS_CONNECTED

    WS_CONNECTED --> WS_CONNECTED : recv {"cmd":"goto","id":"X"}\n→ screen_mgr_goto(id)

    WS_CONNECTED --> WS_CONNECTED : recv {"cmd":"btn","id":N,"evt":E}\n→ cb_btn(N, E)\n→ screen_mgr_button()\nor screen_mgr_enc_click()

    WS_CONNECTED --> WS_CONNECTED : recv {"cmd":"enc","delta":D}\n→ cb_enc(D)\n→ screen_mgr_encoder(D)

    WS_CONNECTED --> WS_CONNECTED : recv {"cmd":"restart"}\n→ 250ms delay → esp_restart()

    WS_CONNECTED --> WS_CONNECTED : state push every tick\n{"screen","pomo_running",\n"remaining_s","mode","session",\n"ntp_synced"}

    WS_CONNECTED --> WS_DISCONNECTED : socket close / network loss
    WS_DISCONNECTED --> WS_CONNECTED : browser reconnect (auto-retry)

    note right of WS_CONNECTED
        btn id mapping:
          0 = BTN_1 (back/prev)
          1 = BTN_2 (next/context)
          2 = BTN_3 (pomo/action)
        evt: 0=short, 1=long
        enc delta: +1=CW, -1=CCW
    end note
```

---

## 12. Complete System Overview

```mermaid
flowchart TD
    BOOT([Boot: NVS → cfg_load → HW init → WiFi → NTP])

    BOOT --> CLOCK

    subgraph SCREENS["Screen Navigation (cyclic B1s=prev, B2s=next)"]
        CLOCK["🕐 CLOCK\n7-seg HH:MM\n60-sq ring"]
        POMO["🍅 POMODORO\nFOCUS / BREAK / LONG\nDot-ring progress"]
        ALARM["⏰ ALARM\nList + toggle\nalarm_check_now()"]
        CAL["📅 CALENDAR\nMonth grid\nprev/next month"]
        TASKS["✓ TASKS\nScrollable list\ntoggle complete"]
        SETTINGS["⚙ SETTINGS\nMenu + edit\nOTA trigger"]
    end

    CLOCK --> POMO --> ALARM --> CAL --> TASKS --> SETTINGS --> CLOCK

    subgraph INPUTS["Input Sources"]
        BTN["Physical Buttons\nBTN_1/2/3 + ENC\n(ENABLE_INPUTS=1)"]
        WEB["Web Console\nWS virtual sim\ncb_btn / cb_enc"]
        UART["UART Serial\nn/p/[/]/e/+/r\n(ENABLE_INPUTS=0)"]
    end

    BTN --> SCREENS
    WEB --> SCREENS
    UART --> SCREENS

    subgraph SERVICES["Background Services"]
        NTP["time_svc\nNTP sync\nlocaltime_r"]
        WEBSVR["web_server\nHTTP + WS\n/api/* + /ws"]
        OTA["ota_task\nPush OTA\napp0↔app1"]
        CFG["config_store\nNVS-backed\nwatcher_cfg_t"]
    end

    SCREENS --> SERVICES
    SERVICES --> SCREENS

    subgraph DISPLAY["Display Pipeline"]
        FB["fb_t\n15KB 1-bit\n400×300"]
        EPD["SSD1683\nGDEY042T81\n4.2\" e-paper"]
    end

    SCREENS --> FB --> EPD

    BUZZ["Buzzer\nLEDC PWM\nnamed patterns"]
    SCREENS --> BUZZ
```
