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
 * [2025-09-02]
 * - Try to reduce FLASH size (please see the main.h)
 *
 * [2025-08-08]
 * - Implemented SX127x_Transmit_Interrupt.ino from the RadioLib library, as we
 * have to avoid blocking transmit.
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

/* main header file for definitions and etc */
#include "main.h"

// include for internal voltage reference
#include "STM32IntRef.h"

/* include for BME280 library */
#include "BME280.h"

#define NSS_BME PA1

/* A BME280 object using SPI, chip select pin PA1 */
BME280 bme(SPI, NSS_BME);

// save transmission state between loops
int transmission_state = RADIOLIB_ERR_NONE;

// flag to indicate that a packet was sent
volatile bool transmitted_flag = true;

void set_flag(void) {
  // we sent a packet, set the flag
  transmitted_flag = true;
}

bool initializeBME280() {

  /* set forced mode to control the NSS pin */
  bme.setForcedMode();

  if (bme.begin() < 0) {
#ifdef DEBUG_MAIN
    DEBUG_PRINTLN(
        "Error communicating with BME280 sensor, please check wiring");
#endif
    return false;
  }
#ifdef DEBUG_MAIN
  DEBUG_PRINTLN("[BME280] Initialized ");
#endif
  return true;
}

void setup() {
  /* Setup serial debug */
#ifdef DEBUG_MAIN
  DEBUG_BEGIN(9600);
#endif

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
      radio.begin(915.0, 125.0, 9, 7, RADIOLIB_SX127X_SYNC_WORD, 17, 8, 0);
  if (state == RADIOLIB_ERR_NONE) {
#ifdef DEBUG_MAIN
    DEBUG_PRINTLN("[RFM95/SX1276] Initialized");
#endif
  } else {

#ifdef DEBUG_MAIN
    DEBUG_PRINT("[RFM95/SX1276] failed, code ");
    DEBUG_PRINTLN(state);
#endif
    while (true) {
      delay(10);
    }
  }

  // set the function that will be called
  // when packet transmission is finished
  radio.setPacketSentAction(set_flag);

  digitalWrite(NSS_RADIO, HIGH); // Disable RFM95

// Configure low power
#ifdef USE_LOW_POWER
  LowPower.begin();
#endif
}

void loop() {

  // check if the previous transmission finished
  if (transmitted_flag) {

    // reset flag
    transmitted_flag = false;

    if (transmission_state == RADIOLIB_ERR_NONE) {
// packet was successfully sent
#ifdef DEBUG_MAIN
      DEBUG_PRINTLN("PACKET SUCCESSFULLY TRANSMITTED!");
#endif

      // NOTE: when using interrupt-driven transmit method,
      //       it is not possible to automatically measure
      //       transmission data rate using getDataRate()

    } else {
#ifdef DEBUG_MAIN
      DEBUG_PRINT(F("failed, code "));
      DEBUG_PRINTLN(transmission_state);
#endif
    }

    // clean up after transmission is finished
    // this will ensure transmitter is disabled,
    // RF switch is powered down etc.
    radio.finishTransmit();

    // wait before transmitting again
#ifdef USE_LOW_POWER
    LowPower.deepSleep(SLEEP_INTERVAL);
#else
    delay(SLEEP_INTERVAL);
#endif

    // send another one
#ifdef DEBUG_MAIN
    DEBUG_PRINTLN(F("[SX1278] Sending another packet ... "));
#endif

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

/* uncomment to debug */
#ifdef DEBUG_MAIN
    DEBUG_PRINT("Temperature: ");
    DEBUG_PRINTLN(tempInt);
    DEBUG_PRINT("Humidity: ");
    DEBUG_PRINTLN(humInt);
    DEBUG_PRINT("Pressure: ");
    DEBUG_PRINTLN(pressInt);
#endif

    // set forced mode to be shure it will use minimal power and send it to
    // sleep bme.setForcedMode(); // moved in the setup
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
    transmission_state = radio.startTransmit(str.c_str());
    radio.sleep();
    digitalWrite(NSS_RADIO, HIGH);
  }
}