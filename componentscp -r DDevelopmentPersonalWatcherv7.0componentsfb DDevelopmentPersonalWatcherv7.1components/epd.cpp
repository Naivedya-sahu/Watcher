#include "epd.h"
#include "gxepd2/gdey/GxEPD2_420_GDEY042T81.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "epd";
static GxEPD2_420_GDEY042T81 *s_display = nullptr;
static epd_pins_t s_pins = {};

static void epd_gpio_reset(void)
{
    gpio_set_level((gpio_num_t)s_pins.rst, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level((gpio_num_t)s_pins.rst, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level((gpio_num_t)s_pins.rst, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

void epd_wait_busy(void)
{
    int timeout = 5000;
    while (gpio_get_level((gpio_num_t)s_pins.busy) == 1 && timeout-- > 0)
    {
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    if (timeout <= 0)
    {
        ESP_LOGW(TAG, "BUSY timeout");
    }
}

void epd_hw_reset(void)
{
    epd_gpio_reset();
}

void epd_init(const epd_pins_t *pins)
{
    s_pins = *pins;

    gpio_config_t io = {};
    io.pin_bit_mask = (1ULL << pins->dc) | (1ULL << pins->rst);
    io.mode = GPIO_MODE_OUTPUT;
    gpio_config(&io);

    io = {};
    io.pin_bit_mask = (1ULL << pins->busy);
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io);

    if (s_display == nullptr)
    {
        s_display = new GxEPD2_420_GDEY042T81(pins->cs, pins->dc, pins->rst, pins->busy);
    }

    s_display->configureSPI(pins->mosi, pins->clk, SPI2_HOST);
    s_display->init();

    ESP_LOGI(TAG, "EPD init complete");
}

void epd_flush(const uint8_t *buf, epd_refresh_t refresh)
{
    if (s_display == nullptr)
    {
        return;
    }

    if (refresh == EPD_REFRESH_FULL)
    {
        s_display->writeImageForFullRefresh(buf, 0, 0, EPD_W, EPD_H);
        s_display->refresh(false);
    }
    else
    {
        s_display->writeImage(buf, 0, 0, EPD_W, EPD_H);
        s_display->refresh(true);
    }
}

void epd_flush_region(const uint8_t *buf, int x, int y, int w, int h)
{
    if (s_display == nullptr)
    {
        return;
    }

    s_display->writeImagePart(buf, x, y, EPD_W, EPD_H, x, y, w, h);
    s_display->refresh(x, y, w, h);
}

void epd_sleep(void)
{
    if (s_display != nullptr)
    {
        s_display->hibernate();
    }
}
