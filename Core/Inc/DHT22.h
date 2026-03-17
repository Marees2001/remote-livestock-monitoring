#ifndef DHT22_H
#define DHT22_H

/**
 * @file    DHT22.h
 * @brief   STM32 HAL driver for DHT22 (AM2302) Temperature & Humidity sensor.
 *
 * Wiring:
 *   DHT22 VCC  → 3.3V or 5V
 *   DHT22 DATA → Any GPIO pin (e.g. PA1)  +  4.7kΩ pull-up to VCC
 *   DHT22 GND  → GND
 *
 * Requirements:
 *   - A hardware timer running at 1 MHz (1 µs tick) must be passed in.
 *     Configure TIM2 (or any free timer) with:
 *       Prescaler     = (APB_CLK_MHz - 1)   e.g. 79 for 80 MHz → 1 µs tick
 *       Counter Period = 0xFFFF (free-running)
 *       No interrupts needed.
 *   - Call DHT22_init() once before reading.
 *   - Wait at least 2 seconds between reads (sensor limitation).
 */

#include "stm32f4xx_hal.h"   /* Change to match your MCU family */
#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/*  Status codes                                                        */
/* ------------------------------------------------------------------ */
typedef enum {
    DHT22_OK            =  0,   /**< Read successful                   */
    DHT22_ERR_TIMEOUT   = -1,   /**< No response / line stuck          */
    DHT22_ERR_CHECKSUM  = -2,   /**< Data corrupted                    */
    DHT22_ERR_NOT_READY = -3,   /**< Called too soon after last read   */
} DHT22_Status;

/* ------------------------------------------------------------------ */
/*  Handle struct                                                       */
/* ------------------------------------------------------------------ */
typedef struct {
    GPIO_TypeDef     *port;         /**< GPIO port  (e.g. GPIOA)       */
    uint16_t          pin;          /**< GPIO pin   (e.g. GPIO_PIN_1)  */
    TIM_HandleTypeDef *htim;        /**< 1 MHz timer handle            */
    float             temperature;  /**< Last read temperature (°C)    */
    float             humidity;     /**< Last read humidity    (%)     */
    uint32_t          lastReadTick; /**< HAL_GetTick() of last read    */
} DHT22_t;

/* ------------------------------------------------------------------ */
/*  API                                                                 */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialise the DHT22 handle.
 * @param  dev    Pointer to a DHT22_t struct.
 * @param  port   GPIO port the DATA pin is connected to.
 * @param  pin    GPIO pin  (e.g. GPIO_PIN_1).
 * @param  htim   Pointer to a TIM handle configured at 1 MHz.
 */
void DHT22_init(DHT22_t *dev, GPIO_TypeDef *port, uint16_t pin,
                TIM_HandleTypeDef *htim);

/**
 * @brief  Read temperature and humidity from the sensor.
 *
 * On success, dev->temperature and dev->humidity are updated.
 * Enforces a minimum 2-second interval between reads automatically.
 *
 * @param  dev  Pointer to an initialised DHT22_t struct.
 * @retval DHT22_OK, DHT22_ERR_TIMEOUT, DHT22_ERR_CHECKSUM,
 *         or DHT22_ERR_NOT_READY.
 */
DHT22_Status DHT22_read(DHT22_t *dev);

/**
 * @brief  Get the last successfully read temperature in Fahrenheit.
 * @param  dev  Pointer to an initialised DHT22_t struct.
 * @retval Temperature in °F.
 */
float DHT22_getTempF(DHT22_t *dev);

#endif /* DHT22_H */
