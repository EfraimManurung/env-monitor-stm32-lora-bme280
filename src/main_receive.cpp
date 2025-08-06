/**
 * Environmental Sensor Node - STM32 + LoRa (RFM95/SX1276)
 *
 * Author: Efraim Manurung
 * Email: efraim.manurung@gmail.com
 *
 * Changelog:
 *
 * [2025-08-06]
 * - First commit to make a program for receiving JSON data from the other node
 */

/* main header file for definitions and etc */
#include "main.h"

void setup() {
  /* Setup serial debug */
  DEBUG_BEGIN(9600);

  pinMode(NSS_RADIO, OUTPUT);

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

#ifdef DEBUG_MAIN
  DEBUG_PRINTLN("[RFM95/SX1276] Waiting for incoming transmission ... ");
#endif

  // you can receive data as an Arduino String
  // NOTE: receive() is a blocking method!
  //       See example ReceiveInterrupt for details
  //       on non-blocking reception method.
  String str;
  digitalWrite(NSS_RADIO, LOW);
  int state = lora_rfm95.receive(str);
  digitalWrite(NSS_RADIO, HIGH);

  if (state == RADIOLIB_ERR_NONE) {
    // packet was successfully received
    DEBUG_PRINTLN("success!");
    // print the data of the packet
    DEBUG_PRINT("[RFM95/SX1276] Data:\t\t\t");
    DEBUG_PRINTLN(str);

    // print the RSSI (Received Signal Strength Indicator)
    // of the last received packet
    DEBUG_PRINT("[RFM95/SX1276] RSSI:\t\t\t");
    DEBUG_PRINT(lora_rfm95.getRSSI());
    DEBUG_PRINTLN(" dBm");

    // print the SNR (Signal-to-Noise Ratio)
    // of the last received packet
    DEBUG_PRINT("[RFM95/SX1276] SNR:\t\t\t");
    DEBUG_PRINT(lora_rfm95.getSNR());
    DEBUG_PRINTLN(" dB");

    // print frequency error
    // of the last received packet
    DEBUG_PRINT("[RFM95/SX1276] Frequency error:\t");
    DEBUG_PRINT(lora_rfm95.getFrequencyError());
    DEBUG_PRINTLN(" Hz");

  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    // timeout occurred while waiting for a packet
    DEBUG_PRINTLN("timeout!");

  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    DEBUG_PRINTLN("CRC error!");

  } else {
    // some other error occurred
    DEBUG_PRINT("failed, code ");
    DEBUG_PRINTLN(state);
  }

  /* so we can read the values more easy */
#ifdef USE_LOW_POWER
  LowPower.deepSleep(SLEEP_INTERVAL);
#else
  delay(SLEEP_INTERVAL);
#endif
}