/**
 * @file app_main.h
 * @author Your Name
 * @brief 物联网应用主逻辑模块头文件
 * @version 1.0
 * @date 2025-06-08
 *
 * @copyright Copyright (c) 2025
 *
 * @par 模块功能:
 *      - 对外提供应用层唯一的初始化函数和主任务函数接口。
 *      - 将所有应用层逻辑与底层硬件/main函数解耦。
 */
/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : app_main.h
 * @brief          : Header for app_main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 */
/* USER CODE END Header */

#ifndef __APP_MAIN_H
#define __APP_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/


/* Exported functions prototypes ---------------------------------------------*/
/**
 * @brief  执行应用层的一次性初始化。
 * @details
 *         此函数应在RTOS启动后、主任务循环开始前被调用。
 *         它主要负责设置cJSON库的内存管理钩子，确保其在FreeRTOS环境下正确工作。
 */
void App_Main_Init(void);

/**
 * @brief  应用主任务函数。
 * @details
 *         此函数是整个物联网应用的核心，内部实现了一个无限循环的状态机，
 *         用于管理设备的初始化、云平台连接、正常运行和断线自动重连。
 *         它应该在RTOS中被创建为一个任务。
 * @note   此函数永远不会返回。
 */
void App_Main_Task(void);


#ifdef __cplusplus
}
#endif

#endif /* __APP_MAIN_H */
