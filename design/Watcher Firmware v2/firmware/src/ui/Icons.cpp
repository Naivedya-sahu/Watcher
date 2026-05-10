#include "Icons.h"
#include "../Display.h"
#include <string.h>

namespace ui {

void drawPlay(int x, int y, int size) {
    auto& g = Display::get();
    // Filled triangle pointing right.
    int x0 = x, y0 = y;
    int x1 = x, y1 = y + size;
    int x2 = x + size, y2 = y + size / 2;
    g.fillTriangle(x0, y0, x1, y1, x2, y2, Display::BLACK);
}

void drawPause(int x, int y, int size) {
    auto& g = Display::get();
    int barW = size / 4;
    int gap  = barW;
    g.fillRect(x,                 y, barW, size, Display::BLACK);
    g.fillRect(x + barW + gap,    y, barW, size, Display::BLACK);
}

void drawStop(int x, int y, int size) {
    auto& g = Display::get();
    g.fillRect(x, y, size, size, Display::BLACK);
}

void drawPill(int centerX, int centerY, const char* label, bool active) {
    auto& g = Display::get();
    int w = 50, h = 18;
    int x = centerX - w / 2;
    int y = centerY - h / 2;
    int r = h / 2;
    if (active) g.fillRoundRect(x, y, w, h, r, Display::BLACK);
    else        g.drawRoundRect(x, y, w, h, r, Display::BLACK);
    g.setTextSize(1);
    g.setTextColor(active ? Display::WHITE : Display::BLACK);
    int16_t bx, by; uint16_t bw, bh;
    g.getTextBounds(label, 0, 0, &bx, &by, &bw, &bh);
    g.setCursor(x + (w - bw) / 2, y + (h - bh) / 2 + 1);
    g.print(label);
}

} // namespace ui
