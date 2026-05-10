#pragma once
// ============================================================
// Board Config — Main PCB Rev 2.0 (FROZEN)
// Module: ESP32-S3-WROOM-1-N8R8
// ============================================================

// ── Hardware enable flag ──────────────────────────────────────
// 0 = simulation mode: all peripheral init is skipped, fb_flush is a
//     no-op, navigation runs via serial console (UART0 at 115200 baud).
// 1 = full hardware: EPD and buzzer active. Use `ENABLE_INPUTS` to
// separately enable/disable user input peripherals (buttons/encoder).
// Minimal change: set HARDWARE_ENABLED to 1 to enable display/buzzer,
// but keep ENABLE_INPUTS=0 to disable buttons/encoder when commands
// are used instead of physical inputs.
#define HARDWARE_ENABLED 1
// 0 = inputs (buttons/encoder) disabled; 1 = enable inputs
#define ENABLE_INPUTS 1

// EPD SPI (SPI2_HOST / FSPI — native pins, no remapping needed)
#define PIN_EPD_DIN   11  // MOSI
#define PIN_EPD_CLK   12  // SCLK
#define PIN_EPD_CS    10
#define PIN_EPD_DC    15
#define PIN_EPD_RST   16
#define PIN_EPD_BUSY  17

// I2C — NO RTC MODULE. Pins reserved for future use.
// #define PIN_SDA        8
// #define PIN_SCL        9

// Buttons (active-low, 10K external pull-ups on peripheral board)
// IO35/36/37 conflict with Octal PSRAM on N8R8 — PSRAM must remain disabled in sdkconfig.
#define PIN_BTN_1     40  // SW1: BTN A — back
#define PIN_BTN_2     39  // SW2: BTN B — next
#define PIN_BTN_3     38  // SW3: BTN C — Pomodoro toggle (short) / stop+reset (long)
#define BTN_LONG_MS   600  // Long press threshold (ms)

// Buzzer (LEDC timer 0, channel 0)
#define PIN_BUZZER    41
#define BUZZER_CH      0

// EC11 rotary encoder (optical mouse scroll wheel)
// IO35/36/37 are Octal PSRAM pins on N8R8 — PSRAM disabled to free these as GPIOs.
// 4.7K pull-ups on peripheral board; no debounce needed on A/B
#define PIN_ENC_A   35   // ISR ANYEDGE
#define PIN_ENC_B   36   // ISR ANYEDGE
#define PIN_ENC_SW  37   // encoder click — registered as BTN_ID_ENC via button driver

// Display dimensions
#define DISP_W  400
#define DISP_H  300
