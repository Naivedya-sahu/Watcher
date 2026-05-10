// Pins.h — ESP32-S3 wiring for the 4.2" e-paper + 3 input buttons.
//
// Adjust to your board if these collide with strapping pins. The defaults
// below match a Waveshare ESP32-S3-Pico paired with the 4.2" Universal
// e-Paper Raw Panel Driver HAT.
#pragma once

// ── E-paper SPI bus (HSPI) ───────────────────────────────────────────────────
#define EPD_PIN_MOSI   11
#define EPD_PIN_SCK    12
#define EPD_PIN_CS     10
#define EPD_PIN_DC      9
#define EPD_PIN_RST     8
#define EPD_PIN_BUSY    7

// ── Input buttons (active LOW, internal pull-up) ─────────────────────────────
//   PREV  — previous item / decrement
//   NEXT  — next item / increment
//   SEL   — select / toggle / start-stop
#define BTN_PIN_PREV    4
#define BTN_PIN_NEXT    5
#define BTN_PIN_SEL     6

// ── Battery monitor (optional) ───────────────────────────────────────────────
#define BAT_ADC_PIN     1   // GPIO1 == ADC1_CH0 on S3
#define BAT_DIV_RATIO   2.0f
