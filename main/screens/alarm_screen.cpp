// alarm_screen.cpp — alarm list with encoder navigation
// BTN_1=prev item, BTN_2=next item, BTN_3=toggle, BTN_1 long=back to clock.
// Encoder rotate = navigate, encoder click = toggle.
// Alarm data pushed from web console via POST /api/alarms (JSON).

#include "alarm_screen.h"
#include "screen_mgr.h"
#include "fb.h"
#include "buzzer.h"
#include "time_svc.h"
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

static const char *TAG = "alarm";

// ── Alarm data ────────────────────────────────────────────────
// days bitmask: bit0=Mon, bit1=Tue, bit2=Wed, bit3=Thu,
//               bit4=Fri, bit5=Sat, bit6=Sun.
// days=0x00 fires every day (no filter).
#define MAX_ALARMS 8

typedef struct {
    int     hour, min;
    char    label[16];
    bool    on;
    uint8_t days;   // 7-bit: Mon=0 … Sun=6
} alarm_t;

static alarm_t s_alarms[MAX_ALARMS] = {
    {  7,  0, "WAKE",      true,  0x1F },  // Mon–Fri
    {  9, 30, "STANDUP",   true,  0x1F },  // Mon–Fri
    { 12, 30, "LUNCH",     false, 0x7F },  // Every day
    { 17, 45, "WIND DOWN", true,  0x1F },  // Mon–Fri
};
static int s_alarm_count = 4;
static int s_focus       = 0;

// ── Layout ────────────────────────────────────────────────────
#define HEADER_Y    8
#define HDR_LINE_Y  20
#define ROW_START   22
#define ROW_H       32
#define ROW_BODY   (ROW_H - 2)   // 2 px separator line at y+ROW_BODY

// Column x-positions
#define COL_TIME   10
#define COL_LABEL  52
#define COL_DAYS  228
#define COL_TOG   380   // 12×12 square, right edge at 392

// ── Ringing state ─────────────────────────────────────────────
#define RING_REPEAT_TICKS  150   // 3 s at 50 Hz
#define RING_MAX_REPEATS     5   // auto-dismiss after 15 s

static bool s_ringing       = false;
static int  s_ring_tick     = 0;
static int  s_ring_count    = 0;
static char s_ring_label[16] = "";
static int  s_last_alarm_min = -1;  // tracks last fired minute to avoid double-fire

// ── Alarm check — call from main loop every second ────────────
void alarm_check_now(void) {
    if (!time_svc_is_synced()) return;
    time_t t = time_svc_get();
    struct tm tm; localtime_r(&t, &tm);
    // Allow sec 0 or 1 — EPD full refresh (1200ms) can cause main loop to skip sec=0.
    // s_last_alarm_min prevents firing twice within the same minute.
    if (tm.tm_sec > 1) return;
    if (s_last_alarm_min == tm.tm_min) return;  // already fired this minute
    ESP_LOGD(TAG, "check %02d:%02d day=%d", tm.tm_hour, tm.tm_min, tm.tm_wday);
    s_last_alarm_min = tm.tm_min;  // mark this minute as checked
    // tm_wday: 0=Sun,1=Mon..6=Sat → map to bit0=Mon..bit6=Sun
    int day_bit = (tm.tm_wday == 0) ? 6 : (tm.tm_wday - 1);
    for (int i = 0; i < s_alarm_count; i++) {
        bool day_ok = (s_alarms[i].days == 0) ||
                      (s_alarms[i].days & (1 << day_bit));
        if (s_alarms[i].on && day_ok &&
            s_alarms[i].hour == tm.tm_hour &&
            s_alarms[i].min  == tm.tm_min) {
            ESP_LOGI(TAG, "RING '%s' %02d:%02d", s_alarms[i].label,
                     s_alarms[i].hour, s_alarms[i].min);
            s_ringing    = true;
            s_ring_tick  = 0;
            s_ring_count = 0;
            strncpy(s_ring_label, s_alarms[i].label, sizeof(s_ring_label) - 1);
            s_ring_label[sizeof(s_ring_label) - 1] = '\0';
            alarm_screen.needs_render = true;
            buzzer_tone(BUZZ_ALERT);
            screen_goto("alarm");  // Mo-2: navigate to alarm screen so banner is visible
        }
    }
}

