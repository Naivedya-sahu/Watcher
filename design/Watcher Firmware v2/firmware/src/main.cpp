// Main.cpp — input dispatch + render scheduler.
//
// Hardware: Lilygo T5 4.2" e-paper (ESP32) or any ESP32-S3 board wired to a
// 400x300 GxEPD2 panel. Three buttons (PREV / NEXT / SELECT) on configurable
// pins. Real-time clock provided by ESP32Time (RTC backed by NTP at boot).
#include <Arduino.h>
#include <ESP32Time.h>
#include <WiFi.h>
#include "Display.h"
#include "State.h"
#include "screens/Screens.h"

// ── Pin mapping ─────────────────────────────────────────────────────────────
constexpr int PIN_PREV   = 39;
constexpr int PIN_NEXT   = 34;
constexpr int PIN_SELECT = 35;

constexpr unsigned long FULL_REFRESH_MS = 60UL * 1000UL; // ghost-buster

ESP32Time rtc;
UIState   ui;
extern const char* g_dateString;
static char g_dateBuf[40];

enum Page { PAGE_CLOCK, PAGE_ALARM, PAGE_POMO, PAGE_CAL, PAGE_TASKS, PAGE_SET, PAGE_COUNT };
static int g_page = PAGE_CLOCK;

// Debounced edge-detect on a button pin.
struct Button { int pin; bool last; uint32_t tEdge; };
static Button btns[3] = {
    { PIN_PREV,   true, 0 },
    { PIN_NEXT,   true, 0 },
    { PIN_SELECT, true, 0 },
};
static bool pressed(Button& b) {
    bool now = digitalRead(b.pin);
    uint32_t t = millis();
    bool fire = false;
    if (now != b.last && (t - b.tEdge) > 30) {
        b.tEdge = t;
        if (now == LOW) fire = true; // active-low
        b.last = now;
    }
    return fire;
}

// Per-page input handler.
void handleInput(int idx) {
    bool prev = pressed(btns[0]);
    bool next = pressed(btns[1]);
    bool sel  = pressed(btns[2]);

    switch (g_page) {
        case PAGE_ALARM:
            if (prev && ui.alarmFocus > 0) ui.alarmFocus--;
            if (next && ui.alarmFocus < ui.alarmCount - 1) ui.alarmFocus++;
            if (sel) ui.alarms[ui.alarmFocus].enabled = !ui.alarms[ui.alarmFocus].enabled;
            break;
        case PAGE_POMO:
            if (sel) ui.pomoRunning = !ui.pomoRunning;
            if (prev) ui.pomoMode = (PomoMode)((ui.pomoMode + 2) % 3);
            if (next) ui.pomoMode = (PomoMode)((ui.pomoMode + 1) % 3);
            break;
        case PAGE_TASKS:
            if (prev && ui.taskFocus > 0) ui.taskFocus--;
            if (next && ui.taskFocus < ui.taskCount - 1) ui.taskFocus++;
            if (sel) ui.tasks[ui.taskFocus].done = !ui.tasks[ui.taskFocus].done;
            break;
        case PAGE_SET: {
            int n = 6;
            if (prev && ui.settingsFocus > 0) ui.settingsFocus--;
            if (next && ui.settingsFocus < n - 1) ui.settingsFocus++;
            if (sel) {
                if (ui.settingsFocus == 0) ui.wifiOn = !ui.wifiOn;
                else if (ui.settingsFocus == 1) ui.sound = !ui.sound;
                else if (ui.settingsFocus == 2) ui.ringSecondsMode = !ui.ringSecondsMode;
            }
            break;
        }
        case PAGE_CAL: break;
        case PAGE_CLOCK: default: break;
    }

    // Page nav: SELECT long-press cycles pages. For brevity, NEXT on clock.
    if (g_page == PAGE_CLOCK && next) g_page = (g_page + 1) % PAGE_COUNT;
    if (g_page == PAGE_CLOCK && prev) g_page = (g_page - 1 + PAGE_COUNT) % PAGE_COUNT;
}

void renderCurrent() {
    int h = rtc.getHour(true), m = rtc.getMinute(), s = rtc.getSecond();
    const char* wd[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
    const char* mo[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    snprintf(g_dateBuf, sizeof(g_dateBuf), "%s, %d %s. %d",
             wd[rtc.getDayofWeek()], rtc.getDay(), mo[rtc.getMonth()], rtc.getYear());
    g_dateString = g_dateBuf;

    switch (g_page) {
        case PAGE_CLOCK: screens::renderClock(ui, h, m, s); break;
        case PAGE_ALARM: screens::renderAlarm(ui); break;
        case PAGE_POMO:  screens::renderPomodoro(ui, s); break;
        case PAGE_CAL:   screens::renderCalendar(ui, rtc.getDay()); break;
        case PAGE_TASKS: screens::renderTasks(ui); break;
        case PAGE_SET:   screens::renderSettings(ui); break;
    }
    Display::refresh(/*partial=*/g_page == PAGE_CLOCK || g_page == PAGE_POMO);
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_PREV, INPUT_PULLUP);
    pinMode(PIN_NEXT, INPUT_PULLUP);
    pinMode(PIN_SELECT, INPUT_PULLUP);

    Display::begin();

    // Pull RTC from NTP if Wi-Fi configured; otherwise rely on RTC battery.
    if (ui.wifiOn) {
        WiFi.begin();  // creds expected via Wi-Fi provisioning
        // configTime(...) elsewhere
    }

    State::seed(ui);
    renderCurrent();
}

uint32_t lastTick = 0;
uint32_t lastFull = 0;
void loop() {
    handleInput(g_page);

    uint32_t t = millis();
    if (t - lastTick >= 1000) {
        lastTick = t;
        // Pomo countdown
        if (ui.pomoRunning && ui.pomoSecondsLeft > 0) ui.pomoSecondsLeft--;
        renderCurrent();
    }
    if (t - lastFull >= FULL_REFRESH_MS) {
        lastFull = t;
        Display::clearGhost(); // full refresh once a minute
    }
    delay(20);
}
