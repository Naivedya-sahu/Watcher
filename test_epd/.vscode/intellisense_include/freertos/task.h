#pragma once
#include "FreeRTOS.h"

typedef void (*TaskFunction_t)(void*);
typedef void* TaskHandle_t;

static inline void vTaskDelay(TickType_t ticks) {}
static inline void vTaskDelete(void* t) {}

static inline int xTaskCreate(TaskFunction_t task, const char* name, unsigned stack, void* params, unsigned prio, TaskHandle_t* handle) {
    return 0;
}
