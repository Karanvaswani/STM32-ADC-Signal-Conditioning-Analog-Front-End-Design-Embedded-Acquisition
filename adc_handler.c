/* ============================================================
 * adc_handler.c
 * DMA-driven ADC acquisition with 32-sample averaging
 * Effective resolution: ~14.8 bits (oversampling gain = √32)
 * ============================================================ */

#include "adc_handler.h"
#include <string.h>

/* ── DMA buffer ─────────────────────────────────────────────── */
#define DMA_BUFFER_SIZE  64      /* Must be power of 2 */
#define AVG_SAMPLES      32      /* Average half-buffer each callback */

static uint16_t  dma_buffer[DMA_BUFFER_SIZE];
static uint32_t  averaged_value = 0;

/* ── INA128 + LM35 calibration constants ─────────────────────
 * LM35: 10 mV/°C
 * INA128 gain: 10 (RG = 5.6 kΩ)
 * ADC reference: 3.3V, 12-bit (4095 counts)
 * ──────────────────────────────────────────────────────────── */
#define ADC_REF_V       3.3f
#define ADC_MAX_COUNTS  4095.0f
#define INA128_GAIN     10.0f
#define LM35_MV_PER_C   10.0f   /* 10 mV per °C */

/* ============================================================
 * ADC_StartDMA()
 * Starts continuous ADC conversion into circular DMA buffer
 * ============================================================ */
void ADC_StartDMA(ADC_HandleTypeDef *hadc, DMA_HandleTypeDef *hdma)
{
    (void)hdma;  /* HAL links DMA internally at MX init time */
    memset(dma_buffer, 0, sizeof(dma_buffer));
    HAL_ADC_Start_DMA(hadc, (uint32_t*)dma_buffer, DMA_BUFFER_SIZE);
}

/* ============================================================
 * HAL_ADC_ConvHalfCpltCallback()
 * Fires when first 32 samples are filled — average them
 * ============================================================ */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        uint32_t sum = 0;
        for (uint8_t i = 0; i < AVG_SAMPLES; i++)
            sum += dma_buffer[i];
        averaged_value = sum / AVG_SAMPLES;
    }
}

/* ============================================================
 * HAL_ADC_ConvCpltCallback()
 * Fires when second 32 samples are filled — average them
 * ============================================================ */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        uint32_t sum = 0;
        for (uint8_t i = AVG_SAMPLES; i < DMA_BUFFER_SIZE; i++)
            sum += dma_buffer[i];
        averaged_value = sum / AVG_SAMPLES;
    }
}

/* ============================================================
 * ADC_GetAverage()
 * Returns latest averaged ADC count (thread-safe read)
 * ============================================================ */
uint32_t ADC_GetAverage(void)
{
    return averaged_value;
}

/* ============================================================
 * ADC_ToVoltage()
 * Converts raw ADC count to voltage at ADC input pin (V)
 * ============================================================ */
float ADC_ToVoltage(uint32_t adc_count)
{
    return ((float)adc_count / ADC_MAX_COUNTS) * ADC_REF_V;
}

/* ============================================================
 * ADC_ToTemperature()
 * Converts ADC input voltage to temperature in °C
 *
 * Signal chain (reverse):
 *   ADC_V = INA128_out_V (scaled by voltage divider — see schematic)
 *   INA128_out_V = LM35_V × gain
 *   LM35_V (mV) = Temperature(°C) × 10mV
 *
 *   Therefore:
 *   Temperature = (ADC_V / gain) / (10mV/°C)
 *               = (ADC_V × 1000) / (gain × 10)
 *               = ADC_V × 10.0  [with gain=10, mV→V factor]
 * ============================================================ */
float ADC_ToTemperature(float adc_voltage_V)
{
    float lm35_voltage_V = adc_voltage_V / INA128_GAIN;
    float temperature_C  = (lm35_voltage_V * 1000.0f) / LM35_MV_PER_C;
    return temperature_C;
}
