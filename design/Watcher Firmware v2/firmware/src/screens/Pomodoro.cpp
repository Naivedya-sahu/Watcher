// Pomodoro screen — title + session, big 2-digit minutes, perimeter wave ring,
// play/pause + mode pills + stop on the bottom row.
#include "Screens.h"
#include "Display.h"
#include "ui/Segments.h"
#include "ui/DayRing.h"
#include "ui/Icons.h"
#include <stdio.h>

namespace {
static int g_minute, g_second;

bool pomoFill(int i) { return ui::ringSecondsFill(i, g_minute, g_second); }
}

namespace screens {

void renderPomodoro(const UIState& s, int second) {
    auto& g = Display::get();
    g.fillScreen(Display::WHITE);

    // The pomo wave is driven off (totalSecondsLeft) so it visibly ticks down
    // even when the wall clock hasn't advanced.
    int secsRemain = s.pomoSecondsLeft;
    g_minute = (secsRemain / 60);
    g_second = second;
    ui::drawDayRing(pomoFill);

    // Header — POMODORO left, SESSION x/y right.
    g.setTextColor(Display::BLACK);
    g.setTextSize(1);
    g.setCursor(32, 24);
    g.print("POMODORO");
    char rh[16];
    snprintf(rh, sizeof(rh), "SESSION %u/%u", (unsigned)s.pomoSession, (unsigned)s.pomoTotal);
    int16_t bx, by; uint16_t bw, bh;
    g.getTextBounds(rh, 0, 0, &bx, &by, &bw, &bh);
    g.setCursor(Display::W - 32 - bw, 24);
    g.print(rh);

    // Big minutes-only readout.
    int mins = secsRemain / 60;
    if (mins > 99) mins = 99;
    char mm[3];
    snprintf(mm, sizeof(mm), "%02d", mins);

    constexpr int DIGIT_H = 150;
    int dw = ui::digitWidth(DIGIT_H);
    int totalW = dw * 2 + 12;
    int x = (Display::W - totalW) / 2;
    int y = 139 - DIGIT_H / 2;
    ui::drawDigit(x, y, DIGIT_H, mm[0]);
    ui::drawDigit(x + dw + 12, y, DIGIT_H, mm[1]);

    // Bottom controls row (sits above bottom ring row at y=281).
    int rowY = 250;
    if (s.pomoRunning) ui::drawPause(32, rowY, 22);
    else               ui::drawPlay (32, rowY, 22);
    ui::drawStop(Display::W - 32 - 22, rowY, 22);

    // Mode pills (FOCUS / BREAK / LONG) centered.
    const char* labels[] = { "FOCUS", "BREAK", "LONG" };
    int pillW = 50; int pillGap = 6;
    int totalPills = pillW * 3 + pillGap * 2;
    int px = (Display::W - totalPills) / 2 + pillW / 2;
    for (int i = 0; i < 3; i++) {
        ui::drawPill(px, rowY + 11, labels[i], (int)s.pomoMode == i);
        px += pillW + pillGap;
    }
}

} // namespace screens
