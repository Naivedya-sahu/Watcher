#include "Segments.h"
#include "../Display.h"

namespace {
// Active segments per digit, in the order: top, tl, tr, mid, bl, br, bot.
const uint8_t SEG_MAP[10] = {
    0b1110111, // 0: top tl tr - bl br bot
    0b0010010, // 1: tr br
    0b1011101, // 2: top tr mid bl bot
    0b1011011, // 3: top tr mid br bot
    0b0111010, // 4: tl tr mid br
    0b1101011, // 5: top tl mid br bot
    0b1101111, // 6: top tl mid bl br bot
    0b1010010, // 7: top tr br
    0b1111111, // 8: all
    0b1111011, // 9: top tl tr mid br bot
};
inline bool segOn(char ch, int idx) {
    if (ch < '0' || ch > '9') return false;
    return (SEG_MAP[ch - '0'] >> (6 - idx)) & 1;
}

// Native segment polygons in 62x110 space (matches React SvgDigit shapes).
// Each segment is drawn as a chamfered rect — we approximate with two
// triangles + a rect on the e-ink (filled poly is fine for B/W).
struct Pt { int x, y; };
const Pt SEG_TOP[]  = {{14,0},{48,0},{54,4},{48,9},{14,9},{8,4}};
const Pt SEG_TL[]   = {{4,14},{0,18},{0,46},{4,50},{9,46},{9,18}};
const Pt SEG_TR[]   = {{57,14},{53,18},{53,46},{57,50},{62,46},{62,18}};
const Pt SEG_MID[]  = {{14,50},{48,50},{54,55},{48,59},{14,59},{8,55}};
const Pt SEG_BL[]   = {{4,65},{0,69},{0,96},{4,101},{9,96},{9,69}};
const Pt SEG_BR[]   = {{57,65},{53,69},{53,96},{57,101},{62,96},{62,69}};
const Pt SEG_BOT[]  = {{14,101},{48,101},{54,105},{48,110},{14,110},{8,105}};
const Pt* const SEGS[] = { SEG_TOP, SEG_TL, SEG_TR, SEG_MID, SEG_BL, SEG_BR, SEG_BOT };

// Bounding boxes (for fast fill — we'll just fill a tight rect; on B/W e-ink
// the chamfered look is barely perceptible at the panel DPI).
struct BB { int x, y, w, h; };
const BB SEG_BB[] = {
    { 8,   0, 46,  9},
    { 0,  14,  9, 36},
    {53,  14,  9, 36},
    { 8,  50, 46,  9},
    { 0,  65,  9, 36},
    {53,  65,  9, 36},
    { 8, 101, 46,  9},
};

void fillSeg(int x, int y, int size, int idx, uint16_t color) {
    auto& g = Display::get();
    BB bb = SEG_BB[idx];
    int sx = x + (bb.x * size) / 110;
    int sy = y + (bb.y * size) / 110;
    int sw = (bb.w * size) / 110;
    int sh = (bb.h * size) / 110;
    g.fillRect(sx, sy, sw, sh, color);
}
}

namespace ui {
void drawDigit(int x, int y, int size, char ch) {
    for (int i = 0; i < 7; i++) {
        if (segOn(ch, i)) fillSeg(x, y, size, i, Display::BLACK);
    }
}

void drawColon(int x, int y, int size, bool on) {
    if (!on) return;
    auto& g = Display::get();
    // Two 9px squares at y=40 and y=65 in native 110px column.
    int dotW = (9 * size) / 110;
    int dx   = x + (4 * size) / 110;
    int top  = y + (40 * size) / 110;
    int bot  = y + (65 * size) / 110;
    g.fillRect(dx, top, dotW, dotW, Display::BLACK);
    g.fillRect(dx, bot, dotW, dotW, Display::BLACK);
}
} // namespace ui
