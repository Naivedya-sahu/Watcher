#ifndef __EPD_H_
#define __EPD_H_

#include "utility/Debug.h"
#include "DEV_Config.h"
#include "GUI_Paint.h"

// ============================================
// FAST COMPILATION MODE
// ============================================
// Only include the display driver you're using
// Comment out displays you DON'T have
// This reduces compilation time by 50%+
// ============================================

// 4.2" V2 Display (400x300) - Most common
#define EPD_ENABLE_4IN2_V2
#ifdef EPD_ENABLE_4IN2_V2
  #include "utility/EPD_4in2_V2.h"
#endif

#endif
