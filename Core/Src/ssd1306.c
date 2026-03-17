/**
 * @file    SSD1306.c
 * @brief   STM32 HAL C port of the SSD1306 OLED driver.
 * @author  Ported from Saurav Sajeev's Arduino C++ library.
 *
 * Changes from the original Arduino C++ version:
 *  - Arduino Wire  → HAL_I2C_Master_Transmit
 *  - Arduino String → const char*
 *  - PROGMEM / pgm_read_byte removed (STM32 reads flash directly)
 *  - delay() → HAL_Delay()
 *  - yield()  removed
 *  - C++ class OLED → C struct OLED_t + OLED_xxx() functions
 *  - new / delete removed (no dynamic allocation needed in .c driver)
 */

#include "../Inc/SSD1306.h"

/* ================================================================== */
/*  Font table  (96 printable ASCII chars, 5 columns each)             */
/* ================================================================== */
static const uint8_t fonts[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* ' ' */
    {0x00,0x00,0x5F,0x00,0x00}, /* '!' */
    {0x00,0x07,0x00,0x07,0x00}, /* '"' */
    {0x14,0x7F,0x14,0x7F,0x14}, /* '#' */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* '$' */
    {0x23,0x13,0x08,0x64,0x62}, /* '%' */
    {0x36,0x49,0x55,0x22,0x50}, /* '&' */
    {0x00,0x05,0x03,0x00,0x00}, /* ''' */
    {0x00,0x1C,0x22,0x41,0x00}, /* '(' */
    {0x00,0x41,0x22,0x1C,0x00}, /* ')' */
    {0x14,0x08,0x3E,0x08,0x14}, /* '*' */
    {0x08,0x08,0x3E,0x08,0x08}, /* '+' */
    {0x00,0x50,0x30,0x00,0x00}, /* ',' */
    {0x08,0x08,0x08,0x08,0x08}, /* '-' */
    {0x00,0x60,0x60,0x00,0x00}, /* '.' */
    {0x20,0x10,0x08,0x04,0x02}, /* '/' */
    {0x3E,0x51,0x49,0x45,0x3E}, /* '0' */
    {0x00,0x42,0x7F,0x40,0x00}, /* '1' */
    {0x42,0x61,0x51,0x49,0x46}, /* '2' */
    {0x21,0x41,0x45,0x4B,0x31}, /* '3' */
    {0x18,0x14,0x12,0x7F,0x10}, /* '4' */
    {0x27,0x45,0x45,0x45,0x39}, /* '5' */
    {0x3C,0x4A,0x49,0x49,0x30}, /* '6' */
    {0x01,0x71,0x09,0x05,0x03}, /* '7' */
    {0x36,0x49,0x49,0x49,0x36}, /* '8' */
    {0x06,0x49,0x49,0x29,0x1E}, /* '9' */
    {0x00,0x36,0x36,0x00,0x00}, /* ':' */
    {0x00,0x56,0x36,0x00,0x00}, /* ';' */
    {0x08,0x14,0x22,0x41,0x00}, /* '<' */
    {0x14,0x14,0x14,0x14,0x14}, /* '=' */
    {0x00,0x41,0x22,0x14,0x08}, /* '>' */
    {0x02,0x01,0x51,0x09,0x06}, /* '?' */
    {0x32,0x49,0x79,0x41,0x3E}, /* '@' */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 'A' */
    {0x7F,0x49,0x49,0x49,0x36}, /* 'B' */
    {0x3E,0x41,0x41,0x41,0x22}, /* 'C' */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 'D' */
    {0x7F,0x49,0x49,0x49,0x41}, /* 'E' */
    {0x7F,0x09,0x09,0x09,0x01}, /* 'F' */
    {0x3E,0x41,0x49,0x49,0x7A}, /* 'G' */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 'H' */
    {0x00,0x41,0x7F,0x41,0x00}, /* 'I' */
    {0x20,0x40,0x41,0x3F,0x01}, /* 'J' */
    {0x7F,0x08,0x14,0x22,0x41}, /* 'K' */
    {0x7F,0x40,0x40,0x40,0x40}, /* 'L' */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* 'M' */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 'N' */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 'O' */
    {0x7F,0x09,0x09,0x09,0x06}, /* 'P' */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 'Q' */
    {0x7F,0x09,0x19,0x29,0x46}, /* 'R' */
    {0x46,0x49,0x49,0x49,0x31}, /* 'S' */
    {0x01,0x01,0x7F,0x01,0x01}, /* 'T' */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 'U' */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 'V' */
    {0x3F,0x40,0x38,0x40,0x3F}, /* 'W' */
    {0x63,0x14,0x08,0x14,0x63}, /* 'X' */
    {0x07,0x08,0x70,0x08,0x07}, /* 'Y' */
    {0x61,0x51,0x49,0x45,0x43}, /* 'Z' */
    {0x00,0x7F,0x41,0x41,0x00}, /* '[' */
    {0x02,0x04,0x08,0x10,0x20}, /* '\' */
    {0x00,0x41,0x41,0x7F,0x00}, /* ']' */
    {0x04,0x02,0x01,0x02,0x04}, /* '^' */
    {0x40,0x40,0x40,0x40,0x40}, /* '_' */
    {0x00,0x01,0x02,0x04,0x00}, /* '`' */
    {0x20,0x54,0x54,0x54,0x78}, /* 'a' */
    {0x7F,0x48,0x44,0x44,0x38}, /* 'b' */
    {0x38,0x44,0x44,0x44,0x20}, /* 'c' */
    {0x38,0x44,0x44,0x48,0x7F}, /* 'd' */
    {0x38,0x54,0x54,0x54,0x18}, /* 'e' */
    {0x08,0x7E,0x09,0x01,0x02}, /* 'f' */
    {0x0C,0x52,0x52,0x52,0x3E}, /* 'g' */
    {0x7F,0x08,0x04,0x04,0x78}, /* 'h' */
    {0x00,0x44,0x7D,0x40,0x00}, /* 'i' */
    {0x20,0x40,0x44,0x3D,0x00}, /* 'j' */
    {0x7F,0x10,0x28,0x44,0x00}, /* 'k' */
    {0x00,0x41,0x7F,0x40,0x00}, /* 'l' */
    {0x7C,0x04,0x18,0x04,0x78}, /* 'm' */
    {0x7C,0x08,0x04,0x04,0x78}, /* 'n' */
    {0x38,0x44,0x44,0x44,0x38}, /* 'o' */
    {0x7C,0x14,0x14,0x14,0x08}, /* 'p' */
    {0x08,0x14,0x14,0x18,0x7C}, /* 'q' */
    {0x7C,0x08,0x04,0x04,0x08}, /* 'r' */
    {0x48,0x54,0x54,0x54,0x20}, /* 's' */
    {0x04,0x3F,0x44,0x40,0x20}, /* 't' */
    {0x3C,0x40,0x40,0x20,0x7C}, /* 'u' */
    {0x1C,0x20,0x40,0x20,0x1C}, /* 'v' */
    {0x3C,0x40,0x30,0x40,0x3C}, /* 'w' */
    {0x44,0x28,0x10,0x28,0x44}, /* 'x' */
    {0x0C,0x50,0x50,0x50,0x3C}, /* 'y' */
    {0x44,0x64,0x54,0x4C,0x44}, /* 'z' */
    {0x00,0x08,0x36,0x41,0x00}, /* '{' */
    {0x00,0x00,0x7F,0x00,0x00}, /* '|' */
    {0x00,0x41,0x36,0x08,0x00}, /* '}' */
    {0x08,0x08,0x2A,0x1C,0x08}  /* '~' */
};

