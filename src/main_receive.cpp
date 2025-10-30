/**
 * Environmental Sensor Node - STM32 + LoRa (RFM95/SX1276)
 *
 * Author: Efraim Manurung
 * Email: efraim.manurung@gmail.com
 *
 * Changelog:
 *
 * [2025-08-08]
 * - Implemented SX127x_Receive_Interrupt.ino from RadioLib library.
 *
 * [2025-08-06]
 * - First commit to make a program for receiving JSON data from the other node
 */

/* main header file for definitions and etc */
#include "main.h"

// flag to indicate that a packet was received
volatile bool received_flag = false;

void set_flag(void) {
  // we got a packet, set the flag
  received_flag = true;
}

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
  DEBUG_PRINTLN(F("[RFM95/SX1276] Initializing ... "));
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
    DEBUG_PRINT(state);
#endif
    while (true) {
      delay(10);
    }
  }

  // set the function that will be called
  // when new packet is received
  radio.setPacketReceivedAction(set_flag);

  // start listening for LoRa packets
  DEBUG_PRINTLN(F("[SX1278] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    DEBUG_PRINT(F("SUCCESS!"));
  } else {
    DEBUG_PRINT(F("failed, code "));
    DEBUG_PRINTLN(state);
    while (true) {
      delay(10);
    }
  }

  digitalWrite(NSS_RADIO, HIGH); // Disable RFM95

// Configure low power
#ifdef USE_LOW_POWER
  LowPower.begin();
#endif
}

void loop() {
  // check if the flag is set
  if (received_flag) {

    // reset flag
    received_flag = false;

#ifdef DEBUG_MAIN
    DEBUG_PRINT("[RFM95/SX1276] Waiting for incoming transmission ... ");
#endif

    // you can receive data as an Arduino String
    // NOTE: receive() is a blocking method!
    //       See example ReceiveInterrupt for details
    //       on non-blocking reception method.
    String str;
    digitalWrite(NSS_RADIO, LOW);
    int state = radio.readData(str);
    digitalWrite(NSS_RADIO, HIGH);

    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      DEBUG_PRINTLN("SUCCESS!");
      // print the data of the packet
      DEBUG_PRINT("[RFM95/SX1276] Data:\t\t\t");
      DEBUG_PRINTLN(str);

      // print the RSSI (Received Signal Strength Indicator)
      DEBUG_PRINT("[RFM95/SX1276] RSSI:\t\t\t");
      DEBUG_PRINT(radio.getRSSI());
      DEBUG_PRINTLN(" dBm");

      // print the SNR (Signal-to-Noise Ratio)
      DEBUG_PRINT("[RFM95/SX1276] SNR:\t\t\t");
      DEBUG_PRINT(radio.getSNR());
      DEBUG_PRINTLN(" dB");

      // print frequency error
      DEBUG_PRINT("[RFM95/SX1276] Frequency error:\t");
      DEBUG_PRINT(radio.getFrequencyError());
      DEBUG_PRINTLN(" Hz");

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      DEBUG_PRINTLN(F("[SX1278] CRC error!"));

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
  }
}