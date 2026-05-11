// main.cpp — Watcher v7.1 entry point
//
// Architecture overview:
//   screen_mgr  — extensible screen registry; cyclic prev/next navigation
//   screen_def_t — per-screen vtable: enter/exit/tick/render/on_button/on_encoder
//   fb_t        — 400×300 1-bit framebuffer; fb_flush() pushes to EPD
//   web_server  — HTTP + WebSocket; serves React console + REST API
//   config_store — NVS-backed settings (WiFi, timezone, pomo timings…)
//
// Hardware placeholder (HARDWARE_ENABLED 0):
//   All peripheral init calls (EPD, buttons, encoder, buzzer) are compiled
//   out. fb_flush() becomes a no-op. Navigation comes from UART0 serial
//   console instead. Flip HARDWARE_ENABLED to 1 in board_config.h once
//   each peripheral passes its bring-up test.
//
// Serial console commands (active when ENABLE_INPUTS=0):
//   n/p     next / prev screen
//   c/o/a/k/t/s  jump to clock/pomo/alarm/calendar/tasks/settings
//   [ / ]   encoder rotate left / right  (delta = -1 / +1)
//   e       encoder click
//   + / r   pomo start-stop / reset
//   f       force full EPD refresh
//   u       trigger OTA update (requires ota_url in NVS)
//   ?       print this help

#include "board_config.h"
#include "screen_mgr.h"
#include "screens/clock_screen.h"
#include "screens/alarm_screen.h"
#include "screens/pomo_screen.h"
#include "screens/cal_screen.h"
#include "screens/tasks_screen.h"
#include "screens/settings_screen.h"
#include "config_store.h"
#include "web_server.h"
#include "fb.h"
#include "time_svc.h"
#include "ota_task.h"

// Hardware-only headers — EPD and buzzer enabled when HARDWARE_ENABLED.
#if HARDWARE_ENABLED
#include "epd.h"
#include "buzzer.h"
#endif
// Input headers (buttons + encoder) are optional and controlled by
// ENABLE_INPUTS in board_config.h.
#if defined(ENABLE_INPUTS) && ENABLE_INPUTS
#include "button.h"
#include "encoder.h"
#endif

#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

static const char *TAG = "main";

// ── Framebuffer ───────────────────────────────────────────────
// Single global fb shared with web_server (bitmap endpoint) and all screens.
static fb_t s_fb;

// ── NTP / TZ config (populated from NVS, used on WiFi IP event) ──
static char s_tz[48]  = "UTC0";
static char s_ntp[64] = "pool.ntp.org";

// ─────────────────────────────────────────────────────────────
// Hardware input callbacks (only compiled when ENABLE_INPUTS)
// ─────────────────────────────────────────────────────────────
#if defined(ENABLE_INPUTS) && ENABLE_INPUTS

static void on_button(button_id_t id, button_evt_t evt) {
    // Encoder click is registered as BTN_ID_ENC; route separately
    if (id == BTN_ID_ENC) {
        screen_mgr_enc_click();
    } else {
        screen_mgr_button((btn_id_t)id, (btn_evt_t)evt);
    }
}

static void on_enc_step(int delta) {
    screen_mgr_encoder(delta);
}

#endif // ENABLE_INPUTS

// ─────────────────────────────────────────────────────────────
// Serial console task (active when inputs are unavailable)
// Reads single-byte commands from UART0 and dispatches to screen_mgr.
// Active when HARDWARE_ENABLED==0 or when ENABLE_INPUTS==0.
// ─────────────────────────────────────────────────────────────
#if !HARDWARE_ENABLED || !(defined(ENABLE_INPUTS) && ENABLE_INPUTS)

static void serial_console_help(void) {
    printf("\n=== Watcher v7.1 Serial Console ========================\n"
           "  Screen nav:    n  next      p  prev\n"
           "  Jump to:       c  clock     o  pomo      a  alarm\n"
           "                 k  calendar  t  tasks     s  settings\n"
           "  Encoder sim:   [  rotate left (-1)\n"
           "                 ]  rotate right (+1)\n"
           "                 e  encoder click\n"
           "  Pomodoro:      +  start/stop    r  reset\n"
           "  Display:       f  force full refresh\n"
           "  Firmware:      u  trigger OTA update\n"
           "  Help:          ?  this menu\n"
           "========================================================\n\n");
}

