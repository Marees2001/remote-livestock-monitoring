#ifndef SSD1306_H
#define SSD1306_H

/**
 * @file    SSD1306.h
 * @brief   STM32 HAL C port of the SSD1306 OLED driver.
 * @author  Ported from Saurav Sajeev's Arduino C++ library.
 *
 * Usage:
 *   1. Declare an OLED_t struct and call OLED_init().
 *   2. Draw shapes/text into the internal buffer.
 *   3. Call OLED_inflate() to push the buffer to the display.
 *
 * Replace `&hi2c1` with your actual I2C handle.
 */

#include "stm32f1xx_hal.h"   /* <-- Change to match your MCU family */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ------------------------------------------------------------------ */
/*  I2C default address                                                 */
/* ------------------------------------------------------------------ */
#define OLED_ADDR_DEFAULT  0x3C   /* Shift left by 1 for HAL: 0x78   */

/* ------------------------------------------------------------------ */
/*  Display register addresses                                          */
/* ------------------------------------------------------------------ */
#define OLED_OFF               0xAE
#define OLED_ON                0xAF
#define OLED_INVERT            0xA7
#define OLED_REVERT            0xA6
#define OLED_ENTIRE_DISP_ON    0xA5
#define OLED_RESUME_FROM_VRAM  0xA4
#define OLED_DISP_CLOCK_DIV    0xD5
#define OLED_MULTIPLEX         0xA8
#define OLED_CHRG_PUMP         0x8D
#define OLED_DISP_OFFSET       0xD3
#define OLED_MEM_ADDR_MODE     0x20
#define OLED_COM_CONFIG        0xDA
#define OLED_CONTRAST          0x81
#define OLED_PRE_CHRG          0xD9
#define OLED_VCOMH_DESEL       0xDB
#define OLED_TURN_ON_ALL       0xA4
#define OLED_SET_DISP_NORMAL   0xA6
#define OLED_SET_COL_ADDR      0x21
#define OLED_SET_PG_ADDR       0x22

/* ------------------------------------------------------------------ */
/*  Register command values                                             */
/* ------------------------------------------------------------------ */
#define OLED_CLK_RATIO_RST     0x80
#define OLED_CLK_RATIO_HIGH    0x80
#define OLED_CLK_RATIO_MED     0xB0
#define OLED_CLK_RATIO_LOW     0xF0
#define OLED_OFFSET_NO         0x00
#define OLED_START_LINE        0x40
#define OLED_CHRG_PUMP_75      0x24
#define OLED_CHRG_PUMP_85      0x27
#define OLED_CHRG_PUMP_90      0x4F
#define OLED_CHRG_PUMP_OFF     0x10
#define OLED_HORIZONTAL        0x00
#define OLED_SEG_REMAP_127     0xA1
#define OLED_COM_SCAN_REMAP    0xC8
#define OLED_COM_ALT           0x12
#define OLED_CONTRAST_MAX      0xFF
#define OLED_PRE_CHRG_RST      0x22
#define OLED_PRE_CHRG_MAX      0xAA
#define OLED_VCOMH_65          0x00
#define OLED_VCOMH_71          0x10
#define OLED_VCOMH_77          0x20
#define OLED_VCOMH_83          0x30

/* ------------------------------------------------------------------ */
/*  Power modes                                                         */
/* ------------------------------------------------------------------ */
typedef enum {
    OLED_LOW_POWER_MODE   = 0x01,
    OLED_BALANCED_MODE    = 0x02,
    OLED_PERFORMANCE_MODE = 0x03,
    OLED_TURBO_MODE       = 0x04
} OLED_PowerMode;

/* ------------------------------------------------------------------ */
/*  OLED handle struct  (equivalent to the C++ OLED class)             */
/* ------------------------------------------------------------------ */
#define OLED_BUFFER_SIZE  (128 * 64 / 8)   /* 1024 bytes              */

typedef struct {
    I2C_HandleTypeDef *hi2c;        /* HAL I2C handle pointer          */
    uint8_t  address;               /* 7-bit I2C address (e.g. 0x3C)   */
    uint8_t  width;                 /* Display width  in pixels         */
    uint8_t  height;                /* Display height in pixels         */
    uint8_t  buffer[OLED_BUFFER_SIZE];
    const uint8_t (*fontSet)[5];    /* Active font table               */
    uint8_t  fontWidth;
    uint8_t  charWidth;
    uint8_t  stepVal;
    bool     isSetup;
    bool     pixelState;            /* true = set pixel, false = clear  */
    bool     clearOnClr;
    uint8_t  invertState;
    uint8_t  currentPowerMode;
} OLED_t;

/* ------------------------------------------------------------------ */
/*  Initialisation                                                      */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialise the OLED handle and run the auto-setup sequence.
 * @param  oled     Pointer to an OLED_t struct.
 * @param  hi2c     Pointer to the HAL I2C handle (e.g. &hi2c1).
 * @param  width    Display width  (e.g. 128).
 * @param  height   Display height (e.g. 64).
 * @param  address  7-bit I2C address (e.g. OLED_ADDR_DEFAULT = 0x3C).
 */
