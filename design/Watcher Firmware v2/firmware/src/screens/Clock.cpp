// Clock screen — big 7-seg HH:MM, perimeter day/seconds ring, date below.
#include "Screens.h"
#include "Display.h"
#include "ui/Segments.h"
#include "ui/DayRing.h"
#include <stdio.h>

namespace {
const char* WDAYS[]   = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
const char* MONTHS3[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

static int g_hour, g_minute, g_second;
static const UIState* g_state;

bool fillFn(int i) {
    return g_state->ringSecondsMode
        ? ui::ringSecondsFill(i, g_minute, g_second)
        : ui::ringDayFill(i, g_hour, g_minute, g_second);
}
}

namespace screens {
void renderClock(const UIState& s, int hour, int minute, int second) {
    auto& g = Display::get();
    g_state = &s; g_hour = hour; g_minute = minute; g_second = second;

    g.fillScreen(Display::WHITE);
    ui::drawDayRing(fillFn);

    // Big HH:MM
    constexpr int DIGIT_H = 116;
    int dw = ui::digitWidth(DIGIT_H);
    int cw = ui::colonWidth(DIGIT_H);
    int totalW = dw * 4 + cw + 4 * 8;
    int x = (Display::W - totalW) / 2;
    int y = (Display::H / 2) - (DIGIT_H / 2) - 10;

    char hh[3], mm[3];
    snprintf(hh, sizeof(hh), "%02d", hour);
    snprintf(mm, sizeof(mm), "%02d", minute);

    ui::drawDigit(x, y, DIGIT_H, hh[0]); x += dw + 8;
    ui::drawDigit(x, y, DIGIT_H, hh[1]); x += dw + 8;
    ui::drawColon(x, y, DIGIT_H, true);  x += cw + 8;
    ui::drawDigit(x, y, DIGIT_H, mm[0]); x += dw + 8;
    ui::drawDigit(x, y, DIGIT_H, mm[1]);

    // Date — using a calendar struct passed in via params would be cleaner
    // but for the bring-up we render today's date string from g_*
    // The ESP32Time RTC is consulted in main.cpp.
    extern const char* g_dateString;
    if (g_dateString && *g_dateString) {
        g.setTextSize(1);
        g.setTextColor(Display::BLACK);
        int16_t bx, by; uint16_t bw, bh;
        g.getTextBounds(g_dateString, 0, 0, &bx, &by, &bw, &bh);
        g.setCursor((Display::W - bw) / 2, y + DIGIT_H + 30);
        g.print(g_dateString);
    }
}
} // namespace screens

// Set by main.cpp before each render so the date can be drawn without a
// time.h dependency leaking into the screen module.
const char* g_dateString = "";
