/**
 * @file      system_monitor.h
 * @author    Your Name
 * @brief     系统资源监控任务 - 头文件
 * @version   1.0
 * @date      2025-06-22
 * 
 * @copyright Copyright (c) 2025
 *
 * @par 模块功能:
 *      提供一个低优先级的后台任务，用于周期性地打印系统关键资源的使用情况，
 *      包括FreeRTOS堆内存和各个关键任务的堆栈高水位线。这对于调试内存问题、
 *      优化资源分配和确保系统长期稳定性至关重要。
 */
#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include "cmsis_os2.h"

/**
 * @brief 初始化并创建系统资源监控任务。
 * @param defaultTaskHandle App_Main_Task 的任务句柄。
 * @param loraTaskHandle LoRa_APP_Task 的任务句柄。
 * @return osOK 表示成功, 其他值表示失败。
 */
osStatus_t SystemMonitor_Init(osThreadId_t defaultTaskHandle, osThreadId_t loraTaskHandle);

#endif // SYSTEM_MONITOR_H 