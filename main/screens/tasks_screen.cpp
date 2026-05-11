// tasks_screen.cpp — Obsidian task list
// Up to 7 visible tasks. Encoder scrolls, encoder click toggles.
// Task data pushed from web console via POST /api/tasks (JSON).

#include "tasks_screen.h"
#include "screen_mgr.h"
#include "fb.h"
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define TASKS_PATH "/spiffs/tasks.json"
static const char *TAG_T = "tasks";

static void tasks_save_to_spiffs(void) {
    char *json = tasks_get_json();
    if (!json) return;
    FILE *f = fopen(TASKS_PATH, "w");
    if (f) {
        fputs(json, f);
        fclose(f);
    } else {
        ESP_LOGW(TAG_T, "Cannot write %s", TASKS_PATH);
    }
    free(json);
}

static void tasks_load_from_spiffs(void) {
    FILE *f = fopen(TASKS_PATH, "r");
    if (!f) return;   // file absent — use compiled-in defaults
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 8192) { fclose(f); return; }
    char *buf = (char *)malloc(sz + 1);
    if (!buf) { fclose(f); return; }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    tasks_set_json(buf);
    free(buf);
}

#define MAX_TASKS 16
#define VISIBLE    7
#define ROW_H     32
#define ROW_START 22   // just below header hline at y=20

typedef struct { char text[48]; char tag[8]; bool done; } task_t;

static task_t s_tasks[MAX_TASKS] = {
    { "Review PR #42",             "WORK", true  },
    { "Weekly review - Obsidian",  "NOTE", false },
    { "Update vault index",        "WORK", false },
    { "Watcher v7.2 plan",         "IDEA", false },
    { "ESP32 deep sleep notes",    "NOTE", false },
};
static int s_task_count = 5;
static int s_scroll     = 0;

static void tasks_render(fb_t *fb) {
    // ── Header — "TASKS" left, "OBSIDIAN · N OPEN" right ─────
    int open = 0;
    for (int i = 0; i < s_task_count; i++) if (!s_tasks[i].done) open++;
    fb_draw_str(fb, 32, 8, "TASKS", FB_BLACK);
    char hdr[32];
    snprintf(hdr, sizeof(hdr), "OBSIDIAN · %d OPEN", open);
    int hdw = (int)strlen(hdr) * FONT_W;
    fb_draw_str(fb, 368 - hdw, 8, hdr, FB_BLACK);
    fb_draw_hline(fb, 0, 20, FB_W, FB_BLACK);

    int visible = s_task_count - s_scroll;
    if (visible > VISIBLE) visible = VISIBLE;
    if (visible < 0)       visible = 0;

    for (int i = 0; i < visible; i++) {
        int idx     = s_scroll + i;
        int y       = ROW_START + i * ROW_H;
        int ty      = y + (ROW_H - 2 - FONT_H) / 2;  // vertically centred text y
        task_t *t   = &s_tasks[idx];
        bool focused = (idx == s_scroll);  // focused = current scrolled-to item
        int  fg      = focused ? FB_WHITE : FB_BLACK;

        // Full row inversion for the focused (top-most / BTN_3 target) item
        if (focused)
            fb_fill_rect(fb, 0, y, FB_W, ROW_H - 2, FB_BLACK);

        // Checkbox 11×11
        int cb_y = y + (ROW_H - 2 - 11) / 2;
        fb_draw_rect(fb, 10, cb_y, 11, 11, fg);
        if (t->done)
            fb_fill_rect(fb, 12, cb_y + 2, 7, 7, fg);

        // Text — right boundary = tag start - 4 or right margin
        int tag_w   = t->tag[0] ? (int)strlen(t->tag) * FONT_W + 4 : 0;
        int text_rx = (tag_w ? (384 - tag_w - 4) : 384) - 34;  // available width from x=34
        int tw      = (int)strlen(t->text) * FONT_W;
        if (tw > text_rx) tw = text_rx;

        fb_draw_str(fb, 34, ty, t->text, fg);

        // Strikethrough for done tasks
        if (t->done && tw > 0)
            fb_draw_hline(fb, 34, ty + FONT_H / 2, tw, fg);

        // Tag badge (right-aligned) — never invert the tag, show on white bg
        if (t->tag[0]) {
            int bx = 384 - tag_w;
            // If row is focused (black), draw tag on white pill for contrast
            if (focused) {
                fb_fill_rect(fb, bx - 2, cb_y, tag_w + 2, 11, FB_WHITE);
                fb_draw_str(fb, bx, ty, t->tag, FB_BLACK);
            } else {
                fb_draw_rect(fb, bx - 2, cb_y, tag_w + 2, 11, FB_BLACK);
                fb_draw_str(fb, bx, ty, t->tag, FB_BLACK);
            }
        }

        // Row separator
        if (i < visible - 1)
            fb_draw_hline(fb, 0, y + ROW_H - 2, FB_W, FB_BLACK);
    }

    // Scroll indicator
    if (s_task_count > VISIBLE) {
        char ind[24];
        snprintf(ind, sizeof(ind), "%d/%d", s_scroll + 1, s_task_count);
        int iw = (int)strlen(ind) * FONT_W;
        fb_draw_str(fb, 392 - iw, 285, ind, FB_BLACK);
    }
}

