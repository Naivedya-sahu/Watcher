// ESP-IDF port of the minimal GxEPD2 base class used by the 4.2" GDEY042T81 driver.
#ifndef _GxEPD2_EPD_H_
#define _GxEPD2_EPD_H_

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "GxEPD2.h"

#include <stdint.h>

class GxEPD2_EPD
{
  public:
    const uint16_t WIDTH;
    const uint16_t HEIGHT;
    const GxEPD2::Panel panel;
    const bool hasColor;
    const bool hasPartialUpdate;
    const bool hasFastPartialUpdate;

    GxEPD2_EPD(int16_t cs, int16_t dc, int16_t rst, int16_t busy, int16_t busy_level, uint32_t busy_timeout,
               uint16_t w, uint16_t h, GxEPD2::Panel p, bool c, bool pu, bool fpu);
    virtual ~GxEPD2_EPD();

    virtual void init(uint32_t serial_diag_bitrate = 0);
    virtual void init(uint32_t serial_diag_bitrate, bool initial, uint16_t reset_duration = 10, bool pulldown_rst_mode = false);
    virtual void end();

    virtual void clearScreen(uint8_t value) = 0;
    virtual void writeScreenBuffer(uint8_t value) = 0;
    virtual void writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false) = 0;
    virtual void writeImageForFullRefresh(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false)
    {
      writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
    }
    virtual void writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false) = 0;
    virtual void writeScreenBufferAgain(uint8_t value = 0xFF)
    {
      writeScreenBuffer(value);
    }
    virtual void writeImageAgain(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false)
    {
      writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
    }
    virtual void writeImagePartAgain(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                     int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false)
    {
      writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
    }
    virtual void refresh(bool partial_update_mode = false) = 0;
    virtual void refresh(int16_t x, int16_t y, int16_t w, int16_t h) = 0;
    virtual void powerOff() = 0;
    virtual void hibernate() = 0;
    virtual void selectFastFullUpdate(bool) {}
    void setBusyCallback(void (*busyCallback)(const void*), const void* busy_callback_parameter = 0);
    void configureSPI(int16_t mosi, int16_t clk, spi_host_device_t host = SPI2_HOST);

  protected:
    void _reset();
    void _waitWhileBusy(const char* comment = 0, uint16_t busy_time = 5000);
    void _writeCommand(uint8_t c);
    void _writeData(uint8_t d);
    void _writeData(const uint8_t* data, uint16_t n);
    void _startTransfer();
    void _transfer(uint8_t value);
    void _endTransfer();

  protected:
    int16_t _cs, _dc, _rst, _busy, _busy_level;
    uint32_t _busy_timeout;
    bool _diag_enabled, _pulldown_rst_mode;
    bool _initial_write, _initial_refresh;
    bool _power_is_on, _using_partial_mode, _hibernating;
    bool _init_display_done;
    uint16_t _reset_duration;
    void (*_busy_callback)(const void*);
    const void* _busy_callback_parameter;

    spi_host_device_t _spi_host;
    spi_device_handle_t _spi_dev;
    int16_t _mosi, _clk;
    bool _spi_initialized;
    spi_device_interface_config_t _spi_dev_cfg;
    spi_bus_config_t _spi_bus_cfg;
};

#endif
