#include "DayRing.h"
#include "../Display.h"

namespace {
constexpr int SQ = 10;
constexpr int PX = 22, PY = 21;
constexpr int COLS = 18, ROWS = 14;

struct P { int x, y; };
P g_pos[60];
bool g_init = false;

void initPositions() {
    if (g_init) return;
    g_init = true;
    int W = (COLS - 1) * PX + SQ;       // 384
    int H = (ROWS - 1) * PY + SQ;       // 283
    int x0 = (Display::W - W) / 2;      // 8
    int y0 = (Display::H - H) / 2;      // 8
    int n = 0;
    for (int i = 0; i < COLS; i++)            g_pos[n++] = { x0 + i * PX, y0 };
    for (int i = 1; i < ROWS; i++)            g_pos[n++] = { x0 + (COLS - 1) * PX, y0 + i * PY };
    for (int i = COLS - 2; i >= 0; i--)       g_pos[n++] = { x0 + i * PX, y0 + (ROWS - 1) * PY };
    for (int i = ROWS - 2; i >= 1; i--)       g_pos[n++] = { x0, y0 + i * PY };
    // n == 60
}
}

namespace ui {

void drawDayRing(bool (*fill)(int)) {
    initPositions();
    auto& g = Display::get();
    for (int i = 0; i < 60; i++) {
        if (fill(i)) g.fillRect(g_pos[i].x, g_pos[i].y, SQ, SQ, Display::BLACK);
        else         g.drawRect(g_pos[i].x, g_pos[i].y, SQ, SQ, Display::BLACK);
    }
}

bool ringSecondsFill(int i, int minute, int second) {
    int cyclePos = (minute * 60 + second) % 120;
    bool emptying = cyclePos < 60;
    int head = emptying ? cyclePos : cyclePos - 60;
    return emptying ? (i > head) : (i <= head);
}

bool ringDayFill(int i, int hour, int minute, int second) {
    long total  = (long)hour * 3600 + (long)minute * 60 + second;
    int filled = (int)((total * 60L) / 86400L);
    return i < filled;
}

} // namespace ui
