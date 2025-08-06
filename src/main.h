#include <Arduino.h>

/* MiniPill LoRa v1.x mapping - LoRa module RFM95W.

Source: https://www.iot-lab.org/blog/370/

 PA1  //            NSS           - BME280
 PA4  // SPI1_NSS   NSS  - RFM95W
 PA5  // SPI1_SCK   SCK  - RFM95W - BME280
 PA6  // SPI1_MISO  MISO - RFM95W - BME280
 PA7  // SPI1_MOSI  MOSI - RFM95W - BME280

 PA10 // USART1_RX  DIO0 - RFM95W
 PB4  //            DIO1 - RFM95W
 PB5  //            DIO2 - RFM95W

 PA9  // USART1_TX  RST  - RFM95W

 VCC - 3V3
 GND - GND
*/

// include the library for RadioLib
#include <RadioLib.h>

#ifdef USE_LOW_POWER
#include "STM32LowPower.h"
#endif

/* SX1278/RFM95 in the MiniPill LoRa has the following connections: */
#define NSS_RADIO PA4
#define DIO0 PA10
#define RST PA9
#define DIO1 PB4

SX1276 lora_rfm95 = new Module(NSS_RADIO, DIO0, RST, DIO1);

#ifdef DEBUG_MAIN
// Redirect debug output to Serial2 (Tx on PA2)
HardwareSerial Serial2(USART2); // or HardwareSerial Serial2(PA3, PA2)
#define DEBUG_BEGIN(...) Serial2.begin(__VA_ARGS__)
#define DEBUG_PRINT(...) Serial2.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) Serial2.println(__VA_ARGS__)
#else
#define DEBUG_BEGIN(...)
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#endif

// include for internal voltage reference
#include "STM32IntRef.h"

#ifdef USE_LOW_POWER_CAL
#include "STM32LowPowerCal.h"

const double calTimeDivider = 7980.0;
#endif

// Sleep this many microseconds. Notice that the sending and waiting for
// downlink will extend the time between send packets. You have to extract this
// time
#define SLEEP_INTERVAL 10000