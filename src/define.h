#ifndef DEFINE_H
#define DEFINE_H

#define FIRMWARE_VERSION 110
// USART1 for GSM module (PA2=TX, PA3=RX)
#define GSM_SERIAL Serial2 
#define SCREEN_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

//Define for TDS
#define VREF 3.3          // STM32 ADC reference voltage
#define ADC_RESOLUTION 4095.0  // 12-bit ADC
//Define for TH
#define DHTTYPE DHT22

#define STATUS_LED_Pin PC13
#define RELAY_FENCE_Pin PA5
#define RELAY_LAMP_Pin PA6
#define RELAY_FAN_Pin PA7
#define TDS_SENSOR_Pin PB0
#define TH_SENSOR_Pin PB1
#define GSM_RX_Pin PA3
#define GSM_TX_Pin PA2
#define MCU_SCL_Pin PB6
#define MCU_SDA_Pin PB7

#define NONE 0
#define DASHBOARD 1
#define SETINGS 1
#define ADD_CONTACT 3
#define SET_SENSOR 4
#define SET_TH 5
#define SET_TDS 6


// Bit manipulation macros
#define BITREAD(value, bit) ((value >> bit) & 0x01) // Macro to read a specific bit from a value
#define BITSET(value, bit) (value |= (1 << bit)) // Macro to set a specific bit in a value
#define BITCLEAR(value, bit) (value &= ~(1 << bit)) // Macro to clear a specific bit in a value
#define BITTOGGLE(value, bit) (value ^= (1 << bit)) // Macro to toggle

#endif // DEFINE_H