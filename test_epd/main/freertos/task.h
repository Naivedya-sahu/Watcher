#pragma once
#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*TaskFunction_t)(void*);

static inline int xTaskCreate(TaskFunction_t task, const char* name, unsigned int stackDepth, void* params, unsigned int priority, void* handle) {
    (void)task; (void)name; (void)stackDepth; (void)params; (void)priority; (void)handle; return 1;
}

static inline void vTaskDelay(TickType_t ticks) { (void)ticks; }
static inline void vTaskDelete(void* handle) { (void)handle; }

#ifdef __cplusplus
}
#endif
