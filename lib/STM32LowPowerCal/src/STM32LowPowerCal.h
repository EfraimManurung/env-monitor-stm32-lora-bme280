/*
@author Leo Korbee
@file   STM32LowPowerCal.h
@brief  Correction for usign LowPower in sleep modes with no crystal


*/

#include "STM32LowPower.h"

#ifndef _STM32_LOW_POWER_CAL_H_
#define _STM32_LOW_POWER_CAL_H_

class STM32LowPowerCal : public STM32LowPower {

public:
  void deepSleep(uint32_t ms, int correction_Time);
  void deepSleep(int ms, int correction_Time) {
    deepSleep((uint32_t)ms, correction_Time);
  }

  void setRTCCalibrationTime(float calibration_Time);
  void calibrateRTC();
  float getRTCTimeCorrection();

private:
  // declare timing parameters
  float rtc_Time_Correction_Factor;
  float rct_Calibration_Time; // time it should be.
};

extern STM32LowPowerCal LowPowerCal;

#endif // _STM32_LOW_POWER_H_
