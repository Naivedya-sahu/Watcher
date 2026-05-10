// Tasks — Obsidian-style checkbox list. Focused row inverts, completed rows
// strike through.
#include "Screens.h"
#include "Display.h"
#include <stdio.h>

namespace screens {

void renderTasks(const UIState& s) {
    auto& g = Display::get();
    g.fillScreen(Display::WHITE);

    constexpr int HEADER_H = 28;
    constexpr int ROW_H    = 26;

    g.setTextColor(Display::BLACK);
    g.setTextSize(1);
    g.setCursor(14, 10);
    g.print("TASKS");
    g.drawFastHLine(0, HEADER_H, Display::W, Display::BLACK);

    int done = 0;
    int n = s.taskCount > 16 ? 16 : s.taskCount;
    for (int i = 0; i < n; i++) if (s.tasks[i].done) done++;
    char rh[16];
    snprintf(rh, sizeof(rh), "%d / %d DONE", done, n);
    int16_t bx, by; uint16_t bw, bh;
    g.getTextBounds(rh, 0, 0, &bx, &by, &bw, &bh);
    g.setCursor(Display::W - 14 - bw, 10);
    g.print(rh);

    int focus = s.taskFocus < n ? s.taskFocus : 0;
    int rowTop = focus * ROW_H;
    int rowsVisible = (Display::H - HEADER_H) / ROW_H;
    int scroll = 0;
    if (rowTop > (rowsVisible - 1) * ROW_H) {
        scroll = ((rowTop - (rowsVisible - 1) * ROW_H) / ROW_H) * ROW_H;
    }

    for (int i = 0; i < n; i++) {
        int y = HEADER_H + i * ROW_H - scroll;
        if (y + ROW_H <= HEADER_H || y >= Display::H) continue;
        bool focused = (i == focus);
        uint16_t fg = focused ? Display::WHITE : Display::BLACK;
        if (focused) g.fillRect(0, y, Display::W, ROW_H, Display::BLACK);

        // Checkbox
        int boxX = 14, boxY = y + 6;
        if (s.tasks[i].done) g.fillRect(boxX, boxY, 12, 12, fg);
        else                 g.drawRect(boxX, boxY, 12, 12, fg);

        g.setTextColor(fg);
        g.setCursor(36, y + 8);
        g.print(s.tasks[i].text);

        // Strikethrough for completed
        if (s.tasks[i].done) {
            int16_t tx, ty; uint16_t tw, th;
            g.getTextBounds(s.tasks[i].text, 36, y + 8, &tx, &ty, &tw, &th);
            g.drawFastHLine(36, y + 12, tw, fg);
        }
        if (!focused) g.drawFastHLine(0, y + ROW_H - 1, Display::W, 0xC618);
    }
}

} // namespace screens
