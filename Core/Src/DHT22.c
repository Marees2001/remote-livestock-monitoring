/**
 * @file    DHT22.c
 * @brief   STM32 HAL driver for DHT22 (AM2302) Temperature & Humidity sensor.
 *
 * Protocol (single-bus, open-drain):
 *  1. Host pulls DATA low for ≥ 1 ms  (start signal)
 *  2. Host releases line (pull-up takes it high)
 *  3. DHT22 pulls low 80 µs, then high 80 µs  (ACK)
 *  4. DHT22 sends 40 bits:
 *       bit '0' → 50 µs low + ~26–28 µs high
 *       bit '1' → 50 µs low + ~70 µs  high
 *  5. DHT22 pulls low 50 µs (end signal)
 *
 *  Data layout (40 bits = 5 bytes):
 *    Byte 0-1 : Humidity    × 10  (16-bit, big-endian)
 *    Byte 2-3 : Temperature × 10  (16-bit, big-endian, bit15 = sign)
 *    Byte 4   : Checksum (sum of bytes 0-3, low 8 bits)
 */

#include "DHT22.h"

/* ------------------------------------------------------------------ */
/*  Timing constants (µs)                                               */
/* ------------------------------------------------------------------ */
#define DHT22_START_LOW_US      1200u   /* Host pulls low (spec: ≥1ms) */
#define DHT22_START_RELEASE_US    40u   /* Host releases before sampling*/
#define DHT22_RESPONSE_WAIT_US   100u   /* Max wait for sensor ACK low  */
#define DHT22_BIT_TIMEOUT_US     100u   /* Max wait for any edge        */
#define DHT22_BIT_THRESHOLD_US    40u   /* >40 µs high → bit '1'        */
#define DHT22_MIN_INTERVAL_MS   2000u   /* Minimum ms between reads     */

/* ------------------------------------------------------------------ */
/*  µs delay using the 1 MHz timer                                      */
/* ------------------------------------------------------------------ */
static void delay_us(DHT22_t *dev, uint32_t us)
{
    __HAL_TIM_SET_COUNTER(dev->htim, 0);
    while (__HAL_TIM_GET_COUNTER(dev->htim) < us);
}

/* ------------------------------------------------------------------ */
/*  GPIO helpers                                                        */
/* ------------------------------------------------------------------ */

/** Set DATA pin as push-pull output. */
static void pin_output(DHT22_t *dev)
{
    GPIO_InitTypeDef cfg = {0};
    cfg.Pin   = dev->pin;
    cfg.Mode  = GPIO_MODE_OUTPUT_PP;
    cfg.Speed = GPIO_SPEED_FREQ_HIGH;
    cfg.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(dev->port, &cfg);
}

/** Set DATA pin as floating input (external pull-up on hardware). */
static void pin_input(DHT22_t *dev)
{
    GPIO_InitTypeDef cfg = {0};
    cfg.Pin  = dev->pin;
    cfg.Mode = GPIO_MODE_INPUT;
    cfg.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(dev->port, &cfg);
}

static inline void pin_high(DHT22_t *dev)
{
    HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_SET);
}

static inline void pin_low(DHT22_t *dev)
{
    HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_RESET);
}

static inline uint8_t pin_read(DHT22_t *dev)
{
    return (uint8_t)HAL_GPIO_ReadPin(dev->port, dev->pin);
}

/* ------------------------------------------------------------------ */
/*  Wait for a specific pin level, return elapsed µs or 0 on timeout   */
/* ------------------------------------------------------------------ */
static uint32_t wait_for_level(DHT22_t *dev, uint8_t level, uint32_t timeout_us)
{
    __HAL_TIM_SET_COUNTER(dev->htim, 0);
    while (pin_read(dev) != level) {
        if (__HAL_TIM_GET_COUNTER(dev->htim) >= timeout_us)
            return 0;   /* timeout */
    }
    return __HAL_TIM_GET_COUNTER(dev->htim);
}

/* ================================================================== */
/*  Public API                                                          */
/* ================================================================== */

