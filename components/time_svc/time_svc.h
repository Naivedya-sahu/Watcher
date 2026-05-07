#pragma once
#include <time.h>
#include <stdbool.h>

// ============================================================
// Time Service — NTP-only (no RTC hardware)
// On boot: system clock at epoch 0 until NTP syncs.
// After NTP sync: system time is correct UTC; localtime_r
//   uses TZ env var set by time_svc_sync_ntp().
// ============================================================

// Called once at boot. No-op currently (no RTC to load).
void time_svc_init(void);

// Start NTP sync. Call after WiFi IP acquired.
// tz:         POSIX TZ string e.g. "IST-5:30" or "UTC0"
// ntp_server: e.g. "pool.ntp.org"
void time_svc_sync_ntp(const char *tz, const char *ntp_server);

// Returns current UTC epoch. Use localtime_r() for local display.
time_t time_svc_get(void);

// True after first successful NTP sync this boot.
bool time_svc_is_synced(void);

// Manual time set (no RTC write).
void time_svc_set(time_t t);