// ── Shared ring advance — called by both alarm_tick and alarm_bg_tick ─
static void alarm_ring_advance(bool on_screen) {
    if (!s_ringing) return;
    s_ring_tick++;
    if (s_ring_tick % RING_REPEAT_TICKS == 0) {
        s_ring_count++;
        if (s_ring_count >= RING_MAX_REPEATS) {
            ESP_LOGI(TAG, "auto-dismiss after %d repeats", s_ring_count);
            s_ringing = false;
        } else {
            ESP_LOGI(TAG, "repeat %d/%d", s_ring_count, RING_MAX_REPEATS);
            buzzer_tone(BUZZ_ALERT);
        }
        if (on_screen) screen_force_render();
        else           alarm_screen.needs_render = true;
    }
}

// ── Tick — called at 50 Hz when alarm screen is active ────────
static void alarm_tick(void) { alarm_ring_advance(true); }

// ── Background tick — called from main loop every iteration ───
// Keeps ring + auto-dismiss alive regardless of active screen.
void alarm_bg_tick(void) { alarm_ring_advance(false); }

// ── Render ────────────────────────────────────────────────────
static const char DAY_CHARS[] = "MTWTFSS";

static void alarm_render(fb_t *fb) {
    // Ringing banner — full-width inverted bar at top
    if (s_ringing) {
        fb_fill_rect(fb, 0, 0, FB_W, 36, FB_BLACK);
        char banner[32];
        snprintf(banner, sizeof(banner), "ALARM  %s", s_ring_label);
        int bw = (int)strlen(banner) * FONT_W;
        fb_draw_str(fb, (FB_W - bw) / 2, (36 - FONT_H) / 2, banner, FB_WHITE);
    }

    // Header — "ALARMS" left, "N ACTIVE" right
    int header_y = s_ringing ? 42 : HEADER_Y;
    fb_draw_str(fb, 32, header_y, "ALARMS", FB_BLACK);
    int active = 0;
    for (int i = 0; i < s_alarm_count; i++) if (s_alarms[i].on) active++;
    char act[16];
    snprintf(act, sizeof(act), "%d ACTIVE", active);
    int actw = (int)strlen(act) * FONT_W;
    fb_draw_str(fb, 368 - actw, header_y, act, FB_BLACK);
    int hdr_line_y = s_ringing ? header_y + 12 : HDR_LINE_Y;
    fb_draw_hline(fb, 0, hdr_line_y, FB_W, FB_BLACK);
    int row_start = hdr_line_y + 2;

    for (int i = 0; i < s_alarm_count; i++) {
        int y  = row_start + i * ROW_H;
        int ty = y + (ROW_BODY - FONT_H) / 2;   // vertically centred text y
        bool focused = (i == s_focus);
        int  fg      = focused ? FB_WHITE : FB_BLACK;

        // Full row inversion for focused item
        if (focused)
            fb_fill_rect(fb, 0, y, FB_W, ROW_BODY, FB_BLACK);

        // Time "HH:MM"
        char tstr[8];
        snprintf(tstr, sizeof(tstr), "%02d:%02d", s_alarms[i].hour, s_alarms[i].min);
        fb_draw_str(fb, COL_TIME, ty, tstr, fg);

        // Label
        fb_draw_str(fb, COL_LABEL, ty, s_alarms[i].label, fg);

        // Days "MTWTFSS" — active letter, '-' for inactive
        char day_str[8];
        for (int d = 0; d < 7; d++)
            day_str[d] = (s_alarms[i].days & (1 << d)) ? DAY_CHARS[d] : '-';
        day_str[7] = '\0';
        fb_draw_str(fb, COL_DAYS, ty, day_str, fg);

        // Toggle — filled square = ON, outline = OFF
        int sq_y = y + (ROW_BODY - 12) / 2;
        if (s_alarms[i].on)
            fb_fill_rect(fb, COL_TOG, sq_y, 12, 12, fg);
        else
            fb_draw_rect(fb, COL_TOG, sq_y, 12, 12, fg);

        // Separator (skip after last row)
        if (i < s_alarm_count - 1)
            fb_draw_hline(fb, 0, y + ROW_BODY, FB_W, FB_BLACK);
    }
}

// ── JSON — web console push/pull ──────────────────────────────
// POST /api/alarms body:
//   {"alarms":[{"hour":7,"min":0,"label":"WAKE","on":true,"days":31},...]}

char *alarm_get_json(void) {
    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < s_alarm_count; i++) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(obj, "hour",  s_alarms[i].hour);
        cJSON_AddNumberToObject(obj, "min",   s_alarms[i].min);
        cJSON_AddStringToObject(obj, "label", s_alarms[i].label);
        cJSON_AddBoolToObject  (obj, "on",    s_alarms[i].on);
        cJSON_AddNumberToObject(obj, "days",  s_alarms[i].days);
        cJSON_AddItemToArray(arr, obj);
    }
    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "alarms", arr);
    char *s = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return s;  // caller must free()
}

