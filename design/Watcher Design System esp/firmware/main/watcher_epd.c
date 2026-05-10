// firmware/main/watcher_epd.c
//
// SSD1683 e-paper driver stub. Wire your existing GxEPD2 / lvgl / custom
// rasterizer into the marked TODO blocks. This file owns:
//   - SPI bus init (HSPI)
//   - 1-bit framebuffer (15000 bytes for 400×300)
//   - public refresh entry points used by the HTTP layer

#include "watcher_epd.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "watcher_epd";

#define PIN_MOSI 11
#define PIN_SCLK 12
#define PIN_CS   10
#define PIN_DC    9
#define PIN_RST   8
#define PIN_BUSY  7

static uint8_t s_fb[EPD_W * EPD_H / 8];
static spi_device_handle_t s_spi;
static epd_screen_t s_screen = EPD_CLOCK;

esp_err_t watcher_epd_init(void) {
    spi_bus_config_t bus = { .mosi_io_num = PIN_MOSI, .sclk_io_num = PIN_SCLK,
                             .miso_io_num = -1, .quadwp_io_num = -1, .quadhd_io_num = -1,
                             .max_transfer_sz = 4096 };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
    spi_device_interface_config_t dev = { .clock_speed_hz = 10*1000*1000,
                                          .mode = 0, .spics_io_num = PIN_CS,
                                          .queue_size = 4 };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &s_spi));
    gpio_set_direction(PIN_DC,   GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_RST,  GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_BUSY, GPIO_MODE_INPUT);
    watcher_epd_clear(1);
    ESP_LOGI(TAG, "epd init ok · ssd1683 · %dx%d", EPD_W, EPD_H);
    return ESP_OK;
}

void watcher_epd_clear(int white) { memset(s_fb, white ? 0xFF : 0x00, sizeof s_fb); }

void watcher_epd_partial_refresh(void) {
    // TODO: hold CS, send 0x24 (RAM), clock out s_fb, send 0x22 (display update partial)
    ESP_LOGI(TAG, "partial refresh");
}

void watcher_epd_full_refresh(void) {
    // TODO: same as partial but with 0xF7 update sequence and ~412 ms wait on BUSY
    ESP_LOGI(TAG, "full refresh");
}

void watcher_epd_set_screen(epd_screen_t s) { s_screen = s; watcher_epd_partial_refresh(); }
epd_screen_t watcher_epd_get_screen(void)   { return s_screen; }
