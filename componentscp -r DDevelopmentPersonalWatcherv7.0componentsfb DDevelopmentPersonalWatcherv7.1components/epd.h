#pragma once
#include <stdint.h>
#include <stdbool.h>

// ============================================================
// EPD Driver — Waveshare GDEY042T81 (SSD1683 controller)
// 400×300 B/W e-paper, 4.2"
//
// Pin config — all from board_config.h, passed to epd_init()
// ============================================================

#define EPD_W  400
#define EPD_H  300
#define EPD_BUF_SIZE  (EPD_W * EPD_H / 8)  // 15000 bytes

// Refresh type passed to epd_flush()
typedef enum {
    EPD_REFRESH_FULL    = 0,  // Full waveform ~1200ms, clears ghosting
    EPD_REFRESH_PARTIAL = 1,  // Partial waveform ~400ms, small regions
} epd_refresh_t;

// Pin config struct — filled by main from board_config.h
typedef struct {
    int mosi;
    int clk;
    int cs;
    int dc;
    int rst;
    int busy;
} epd_pins_t;

// Init SPI bus + GPIO, run display init sequence
void epd_init(const epd_pins_t *pins);

// Flush framebuffer to display (full screen)
// buf: 15000 bytes, 1bit/pixel, bit=1→white, bit=0→black
// refresh: FULL or PARTIAL
void epd_flush(const uint8_t *buf, epd_refresh_t refresh);

// Flush a sub-region (partial refresh only)
// x,y,w,h must be byte-aligned on x axis (x%8==0, w%8==0)
void epd_flush_region(const uint8_t *buf, int x, int y, int w, int h);

// Put display into deep sleep (saves power, requires full reset to wake)
void epd_sleep(void);

// Hardware reset (required after sleep)
void epd_hw_reset(void);

// Wait for BUSY pin LOW (display ready)
void epd_wait_busy(void);
