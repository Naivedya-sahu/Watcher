#pragma once
#include "screen_mgr.h"
extern screen_def_t alarm_screen;
void  alarm_check_now(void);   // call from main loop every second
char *alarm_get_json(void);    // returns malloc'd JSON string — caller frees
bool  alarm_set_json(const char *json);  // parse and replace alarm list
