// Settings — list of toggles + values.
#include "Screens.h"
#include "Display.h"
#include <stdio.h>

namespace screens {

void renderSettings(const UIState& s) {
    auto& g = Display::get();
    g.fillScreen(Display::WHITE);

    constexpr int HEADER_H = 28;
    constexpr int ROW_H    = 32;

    g.setTextColor(Display::BLACK);
    g.setTextSize(1);
    g.setCursor(14, 10);
    g.print("SETTINGS");
    g.drawFastHLine(0, HEADER_H, Display::W, Display::BLACK);

    struct Row { const char* label; const char* value; bool isToggle; bool toggleOn; };
    char briBuf[8]; snprintf(briBuf, sizeof(briBuf), "%u", (unsigned)s.brightness);
    Row rows[] = {
        { "WI-FI",         s.wifiOn ? "ON" : "OFF",         true,  s.wifiOn   },
        { "SOUND",         s.sound  ? "ON" : "OFF",         true,  s.sound    },
        { "RING MODE",     s.ringSecondsMode ? "SECONDS" : "DAY", false, false },
        { "BRIGHTNESS",    briBuf,                          false, false      },
        { "FACTORY RESET", "",                              false, false      },
        { "ABOUT",         "v0.1",                          false, false      },
    };
    int n = sizeof(rows) / sizeof(rows[0]);
    int focus = s.settingsFocus < n ? s.settingsFocus : 0;

    for (int i = 0; i < n; i++) {
        int y = HEADER_H + i * ROW_H;
        if (y + ROW_H > Display::H) break;
        bool focused = (i == focus);
        if (focused) g.fillRect(0, y, Display::W, ROW_H, Display::BLACK);
        uint16_t fg = focused ? Display::WHITE : Display::BLACK;
        g.setTextColor(fg);
        g.setCursor(14, y + 12);
        g.print(rows[i].label);

        if (rows[i].isToggle) {
            int sx = Display::W - 14 - 12;
            int sy = y + (ROW_H - 12) / 2;
            if (rows[i].toggleOn) g.fillRect(sx, sy, 12, 12, fg);
            else                  g.drawRect(sx, sy, 12, 12, fg);
        } else if (rows[i].value && *rows[i].value) {
            int16_t bx, by; uint16_t bw, bh;
            g.getTextBounds(rows[i].value, 0, 0, &bx, &by, &bw, &bh);
            g.setCursor(Display::W - 14 - bw, y + 12);
            g.print(rows[i].value);
        }
        if (!focused) g.drawFastHLine(0, y + ROW_H - 1, Display::W, 0xC618);
    }
}

} // namespace screens
