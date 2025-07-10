/**
 * @file      system_monitor.c
 * @author    Your Name
 * @brief     系统资源监控任务 - 源文件
 * @version   1.0
 * @date      2025-06-22
 * 
 * @copyright Copyright (c) 2025
 */
#include "system_monitor.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/
#define MONITOR_TASK_STACK_SIZE    (configMINIMAL_STACK_SIZE * 2) // 1024 bytes
#define MONITOR_TASK_PRIORITY      (osPriorityLow)
#define MONITOR_PERIOD_MS          (5000) // 每5秒打印一次

/* Private variables ---------------------------------------------------------*/
static osThreadId_t s_default_task_handle = NULL;
static osThreadId_t s_lora_task_handle = NULL;

/* Private function prototypes -----------------------------------------------*/
static void SystemMonitor_Task(void *argument);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief 初始化并创建系统资源监控任务。
 */
osStatus_t SystemMonitor_Init(osThreadId_t defaultTaskHandle, osThreadId_t loraTaskHandle)
{
    s_default_task_handle = defaultTaskHandle;
    s_lora_task_handle = loraTaskHandle;

    const osThreadAttr_t task_attributes = {
        .name = "SysMonitorTask",
        .stack_size = MONITOR_TASK_STACK_SIZE,
        .priority = (osPriority_t) MONITOR_TASK_PRIORITY,
    };
    
    osThreadId_t task_handle = osThreadNew(SystemMonitor_Task, NULL, &task_attributes);
    
    return (task_handle == NULL) ? osError : osOK;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 系统监控任务的实现函数。
 */
static void SystemMonitor_Task(void *argument)
{
    (void)argument;

    printf("\r\n[Monitor] System Monitor Task Started.\r\n");

    for(;;)
    {
        // 等待指定周期
        osDelay(MONITOR_PERIOD_MS);

        // 1. 获取并打印堆内存信息
        size_t free_heap_now = xPortGetFreeHeapSize();
        size_t min_free_heap_ever = xPortGetMinimumEverFreeHeapSize();

        printf("\r\n--- System Status ---\r\n");
        printf("[HEAP] Current Free: %u B, Minimum Ever: %u B\r\n", free_heap_now, min_free_heap_ever);

        // 2. 获取并打印任务堆栈高水位线
        // 高水位线(High Water Mark)指的是任务自启动以来，堆栈指针距离堆栈顶部的最小剩余字节数。
        // 这个值越小，说明堆栈使用得越满，发生溢出的风险越高。
        UBaseType_t main_stack_hwm = uxTaskGetStackHighWaterMark(s_default_task_handle);
        UBaseType_t lora_stack_hwm = uxTaskGetStackHighWaterMark(s_lora_task_handle);

        printf("[STACK] AppMainTask HWM: %lu words (%lu B)\r\n", main_stack_hwm, main_stack_hwm * sizeof(StackType_t));
        printf("[STACK] LoRaAppTask HWM: %lu words (%lu B)\r\n", lora_stack_hwm, lora_stack_hwm * sizeof(StackType_t));
        printf("---------------------\r\n");
    }
} 