/* ================================================================== */
/*  Progress-bar bitmaps (identical to original, PROGMEM removed)      */
/* ================================================================== */
static const uint8_t progressBarSet[11][6][5] = {
    { {0x3E,0x41,0x41,0x41,0x41},{0x41,0x41,0x41,0x41,0x3E},{0x41,0x41,0x41,0x41,0x41},
      {0x3E,0x7F,0x7F,0x7F,0x7F},{0x7F,0x7F,0x7F,0x7F,0x3E},{0x7F,0x7F,0x7F,0x7F,0x7F} },
    { {0x18,0x18,0x18,0x18,0x18},{0x18,0x18,0x18,0x18,0x18},{0x18,0x18,0x18,0x18,0x18},
      {0x3E,0x7F,0x7F,0x7F,0x7F},{0x7F,0x7F,0x7F,0x7F,0x3E},{0x7F,0x7F,0x7F,0x7F,0x7F} },
    { {0x7E,0xC3,0xC3,0xC3,0xC3},{0xC3,0xC3,0xC3,0xC3,0x7E},{0xC3,0xC3,0xC3,0xC3,0xC3},
      {0x7E,0xDB,0xDB,0xDB,0xDB},{0xDB,0xDB,0xDB,0xDB,0x7E},{0xDB,0xDB,0xDB,0xDB,0xDB} },
    { {0xFF,0x81,0x81,0x81,0x81},{0x81,0x81,0x81,0x81,0xFF},{0x81,0x81,0x81,0x81,0x81},
      {0xFF,0xFF,0xFF,0xFF,0xFF},{0xFF,0xFF,0xFF,0xFF,0xFF},{0xFF,0xFF,0xFF,0xFF,0xFF} },
    { {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00},
      {0x1C,0x1C,0x1C,0x1C,0x1C},{0x1C,0x1C,0x1C,0x1C,0x1C},{0x1C,0x1C,0x1C,0x1C,0x1C} },
    { {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00},
      {0x10,0x38,0x38,0x38,0x38},{0x38,0x38,0x38,0x38,0x10},{0x38,0x38,0x38,0x38,0x38} },
    { {0x38,0x44,0x44,0x44,0x44},{0x44,0x44,0x44,0x44,0x38},{0x44,0x44,0x44,0x44,0x44},
      {0x38,0x7C,0x7C,0x7C,0x7C},{0x7C,0x7C,0x7C,0x7C,0x38},{0x7C,0x7C,0x7C,0x7C,0x7C} },
    { {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00},
      {0x38,0x7C,0x7C,0x7C,0x7C},{0x7C,0x7C,0x7C,0x7C,0x38},{0x7C,0x7C,0x7C,0x7C,0x7C} },
    { {0x10,0x38,0x38,0x38,0x10},{0x10,0x38,0x38,0x38,0x10},{0x10,0x38,0x38,0x38,0x10},
      {0x38,0x7C,0x7C,0x7C,0x38},{0x38,0x7C,0x7C,0x7C,0x38},{0x38,0x7C,0x7C,0x7C,0x38} },
    { {0x38,0x44,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x38},
      {0x38,0x7C,0x7C,0x7C,0x38},{0x38,0x7C,0x7C,0x7C,0x38},{0x38,0x7C,0x7C,0x7C,0x38} },
    { {0x3E,0x7F,0x7F,0x7F,0x7F},{0x7F,0x7F,0x7F,0x7F,0x3E},{0x38,0x7C,0x7C,0x7C,0x38},
      {0x3E,0x7F,0x7F,0x7F,0x3E},{0x7F,0x7F,0x7F,0x7F,0x7F},{0x7F,0x7F,0x7F,0x7F,0x3E} }
};

