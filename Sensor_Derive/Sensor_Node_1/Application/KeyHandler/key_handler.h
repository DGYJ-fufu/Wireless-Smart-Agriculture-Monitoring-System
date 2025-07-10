#ifndef __KEY_HANDLER_H
#define __KEY_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

// 定义短按和长按的时间阈值，单位：毫秒 (ms)
#define SHORT_PRESS_TIME_MS     50     // 按下时间超过此值，视为一次有效的短按
#define LONG_PRESS_TIME_MS      2000   // 按下时间超过此值，视为一次长按

// 为按键事件定义一个回调函数指针类型
typedef void (*Key_EventCallback_t)(void);

/**
 * @brief  初始化按键处理模块。
 * @note   此函数应在系统启动时调用一次。
 */
void Key_Init(void);

/**
 * @brief  为按键事件注册回调函数。
 * @param  short_press_callback: 短按事件的回调函数，可为NULL。
 * @param  long_press_callback:  长按事件的回调函数，可为NULL。
 */
void Key_Register_Callbacks(Key_EventCallback_t short_press_callback, Key_EventCallback_t long_press_callback);

/**
 * @brief  处理按键状态机，必须在主循环中调用。
 * @note   此函数负责处理长按事件的检测。
 */
void Key_Process(void);

/**
 * @brief  处理按键的EXTI中断回调。
 * @note   此函数应在 stm32u0xx_it.c 的 HAL_GPIO_EXTI_Callback 中被调用。
 * @param  GPIO_Pin: 触发中断的引脚。
 */
void Key_EXTI_Callback(uint16_t GPIO_Pin);

#endif // __KEY_HANDLER_H 