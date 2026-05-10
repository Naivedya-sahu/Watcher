// State.h — global UI state shared across screens.
//
// The firmware persists a copy of this struct in RTC memory across deep-sleep
// wakes so the UI remembers which screen / focused row was last shown.
#pragma once
#include <Arduino.h>

enum ScreenId : uint8_t {
    SCREEN_CLOCK    = 0,
    SCREEN_ALARM    = 1,
    SCREEN_POMODORO = 2,
    SCREEN_CALENDAR = 3,
    SCREEN_TASKS    = 4,
    SCREEN_SETTINGS = 5,
    SCREEN_COUNT
};

enum PomoMode : uint8_t {
    POMO_FOCUS = 0,
    POMO_BREAK = 1,
    POMO_LONG  = 2
};

struct Alarm {
    uint8_t  hour;
    uint8_t  minute;
    char     label[16];
    uint8_t  dayMask;   // bit0=Sun..bit6=Sat
    bool     enabled;
};

struct Task {
    char    text[40];
    bool    done;
    uint8_t priority;   // 0..3
};

struct UIState {
    ScreenId  screen;

    // Clock
    bool      ringSecondsMode;     // true = seconds wave, false = % of day

    // Alarms
    Alarm     alarms[12];
    uint8_t   alarmCount;
    uint8_t   alarmFocus;

    // Pomodoro
    PomoMode  pomoMode;
    uint8_t   pomoSession;
    uint8_t   pomoTotal;
    uint16_t  pomoSecondsLeft;
    bool      pomoRunning;

    // Calendar
    uint16_t  calYear;
    uint8_t   calMonth;             // 0..11

    // Tasks
    Task      tasks[16];
    uint8_t   taskCount;
    uint8_t   taskFocus;

    // Settings
    uint8_t   settingsFocus;
    bool      wifiOn;
    uint8_t   brightness;           // 0..100 (cosmetic on B/W e-ink)
    bool      sound;
};

extern UIState ui;