static const uint8_t progressBarSet1[2][20] = {
    {0x3E,0x7F,0x7F,0x7F,0x7F, 0x7F,0x7F,0x7F,0x7F,0x7F,
     0x7F,0x7F,0x7F,0x7F,0x7F, 0x7F,0x7F,0x7F,0x7F,0x3E},
    {0x1C,0x3E,0x3E,0x3E,0x3E, 0x3E,0x3E,0x3E,0x3E,0x3E,
     0x3E,0x3E,0x3E,0x3E,0x3E, 0x3E,0x3E,0x3E,0x3E,0x1C}
};

/* ================================================================== */
/*  Internal helpers (static – not exposed in the header)              */
/* ================================================================== */

/** Send one command byte over I2C. */
static void s_execute(OLED_t *oled, uint8_t cmd)
{
    uint8_t buf[2] = { 0x00, cmd };
    HAL_I2C_Master_Transmit(oled->hi2c, (uint16_t)(oled->address << 1),
                            buf, 2, HAL_MAX_DELAY);
}

/** Send one data byte over I2C. */
static void s_sendData(OLED_t *oled, uint8_t data)
{
    uint8_t buf[2] = { 0x40, data };
    HAL_I2C_Master_Transmit(oled->hi2c, (uint16_t)(oled->address << 1),
                            buf, 2, HAL_MAX_DELAY);
}

/** Clamp x to [0, width-1]. */
static uint8_t s_checkX(OLED_t *oled, uint8_t x)
{
    return (x >= oled->width) ? (oled->width - 1) : x;
}

/** Clamp y to [0, height-1]. */
static uint8_t s_checkY(OLED_t *oled, uint8_t y)
{
    return (y >= oled->height) ? (oled->height - 1) : y;
}

/** Set I2C page-addressing cursor. */
static void s_setPosition(OLED_t *oled, uint8_t x, uint8_t y)
{
    uint8_t maxPage = (oled->height / 8) - 1;
    s_execute(oled, OLED_SET_PG_ADDR);
    s_execute(oled, y < (oled->height / 8) ? y : maxPage);
    s_execute(oled, maxPage);
    s_execute(oled, 0x00 + (x % 16));
    s_execute(oled, 0x10 + (x / 16));
}

/** Write one pixel into the buffer. */
static void s_drawPixel(OLED_t *oled, uint8_t x, uint8_t y)
{
    if (x >= oled->width || y >= oled->height) return;
    uint8_t page = y / 8;
    uint8_t bit  = y % 8;
    if (oled->pixelState)
        oled->buffer[x + page * 128] |=  (1 << bit);
    else
        oled->buffer[x + page * 128] &= ~(1 << bit);
}

/** Zero-fill the buffer. */
static void s_clearBuffer(OLED_t *oled)
{
    memset(oled->buffer, 0, OLED_BUFFER_SIZE);
}

/** Flush the buffer to the display via I2C. */
static void s_displayBuffer(OLED_t *oled)
{
    for (uint8_t page = 0; page < 8; page++) {
        s_execute(oled, 0xB0 + page);
        s_execute(oled, 0x00);
        s_execute(oled, 0x10);
        for (uint8_t col = 0; col < 128; col++)
            s_sendData(oled, oled->buffer[page * 128 + col]);
    }
}

/** Draw one glyph column into the buffer (supports y-alignment). */
static void s_drawFontPixel(OLED_t *oled, uint8_t data, uint8_t x, uint8_t y)
{
    if (x >= oled->width || y >= oled->height) return;
    uint8_t page   = y / 8;
    uint8_t bitPos = y % 8;
    if (bitPos == 0) {
        oled->buffer[x + page * oled->width] = data;
    } else {
        uint8_t prevData = oled->buffer[x + page * oled->width];
        uint8_t nextData = oled->buffer[x + (page + 1) * oled->width];
        prevData &= ~(0xFF << bitPos);
        nextData &= ~(0xFF >> (8 - bitPos));
        if (oled->pixelState) {
            oled->buffer[x + page       * oled->width] = prevData | (data << bitPos);
            oled->buffer[x + (page + 1) * oled->width] = nextData | (data >> (8 - bitPos));
        } else {
            oled->buffer[x + page       * oled->width] = prevData & ~(data << bitPos);
            oled->buffer[x + (page + 1) * oled->width] = nextData & ~(data >> (8 - bitPos));
        }
    }
}

