#pragma once
#include <stdint.h>

typedef uint32_t TickType_t;
#define pdMS_TO_TICKS(ms) ((ms) / portTICK_PERIOD_MS)
static const TickType_t portTICK_PERIOD_MS = 1;
