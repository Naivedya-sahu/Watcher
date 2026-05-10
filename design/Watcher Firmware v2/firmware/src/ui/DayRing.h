// 60-square perimeter ring shared by clock + pomodoro. Geometry mirrors the
// DAY_RING constant in screens.jsx so the firmware visually matches the
// HTML prototype.
#pragma once
namespace ui {
// Iterate the 60 ring positions and ask `fill(i)` whether each is filled.
// Positions are computed once and cached at static-init.
void drawDayRing(bool (*fill)(int));

// Fill predicates matching the React behaviors:
bool ringSecondsFill(int i, int minute, int second);
bool ringDayFill   (int i, int hour, int minute, int second);
} // namespace ui
