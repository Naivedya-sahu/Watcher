#include "Display.h"

namespace {
GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT> g_display(
    GxEPD2_420(/*CS=*/PIN_EPD_CS, /*DC=*/PIN_EPD_DC,
               /*RST=*/PIN_EPD_RST, /*BUSY=*/PIN_EPD_BUSY)
);
}

GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT>& Display::get() { return g_display; }

void Display::begin() {
    g_display.init(115200, true, 50, false);
    g_display.setRotation(0);
    g_display.setTextWrap(false);
}

void Display::refresh(bool partial) {
    // For the 4.2" panel partial refresh is fine at second-tick cadence;
    // do a full refresh occasionally to clear ghosting (caller controls).
    if (partial) g_display.displayWindow(0, 0, W, H);
    else         g_display.display();
}