bool alarm_set_json(const char *json) {
    cJSON *root = cJSON_Parse(json);
    if (!root) return false;
    cJSON *arr = cJSON_GetObjectItem(root, "alarms");
    if (!cJSON_IsArray(arr)) { cJSON_Delete(root); return false; }
    int n = cJSON_GetArraySize(arr);
    if (n > MAX_ALARMS) n = MAX_ALARMS;
    for (int i = 0; i < n; i++) {
        cJSON *obj = cJSON_GetArrayItem(arr, i);
        s_alarms[i].hour = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(obj, "hour"));
        s_alarms[i].min  = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(obj, "min"));
        s_alarms[i].on   = cJSON_IsTrue(cJSON_GetObjectItem(obj, "on"));
        s_alarms[i].days = (uint8_t)(int)cJSON_GetNumberValue(cJSON_GetObjectItem(obj, "days"));
        const char *lbl  = cJSON_GetStringValue(cJSON_GetObjectItem(obj, "label"));
        if (lbl) {
            strncpy(s_alarms[i].label, lbl, sizeof(s_alarms[i].label) - 1);
            s_alarms[i].label[sizeof(s_alarms[i].label) - 1] = '\0';
        } else {
            s_alarms[i].label[0] = '\0';
        }
    }
    s_alarm_count = n;
    if (s_focus >= s_alarm_count && s_alarm_count > 0)
        s_focus = s_alarm_count - 1;
    cJSON_Delete(root);
    ESP_LOGI(TAG, "set_json: loaded %d alarms", n);
    return true;
}

// ── Button handler ────────────────────────────────────────────
static void alarm_btn(btn_id_t id, btn_evt_t evt) {
    if (s_ringing) {
        ESP_LOGI(TAG, "btn dismiss ring");
        s_ringing = false; screen_force_render(); return;
    }
    if (id == BTN_1 && evt == BTN_LONG)  { screen_goto("clock"); return; }
    if (id == BTN_1 && evt == BTN_SHORT) {
        if (s_alarm_count > 0) s_focus = (s_focus - 1 + s_alarm_count) % s_alarm_count;
        ESP_LOGI(TAG, "focus → %d", s_focus);
        screen_force_render();
    }
    if (id == BTN_2 && evt == BTN_SHORT) {
        if (s_alarm_count > 0) s_focus = (s_focus + 1) % s_alarm_count;
        ESP_LOGI(TAG, "focus → %d", s_focus);
        screen_force_render();
    }
    if (id == BTN_3 && evt == BTN_SHORT) {
        if (s_alarm_count > 0) {
            s_alarms[s_focus].on = !s_alarms[s_focus].on;
            ESP_LOGI(TAG, "toggle[%d] '%s' → %s", s_focus,
                     s_alarms[s_focus].label, s_alarms[s_focus].on ? "ON" : "OFF");
        }
        screen_force_render();
    }
}

// ── Encoder ───────────────────────────────────────────────────
static void alarm_enc(int delta) {
    if (s_alarm_count > 0) {
        s_focus = (s_focus + delta + s_alarm_count) % s_alarm_count;
        ESP_LOGI(TAG, "enc delta=%d focus=%d", delta, s_focus);
    }
    screen_force_render();
}
static void alarm_enc_click(void) {
    if (s_ringing) {
        ESP_LOGI(TAG, "enc_click dismiss ring");
        s_ringing = false; screen_force_render(); return;
    }
    if (s_alarm_count > 0) {
        s_alarms[s_focus].on = !s_alarms[s_focus].on;
        ESP_LOGI(TAG, "enc_click toggle[%d] '%s' → %s", s_focus,
                 s_alarms[s_focus].label, s_alarms[s_focus].on ? "ON" : "OFF");
    }
    screen_force_render();
}

static void alarm_enter(void) {
    ESP_LOGI(TAG, "enter (%d alarms)", s_alarm_count);
    s_focus = 0; screen_force_render();
}

screen_def_t alarm_screen = {
    .id           = "alarm",
    .label        = "ALARM",
    .group        = "time",
    .enter        = alarm_enter,
    .exit         = NULL,
    .tick         = alarm_tick,
    .render       = alarm_render,
    .on_button    = alarm_btn,
    .on_encoder   = alarm_enc,
    .on_enc_click = alarm_enc_click,
    .needs_render = true,
    .force_full   = true,
};
