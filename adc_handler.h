/* adc_handler.h */
#ifndef ADC_HANDLER_H
#define ADC_HANDLER_H

#include "stm32f1xx_hal.h"

void     ADC_StartDMA(ADC_HandleTypeDef *hadc, DMA_HandleTypeDef *hdma);
uint32_t ADC_GetAverage(void);
float    ADC_ToVoltage(uint32_t adc_count);
float    ADC_ToTemperature(float adc_voltage_V);

#endif /* ADC_HANDLER_H */