static void serial_console_task(void *arg) {
    (void)arg;
    serial_console_help();

    while (true) {
        // getchar() blocks until a byte arrives on UART0.
        // With CONFIG_ESP_CONSOLE_UART_DEFAULT=y, this reads from UART0.
        int ch = getchar();
        if (ch == EOF || ch <= 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        printf("[con] '%c'\n", (char)ch);

        switch ((char)ch) {
            // ── Screen navigation ─────────────────────────────
            case 'n': screen_next();                       break;
            case 'p': screen_prev();                       break;
            case 'c': screen_goto("clock");                break;
            case 'o': screen_goto("pomodoro");             break;
            case 'a': screen_goto("alarm");                break;
            case 'k': screen_goto("calendar");             break;
            case 't': screen_goto("tasks");                break;
            case 's': screen_goto("settings");             break;

            // ── Encoder simulation ────────────────────────────
            // '[' / ']' simulate one rotary detent left/right.
            // Dispatches to the active screen's on_encoder handler.
            case '[': screen_mgr_encoder(-1);
                      screen_force_render();               break;
            case ']': screen_mgr_encoder(+1);
                      screen_force_render();               break;
            // 'e' simulates encoder button click.
            case 'e': screen_mgr_enc_click();
                      screen_force_render();               break;

            // ── Pomodoro ──────────────────────────────────────
            case '+': pomo_start_stop();
                      screen_force_render();               break;
            case 'r': pomo_reset();
                      screen_force_render();               break;

            // ── Display ───────────────────────────────────────
            // Force a full refresh — useful after ghosting or settings change.
            case 'f': screen_force_full();                 break;

            // ── OTA firmware update ───────────────────────────
            // Arms OTA upload mode. Host should then POST build/watcher.bin
            // to /api/ota/upload (see ota_push_upload.bat).
            case 'u':
                if (!ota_trigger(NULL))
                    printf("[ota] Busy — OTA already running\n");
                else
                    printf("[ota] OTA mode armed — upload firmware to /api/ota/upload\n");
                break;

            case '?': serial_console_help();               break;
            default:  break;
        }
    }
}

#endif // !HARDWARE_ENABLED

// ─────────────────────────────────────────────────────────────
// WiFi helpers
// ─────────────────────────────────────────────────────────────

// Maximum STA reconnect attempts before falling back to AP mode.
// 5 × ~6s ≈ 30s of retrying before the user can reach the AP portal.
#define WIFI_RETRY_MAX 5

static int             s_wifi_retry_count  = 0;
// Set from the WiFi event callback; consumed in the main loop.
// Using volatile because it crosses task/ISR context.
static volatile bool   s_wifi_ap_fallback  = false;

// Forward declarations for WiFi functions
static void wifi_start_sta(void);
static void wifi_start_ap(void);
static void wifi_switch_to_ap(void);

static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        // Fresh STA start — reset counter and attempt first connect.
        s_wifi_retry_count = 0;
        esp_wifi_connect();

    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_wifi_retry_count < WIFI_RETRY_MAX) {
            s_wifi_retry_count++;
            ESP_LOGW(TAG, "WiFi disconnected — retry %d/%d",
                     s_wifi_retry_count, WIFI_RETRY_MAX);
            esp_wifi_connect();
        } else {
            // Retries exhausted.  Signal the main loop to switch to AP mode.
            // Do NOT call esp_wifi_deinit() here — calling it from a WiFi
            // event handler context will deadlock.
            ESP_LOGE(TAG, "WiFi: max retries reached — requesting AP fallback");
            s_wifi_ap_fallback = true;
        }

    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_wifi_retry_count = 0;  // connected — reset counter for future drops
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        
        // Set hostname for mDNS advertising (watcher.local)
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (netif) {
            esp_netif_set_hostname(netif, "watcher");
            ESP_LOGI(TAG, "Hostname set to 'watcher' (watcher.local)");
        }
        
        // NTP sync can only start after a valid IP is assigned.
        time_svc_sync_ntp(s_tz, s_ntp);
        web_server_push_state();
    }
}

