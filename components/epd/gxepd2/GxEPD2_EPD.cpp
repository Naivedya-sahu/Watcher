#include "GxEPD2_EPD.h"

#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "gxepd2";

GxEPD2_EPD::GxEPD2_EPD(int16_t cs, int16_t dc, int16_t rst, int16_t busy, int16_t busy_level, uint32_t busy_timeout,
                       uint16_t w, uint16_t h, GxEPD2::Panel p, bool c, bool pu, bool fpu) :
  WIDTH(w), HEIGHT(h), panel(p), hasColor(c), hasPartialUpdate(pu), hasFastPartialUpdate(fpu),
  _cs(cs), _dc(dc), _rst(rst), _busy(busy), _busy_level(busy_level), _busy_timeout(busy_timeout),
  _diag_enabled(false), _pulldown_rst_mode(false), _initial_write(true), _initial_refresh(true),
  _power_is_on(false), _using_partial_mode(false), _hibernating(false), _init_display_done(false),
  _reset_duration(10), _busy_callback(nullptr), _busy_callback_parameter(nullptr),
  _spi_host(SPI2_HOST), _spi_dev(nullptr), _mosi(-1), _clk(-1), _spi_initialized(false)
{
  _spi_bus_cfg = {};
  _spi_dev_cfg = {};
}

GxEPD2_EPD::~GxEPD2_EPD()
{
  end();
}

void GxEPD2_EPD::configureSPI(int16_t mosi, int16_t clk, spi_host_device_t host)
{
  _mosi = mosi;
  _clk = clk;
  _spi_host = host;
}

void GxEPD2_EPD::init(uint32_t serial_diag_bitrate)
{
  (void)serial_diag_bitrate;
  init(0, true, 10, false);
}

void GxEPD2_EPD::init(uint32_t serial_diag_bitrate, bool initial, uint16_t reset_duration, bool pulldown_rst_mode)
{
  (void)serial_diag_bitrate;

  _initial_write = initial;
  _initial_refresh = initial;
  _pulldown_rst_mode = pulldown_rst_mode;
  _power_is_on = false;
  _using_partial_mode = false;
  _hibernating = false;
  _init_display_done = false;
  _reset_duration = reset_duration;

  if (_cs >= 0)
  {
    gpio_set_level((gpio_num_t)_cs, 1);
    gpio_set_direction((gpio_num_t)_cs, GPIO_MODE_OUTPUT);
  }
  if (_dc >= 0)
  {
    gpio_set_level((gpio_num_t)_dc, 1);
    gpio_set_direction((gpio_num_t)_dc, GPIO_MODE_OUTPUT);
  }
  if (_rst >= 0)
  {
    gpio_set_level((gpio_num_t)_rst, 1);
    gpio_set_direction((gpio_num_t)_rst, GPIO_MODE_OUTPUT);
  }
  if (_busy >= 0)
  {
    gpio_set_direction((gpio_num_t)_busy, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)_busy, GPIO_PULLUP_ONLY);
  }

  if (!_spi_initialized)
  {
    _spi_bus_cfg = {};
    _spi_bus_cfg.mosi_io_num = _mosi;
    _spi_bus_cfg.miso_io_num = -1;
    _spi_bus_cfg.sclk_io_num = _clk;
    _spi_bus_cfg.quadwp_io_num = -1;
    _spi_bus_cfg.quadhd_io_num = -1;
    _spi_bus_cfg.max_transfer_sz = 16384;

    ESP_ERROR_CHECK(spi_bus_initialize(_spi_host, &_spi_bus_cfg, SPI_DMA_CH_AUTO));

    _spi_dev_cfg = {};
    _spi_dev_cfg.clock_speed_hz = 4 * 1000 * 1000;
    _spi_dev_cfg.mode = 0;
    _spi_dev_cfg.spics_io_num = _cs;
    _spi_dev_cfg.queue_size = 1;

    ESP_ERROR_CHECK(spi_bus_add_device(_spi_host, &_spi_dev_cfg, &_spi_dev));
    _spi_initialized = true;
  }

  _reset();
}

void GxEPD2_EPD::end()
{
  if (_spi_initialized)
  {
    if (_spi_dev != nullptr)
    {
      spi_bus_remove_device(_spi_dev);
      _spi_dev = nullptr;
    }
    spi_bus_free(_spi_host);
    _spi_initialized = false;
  }

  if (_cs >= 0) gpio_set_direction((gpio_num_t)_cs, GPIO_MODE_INPUT);
  if (_dc >= 0) gpio_set_direction((gpio_num_t)_dc, GPIO_MODE_INPUT);
  if (_rst >= 0) gpio_set_direction((gpio_num_t)_rst, GPIO_MODE_INPUT);
}

