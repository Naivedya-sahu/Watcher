#pragma once

/* Minimal ESP log stub for local IntelliSense/builds */
int printf(const char* fmt, ...);
#define ESP_LOGI(tag, fmt, ...) (void)0
