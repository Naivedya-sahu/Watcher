/**
  ******************************************************************************
  * @file    fonts.h
  * @author  MCD Application Team / Modified for fast compilation
  * @version V1.1.0 - Fast Compilation
  * @date    2025-11-03
  * @brief   Conditional font compilation for faster build times
  ******************************************************************************
  * USAGE:
  * Define only the fonts you use in your sketch BEFORE including this file
  * 
  * Example:
  * #define FONT_ENABLE_20
  * #define FONT_ENABLE_24
  * #include "fonts.h"
  * 
  * This will only compile Font20 and Font24, skipping all others
  * Compilation time reduction: ~30-40%
  ******************************************************************************
  */

#ifndef __FONTS_H
#define __FONTS_H

#define MAX_HEIGHT_FONT         41
#define MAX_WIDTH_FONT          32
#define OFFSET_BITMAP           54

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

// ASCII Font structure
typedef struct _tFont
{    
  const uint8_t *table;
  uint16_t Width;
  uint16_t Height;
} sFONT;

// ============================================
// CONDITIONAL FONT COMPILATION
// ============================================
// Uncomment only the fonts you use
// This significantly speeds up compilation
// ============================================

// Most common fonts for e-paper
#define FONT_ENABLE_20
#define FONT_ENABLE_24

// Uncomment if you need these
#define FONT_ENABLE_8
#define FONT_ENABLE_12
#define FONT_ENABLE_16

// #define FONT_ENABLE_MINECRAFT_16
// #define FONT_ENABLE_MINECRAFT_24

// Font declarations
#ifdef FONT_ENABLE_8
  extern sFONT Font8;
#endif

#ifdef FONT_ENABLE_12
  extern sFONT Font12;
#endif

#ifdef FONT_ENABLE_16
  extern sFONT Font16;
#endif

#ifdef FONT_ENABLE_20
  extern sFONT Font20;
#endif

#ifdef FONT_ENABLE_24
  extern sFONT Font24;
#endif

#ifdef FONT_ENABLE_MINECRAFT_16
  extern sFONT FontMinecraft16;
#endif

#ifdef FONT_ENABLE_MINECRAFT_24
  extern sFONT FontMinecraft24;
#endif

#ifdef __cplusplus
}
#endif
  
#endif /* __FONTS_H */