#ifndef __PWM_CONTROL_H
#define __PWM_CONTROL_H

#include "main.h"

extern TIM_HandleTypeDef htim3;

// 定义定时器句柄
#define PWM_TIMER htim3
// 定义 PWM 通道
#define PUMP_PWM_CHANNEL TIM_CHANNEL_1 // PB4 -> TIM3_CH1
#define FAN_PWM_CHANNEL  TIM_CHANNEL_2 // PB5 -> TIM3_CH2

// 定义计数器周期 (ARR 值，由 CubeMX 配置决定)
// ARR = 99, 所以最大值为 99
#define PWM_MAX_DUTY 99

// 占空比步长
#define VALUE_STEP 5

HAL_StatusTypeDef PWM_Init(void);
void set_pump_speed(uint32_t duty_cycle);
void set_fan_speed(uint32_t duty_cycle);

#endif