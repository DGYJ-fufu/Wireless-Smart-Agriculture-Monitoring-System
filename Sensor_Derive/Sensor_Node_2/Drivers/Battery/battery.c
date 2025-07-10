/**
 * @file battery.c
 * @brief 电池电压检测库的实现 (使用VREFINT进行动态校准)
 */

#include "battery.h"
#include "adc.h" // 需要ADC句柄 hadc1
#include "stm32u0xx_hal.h" // 需要HAL库和VREFINT定义
#include <stdio.h> // for printf

// --- 宏定义 ---
#define BATTERY_ADC_HANDLE hadc1
// 根据电路图，R24和R25组成分压电路。此处为 (R24+R25)/R25
// 您使用的是两个30k电阻，所以分压比是 (30k+30k)/30k = 2.0
#define VOLTAGE_DIVIDER_RATIO 2.0f
#define ADC_MAX_VALUE 4095.0f      // 12位ADC的最大值

// 根据CubeMX配置，电池电压采样引脚为 IN8 (对应PA8)
#define BATTERY_ADC_CHANNEL ADC_CHANNEL_8

// VREFINT在校准时，VDDA通常为3.0V
// VREFINT_CAL_ADDR 由 stm32u0xx_ll_adc.h 提供，我们直接使用它
#define VDD_CALIB_VOLTS (3.0f)

// --- 私有函数声明 ---
static void Battery_StartMeasure(void);
static void Battery_StopMeasure(void);
static uint16_t Battery_ReadAdcChannel(uint32_t channel);

/**
 * @brief 初始化电池电压检测功能
 */
void Battery_Init(void)
{
    // 确保在不测量时，默认关闭电路以省电
    Battery_StopMeasure();
}

/**
 * @brief 获取当前锂电池的真实电压值
 * @note  此函数通过测量VREFINT来动态校准VDDA，以获得精确的测量结果。
 */
float Battery_GetVoltage(void)
{
    float vdda = 3.3f; // 默认值，如果VREFINT测量失败则使用
    uint32_t adc_sum = 0;
    const int sample_count = 10;

    Battery_StartMeasure();

    // --- 1. 测量VREFINT以计算真实的VDDA电压 ---
    uint32_t vrefint_sum = 0;
    for (int i = 0; i < sample_count; i++)
    {
        vrefint_sum += Battery_ReadAdcChannel(ADC_CHANNEL_VREFINT);
        HAL_Delay(1);
    }
    uint16_t vrefint_raw = vrefint_sum / sample_count;

    if (vrefint_raw > 0)
    {
        // VREFINT_CAL_ADDR 是由 stm32u0xx_ll_adc.h 定义的宏
        uint16_t vrefint_cal = *VREFINT_CAL_ADDR;
        // 公式: VDDA = VDD_CALIB * (VREFINT_CAL / VREFINT_RAW)
        vdda = VDD_CALIB_VOLTS * (float)vrefint_cal / (float)vrefint_raw;
    }

    // --- 2. 测量电池分压通道 ---
    for (int i = 0; i < sample_count; i++)
    {
        adc_sum += Battery_ReadAdcChannel(BATTERY_ADC_CHANNEL);
        HAL_Delay(1);
    }
    uint16_t adc_avg_raw = adc_sum / sample_count;

    Battery_StopMeasure();

    // --- 3. 根据动态计算出的VDDA计算电池电压 ---
    float pin_voltage = (adc_avg_raw / ADC_MAX_VALUE) * vdda;
    float battery_voltage = pin_voltage * VOLTAGE_DIVIDER_RATIO;

    return battery_voltage;
}

/**
 * @brief 根据真实电池电压估算电量百分比
 */
uint8_t Battery_GetPercentage(float voltage)
{
    // 锂电池满电电压
    const float max_voltage = 4.2f;
    // LDO正常工作的最低输入电压 (3.3V + 0.25V压差)
    const float min_voltage = 3.55f;

    if (voltage >= max_voltage) {
        return 100;
    }
    if (voltage <= min_voltage) {
        return 0;
    }

    // 在有效工作电压范围内进行线性插值
    uint8_t percentage = (uint8_t)(((voltage - min_voltage) / (max_voltage - min_voltage)) * 100.0f);

    return percentage;
}


// --- 私有函数实现 ---

/**
 * @brief 打开电池电压测量电路 (PF1拉低)
 */
static void Battery_StartMeasure(void)
{
    HAL_GPIO_WritePin(BATVOL_CTRL_GPIO_Port, BATVOL_CTRL_Pin, GPIO_PIN_RESET);
    HAL_Delay(100); // 等待RC电路稳定
}

/**
 * @brief 关闭电池电压测量电路 (PF1拉高)
 */
static void Battery_StopMeasure(void)
{
    HAL_GPIO_WritePin(BATVOL_CTRL_GPIO_Port, BATVOL_CTRL_Pin, GPIO_PIN_SET);
}

/**
 * @brief 动态配置并测量指定的ADC通道
 */
static uint16_t Battery_ReadAdcChannel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint16_t adc_value = 0;

    // 配置要测量的ADC通道
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_160CYCLES_5;
    // 注意: STM32U0系列在通道配置中没有SingleDiff, OffsetNumber等成员
    if (HAL_ADC_ConfigChannel(&BATTERY_ADC_HANDLE, &sConfig) != HAL_OK)
    {
        // 错误处理
        return 0;
    }

    // 在切换通道后增加一个短暂延时，确保ADC输入稳定
    HAL_Delay(1);

    // 启动ADC转换
    if (HAL_ADC_Start(&BATTERY_ADC_HANDLE) != HAL_OK)
    {
        return 0;
    }

    // 等待转换完成
    if (HAL_ADC_PollForConversion(&BATTERY_ADC_HANDLE, 100) == HAL_OK)
    {
        adc_value = HAL_ADC_GetValue(&BATTERY_ADC_HANDLE);
    }
    
    // 停止ADC
    HAL_ADC_Stop(&BATTERY_ADC_HANDLE);

    return adc_value;
} 