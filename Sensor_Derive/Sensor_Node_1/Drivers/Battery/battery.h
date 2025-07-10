#ifndef __BATTERY_H
#define __BATTERY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>

/**
 * @brief 初始化电池电压检测功能所需的GPIO
 * @note  此函数应在主程序的初始化部分调用一次
 */
void Battery_Init(void);

/**
 * @brief 获取当前锂电池的真实电压值
 * @note  此函数已包含分压比的计算，返回的是实际电池电压。
 * @return float 返回计算出的真实电池电压 (单位: V)。如果读取失败，可能返回0。
 */
float Battery_GetVoltage(void);

/**
 * @brief 根据真实电池电压估算电量百分比
 * @param voltage 当前的真实电池电压 (V)
 * @return uint8_t 返回估算的电量百分比 (0-100)。
 */
uint8_t Battery_GetPercentage(float voltage);


#ifdef __cplusplus
}
#endif

#endif /* __BATTERY_H */ 