/** Render one character and advance charWidth. */
static void s_getFont(OLED_t *oled, char c, bool highlight, uint8_t y)
{
    uint8_t fw = oled->fontWidth;
    if (c == '\n' || (oled->width - oled->charWidth) < fw) {
        oled->charWidth += oled->width - oled->charWidth;
        if (c != '\n') {
            oled->charWidth = 0;
            for (uint8_t i = 0; i < fw; i++) {
                uint8_t col = oled->fontSet[(uint8_t)(c - 32)][i];
                s_drawFontPixel(oled, highlight ? ~col : col, oled->charWidth, y + 8);
                oled->charWidth++;
            }
            s_drawFontPixel(oled, highlight ? 0xFF : 0x00, oled->charWidth++, y + 8);
            oled->stepVal++;
            oled->charWidth = oled->width + fw;
        } else {
            oled->charWidth = oled->width;
        }
    } else {
        for (uint8_t i = 0; i < fw; i++) {
            oled->charWidth++;
            uint8_t col = oled->fontSet[(uint8_t)(c - 32)][i];
            s_drawFontPixel(oled, highlight ? ~col : col, oled->charWidth, y);
        }
        oled->charWidth++;
        s_drawFontPixel(oled, highlight ? 0xFF : 0x00, oled->charWidth, y);
    }
}

/** Bresenham circle-quarter helper for rounded rects. */
static void s_drawCircleHelper(OLED_t *oled, int x0, int y0, int r, uint8_t corner)
{
    int f     = 1 - r;
    int ddx   = 1;
    int ddy   = -2 * r;
    int x = 0, y = r;
    while (x <= y) {
        if (f >= 0) { y--; ddy += 2; f += ddy; }
        x++; ddx += 2; f += ddx;
        if (corner & 0x1) { s_drawPixel(oled, x0-x, y0-y); s_drawPixel(oled, x0-y, y0-x); }
        if (corner & 0x2) { s_drawPixel(oled, x0+x, y0-y); s_drawPixel(oled, x0+y, y0-x); }
        if (corner & 0x4) { s_drawPixel(oled, x0+x, y0+y); s_drawPixel(oled, x0+y, y0+x); }
        if (corner & 0x8) { s_drawPixel(oled, x0-y, y0+x); s_drawPixel(oled, x0-x, y0+y); }
    }
}

/** Quarter-circle helper for rounded rects. */
static void s_drawCircleQuarter(OLED_t *oled, uint8_t cx, uint8_t cy,
                                uint8_t r, uint8_t corner)
{
    int x = r, y = 0;
    int d = 1 - x;
    while (y <= x) {
        switch (corner) {
        case 1: s_drawPixel(oled, cx-x, cy-y); s_drawPixel(oled, cx-y, cy-x); break;
        case 2: s_drawPixel(oled, cx+x, cy-y); s_drawPixel(oled, cx+y, cy-x); break;
        case 3: s_drawPixel(oled, cx-x, cy+y); s_drawPixel(oled, cx-y, cy+x); break;
        case 4: s_drawPixel(oled, cx+x, cy+y); s_drawPixel(oled, cx+y, cy+x); break;
        }
        y++;
        d = (d <= 0) ? d + 2*y + 1 : d + 2*(y - --x) + 1;
    }
}

/* Power-mode register sequences */
static void s_lowPowerMode(OLED_t *oled) {
    s_execute(oled, OLED_DISP_CLOCK_DIV); s_execute(oled, OLED_CLK_RATIO_LOW);
    s_execute(oled, OLED_CHRG_PUMP);      s_execute(oled, OLED_CHRG_PUMP_85);
    s_execute(oled, OLED_PRE_CHRG);       s_execute(oled, OLED_PRE_CHRG_RST);
    s_execute(oled, OLED_VCOMH_DESEL);    s_execute(oled, OLED_VCOMH_65);
}
static void s_balancedPowerMode(OLED_t *oled) {
    s_execute(oled, OLED_DISP_CLOCK_DIV); s_execute(oled, OLED_CLK_RATIO_MED);
    s_execute(oled, OLED_CHRG_PUMP);      s_execute(oled, OLED_CHRG_PUMP_75);
    s_execute(oled, OLED_PRE_CHRG);       s_execute(oled, OLED_PRE_CHRG_RST);
    s_execute(oled, OLED_VCOMH_DESEL);    s_execute(oled, OLED_VCOMH_77);
}
static void s_performancePowerMode(OLED_t *oled) {
    s_execute(oled, OLED_DISP_CLOCK_DIV); s_execute(oled, OLED_CLK_RATIO_HIGH);
    s_execute(oled, OLED_CHRG_PUMP);      s_execute(oled, OLED_CHRG_PUMP_75);
    s_execute(oled, OLED_PRE_CHRG);       s_execute(oled, 0xF1);
    s_execute(oled, OLED_VCOMH_DESEL);    s_execute(oled, OLED_VCOMH_83);
}

/* ================================================================== */
/*  Public API                                                          */
/* ================================================================== */

