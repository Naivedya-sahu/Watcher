#include "screen_mgr.h"
#include "epd.h"
#include "fb.h"
#include "config_store.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "scr";
#define MAX_SCREENS 12

static screen_def_t *s_screens[MAX_SCREENS];
static int           s_count  = 0;
static int           s_active = 0;

static screen_def_t *active(void) {
    return (s_count > 0) ? s_screens[s_active] : NULL;
}

// ── Registration ──────────────────────────────────────────────
void screen_register(screen_def_t *def) {
    if (s_count >= MAX_SCREENS) {
        ESP_LOGE(TAG, "screen table full — cannot register '%s'", def->id);
        return;
    }
    s_screens[s_count++] = def;
    ESP_LOGI(TAG, "registered '%s' (%s)", def->id, def->label);
}

bool screen_mgr_start(const char *initial_id) {
    if (s_count == 0) return false;
    s_active = 0;
    for (int i = 0; i < s_count; i++) {
        if (strcmp(s_screens[i]->id, initial_id) == 0) {
            s_active = i;
            break;
        }
    }
    screen_def_t *sc = active();
    if (sc && sc->enter) sc->enter();
    if (sc) sc->needs_render = true;
    ESP_LOGI(TAG, "start → '%s'", sc ? sc->id : "?");
    return true;
}

bool screen_mgr_tick(void) {
    screen_def_t *sc = active();
    if (!sc) return false;
    if (sc->tick) sc->tick();
    return sc->needs_render;
}

int screen_mgr_render(fb_t *fb) {
    screen_def_t *sc = active();
    if (!sc) return EPD_FULL;
    int mode = sc->force_full ? EPD_FULL : EPD_PARTIAL;
    ESP_LOGD(TAG, "[RENDER] '%s' %s", sc->id, mode == EPD_FULL ? "FULL" : "partial");
    fb_clear(fb);
    if (sc->render) sc->render(fb);
    if (g_cfg.theme_dark) fb_invert(fb);
    sc->needs_render = false;
    sc->force_full   = false;
    return mode;
}

void screen_mgr_button(btn_id_t id, btn_evt_t evt) {
    screen_def_t *sc = active();
    ESP_LOGI(TAG, "[BTN] id=%d evt=%s screen='%s'",
             id, evt == BTN_LONG ? "LONG" : "SHORT", sc ? sc->id : "?");
    if (sc && sc->on_button) sc->on_button(id, evt);
}

// Exposed so screen transitions can reset debounce (first input after nav isn't dropped).
static uint32_t s_last_enc_ms = 0;

void screen_mgr_encoder(int delta) {
    // Software debounce: ignore encoder events within 60ms of each other.
    // Prevents hardware-noise phantom rotations from changing selection.
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    if (now_ms - s_last_enc_ms < 60) {
        ESP_LOGD(TAG, "[ENC] delta=%d debounced (gap=%lums)", delta,
                 (unsigned long)(now_ms - s_last_enc_ms));
        return;
    }
    s_last_enc_ms = now_ms;
    screen_def_t *sc = active();
    ESP_LOGI(TAG, "[ENC] delta=%d screen='%s'", delta, sc ? sc->id : "?");
    if (sc && sc->on_encoder) sc->on_encoder(delta);
}

void screen_mgr_enc_click(void) {
    screen_def_t *sc = active();
    ESP_LOGI(TAG, "[ENC_CLICK] screen='%s'", sc ? sc->id : "?");
    if (sc && sc->on_enc_click) sc->on_enc_click();
}

// ── Navigation ────────────────────────────────────────────────
void screen_goto(const char *id) {
    for (int i = 0; i < s_count; i++) {
        if (strcmp(s_screens[i]->id, id) == 0) {
            if (i == s_active) return;
            screen_def_t *old = active();
            if (old && old->exit) old->exit();
            s_active = i;
            s_last_enc_ms = 0;  // reset debounce — first enc input after nav isn't dropped
            screen_def_t *sc = active();
            if (sc && sc->enter) sc->enter();
            if (sc) { sc->needs_render = true; sc->force_full = true; }
            ESP_LOGI(TAG, "→ '%s'", id);
            return;
        }
    }
    ESP_LOGW(TAG, "screen '%s' not found", id);
}

void screen_next(void) {
    if (s_count == 0) return;
    screen_def_t *old = active();
    if (old && old->exit) old->exit();
    s_active = (s_active + 1) % s_count;
    s_last_enc_ms = 0;
    screen_def_t *sc = active();
    if (sc && sc->enter) sc->enter();
    if (sc) { sc->needs_render = true; sc->force_full = true; }
    ESP_LOGI(TAG, "→ '%s'", sc ? sc->id : "?");
}

void screen_prev(void) {
    if (s_count == 0) return;
    screen_def_t *old = active();
    if (old && old->exit) old->exit();
    s_active = (s_active + s_count - 1) % s_count;
    s_last_enc_ms = 0;
    screen_def_t *sc = active();
    if (sc && sc->enter) sc->enter();
    if (sc) { sc->needs_render = true; sc->force_full = true; }
    ESP_LOGI(TAG, "→ '%s'", sc ? sc->id : "?");
}

void screen_force_full(void) {
    screen_def_t *sc = active();
    if (sc) sc->force_full = true;
}

void screen_force_render(void) {
    screen_def_t *sc = active();
    if (sc) sc->needs_render = true;
}

const char *screen_current_id(void) {
    screen_def_t *sc = active();
    return sc ? sc->id : "";
}

int screen_count(void) { return s_count; }

const screen_def_t *screen_get(int idx) {
    return (idx >= 0 && idx < s_count) ? s_screens[idx] : NULL;
}

const screen_def_t *screen_find(const char *id) {
    for (int i = 0; i < s_count; i++)
        if (strcmp(s_screens[i]->id, id) == 0) return s_screens[i];
    return NULL;
}
