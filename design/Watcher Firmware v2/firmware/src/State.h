// UIState — runtime state shared by all screens. Mirrors the React Tweaks
// model from the prototypes so behavior is consistent with the design.
#pragma once
#include <stdint.h>

struct Alarm {
    uint8_t  hour;
    uint8_t  minute;
    char     label[20];
    char     days[16];     // e.g. "M T W T F · ·"
    bool     enabled;
};

struct Task {
    char     text[40];
    bool     done;
};

enum PomoMode : uint8_t { POMO_FOCUS = 0, POMO_BREAK = 1, POMO_LONG = 2 };

struct UIState {
    // Clock
    bool      ringSecondsMode = true;   // true = wave, false = % of day

    // Alarm
    Alarm     alarms[12];
    uint8_t   alarmCount = 0;
    uint8_t   alarmFocus = 0;

    // Pomodoro
    PomoMode  pomoMode       = POMO_FOCUS;
    bool      pomoRunning    = true;
    uint8_t   pomoSession    = 1;
    uint8_t   pomoTotal      = 4;
    int       pomoSecondsLeft = 25 * 60;

    // Calendar
    int       calYear  = 2026;
    uint8_t   calMonth = 4;     // 0-indexed: May

    // Tasks
    Task      tasks[16];
    uint8_t   taskCount = 0;
    uint8_t   taskFocus = 0;

    // Settings
    bool      wifiOn      = true;
    bool      sound       = false;
    uint8_t   brightness  = 60;
    uint8_t   settingsFocus = 0;
};
