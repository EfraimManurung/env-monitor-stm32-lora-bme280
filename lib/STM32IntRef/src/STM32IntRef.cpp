/*
  STM32IntRef.cpp
  Code to use interal reference data
  @author  Leo Korbee (c), Leo.Korbee@xs4all.nl

  @version 2020-08-07
  A first version created form code found on the internet
  Must still be cleaned
*/
#include "STM32IntRef.h"

STM32IntRef IntRef;

/**
  * constructor
  */
STM32IntRef::STM32IntRef()
{
  analogReadResolution(ADC_RESOLUTION);
}

/**
  * @brief  Read VCC voltage to VRef
  * @param  None
  * @retval None
  */
int32_t STM32IntRef::readVref()
{
#ifdef __LL_ADC_CALC_VREFANALOG_VOLTAGE
  return (__LL_ADC_CALC_VREFANALOG_VOLTAGE(analogRead(AVREF), LL_ADC_RESOLUTION));
#else
  return (VREFINT * ADC_RANGE / analogRead(AVREF)); // ADC sample to mV
#endif
}

#ifdef ATEMP
int32_t STM32IntRef::readTempSensor(int32_t VRef)
{
#ifdef __LL_ADC_CALC_TEMPERATURE
  return (__LL_ADC_CALC_TEMPERATURE(VRef, analogRead(ATEMP), LL_ADC_RESOLUTION));
#elif defined(__LL_ADC_CALC_TEMPERATURE_TYP_PARAMS)
  return (__LL_ADC_CALC_TEMPERATURE_TYP_PARAMS(AVG_SLOPE, V25, CALX_TEMP, VRef, analogRead(ATEMP), LL_ADC_RESOLUTION));
#else
  return 0;
#endif
}
#endif

int32_t STM32IntRef::readVoltage(int32_t VRef, uint32_t pin)
{
  return (__LL_ADC_CALC_DATA_TO_VOLTAGE(VRef, analogRead(pin), LL_ADC_RESOLUTION));
}
