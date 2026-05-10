// 7-segment digit + colon renderer. Native cell is 62×110 (matches the SVG
// units used in the React prototype) and scales by `size` (height in px).
#pragma once
#include <stdint.h>

namespace ui {
// Width helpers so callers can lay out rows.
inline int digitWidth(int size) { return (62 * size) / 110; }
inline int colonWidth(int size) { return (18 * size) / 110; }

// Draw a single ASCII digit '0'..'9' at (x,y) with the given height.
void drawDigit(int x, int y, int size, char ch);

// Draw the two-dot colon spacer.
void drawColon(int x, int y, int size, bool on);
} // namespace ui
