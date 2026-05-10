// Calendar — month grid (Sun..Sat) with today highlighted. Lightweight: no
// event dots wired yet (events are stored in s.tasks for the bring-up).
#include "Screens.h"
#include "Display.h"
#include <stdio.h>

namespace {
const char* MONTHS_LONG[] = {
    "JANUARY","FEBRUARY","MARCH","APRIL","MAY","JUNE",
    "JULY","AUGUST","SEPTEMBER","OCTOBER","NOVEMBER","DECEMBER"
};
const char* WDAYS3[] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};

// Zeller-ish: weekday of (y, m=0..11, d=1)
int firstWeekday(int y, int m) {
    int mm = m + 1;
    if (mm < 3) { mm += 12; y -= 1; }
    int K = y % 100;
    int J = y / 100;
    int h = (1 + 13*(mm+1)/5 + K + K/4 + J/4 + 5*J) % 7; // 0=Sat
    // map h: 0=Sat,1=Sun,2=Mon... → we want Sun=0..Sat=6
    int sun0 = (h + 6) % 7;
    return sun0;
}
int daysInMonth(int y, int m) {
    static const int d[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 1) {
        bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
        return leap ? 29 : 28;
    }
    return d[m];
}
}

namespace screens {

void renderCalendar(const UIState& s, int today) {
    auto& g = Display::get();
    g.fillScreen(Display::WHITE);

    constexpr int HEADER_H  = 30;
    constexpr int DAYBAR_H  = 16;

    // Header
    g.setTextColor(Display::BLACK);
    g.setTextSize(1);
    char title[24];
    snprintf(title, sizeof(title), "%s %d", MONTHS_LONG[s.calMonth], s.calYear);
    int16_t bx, by; uint16_t bw, bh;
    g.getTextBounds(title, 0, 0, &bx, &by, &bw, &bh);
    g.setCursor((Display::W - bw) / 2, 11);
    g.print(title);
    g.setCursor(14, 11); g.print("<");
    g.setCursor(Display::W - 18, 11); g.print(">");
    g.drawFastHLine(0, HEADER_H, Display::W, Display::BLACK);

    // Day strip
    for (int i = 0; i < 7; i++) {
        int colX = (Display::W * i) / 7;
        int colW = (Display::W * (i+1)) / 7 - colX;
        int16_t lx, ly; uint16_t lw, lh;
        g.getTextBounds(WDAYS3[i], 0, 0, &lx, &ly, &lw, &lh);
        g.setCursor(colX + (colW - lw) / 2, HEADER_H + 4);
        g.print(WDAYS3[i]);
    }
    g.drawFastHLine(0, HEADER_H + DAYBAR_H, Display::W, Display::BLACK);

    int gridY = HEADER_H + DAYBAR_H;
    int gridH = Display::H - gridY;
    int first = firstWeekday(s.calYear, s.calMonth);
    int dim   = daysInMonth(s.calYear, s.calMonth);
    int rows  = (first + dim + 6) / 7;
    int cellH = gridH / rows;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < 7; c++) {
            int idx = r * 7 + c;
            int day = idx - first + 1;
            int x   = (Display::W * c) / 7;
            int xN  = (Display::W * (c+1)) / 7;
            int y   = gridY + r * cellH;
            int w   = xN - x;

            if (day < 1 || day > dim) continue;

            bool isToday = (day == today);
            if (isToday) {
                g.fillRect(x, y, w, cellH, Display::BLACK);
                g.setTextColor(Display::WHITE);
            } else {
                g.setTextColor(Display::BLACK);
                if (c < 6) g.drawFastVLine(xN, y, cellH, 0xC618);
                g.drawFastHLine(x, y + cellH, w, 0xC618);
            }
            char db[3]; snprintf(db, sizeof(db), "%d", day);
            int16_t dx, dy; uint16_t dw, dh;
            g.getTextBounds(db, 0, 0, &dx, &dy, &dw, &dh);
            g.setCursor(x + (w - dw) / 2, y + (cellH - dh) / 2);
            g.print(db);
        }
    }
}

} // namespace screens
