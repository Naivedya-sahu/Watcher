// Alarm screen — header + scrollable list. Square indicator (filled = ON,
// outline = OFF). Auto-scrolls so the focused row is visible. Sorted by time.
#include "Screens.h"
#include "Display.h"
#include <stdlib.h>
#include <string.h>

namespace screens {

static int alarmCmp(const void* a, const void* b) {
    const Alarm* aa = (const Alarm*)a;
    const Alarm* bb = (const Alarm*)b;
    int t1 = aa->hour * 60 + aa->minute;
    int t2 = bb->hour * 60 + bb->minute;
    return t1 - t2;
}

void renderAlarm(const UIState& s) {
    auto& g = Display::get();
    g.fillScreen(Display::WHITE);

    // Sort a copy ascending by time.
    Alarm sorted[12];
    int n = s.alarmCount > 12 ? 12 : s.alarmCount;
    memcpy(sorted, s.alarms, n * sizeof(Alarm));
    qsort(sorted, n, sizeof(Alarm), alarmCmp);

    constexpr int HEADER_H = 28;
    constexpr int ROW_H    = 36;
    constexpr int LIST_H   = 300 - HEADER_H;

    // Header
    g.setTextSize(1);
    g.setTextColor(Display::BLACK);
    g.setCursor(14, 10);
    g.print("ALARMS");
    g.drawFastHLine(0, HEADER_H, Display::W, Display::BLACK);

    int active = 0;
    for (int i = 0; i < n; i++) if (sorted[i].enabled) active++;
    char rh[16];
    snprintf(rh, sizeof(rh), "%d / %d ACTIVE", active, n);
    int16_t bx, by; uint16_t bw, bh;
    g.getTextBounds(rh, 0, 0, &bx, &by, &bw, &bh);
    g.setCursor(Display::W - 14 - bw, 10);
    g.print(rh);

    // Auto-scroll: ensure focus row is visible.
    int focus = s.alarmFocus < n ? s.alarmFocus : 0;
    int rowTop = focus * ROW_H;
    int scroll = 0;
    int rowsVisible = LIST_H / ROW_H;
    if (rowTop > (rowsVisible - 1) * ROW_H) {
        scroll = rowTop - (rowsVisible - 1) * ROW_H;
        scroll = (scroll / ROW_H) * ROW_H;
    }

    // Rows
    for (int i = 0; i < n; i++) {
        int yRow = HEADER_H + i * ROW_H - scroll;
        if (yRow + ROW_H <= HEADER_H || yRow >= 300) continue;

        bool focused = (i == focus);
        if (focused) g.fillRect(0, yRow, Display::W, ROW_H, Display::BLACK);

        uint16_t fg = focused ? Display::WHITE : Display::BLACK;
        g.setTextColor(fg);

        // Focus arrow
        if (focused) { g.setCursor(14, yRow + 14); g.print(">"); }

        // Time HH:MM
        char tb[8];
        snprintf(tb, sizeof(tb), "%02d:%02d", sorted[i].hour, sorted[i].minute);
        g.setTextSize(2);
        g.setCursor(36, yRow + 10);
        g.print(tb);

        // Label
        g.setTextSize(1);
        g.setCursor(118, yRow + 14);
        g.print(sorted[i].label);

        // ON/OFF square: 12x12, filled if ON.
        int sx = Display::W - 14 - 12;
        int sy = yRow + (ROW_H - 12) / 2;
        if (sorted[i].enabled) {
            g.fillRect(sx, sy, 12, 12, fg);
        } else {
            g.drawRect(sx, sy, 12, 12, fg);
        }

        // Hairline divider between rows
        if (!focused) g.drawFastHLine(0, yRow + ROW_H - 1, Display::W, 0xC618);
    }
}

} // namespace screens