void DHT22_init(DHT22_t *dev, GPIO_TypeDef *port, uint16_t pin,
                TIM_HandleTypeDef *htim)
{
    dev->port         = port;
    dev->pin          = pin;
    dev->htim         = htim;
    dev->temperature  = 0.0f;
    dev->humidity     = 0.0f;
    dev->lastReadTick = 0;

    /* Start the timer (CubeMX may have already done this) */
    HAL_TIM_Base_Start(dev->htim);

    /* Idle state: pin high */
    pin_output(dev);
    pin_high(dev);
}

DHT22_Status DHT22_read(DHT22_t *dev)
{
    /* ---- Enforce minimum read interval ---- */
    uint32_t now = HAL_GetTick();
    if (dev->lastReadTick != 0 &&
        (now - dev->lastReadTick) < DHT22_MIN_INTERVAL_MS)
        return DHT22_ERR_NOT_READY;

    uint8_t  raw[5] = {0};
    uint8_t  bits[40];

    /* ============================================================== */
    /*  Step 1: Send start signal                                       */
    /* ============================================================== */
    pin_output(dev);
    pin_low(dev);
    delay_us(dev, DHT22_START_LOW_US);
    pin_high(dev);
    delay_us(dev, DHT22_START_RELEASE_US);
    pin_input(dev);

    /* ============================================================== */
    /*  Step 2: Wait for sensor ACK (low → high → low transitions)     */
    /* ============================================================== */
    /* Sensor pulls low ~80 µs */
    if (!wait_for_level(dev, 0, DHT22_RESPONSE_WAIT_US))
        return DHT22_ERR_TIMEOUT;

    /* Sensor releases high ~80 µs */
    if (!wait_for_level(dev, 1, DHT22_RESPONSE_WAIT_US))
        return DHT22_ERR_TIMEOUT;

    /* Sensor pulls low again → data transmission starts */
    if (!wait_for_level(dev, 0, DHT22_RESPONSE_WAIT_US))
        return DHT22_ERR_TIMEOUT;

    /* ============================================================== */
    /*  Step 3: Read 40 bits                                            */
    /* ============================================================== */
    for (uint8_t i = 0; i < 40; i++)
    {
        /* Each bit starts with a ~50 µs low period */
        if (!wait_for_level(dev, 1, DHT22_BIT_TIMEOUT_US))
            return DHT22_ERR_TIMEOUT;

        /* Measure the HIGH duration to distinguish 0 from 1 */
        __HAL_TIM_SET_COUNTER(dev->htim, 0);

        if (!wait_for_level(dev, 0, DHT22_BIT_TIMEOUT_US))
            return DHT22_ERR_TIMEOUT;

        bits[i] = (__HAL_TIM_GET_COUNTER(dev->htim) > DHT22_BIT_THRESHOLD_US) ? 1 : 0;
    }

    /* ============================================================== */
    /*  Step 4: Pack bits into 5 bytes                                  */
    /* ============================================================== */
    for (uint8_t byte = 0; byte < 5; byte++) {
        raw[byte] = 0;
        for (uint8_t bit = 0; bit < 8; bit++)
            raw[byte] = (raw[byte] << 1) | bits[byte * 8 + bit];
    }

    /* ============================================================== */
    /*  Step 5: Verify checksum                                         */
    /* ============================================================== */
    uint8_t checksum = (uint8_t)((raw[0] + raw[1] + raw[2] + raw[3]) & 0xFF);
    if (checksum != raw[4])
        return DHT22_ERR_CHECKSUM;

    /* ============================================================== */
    /*  Step 6: Decode humidity and temperature                         */
    /* ============================================================== */

    /* Humidity: bytes 0-1, value × 10 */
    uint16_t rh_raw = ((uint16_t)raw[0] << 8) | raw[1];
    dev->humidity   = rh_raw / 10.0f;

    /* Temperature: bytes 2-3, value × 10, bit15 = sign */
    uint16_t t_raw  = ((uint16_t)(raw[2] & 0x7F) << 8) | raw[3];
    dev->temperature = t_raw / 10.0f;
    if (raw[2] & 0x80)            /* negative temperature */
        dev->temperature = -dev->temperature;

    /* Idle the line high */
    pin_output(dev);
    pin_high(dev);

    dev->lastReadTick = HAL_GetTick();
    return DHT22_OK;
}

float DHT22_getTempF(DHT22_t *dev)
{
    return (dev->temperature * 9.0f / 5.0f) + 32.0f;
}
