/* ============================================================
 * uart_handler.c
 * Formatted UART serial output for temperature data
 * ============================================================ */

#include "uart_handler.h"
#include <stdio.h>
#include <string.h>

static char tx_buf[128];

void UART_SendString(UART_HandleTypeDef *huart, const char *str)
{
    HAL_UART_Transmit(huart, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

/* Output format:
 * [T=0.500s] Temp: 24.87 C | ADC_raw: 1234 | V_in: 2.493V
 */
void UART_SendTemperature(UART_HandleTypeDef *huart,
                          uint32_t elapsed_ms,
                          float    temperature_C,
                          uint32_t adc_raw,
                          float    voltage_V)
{
    int len = snprintf(tx_buf, sizeof(tx_buf),
        "[T=%lu.%03lus] Temp: %.2f C | ADC_raw: %lu | V_in: %.3fV\r\n",
        (unsigned long)(elapsed_ms / 1000),
        (unsigned long)(elapsed_ms % 1000),
        (double)temperature_C,
        (unsigned long)adc_raw,
        (double)voltage_V);

    if (len > 0)
        HAL_UART_Transmit(huart, (uint8_t*)tx_buf, (uint16_t)len, HAL_MAX_DELAY);
}