static void wifi_start_sta(void) {
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));

    wifi_config_t wcfg = {};
    strncpy((char *)wcfg.sta.ssid,     g_cfg.wifi_ssid, sizeof(wcfg.sta.ssid) - 1);
    strncpy((char *)wcfg.sta.password, g_cfg.wifi_pass,  sizeof(wcfg.sta.password) - 1);
    // Allow open networks too (for hotspot fallback). STA will use whatever
    // the AP offers. The threshold only blocks weaker-than-specified auth.
    wcfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wcfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WiFi STA: ssid='%s'", g_cfg.wifi_ssid);
}

// Switch a running STA stack to AP mode.
// Must NOT be called from a WiFi event handler — call from main task only.
static void wifi_switch_to_ap(void) {
    ESP_LOGW(TAG, "Switching WiFi → AP mode");
    esp_wifi_stop();
    esp_wifi_deinit();
    // Persist so next reboot goes straight to AP (avoids repeated 30s retry cycle).
    cfg_set_ap_mode(true);
    wifi_start_ap();
}

static void wifi_start_ap(void) {
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    wifi_config_t apcfg = {};
    const char *name = g_cfg.device_name[0] ? g_cfg.device_name : "Watcher";
    /* Safely format SSID: limit name length to avoid truncation warnings
     * sizeof(apcfg.ap.ssid) is 32; reserve space for "Watcher-" + NUL. */
    int max_name = (int)sizeof(apcfg.ap.ssid) - (int)strlen("Watcher-") - 1;
    if (max_name < 0) max_name = 0;
    snprintf((char *)apcfg.ap.ssid, sizeof(apcfg.ap.ssid), "Watcher-%.*s", max_name, name);
    apcfg.ap.authmode       = WIFI_AUTH_OPEN;
    apcfg.ap.max_connection = 4;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &apcfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WiFi AP: ssid='%s'", apcfg.ap.ssid);
    
    // Set hostname for AP mode (accessible as watcher.local if mDNS enabled)
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (netif) {
        esp_netif_set_hostname(netif, "watcher");
        ESP_LOGI(TAG, "AP hostname set to 'watcher'");
    }
}

// ─────────────────────────────────────────────────────────────
// SPIFFS mount — serves /spiffs to esp_vfs (web console files)
// ─────────────────────────────────────────────────────────────
static void spiffs_mount(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path              = "/spiffs",
        .partition_label        = NULL,
        .max_files              = 8,
        .format_if_mount_failed = true,
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
        return;
    }
    size_t total = 0, used = 0;
    esp_spiffs_info(NULL, &total, &used);
    ESP_LOGI(TAG, "SPIFFS: %d / %d bytes", (int)used, (int)total);
}

// ─────────────────────────────────────────────────────────────
// Framebuffer flush — hardware or no-op based on HARDWARE_ENABLED
// ─────────────────────────────────────────────────────────────
static void do_flush(int mode) {
#if HARDWARE_ENABLED
    // mode: 0 = EPD_REFRESH_FULL, 1 = EPD_REFRESH_PARTIAL
    fb_flush(&s_fb, (epd_refresh_t)mode);
#else
    // Placeholder: log what would go to the display.
    // The framebuffer is still fully rendered in RAM — just not pushed to EPD.
    ESP_LOGD(TAG, "fb_flush suppressed (HARDWARE_ENABLED=0, mode=%d, screen='%s')",
             mode, screen_current_id());
#endif
}

