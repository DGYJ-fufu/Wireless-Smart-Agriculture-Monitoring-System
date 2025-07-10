#ifndef LORA_APP_H
#define LORA_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32u5xx_hal.h"
#include <stdbool.h>
#include "LoRa.h"
#include "cmsis_os2.h"
#include <stdint.h>

// [ADDED] Extern declaration for the LoRa App task handle
// This allows other modules to access the task handle for monitoring purposes.
extern osThreadId_t s_lora_app_task_handle;

#define LORA_MAX_PAYLOAD_SIZE 240 // LoRa最大应用层载荷长度 (原始数据包255 - 协议开销)

/**
 * @brief 初始化 LoRa 硬件
 * @details
 *  - 配置 LoRa 驱动句柄
 *  - 复位 LoRa 模块
 *  - 初始化 LoRa 模块并检查版本号 (核心通信参数在此设置)
 *  - **重要**: 此函数应在 RTOS 内核启动前调用 (例如在 main() 中)。
 * @param None
 * @return bool - true: 初始化成功, false: 初始化失败 (未找到 LoRa 模块)
 */
bool LoRa_HW_Init(void);

/**
 * @brief 初始化 LoRa 应用层 OS 对象
 * @details
 *  - 创建 LoRa 应用任务 (LoRa_APP_Task)
 *  - 创建用于中断同步的信号量
 *  - **重要**: 此函数应在 RTOS 对象初始化阶段调用 (例如在 app_freertos.c 中)。
 * @param None
 * @return None
 */
void LoRa_APP_Init(void);

/**
 * @brief 将一个数据包放入 LoRa 发送队列
 * 
 * @param data 指向要发送的数据的指针
 * @param len  要发送的数据长度 (字节)
 * @return bool 
 *         - true: 成功放入队列
 *         - false: 队列已满或参数错误
 * 
 * @note 此函数是线程安全的，可以从任何任务或中断中调用。
 * @warning 数据长度 `len` 不能超过 `LORA_MAX_PAYLOAD_SIZE`。
 */
bool LoRa_APP_Send(const uint8_t *data, uint8_t len);

/**
 * @brief LoRa DIO0 引脚的外部中断服务函数 (EXTI ISR)
 * @details
 *  - 当 LoRa 芯片完成数据包接收后，DIO0 会触发此中断。
 *  - 此函数的功能是释放信号量，以唤醒正在等待的 LoRa_APP_Task。
 *  - **重要**: 此函数需要由用户在 `stm32u5xx_it.c` 中相应的中断处理函数里调用。
 * @param GPIO_Pin 触发中断的 GPIO 引脚号
 * @return None
 */
void LoRa_DIO0_ISR(uint16_t GPIO_Pin);


#ifdef __cplusplus
}
#endif

#endif /* LORA_APP_H */ 