void OLED_init(OLED_t *oled, I2C_HandleTypeDef *hi2c,
               uint8_t width, uint8_t height, uint8_t address);

/* ------------------------------------------------------------------ */
/*  Text                                                                */
/* ------------------------------------------------------------------ */

/** @brief Print text at (x, y). */
void OLED_print(OLED_t *oled, const char *str, uint8_t x, uint8_t y);

/** @brief Print inverted (highlighted) text at (x, y). */
void OLED_printHighlighted(OLED_t *oled, const char *str, uint8_t x, uint8_t y);

/** @brief Print text with a typewriter animation (delayMs between chars). */
void OLED_printAnimated(OLED_t *oled, const char *str, uint8_t x, uint8_t y,
                        uint32_t delayMs, bool highlight);

/** @brief Clear screen then print text at (x, y). */
void OLED_printClear(OLED_t *oled, const char *str, uint8_t x, uint8_t y);

/* ------------------------------------------------------------------ */
/*  Drawing primitives                                                  */
/* ------------------------------------------------------------------ */

/** @brief Draw a straight line between two points with given thickness. */
void OLED_line(OLED_t *oled, uint8_t startX, uint8_t startY,
               uint8_t endX, uint8_t endY, uint8_t thickness);

/** @brief Draw a rectangle.  cornerRadius=0 for sharp corners. */
void OLED_rectangle(OLED_t *oled, uint8_t startX, uint8_t startY,
                    uint8_t width, uint8_t height,
                    uint8_t cornerRadius, uint8_t thickness, bool fill);

/** @brief Draw a circle. */
void OLED_circle(OLED_t *oled, uint8_t centerX, uint8_t centerY,
                 uint8_t radius, uint8_t thickness);

/** @brief Draw a bitmap image. */
void OLED_draw(OLED_t *oled, const uint8_t *dataSet,
               uint8_t x, uint8_t y, uint8_t width, uint8_t height);

/** @brief Draw a progress bar (style 1–15). */
void OLED_progressBar(OLED_t *oled, uint8_t progress,
                      uint8_t x, uint8_t y, int style);

/* ------------------------------------------------------------------ */
/*  Data plots                                                          */
/* ------------------------------------------------------------------ */

void OLED_pulsePlot    (OLED_t *oled, uint8_t x, uint8_t y,
                        uint8_t w, uint8_t h, int *data, uint8_t size,
                        int maxVal, int median, uint8_t axisThickness);

void OLED_bitBarPlot   (OLED_t *oled, uint8_t x, uint8_t y,
                        uint8_t w, uint8_t h, int *data, uint8_t size,
                        int maxVal, int median, uint8_t axisThickness);

void OLED_scatterPlot  (OLED_t *oled, uint8_t x, uint8_t y,
                        uint8_t w, uint8_t h, int *data, uint8_t size,
                        int maxVal, int median, uint8_t axisThickness);

void OLED_histogramPlot(OLED_t *oled, uint8_t x, uint8_t y,
                        uint8_t w, uint8_t h, int *data, uint8_t size,
                        int maxVal);

void OLED_barPlot      (OLED_t *oled, uint8_t x, uint8_t y,
                        uint8_t w, uint8_t h, int *data, uint8_t size,
                        int maxVal);

/* ------------------------------------------------------------------ */
/*  Display control                                                     */
/* ------------------------------------------------------------------ */

/** @brief Push the internal buffer to the physical display. */
void OLED_inflate(OLED_t *oled);

/** @brief Clear buffer. If refresh=true, immediately push blank frame. */
void OLED_clearScr(OLED_t *oled, bool refresh);

/** @brief Clear a horizontal segment on the given page (row). */
void OLED_clearArea(OLED_t *oled, uint8_t startX, uint8_t endX, uint8_t page);

/** @brief Set brightness 0–100 %. */
void OLED_setBrightness(OLED_t *oled, uint8_t brightness);

/** @brief Toggle invert mode. */
void OLED_invertDisplay(OLED_t *oled);

/** @brief Force all pixels ON (ignores VRAM). */
void OLED_entireDisplayON(OLED_t *oled);

/** @brief Return to normal VRAM-driven display. */
void OLED_entireDisplayOFF(OLED_t *oled);

/** @brief Choose power / speed trade-off. */
void OLED_setPowerMode(OLED_t *oled, OLED_PowerMode mode);

/** @brief Enable or disable super-brightness (unstable on some panels). */
void OLED_superBrightness(OLED_t *oled, bool enable);

/** @brief Turn off display when clearScr is called. */
void OLED_turnOffOnClr(OLED_t *oled, bool mode);

/**
 * @brief  Set the pixel drawing state.
 * @param  state  true  = subsequent draw calls SET  pixels (default).
 *                false = subsequent draw calls CLEAR pixels (erase).
 */
void OLED_invertPixelState(OLED_t *oled, bool state);

/** @brief Set a custom 5-column font table. */
void OLED_setFont(OLED_t *oled, const uint8_t (*fontArray)[5]);

/** @brief Restore the built-in default font. */
void OLED_clearCustomFont(OLED_t *oled);

#endif /* SSD1306_H */
