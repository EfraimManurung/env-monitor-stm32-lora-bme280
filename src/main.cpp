/**
 * Environmental Sensor Node - STM32 + BME280 + LoRa (RFM95/SX1276)
 *
 * Author: Efraim Manurung
 * Email: efraim.manurung@gmail.com
 *
 * Description:
 * This project implements a low-power environmental sensor node using the
 * BME280 for temperature and humidity measurement, and the SX1276 (RFM95) LoRa
 * module for wireless data transmission. It is designed for remote weather or
 * greenhouse monitoring applications.
 *
 * Changelog:
 *
 * [2025-08-06]
 * - Added low-power sleep capability for energy-efficient operation.
 * - RTC timing correction for Low Power mode from this example and source code
 * of
 *    https://gitlab.com/iot-lab-org/minipill-lora-psll-bme280-example-project/-/blob/main/src/main.cpp?ref_type=heads
 *
 * [2025-08-05]
 * - Implemented NSS pin switching between BME280 and SX1276 (PA1 and PA4).
 * - Used `setForcedMode()` from BME280 driver to manage shared SPI access.
 *
 * [2025-08-04]
 * - Integrated RadioLib with SX1276 using the Ping-Pong example:
 *   https://github.com/jgromes/RadioLib/blob/master/examples/SX127x/SX127x_PingPong/SX127x_PingPong.ino
 *
 * [2025-08-02]
 * - Successfully interfaced BME280 and printed sensor data via Serial.
 *
 * [2025-07-31]
 * - Initial board test with custom STM32 MiniPill LoRa.
 * - Verified serial output and debugging capability.
 */

/* MiniPill LoRa v1.x mapping - LoRa module RFM95W and BME280 sensor

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

#include <Arduino.h>

/* include for BME280 library */
#include "BME280.h"

#define NSS_BME PA1

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
#endif

/* A BME280 object using SPI, chip select pin PA1 */
BME280 bme(SPI, NSS_BME);

// Sleep this many microseconds. Notice that the sending and waiting for
// downlink will extend the time between send packets. You have to extract this
// time
#define SLEEP_INTERVAL 10000

const double calTimeDivider = 7980.0;

bool initializeBME280() {

  /* set forced mode to control the NSS pin */
  bme.setForcedMode();

  if (bme.begin() < 0) {
    DEBUG_PRINTLN(
        "Error communicating with BME280 sensor, please check wiring");
    return false;
  }

  DEBUG_PRINTLN("[BME280] Initialized ");
  return true;
}

void setup() {
  /* Setup serial debug */
  DEBUG_BEGIN(9600);

  pinMode(NSS_RADIO, OUTPUT);
  digitalWrite(NSS_RADIO, HIGH);

  /* Time for serial settings */
  delay(1000);

#ifdef USE_LOW_POWER_CAL
  /************************************************
   * get TimeCorrection number for RTC times
   ************************************************/
  // wait 8 seconds to upload new code and
  // calibrate the RTC times with the HSI internal clock
  // time calibration on device with correct timing (ideal 8000.00)
  // incease this time to shorten time between send and receive
  LowPowerCal.setRTCCalibrationTime(calTimeDivider);
  // callibrate for 8 seconds
  LowPowerCal.calibrateRTC();
  DEBUG_PRINT("RTC Time Correction: ");
  DEBUG_PRINTLN(LowPowerCal.getRTCTimeCorrection(), 5);
#endif

/* Begin communication with BME280 and set to default sampling, iirc, and
 * standby settings */
#ifdef DEBUG_MAIN
  DEBUG_PRINTLN("[BME280] Initializing ... ");
#endif
  if (!initializeBME280()) {
    // Optional: Blink LED or show error
    while (true)
      ; // Halt
  }

  // initialize SX1278 with default settings
#ifdef DEBUG_MAIN
  DEBUG_PRINTLN("[RFM95/SX1276] Initializing ... ");
#endif
  digitalWrite(NSS_RADIO, LOW); // Enable RFM95
  int state =
      lora_rfm95.begin(915.0, 125.0, 9, 7, RADIOLIB_SX127X_SYNC_WORD, 17, 8, 0);
  if (state == RADIOLIB_ERR_NONE) {
#ifdef DEBUG_MAIN
    DEBUG_PRINTLN("[RFM95/SX1276] Initialized");
#endif
  } else {

#ifdef DEBUG_MAIN
    DEBUG_PRINT("[RFM95/SX1276] failed, code ");
    DEBUG_PRINTLN(state);
#endif
    while (true)
      ; // Halt
  }
  digitalWrite(NSS_RADIO, HIGH); // Disable RFM95

// Configure low power
#ifdef USE_LOW_POWER
  LowPower.begin();
#endif
}

void loop() {
  // Prepare upstream data transmission at the next possible time.
  // read vcc and add to bytebuffer
  int32_t vcc = IntRef.readVref();

  // reading data from BME sensor
  digitalWrite(NSS_RADIO, HIGH);
  bme.readSensor();
  float tempFloat = bme.getTemperature_C();
  float humFloat = bme.getHumidity_RH();
  float pressFloat = bme.getPressure_Pa();

  // from float to uint16_t
  uint16_t tempInt = 100 * tempFloat;
  uint16_t humInt = 100 * humFloat;
  // pressure is already given in 100 x mBar = hPa
  uint16_t pressInt = pressFloat / 10;

#ifdef DEBUG_MAIN
  DEBUG_PRINT("Temperature: ");
  DEBUG_PRINTLN(tempInt);
  DEBUG_PRINT("Humidity: ");
  DEBUG_PRINTLN(humInt);
  DEBUG_PRINT("Pressure: ");
  DEBUG_PRINTLN(pressInt);
#endif

  // set forced mode to be shure it will use minimal power and send it to sleep
  // bme.setForcedMode(); // moved in the setup
  bme.goToSleep();

  String clientId = "NS001";
  // String str = String(t) + "," + String(h) + "," + String(p) + "," +
  // String(a);
  String str = String("[{\"h\":") + humInt + ",\"t\":" + tempInt +
               ",\"p\":" + pressInt + ",\"vcc\":" + vcc + "},";
  str += String("{\"node\":\"") + clientId + "}]";

#ifdef DEBUG_MAIN
  DEBUG_PRINT("JSON PAYLOAD: ");
  DEBUG_PRINTLN(str);
#endif

  digitalWrite(NSS_RADIO, LOW);
  int state = lora_rfm95.transmit(str.c_str());
  digitalWrite(NSS_RADIO, HIGH);

  if (state == RADIOLIB_ERR_NONE) {
    // the packet was successfully transmitted

#ifdef DEBUG_MAIN
    DEBUG_PRINTLN("PACKET SUCCESSFULLY TRANSMITTED!");
#endif
  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    // timeout occurred while transmitting packet
  } else {
    // some other error occurred
  }

  /* so we can read the values more easy */
#ifdef USE_LOW_POWER
  LowPower.deepSleep(SLEEP_INTERVAL);
#else
  delay(SLEEP_INTERVAL);
#endif
}