// ─────────────────────────────────────────────────────────────
// Main task — owns the render loop and hardware init
// ─────────────────────────────────────────────────────────────
static void main_task(void *arg) {

    // ── 1. NVS — must come first (buzzer + WiFi need it) ─────
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    cfg_load();  // populates g_cfg from NVS (falls back to defaults)

    // ── 2. Buzzer init ────────────────────────────────────────
    // Placeholder when HARDWARE_ENABLED=0: buzzer_init never called,
    // buzzer_tone() guards on s_pin<0 and is a safe no-op.
#if HARDWARE_ENABLED
    buzzer_init(PIN_BUZZER);   // 1-arg: channel hardcoded to LEDC_CHANNEL_0
    if (g_cfg.buzzer_on) buzzer_tone(BUZZ_BOOT);
#else
    ESP_LOGI(TAG, "[HW STUB] Buzzer init skipped");
#endif

    // ── 3. SPIFFS (web console lives here) ───────────────────
    spiffs_mount();

    // ── 4. EPD display init ───────────────────────────────────
    // Placeholder when HARDWARE_ENABLED=0: epd_init never called,
    // do_flush() is a no-op. The framebuffer still works in RAM.
#if HARDWARE_ENABLED
    epd_pins_t epd_pins = {
        .mosi = PIN_EPD_DIN,
        .clk  = PIN_EPD_CLK,
        .cs   = PIN_EPD_CS,
        .dc   = PIN_EPD_DC,
        .rst  = PIN_EPD_RST,
        .busy = PIN_EPD_BUSY,
    };
    epd_init(&epd_pins);
#else
    ESP_LOGI(TAG, "[HW STUB] EPD init skipped");
#endif
    fb_init(&s_fb);  // always initialise — screens write to this buffer

    // ── 5. Input peripherals ──────────────────────────────────
    // If inputs are enabled, initialise them; otherwise keep the
    // serial-console active so commands can replace switches/encoder.
#if defined(ENABLE_INPUTS) && ENABLE_INPUTS
    const int btn_pins[4] = { PIN_BTN_1, PIN_BTN_2, PIN_BTN_3, PIN_ENC_SW };
    button_init(btn_pins, 4, BTN_LONG_MS, on_button);
    encoder_init(PIN_ENC_A, PIN_ENC_B, on_enc_step);
#else
    ESP_LOGI(TAG, "[HW STUB] Buttons + encoder init skipped — use serial console");
    // Launch serial console task on core 0 (leaves core 1 for wifi/main)
    xTaskCreatePinnedToCore(serial_console_task, "con", 2048, NULL, 3, NULL, 0);
#endif

    // ── 6. Network + time service ─────────────────────────────
    esp_netif_init();
    esp_event_loop_create_default();
    // Three-way selection:
    //   a) ap_mode flag set in NVS         → AP portal (user configured)
    //   b) wifi_ssid is empty              → AP portal (first boot / factory reset)
    //   c) wifi_ssid present + ap_mode off → try STA; fall back to AP after WIFI_RETRY_MAX
    if (g_cfg.ap_mode || g_cfg.wifi_ssid[0] == '\0') {
        if (!g_cfg.ap_mode) {
            ESP_LOGW(TAG, "No WiFi credentials in NVS — starting AP portal");
            cfg_set_ap_mode(true);  // persist so settings screen reflects state
        }
        wifi_start_ap();
    } else {
        wifi_start_sta();
    }
    // Copy TZ/NTP into local buffers for the WiFi IP event handler.
    // The event fires from a different task, so we can't read g_cfg there.
    if (g_cfg.timezone[0])   strncpy(s_tz,  g_cfg.timezone,   sizeof(s_tz)  - 1);
    if (g_cfg.ntp_server[0]) strncpy(s_ntp, g_cfg.ntp_server, sizeof(s_ntp) - 1);
    time_svc_init();  // no-op for now; actual NTP start happens in IP event

    // ── 7. Screen registry ────────────────────────────────────
    // Register in cyclic navigation order: n/p wraps around.
    // screen_register() is additive — adding a new screen only requires
    // creating a new screen_def_t and calling this once here.
    screen_register(&clock_screen);
    screen_register(&alarm_screen);
    screen_register(&pomo_screen);
    screen_register(&cal_screen);
    screen_register(&tasks_screen);
    screen_register(&settings_screen);
    screen_mgr_start("clock");

    // ── 8. Web server ─────────────────────────────────────────
    // Needs SPIFFS (already mounted) and the framebuffer (for /api/bitmap).
    // Register callbacks so the web_server component can call back into
    // application code without linking directly against `main` symbols.
    web_server_set_screen_callbacks(
        screen_current_id,
        screen_goto,
        screen_force_render
    );
    web_server_set_pomo_time_callbacks(
        pomo_start_stop,
        pomo_reset,
        pomo_is_running,
        pomo_get_remaining_s,
        pomo_get_mode_str,
        pomo_get_session,
        time_svc_is_synced
    );

    // Browser push callback: fires after POST /api/push-bitmap loads bitmap.
    // Flushes the updated framebuffer to the EPD immediately (full refresh).
    // In HARDWARE_ENABLED=0 mode this is a no-op; browser can verify via GET /api/bitmap.
    web_server_set_push_callback([](){
#if HARDWARE_ENABLED
        fb_flush(&s_fb, EPD_REFRESH_FULL);
#else
        ESP_LOGI(TAG, "[PUSH] Bitmap loaded (EPD flush skipped — sim mode). Verify via /api/bitmap.");
#endif
    });

    // Wire web console HW button + encoder simulation to the screen manager.
    web_server_set_input_callbacks(
        [](int id, int evt){ screen_mgr_button((btn_id_t)id, (btn_evt_t)evt); },
        [](int delta)       { screen_mgr_encoder(delta); }
    );

    web_server_start(&s_fb);

    ESP_LOGI(TAG, "Watcher v7.1 ready — %d screens | hw=%d",
             screen_count(), HARDWARE_ENABLED);

    // ── Initial render ────────────────────────────────────────
    {
        int mode = screen_mgr_render(&s_fb);
        do_flush(mode);
    }

    // ─────────────────────────────────────────────────────────
    // Main loop — 50 Hz
    // Order: poll WS → inputs → bg tasks → tick → render → poll WS
    // ─────────────────────────────────────────────────────────
    static time_t s_last_sec          = 0;
    static time_t s_last_full_refresh = 0;

    while (true) {
        time_t now = time(NULL);

        // ── Web server — drain before tick so btn/enc land this frame ──
        web_server_poll();

        // ── Input polling ─────────────────────────────────────
        // Poll only when inputs are enabled (otherwise serial console handles input)
    #if defined(ENABLE_INPUTS) && ENABLE_INPUTS
        button_tick();
        encoder_poll();
    #endif

        // ── Background pomo tick — keeps timer alive off-screen ──
        pomo_bg_tick();

        // ── Screen tick + dirty check ─────────────────────────
        bool dirty = screen_mgr_tick();

        // ── 15-minute periodic full EPD refresh (ghost clearing) ──
        if (now > 0 && now - s_last_full_refresh >= 900) {
            screen_force_full();
            dirty = true;
        }

        // ── Re-render if dirty ────────────────────────────────
        if (dirty) {
            int mode = screen_mgr_render(&s_fb);
            if (mode == EPD_FULL) s_last_full_refresh = now;
            web_server_push_state();
            do_flush(mode);
            web_server_bitmap_updated();
        }

        // ── Alarm check — once per real-time second ───────────
        if (now != s_last_sec) {
            s_last_sec = now;
            alarm_check_now();
        }

        // ── WiFi AP fallback (set by event handler after max retries) ─
        // Must be handled from the main task, NOT from the WiFi callback.
        if (s_wifi_ap_fallback) {
            s_wifi_ap_fallback = false;
            wifi_switch_to_ap();
            web_server_push_state();  // update WS clients: ap_mode now true
        }

        // ── Web server — drain after flush to minimise latency ───
        web_server_poll();

        vTaskDelay(pdMS_TO_TICKS(20));  // 50 Hz
    }
}

// ─────────────────────────────────────────────────────────────
// app_main — FreeRTOS entry point called by ESP-IDF bootloader
// 12 KB stack: WiFi events + screen render + OTA need headroom
// ─────────────────────────────────────────────────────────────
extern "C" void app_main(void) {
    xTaskCreate(main_task, "watcher", 12288, NULL, 5, NULL);
}