void GxEPD2_EPD::setBusyCallback(void (*busyCallback)(const void*), const void* busy_callback_parameter)
{
  _busy_callback = busyCallback;
  _busy_callback_parameter = busy_callback_parameter;
}

void GxEPD2_EPD::_reset()
{
  if (_rst >= 0)
  {
    if (_pulldown_rst_mode)
    {
      gpio_set_level((gpio_num_t)_rst, 0);
      gpio_set_direction((gpio_num_t)_rst, GPIO_MODE_OUTPUT);
      gpio_set_level((gpio_num_t)_rst, 0);
      vTaskDelay(pdMS_TO_TICKS(_reset_duration));
      gpio_set_direction((gpio_num_t)_rst, GPIO_MODE_INPUT);
      gpio_set_pull_mode((gpio_num_t)_rst, GPIO_PULLUP_ONLY);
      vTaskDelay(pdMS_TO_TICKS(_reset_duration > 10 ? _reset_duration : 10));
    }
    else
    {
      gpio_set_level((gpio_num_t)_rst, 1);
      gpio_set_direction((gpio_num_t)_rst, GPIO_MODE_OUTPUT);
      gpio_set_level((gpio_num_t)_rst, 1);
      vTaskDelay(pdMS_TO_TICKS(10));
      gpio_set_level((gpio_num_t)_rst, 0);
      vTaskDelay(pdMS_TO_TICKS(_reset_duration));
      gpio_set_level((gpio_num_t)_rst, 1);
      vTaskDelay(pdMS_TO_TICKS(_reset_duration > 10 ? _reset_duration : 10));
    }
    _hibernating = false;
  }
}

void GxEPD2_EPD::_waitWhileBusy(const char* comment, uint16_t busy_time)
{
  if (_busy >= 0)
  {
    vTaskDelay(pdMS_TO_TICKS(1));
    int64_t start = esp_timer_get_time();
    while (1)
    {
      if (gpio_get_level((gpio_num_t)_busy) != _busy_level) break;
      if (_busy_callback) _busy_callback(_busy_callback_parameter);
      else vTaskDelay(pdMS_TO_TICKS(1));
      if (gpio_get_level((gpio_num_t)_busy) != _busy_level) break;
      if ((esp_timer_get_time() - start) > int64_t(_busy_timeout))
      {
        ESP_LOGW(TAG, "Busy Timeout!");
        break;
      }
    }
    if (comment && _diag_enabled)
    {
      ESP_LOGI(TAG, "%s : %lld", comment, (long long)(esp_timer_get_time() - start));
    }
  }
  else
  {
    vTaskDelay(pdMS_TO_TICKS(busy_time));
  }
}

void GxEPD2_EPD::_writeCommand(uint8_t c)
{
  spi_transaction_t t = {};
  t.length = 8;
  t.tx_buffer = &c;
  if (_dc >= 0) gpio_set_level((gpio_num_t)_dc, 0);
  spi_device_transmit(_spi_dev, &t);
  if (_dc >= 0) gpio_set_level((gpio_num_t)_dc, 1);
}

void GxEPD2_EPD::_writeData(uint8_t d)
{
  spi_transaction_t t = {};
  t.length = 8;
  t.tx_buffer = &d;
  spi_device_transmit(_spi_dev, &t);
}

void GxEPD2_EPD::_writeData(const uint8_t* data, uint16_t n)
{
  if (n == 0) return;
  spi_transaction_t t = {};
  t.length = n * 8;
  t.tx_buffer = data;
  spi_device_transmit(_spi_dev, &t);
}

void GxEPD2_EPD::_startTransfer()
{
  if (_cs >= 0) gpio_set_level((gpio_num_t)_cs, 0);
}

void GxEPD2_EPD::_transfer(uint8_t value)
{
  spi_transaction_t t = {};
  t.length = 8;
  t.tx_buffer = &value;
  spi_device_transmit(_spi_dev, &t);
}

void GxEPD2_EPD::_endTransfer()
{
  if (_cs >= 0) gpio_set_level((gpio_num_t)_cs, 1);
}
