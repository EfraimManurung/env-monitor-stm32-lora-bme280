
#ifndef _STM32_INT_REF_H_
#define _STM32_INT_REF_H_

#include <Arduino.h>
#include "stm32yyxx_ll_adc.h"


/* Values available in datasheet */
#define CALX_TEMP 25

#if defined(STM32F1xx)
#define V25       1430
#define AVG_SLOPE 4300
#define VREFINT   1200
#elif defined(STM32F2xx)
#define V25       760
#define AVG_SLOPE 2500
#define VREFINT   1210
#endif

/* Analog read resolution */
#if ADC_RESOLUTION == 10
#define LL_ADC_RESOLUTION LL_ADC_RESOLUTION_10B
#define ADC_RANGE 1024
#else
#define LL_ADC_RESOLUTION LL_ADC_RESOLUTION_12B
#define ADC_RANGE 4096
#endif

class STM32IntRef {
  public:
    STM32IntRef();
    int32_t readVref();
    int32_t readTempSensor(int32_t VRef);
    int32_t readVoltage(int32_t VRef, uint32_t pin);
  private:
};

// create IntRef object
extern STM32IntRef IntRef;

#endif // _STM32_INT_REFS_H_