void OLED_init(OLED_t *oled, I2C_HandleTypeDef *hi2c,
               uint8_t width, uint8_t height, uint8_t address)
{
    memset(oled, 0, sizeof(OLED_t));
    oled->hi2c          = hi2c;
    oled->address       = address;
    oled->width         = width;
    oled->height        = height;
    oled->pixelState    = true;
    oled->fontWidth     = 5;
    oled->fontSet       = fonts;

    /* Auto-setup command sequence (mirrors original autoSetup()) */
    uint8_t cmds[] = {
        OLED_OFF,
        OLED_DISP_CLOCK_DIV,  OLED_CLK_RATIO_RST,
        OLED_MULTIPLEX,       (uint8_t)(height - 1),
        OLED_DISP_OFFSET,     OLED_OFFSET_NO,
        OLED_START_LINE,
        OLED_CHRG_PUMP,       OLED_CHRG_PUMP_75,
        OLED_MEM_ADDR_MODE,   OLED_HORIZONTAL,
        OLED_SEG_REMAP_127,
        OLED_COM_SCAN_REMAP,
        OLED_COM_CONFIG,      OLED_COM_ALT,
        OLED_CONTRAST,        OLED_CONTRAST_MAX,
        OLED_PRE_CHRG,        OLED_PRE_CHRG_RST,
        OLED_VCOMH_DESEL,     OLED_VCOMH_77,
        OLED_TURN_ON_ALL,
        OLED_SET_DISP_NORMAL,
        OLED_ON
    };
    for (int i = 0; i < (int)sizeof(cmds); i++)
        s_execute(oled, cmds[i]);

    oled->isSetup = true;
    s_balancedPowerMode(oled);
    oled->currentPowerMode = OLED_BALANCED_MODE;

    s_clearBuffer(oled);
    s_displayBuffer(oled);
}

/* ------------------------------------------------------------------ */
/*  Text                                                                */
/* ------------------------------------------------------------------ */

void OLED_print(OLED_t *oled, const char *str, uint8_t x, uint8_t y)
{
    if (!oled->isSetup || !str) return;
    oled->stepVal   = 0;
    oled->charWidth = x;
    while (*str) {
        s_getFont(oled, *str++, false, y);
        if (oled->charWidth >= 127) {
            oled->charWidth -= oled->width;
            y += 8;
        }
    }
}

void OLED_printHighlighted(OLED_t *oled, const char *str, uint8_t x, uint8_t y)
{
    if (!oled->isSetup || !str) return;
    oled->stepVal   = 0;
    oled->charWidth = x;
    while (*str) {
        s_getFont(oled, *str++, true, y);
        if (oled->charWidth >= 127) {
            oled->charWidth -= oled->width;
            y += 8;
        }
    }
}

void OLED_printAnimated(OLED_t *oled, const char *str, uint8_t x, uint8_t y,
                        uint32_t delayMs, bool highlight)
{
    if (!oled->isSetup || !str) return;
    oled->stepVal   = 0;
    oled->charWidth = x;
    while (*str) {
        s_getFont(oled, *str++, highlight, y);
        if (oled->charWidth >= 127) {
            oled->charWidth -= oled->width;
            y += 8;
        }
        s_displayBuffer(oled);
        HAL_Delay(delayMs);
    }
}

void OLED_printClear(OLED_t *oled, const char *str, uint8_t x, uint8_t y)
{
    OLED_clearScr(oled, false);
    OLED_print(oled, str, x, y);
}

/* ------------------------------------------------------------------ */
/*  Drawing primitives                                                  */
/* ------------------------------------------------------------------ */

void OLED_line(OLED_t *oled, uint8_t startX, uint8_t startY,
               uint8_t endX,   uint8_t endY,   uint8_t thickness)
{
    startX = s_checkX(oled, startX);
    startY = s_checkY(oled, startY);
    endX   = s_checkX(oled, endX);
    endY   = s_checkY(oled, endY);

    int dx  =  abs(endX - startX);
    int dy  = -abs(endY - startY);
    int sx  = (startX == endX) ? 0 : (startX < endX ? 1 : -1);
    int sy  = (startY == endY) ? 0 : (startY < endY ? 1 : -1);
    int err = dx + dy;
    int half = thickness / 2;

    for (int i = -half; i < half + (thickness % 2); i++) {
        int ox = i * sy,  oy = -i * sx;
        int x0 = startX + ox, y0 = startY + oy;
        int x1 = endX   + ox, y1 = endY   + oy;
        int e2, tx = x0, ty = y0, er = err;
        while (1) {
            s_drawPixel(oled, (uint8_t)tx, (uint8_t)ty);
            if (tx == x1 && ty == y1) break;
            e2 = 2 * er;
            if (e2 >= dy) { er += dy; tx += sx; }
            if (e2 <= dx) { er += dx; ty += sy; }
        }
    }
}

