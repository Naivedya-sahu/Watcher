// Display — thin facade over GxEPD2 so screen code stays driver-agnostic.
//
// Wiring assumes the common Waveshare 4.2" B/W e-paper module on an ESP32-S3
// dev board:
//   BUSY = GPIO 4
//   RST  = GPIO 16
//   DC   = GPIO 17
//   CS   = GPIO 5
//   CLK  = GPIO 18
//   DIN  = GPIO 23
//
// Override at the build level via -D flags if your board is different.
#pragma once
#include <GxEPD2_BW.h>

#ifndef PIN_EPD_BUSY
#define PIN_EPD_BUSY 4
#endif
#ifndef PIN_EPD_RST
#define PIN_EPD_RST 16
#endif
#ifndef PIN_EPD_DC
#define PIN_EPD_DC 17
#endif
#ifndef PIN_EPD_CS
#define PIN_EPD_CS 5
#endif

class Display {
  public:
    static constexpr int W = 400;
    static constexpr int H = 300;
    static constexpr uint16_t BLACK = GxEPD_BLACK;
    static constexpr uint16_t WHITE = GxEPD_WHITE;

    static GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT>& get();

    static void begin();
    static void refresh(bool partial = true);
};
