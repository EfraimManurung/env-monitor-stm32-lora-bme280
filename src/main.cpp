/**
 * Environmental Sensor Node using BME280 and LoRa RFM95
 * Author: Efraim Manurung
 * efraim.manurung@gmail.com
 *
 * 05-08-2025
 *  - Add NSS control between BME280 and SX1276/RFM95, the pins are PA1 and PA4.
 *  - Use setForcedMode() from BME280 class to control NSS pin.
 *
 * 04-08-2025
 *  - Implement RadioLib SX1276/RFM95, with this Ping-Pong example
 *     https://github.com/jgromes/RadioLib/blob/master/examples/SX127x/SX127x_PingPong/SX127x_PingPong.ino.
 *
 * 02-08-2025
 *  - Run BME280.
 *  - Print the output in the serial.
 *
 * 31-07-2025
 * Tested and running with the custom board:
 *  - We can debug and print the program.
 *
 *  */

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

/* SX1278/RFM95 in the MiniPill LoRa has the following connections: */
#define NSS_RADIO PA4
#define DIO0 PA10
#define RST PA9
#define DIO1 PB4

SX1276 lora_rfm95 = new Module(NSS_RADIO, DIO0, RST, DIO1);

/* uncomment for debugging main */
#ifdef DEBUG_MAIN
// for debugging redirect to hardware Serial2
// Tx on PA2
#define DEBUG_INIT(...)                                                        \
  HardwareSerial Serial2(USART2) // or HardWareSerial Serial2(PA3, PA2);
#define DEBUG_BEGIN(...) Serial2.begin(9600)
#define DEBUG_PRINT(...) Serial2.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) Serial2.println(__VA_ARGS__)
DEBUG_INIT();
#else
#define DEBUG_BEGIN(...)
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#endif

// include for internal voltage reference
#include "STM32IntRef.h"

/* A BME280 object using SPI, chip select pin PA1 */
BME280 bme(SPI, NSS_BME);

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
  DEBUG_BEGIN();

  pinMode(NSS_RADIO, OUTPUT);
  digitalWrite(NSS_RADIO, HIGH);

  /* Time for serial settings */
  delay(1000);

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
  delay(1000);
}