// ── JSON — web console push/pull ──────────────────────────────
// POST /api/tasks body:
//   {"tasks":[{"text":"Review PR","tag":"WORK","done":true},...]}

char *tasks_get_json(void) {
    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < s_task_count; i++) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "text", s_tasks[i].text);
        cJSON_AddStringToObject(obj, "tag",  s_tasks[i].tag);
        cJSON_AddBoolToObject  (obj, "done", s_tasks[i].done);
        cJSON_AddItemToArray(arr, obj);
    }
    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "tasks", arr);
    char *s = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return s;  // caller must free()
}

bool tasks_set_json(const char *json) {
    cJSON *root = cJSON_Parse(json);
    if (!root) return false;
    cJSON *arr = cJSON_GetObjectItem(root, "tasks");
    if (!cJSON_IsArray(arr)) { cJSON_Delete(root); return false; }
    int n = cJSON_GetArraySize(arr);
    if (n > MAX_TASKS) n = MAX_TASKS;
    for (int i = 0; i < n; i++) {
        cJSON *obj = cJSON_GetArrayItem(arr, i);
        s_tasks[i].done = cJSON_IsTrue(cJSON_GetObjectItem(obj, "done"));
        const char *text = cJSON_GetStringValue(cJSON_GetObjectItem(obj, "text"));
        const char *tag  = cJSON_GetStringValue(cJSON_GetObjectItem(obj, "tag"));
        if (text) {
            strncpy(s_tasks[i].text, text, sizeof(s_tasks[i].text) - 1);
            s_tasks[i].text[sizeof(s_tasks[i].text) - 1] = '\0';
        } else {
            s_tasks[i].text[0] = '\0';
        }
        if (tag) {
            strncpy(s_tasks[i].tag, tag, sizeof(s_tasks[i].tag) - 1);
            s_tasks[i].tag[sizeof(s_tasks[i].tag) - 1] = '\0';
        } else {
            s_tasks[i].tag[0] = '\0';
        }
    }
    s_task_count = n;
    if (s_scroll >= s_task_count && s_task_count > 0)
        s_scroll = s_task_count - 1;
    else if (s_task_count == 0)
        s_scroll = 0;
    cJSON_Delete(root);
    tasks_save_to_spiffs();
    ESP_LOGI(TAG_T, "set_json: %d tasks saved", s_task_count);
    return true;
}

// ── Button handler ────────────────────────────────────────────
static void tasks_btn(btn_id_t id, btn_evt_t evt) {
    if (id == BTN_1 && evt == BTN_SHORT) { screen_goto("cal");      return; }
    if (id == BTN_2 && evt == BTN_SHORT) { screen_goto("settings"); return; }
    if (id == BTN_3 && evt == BTN_SHORT) {
        if (s_task_count > 0) {
            s_tasks[s_scroll].done = !s_tasks[s_scroll].done;
            ESP_LOGI(TAG_T, "btn toggle[%d] '%s' → %s", s_scroll,
                     s_tasks[s_scroll].text, s_tasks[s_scroll].done ? "DONE" : "OPEN");
            tasks_save_to_spiffs();
            screen_force_render();
        }
    }
}

static void tasks_enc(int delta) {
    s_scroll += delta;
    if (s_scroll < 0) s_scroll = 0;
    if (s_task_count == 0) s_scroll = 0;
    else if (s_scroll >= s_task_count) s_scroll = s_task_count - 1;
    ESP_LOGI(TAG_T, "scroll → %d/%d", s_scroll + 1, s_task_count);
    screen_force_render();
}

static void tasks_enc_click(void) {
    if (s_task_count > 0) {
        s_tasks[s_scroll].done = !s_tasks[s_scroll].done;
        ESP_LOGI(TAG_T, "toggle[%d] '%s' → %s", s_scroll, s_tasks[s_scroll].text,
                 s_tasks[s_scroll].done ? "DONE" : "OPEN");
        tasks_save_to_spiffs();
        screen_force_render();
    }
}

static void tasks_enter(void) {
    tasks_load_from_spiffs();
    ESP_LOGI(TAG_T, "enter: %d tasks loaded", s_task_count);
    s_scroll = 0;
    screen_force_render();
}

screen_def_t tasks_screen = {
    .id           = "tasks",
    .label        = "TASKS",
    .group        = "work",
    .enter        = tasks_enter,
    .exit         = NULL,
    .tick         = NULL,
    .render       = tasks_render,
    .on_button    = tasks_btn,
    .on_encoder   = tasks_enc,
    .on_enc_click = tasks_enc_click,
    .needs_render = true,
    .force_full   = true,
};