void OLED_circle(OLED_t *oled, uint8_t cx, uint8_t cy,
                 uint8_t radius, uint8_t thickness)
{
    if (thickness < 1) thickness = 1;
    for (uint8_t t = 0; t < thickness; t++) {
        int f = 1 - radius, ddx = 1, ddy = -2 * radius, x = 0, y = radius;
        s_drawPixel(oled, cx,          cy + radius);
        s_drawPixel(oled, cx,          cy - radius);
        s_drawPixel(oled, cx + radius, cy);
        s_drawPixel(oled, cx - radius, cy);
        while (x < y) {
            if (f >= 0) { y--; ddy += 2; f += ddy; }
            x++; ddx += 2; f += ddx;
            s_drawPixel(oled, cx+x, cy+y); s_drawPixel(oled, cx-x, cy+y);
            s_drawPixel(oled, cx+x, cy-y); s_drawPixel(oled, cx-x, cy-y);
            s_drawPixel(oled, cx+y, cy+x); s_drawPixel(oled, cx-y, cy+x);
            s_drawPixel(oled, cx+y, cy-x); s_drawPixel(oled, cx-y, cy-x);
        }
        radius--;
    }
}

void OLED_rectangle(OLED_t *oled, uint8_t startX, uint8_t startY,
                    uint8_t width, uint8_t height,
                    uint8_t cornerRadius, uint8_t thickness, bool fill)
{
    if (cornerRadius > width  / 2) cornerRadius = width  / 2;
    if (cornerRadius > height / 2) cornerRadius = height / 2;
    startX = s_checkX(oled, startX);
    startY = s_checkY(oled, startY);
    if (thickness > 10) thickness = 10;

    for (uint8_t t = 0; t < thickness; t++) {
        uint8_t ix = startX + t, iy = startY + t;
        uint8_t iw = width  - 2*t, ih = height - 2*t;
        OLED_line(oled, ix + cornerRadius, iy,       ix + iw - cornerRadius, iy,       1);
        OLED_line(oled, ix + cornerRadius, iy + ih,  ix + iw - cornerRadius, iy + ih,  1);
        OLED_line(oled, ix,       iy + cornerRadius, ix,       iy + ih - cornerRadius, 1);
        OLED_line(oled, ix + iw,  iy + cornerRadius, ix + iw,  iy + ih - cornerRadius, 1);
        if (cornerRadius > 0) {
            s_drawCircleQuarter(oled, ix + cornerRadius,      iy + cornerRadius,      cornerRadius, 1);
            s_drawCircleQuarter(oled, ix + iw - cornerRadius, iy + cornerRadius,      cornerRadius, 2);
            s_drawCircleQuarter(oled, ix + iw - cornerRadius, iy + ih - cornerRadius, cornerRadius, 4);
            s_drawCircleQuarter(oled, ix + cornerRadius,      iy + ih - cornerRadius, cornerRadius, 3);
        }
    }

    if (fill) {
        for (uint8_t i = startX + cornerRadius; i < startX + width - cornerRadius; i++)
            for (uint8_t j = startY + 1; j < startY + height; j++)
                s_drawPixel(oled, i, j);
        for (uint8_t i = startX + 1; i < startX + width; i++)
            for (uint8_t j = startY + cornerRadius; j < startY + height - cornerRadius; j++)
                s_drawPixel(oled, i, j);
    }

    if (cornerRadius > 0 && fill) {
        for (int16_t x = -(int16_t)cornerRadius; x <= (int16_t)cornerRadius; x++) {
            for (int16_t y = -(int16_t)cornerRadius; y <= (int16_t)cornerRadius; y++) {
                if (x*x + y*y <= (int16_t)(cornerRadius * cornerRadius)) {
                    s_drawPixel(oled, startX + cornerRadius          + x, startY + cornerRadius          + y);
                    s_drawPixel(oled, startX + width  - cornerRadius + x, startY + cornerRadius          + y);
                    s_drawPixel(oled, startX + cornerRadius          + x, startY + height - cornerRadius + y);
                    s_drawPixel(oled, startX + width  - cornerRadius + x, startY + height - cornerRadius + y);
                }
            }
        }
    }
}

void OLED_draw(OLED_t *oled, const uint8_t *dataSet,
               uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    y /= 8;
    uint8_t vPos = (y > 7) ? 7 : y;
    s_setPosition(oled, x, vPos);

    for (uint8_t mul = 0; mul < height / 8; mul++) {
        oled->charWidth = x;
        for (uint8_t row = 0; row < width / 8; row++) {
            for (uint8_t b = 0; b < 8; b++) {
                uint8_t byte = 0;
                for (uint8_t col = 0; col < 8; col++) {
                    uint8_t src = dataSet[((width/8) * 8 * mul) + (col * (width/8) + row)];
                    byte |= ((src & (0x80 >> b)) >> (7 - b)) << col;
                }
                oled->buffer[oled->charWidth + vPos * oled->width] = byte;
                oled->charWidth++;
                if (oled->charWidth > 127) break;
            }
            if (oled->charWidth > 127) break;
        }
        vPos++;
        if (vPos > (oled->height / 8) - 1) break;
        s_setPosition(oled, x, vPos);
    }
}

/* ------------------------------------------------------------------ */
/*  Progress bar                                                        */
/* ------------------------------------------------------------------ */

