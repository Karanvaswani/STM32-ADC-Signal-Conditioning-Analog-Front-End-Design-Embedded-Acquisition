/* ============================================================
 * STM32 ADC Signal Conditioning — Main Application
 * Author  : Karan Kumar
 * Board   : STM32F103C8T6 (Blue Pill)
 * IDE     : STM32CubeIDE 1.14+
 * Purpose : DMA-driven 12-bit ADC acquisition with averaging,
 *           temperature conversion (LM35 + INA128 gain=10),
 *           and UART serial output at 1 Hz
 * ============================================================ */

#include "main.h"
#include "adc_handler.h"
#include "uart_handler.h"
#include <stdio.h>
#include <string.h>

/* ── Private variables ─────────────────────────────────────── */
ADC_HandleTypeDef  hadc1;
DMA_HandleTypeDef  hdma_adc1;
UART_HandleTypeDef huart1;
TIM_HandleTypeDef  htim2;

volatile uint8_t   data_ready_flag = 0;
volatile uint32_t  elapsed_ms      = 0;   /* incremented in SysTick */

/* ── Function prototypes ────────────────────────────────────── */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);

/* ============================================================
 * main()
 * ============================================================ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC1_Init();
    MX_USART1_UART_Init();
    MX_TIM2_Init();

    /* Calibrate ADC — improves accuracy by ~3 LSB */
    HAL_ADCEx_Calibration_Start(&hadc1);

    /* Start DMA-driven continuous ADC conversion */
    ADC_StartDMA(&hadc1, &hdma_adc1);

    /* Start 1-second periodic timer */
    HAL_TIM_Base_Start_IT(&htim2);

    UART_SendString(&huart1, "\r\n=== STM32 Temperature Acquisition System ===\r\n");
    UART_SendString(&huart1, "Sensor : LM35 | AFE Gain: 10 | ADC: 12-bit DMA\r\n");
    UART_SendString(&huart1, "Sampling: 1kSPS, 64-sample DMA buffer, averaged x32\r\n\r\n");

    while (1)
    {
        if (data_ready_flag)
        {
            data_ready_flag = 0;

            uint32_t adc_avg     = ADC_GetAverage();
            float    voltage_V   = ADC_ToVoltage(adc_avg);
            float    temperature = ADC_ToTemperature(voltage_V);

            /* LED threshold alert: ON if temperature > 40°C */
            if (temperature > 40.0f)
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);  /* LED on (active low) */
            else
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);    /* LED off */

            /* Transmit formatted reading over UART */
            UART_SendTemperature(&huart1, elapsed_ms, temperature, adc_avg, voltage_V);
        }
    }
}

/* ============================================================
 * TIM2 period elapsed callback — fires every 1 second
 * Sets flag to trigger UART output in main loop
 * ============================================================ */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        elapsed_ms += 1000;
        data_ready_flag = 1;
    }
}

/* ============================================================
 * System Clock: 72 MHz via PLL from 8 MHz HSE
 * ============================================================ */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState            = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue      = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL9;   /* 8MHz × 9 = 72MHz */
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;   /* 36 MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;   /* 72 MHz */
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection    = RCC_ADCPCLK2_DIV6;  /* 72/6 = 12 MHz ADC clock */
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}

/* ============================================================
 * ADC1 Init: PA0, continuous, DMA, 1kSPS
 * Sampling time: 239.5 cycles @ 12MHz ADC clock
 * Actual sample rate: 12MHz / (239.5 + 12.5) = ~47kSPS max
 * TIM2 triggers output at 1 Hz; DMA runs continuously
 * ============================================================ */
static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance                   = ADC1;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode    = ENABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    HAL_ADC_Init(&hadc1);

    sConfig.Channel      = ADC_CHANNEL_0;    /* PA0 — AFE output */
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;  /* longest = lowest noise */
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/* ============================================================
 * USART1 Init: 115200 8N1 on PA9 (TX) / PA10 (RX)
 * ============================================================ */
static void MX_USART1_UART_Init(void)
{
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

/* ============================================================
 * TIM2 Init: 1 Hz interrupt (72MHz / 7200 / 10000 = 1Hz)
 * ============================================================ */
static void MX_TIM2_Init(void)
{
    TIM_ClockConfigTypeDef  sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig      = {0};

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 7200 - 1;    /* 72MHz / 7200 = 10kHz */
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 10000 - 1;   /* 10kHz / 10000 = 1 Hz */
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim2);

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig);
    HAL_TIM_Base_Init(&htim2);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig);
}

/* ============================================================
 * GPIO Init: PC13 = onboard LED (output, active low)
 *            PA0  = ADC input (analog)
 * ============================================================ */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PC13 LED — default OFF (high) */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* PA0 — Analog input (ADC Channel 0) */
    GPIO_InitStruct.Pin  = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* ============================================================
 * DMA Init: DMA1 Channel 1 for ADC1
 * ============================================================ */
static void MX_DMA_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

/* ============================================================
 * Error Handler
 * ============================================================ */
void Error_Handler(void)
{
    __disable_irq();
    /* Flash LED rapidly to signal fault */
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(100);
    }
}
