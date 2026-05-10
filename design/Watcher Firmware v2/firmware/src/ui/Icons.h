// Bottom-row icons + pill helper for Pomodoro.
#pragma once
namespace ui {
void drawPlay (int x, int y, int size);
void drawPause(int x, int y, int size);
void drawStop (int x, int y, int size);
void drawPill (int centerX, int centerY, const char* label, bool active);
} // namespace ui