void OLED_progressBar(OLED_t *oled, uint8_t progress,
                      uint8_t x, uint8_t y, int style)
{
    s_setPosition(oled, x, y);
    progress = (uint8_t)(progress / 6.25f);
    if (style > 15) style = 4;

    if (style < 11) {
        for (uint8_t i = 1; i <= 16; i++) {
            for (uint8_t j = 0; j < 5; j++) {
                if (i <= progress && i > 0) {
                    if      (i == 1)                          s_sendData(oled, progressBarSet[style-1][3][j]);
                    else if (i == 16)                         s_sendData(oled, progressBarSet[style-1][4][j]);
                    else if (i == progress && style == 2)     s_sendData(oled, progressBarSet[style-1][4][j]);
                    else                                      s_sendData(oled, progressBarSet[style-1][5][j]);
                } else {
                    if      (i == 1)  s_sendData(oled, progressBarSet[style-1][0][j]);
                    else if (i == 16) s_sendData(oled, progressBarSet[style-1][1][j]);
                    else              s_sendData(oled, progressBarSet[style-1][2][j]);
                }
            }
        }
    } else if (style > 13) {
        if (progress > 12) progress = 12;
        for (uint8_t i = 0; i < 13; i++) {
            if (i == progress)
                for (uint8_t j = 0; j < 20; j++)
                    s_sendData(oled, progressBarSet1[style - 14][j]);
            else
                for (uint8_t j = 0; j < 5; j++)
                    s_sendData(oled, 0x00);
        }
    } else {
        if (progress < 15) {
            uint8_t setVal = (style == 11) ? 0 : (uint8_t)(style - 10);
            for (uint8_t i = 0; i <= 16; i++) {
                for (uint8_t j = 0; j < 5; j++) {
                    if      (i == progress     && i < 15)              s_sendData(oled, progressBarSet[10][setVal][j]);
                    else if (i == progress + 1 && i < 16 && style==11) s_sendData(oled, progressBarSet[10][setVal+1][j]);
                    else                                                s_sendData(oled, 0x00);
                }
            }
        }
    }
    HAL_Delay(1);
}

/* ------------------------------------------------------------------ */
/*  Data plots                                                          */
/* ------------------------------------------------------------------ */

void OLED_pulsePlot(OLED_t *oled, uint8_t x, uint8_t y,
                    uint8_t w, uint8_t h, int *data, uint8_t size,
                    int maxVal, int median, uint8_t axisThickness)
{
    if (!data || size == 0 || maxVal == 0) return;
    float sf = h / (2.0f * maxVal);
    if (size > w) size = w;
    uint8_t axis = y + h / 2;
    if (median > maxVal) median = maxVal;
    OLED_line(oled, x, axis - (uint8_t)(sf * median),
              x + w,  axis - (uint8_t)(sf * median), axisThickness);

    uint8_t step = w / size;
    uint8_t cy   = axis - (uint8_t)(sf * median);
    for (uint8_t i = 0; i < size; i++) {
        if (data[i] > maxVal) data[i] = maxVal;
        int off = (median * data[i] < median) ? 1 : -1;
        uint8_t ny = axis - (uint8_t)(sf * (data[i] + off));
        OLED_line(oled, x, cy, x + step, ny, 1);
        cy  = ny;
        x  += step;
    }
}

void OLED_bitBarPlot(OLED_t *oled, uint8_t x, uint8_t y,
                     uint8_t w, uint8_t h, int *data, uint8_t size,
                     int maxVal, int median, uint8_t axisThickness)
{
    if (!data || size == 0 || maxVal == 0) return;
    float sf = h / (2.0f * maxVal);
    if (size > w) size = w;
    uint8_t axis = y + h / 2;
    if (median > maxVal) median = maxVal;
    OLED_line(oled, x, axis - (uint8_t)(sf * median),
              x + w,  axis - (uint8_t)(sf * median), axisThickness);

    uint8_t barWidth = 1;
    uint8_t step = barWidth + (w / size);
    uint8_t cx   = x;
    for (uint8_t i = 0; i < size && (cx + barWidth) <= (x + w); i++) {
        int bh = (int)(sf * abs(data[i]));
        int by = (data[i] >= median) ? axis - bh : axis;
        for (uint8_t bw = 0; bw < barWidth; bw++) {
            if (data[i] >= median)
                OLED_line(oled, cx+bw, axis,    cx+bw, (uint8_t)by,        1);
            else
                OLED_line(oled, cx+bw, axis,    cx+bw, (uint8_t)(axis+bh), 1);
        }
        cx += step;
    }
}

void OLED_scatterPlot(OLED_t *oled, uint8_t x, uint8_t y,
                      uint8_t w, uint8_t h, int *data, uint8_t size,
                      int maxVal, int median, uint8_t axisThickness)
{
    if (!data || size == 0 || maxVal == 0) return;
    float sf = h / (2.0f * maxVal);
    if (size > w) size = w;
    int axis = y + h / 2;
    int mly  = axis - (int)roundf(sf * median);
    if (median > maxVal) median = maxVal;
    OLED_line(oled, x, (uint8_t)mly, x + w, (uint8_t)mly, axisThickness);

    uint8_t step = (w / size > 1) ? w / size : 1;
    for (uint8_t i = 0; i < size && x + i * step < x + w; i++) {
        uint8_t px = x + i * step;
        if (data[i] > maxVal) data[i] = maxVal;
        int off = (median != 0 && (median * data[i]) < median) ? 1 : -1;
        int py = axis - (int)roundf(sf * (data[i] + off));
        if (py >= y && py < y + h)
            s_drawPixel(oled, px, (uint8_t)py);
    }
}

