// Screens.h — render functions for each of the six screens.
//
// Each call clears the page buffer and paints into it; the caller decides
// when to flush via Display::refresh().
#pragma once
#include "State.h"

namespace screens {
void renderClock    (const UIState& s, int hour, int minute, int second);
void renderAlarm    (const UIState& s);
void renderPomodoro (const UIState& s, int second);
void renderCalendar (const UIState& s, int today);
void renderTasks    (const UIState& s);
void renderSettings (const UIState& s);
} // namespace screens
