/* uart_handler.h */
#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include "stm32f1xx_hal.h"

void UART_SendString(UART_HandleTypeDef *huart, const char *str);
void UART_SendTemperature(UART_HandleTypeDef *huart,
                          uint32_t elapsed_ms,
                          float    temperature_C,
                          uint32_t adc_raw,
                          float    voltage_V);

#endif /* UART_HANDLER_H */