void OLED_histogramPlot(OLED_t *oled, uint8_t x, uint8_t y,
                        uint8_t w, uint8_t h, int *data, uint8_t size,
                        int maxVal)
{
    if (maxVal == 0) return;
    float sf = (float)h / maxVal;
    if (size > w) size = w;
    uint8_t step     = w / size;
    uint8_t barWidth = 3 + step;
    OLED_line(oled, x, y + h, x + w, y + h, 1);
    for (uint8_t i = 0; i < size && x <= w; i++) {
        if (data[i] > maxVal) data[i] = maxVal;
        if ((int)(data[i] * sf) > 0)
            OLED_line(oled, x, y+h, x, (uint8_t)(y + h - (int)(data[i]*sf)), barWidth);
        x += barWidth;
    }
}

void OLED_barPlot(OLED_t *oled, uint8_t x, uint8_t y,
                  uint8_t w, uint8_t h, int *data, uint8_t size,
                  int maxVal)
{
    if (maxVal == 0) return;
    float sf = (float)h / maxVal;
    if (size > w) size = w;
    uint8_t step     = w / (size * 2);
    uint8_t barWidth = 3 + step;
    OLED_line(oled, x, y + h, x + w, y + h, 1);
    for (uint8_t i = 0; i < size && x <= w; i++) {
        if (data[i] > maxVal) data[i] = maxVal;
        if ((int)(data[i] * sf) > 0)
            OLED_line(oled, x, y+h, x, (uint8_t)(y + h - (int)(data[i]*sf)), barWidth);
        x += barWidth + step;
    }
}

/* ------------------------------------------------------------------ */
/*  Display control                                                     */
/* ------------------------------------------------------------------ */

void OLED_inflate(OLED_t *oled)
{
    s_displayBuffer(oled);
}

void OLED_clearScr(OLED_t *oled, bool refresh)
{
    if (oled->clearOnClr) s_execute(oled, OLED_OFF);
    s_clearBuffer(oled);
    if (refresh)          s_displayBuffer(oled);
    if (oled->clearOnClr) s_execute(oled, OLED_ON);
}

void OLED_clearArea(OLED_t *oled, uint8_t startX, uint8_t endX, uint8_t page)
{
    if (startX == endX || startX >= oled->width || endX >= oled->width
        || page >= oled->height / 8) return;
    uint8_t diff = (endX > startX) ? endX - startX : startX - endX;
    s_setPosition(oled, (endX > startX) ? startX : endX, page);
    for (uint8_t i = 0; i < diff; i++) s_sendData(oled, 0x00);
}

void OLED_setBrightness(OLED_t *oled, uint8_t brightness)
{
    s_execute(oled, OLED_CONTRAST);
    s_execute(oled, (uint8_t)((brightness * 255) / 100));
}

void OLED_invertDisplay(OLED_t *oled)
{
    s_execute(oled, oled->invertState ? OLED_REVERT : OLED_INVERT);
    oled->invertState ^= 1;
}

void OLED_entireDisplayON(OLED_t *oled)  { s_execute(oled, OLED_ENTIRE_DISP_ON);    }
void OLED_entireDisplayOFF(OLED_t *oled) { s_execute(oled, OLED_RESUME_FROM_VRAM);  }

void OLED_setPowerMode(OLED_t *oled, OLED_PowerMode mode)
{
    if (mode < OLED_LOW_POWER_MODE || mode > OLED_TURBO_MODE) return;
    oled->currentPowerMode = mode;
    switch (mode) {
    case OLED_TURBO_MODE:       /* fall-through – same register sequence as PERFORMANCE */
    case OLED_PERFORMANCE_MODE: s_performancePowerMode(oled); break;
    case OLED_BALANCED_MODE:    s_balancedPowerMode(oled);    break;
    case OLED_LOW_POWER_MODE:   s_lowPowerMode(oled);         break;
    }
    /* Note: I2C clock speed is set by CubeMX / MX_I2C_Init, not here */
}

void OLED_superBrightness(OLED_t *oled, bool enable)
{
    if (enable) {
        OLED_setBrightness(oled, 100);
        s_execute(oled, OLED_DISP_CLOCK_DIV); s_execute(oled, 0xF4);
        s_execute(oled, OLED_CHRG_PUMP);      s_execute(oled, OLED_CHRG_PUMP_75);
        s_execute(oled, OLED_PRE_CHRG);       s_execute(oled, OLED_PRE_CHRG_MAX);
        s_execute(oled, OLED_VCOMH_DESEL);    s_execute(oled, OLED_VCOMH_83);
    } else {
        OLED_setPowerMode(oled, (OLED_PowerMode)oled->currentPowerMode);
    }
}

void OLED_turnOffOnClr(OLED_t *oled, bool mode)  { oled->clearOnClr  = mode; }
void OLED_invertPixelState(OLED_t *oled, bool s)  { oled->pixelState  = s;    }

void OLED_setFont(OLED_t *oled, const uint8_t (*fontArray)[5])
{
    oled->fontSet   = fontArray;
    oled->fontWidth = 5;
}

void OLED_clearCustomFont(OLED_t *oled)
{
    oled->fontSet   = fonts;
    oled->fontWidth = 5;
}
