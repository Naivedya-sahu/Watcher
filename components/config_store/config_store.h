#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ── NVS-backed device configuration ──────────────────────────
// All writes go to NVS immediately.
// On boot, load() populates the struct from NVS (or defaults).

typedef struct {
    // WiFi
    char  wifi_ssid[64];
    char  wifi_pass[64];
    char  device_name[32];   // mDNS hostname (default "watcher")
    bool  ap_mode;           // true = broadcast WATCHER-SETUP AP

    // Clock
    char  timezone[48];      // POSIX TZ string e.g. "IST-5:30"
    char  ntp_server[64];    // e.g. "pool.ntp.org"
    bool  time_24h;          // true=24h false=12h
    int   date_format;       // 0=long 1=short 2=iso 3=numeric

    // Pomodoro
    int   pomo_focus_mins;
    int   pomo_break_mins;
    int   pomo_long_mins;
    int   pomo_cycles;       // focus sessions before long break

    // System
    bool  buzzer_on;
    bool  theme_dark;        // future: dark-mode EPD inversion
    int   sleep_timeout_min; // auto-sleep after N minutes idle (0 = never)

    // Encoder
    int   enc_pulses_per_step;   // raw pulses per logical step

    // OTA
    char  ota_url[128];          // HTTPS URL for firmware binary (set via web portal)
} watcher_cfg_t;

extern watcher_cfg_t g_cfg;

void cfg_load(void);    // call once at boot (NVS must be initialised)
void cfg_save(void);    // persist entire struct to NVS
void cfg_defaults(void); // reset to defaults without saving

// Convenience setters (save individually)
void cfg_set_wifi(const char *ssid, const char *pass);
void cfg_set_ap_mode(bool ap);
void cfg_set_time_format(bool h24, int date_fmt);
void cfg_set_pomo(int focus, int brk, int lng, int cycles);
void cfg_set_buzzer(bool on);

#ifdef __cplusplus
}
#endif
