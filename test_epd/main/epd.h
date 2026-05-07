#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int mosi;
    int clk;
    int cs;
    int dc;
    int rst;
    int busy;
} epd_pins_t;

static inline void epd_init(const epd_pins_t* p) { (void)p; }

#ifdef __cplusplus
}
#endif
