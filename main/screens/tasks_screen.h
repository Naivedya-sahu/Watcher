#pragma once
#include "screen_mgr.h"
extern screen_def_t tasks_screen;
char *tasks_get_json(void);    // returns malloc'd JSON string — caller frees
bool  tasks_set_json(const char *json);  // parse and replace task list
