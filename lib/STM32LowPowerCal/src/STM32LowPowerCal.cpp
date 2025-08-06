/*
@author Leo Korbee
@file   STM32LowPowerCal.cpp
@brief  Wrapper for LowPower - Correction for using LowPower in sleep modes with no crystal
*/

#include "STM32LowPower.h"
#include "STM32LowPowerCal.h"

// correction times for time used by powerdown and wakeup.
void STM32LowPowerCal::deepSleep(uint32_t ms, int correction_Time)
{
    int correction_time = round((float) ms * LowPowerCal.rtc_Time_Correction_Factor) - correction_Time;
      // call superclass instance deepsleep methode
    LowPower.deepSleep(correction_time);
}

float STM32LowPowerCal::getRTCTimeCorrection()
{
    return rtc_Time_Correction_Factor;
}

void STM32LowPowerCal::setRTCCalibrationTime(float calibration_Time)
{
    LowPowerCal.rct_Calibration_Time = calibration_Time;
}

void STM32LowPowerCal::calibrateRTC()
{
    // Get the rtc object
    STM32RTC& rtc = STM32RTC::getInstance();
    // intiate rtc timer
    rtc.begin(); 
    rtc.setTime(16, 0, 0);
    // delay 8 seconds using systick timer (HSI clock)
    delay(8000);
    // get time from RTC (LSI clock)
    int rtctime = rtc.getSeconds() * 1000 + rtc.getSubSeconds();
    // calculate time correction factor
    rtc_Time_Correction_Factor = (float) rtctime / (float) rct_Calibration_Time;
}

STM32LowPowerCal LowPowerCal;