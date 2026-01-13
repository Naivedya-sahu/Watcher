/*****************************************************************************
* | File      	:   DEV_Config.h
* | Author      :   Waveshare team
* | Function    :   Hardware underlying interface
* | Info        :
*----------------
* REVERTED TO ORIGINAL ESP32 PINOUT (for verified working configuration)
* Date: 2025-11-03
*
******************************************************************************/
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>
#include <stdio.h>

/**
 * data
**/
#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

/**
 * GPIO config - Original ESP32 pinout (tested working)
**/

#define EPD_MOSI_PIN 11 //DIN
#define EPD_SCK_PIN  12 //CLK
#define EPD_CS_PIN   10 //CS
#define EPD_DC_PIN   15 //DC
#define EPD_RST_PIN  16 //RST
#define EPD_BUSY_PIN 17 //BUSY

#define GPIO_PIN_SET   1
#define GPIO_PIN_RESET 0

/**
 * GPIO read and write
**/
#define DEV_Digital_Write(_pin, _value) digitalWrite(_pin, _value == 0? LOW:HIGH)
#define DEV_Digital_Read(_pin) digitalRead(_pin)

/**
 * delay x ms
**/
#define DEV_Delay_ms(__xms) delay(__xms)

/*------------------------------------------------------------------------------------------------------*/
UBYTE DEV_Module_Init(void);
void GPIO_Mode(UWORD GPIO_Pin, UWORD Mode);
void DEV_SPI_WriteByte(UBYTE data);
UBYTE DEV_SPI_ReadByte();
void DEV_SPI_Write_nByte(UBYTE *pData, UDOUBLE len);

/* New helpers to control CS / transactions across multiple writes */
void DEV_SPI_BeginTransaction(void);   // beginTransaction + assert CS
void DEV_SPI_EndTransaction(void);     // deassert CS + endTransaction
void DEV_CS_Assert(void);
void DEV_CS_Release(void);

extern SPIClass *devSpi;
extern SPISettings devSpiSettings;

#endif
