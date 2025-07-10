/**
 * @file      task_monitor.c
 * @author    Your Name
 * @brief     多任务看门狗监控系统
 * 
 * @par 内部实现机制:
 *      本模块使用一个极为高效的位掩码(bitmask)机制来追踪所有任务的签到状态。
 *      - `s_check_in_mask`: 这是一个32位的无符号整数，被用作"签到板"。它的每一个比特位
 *        都唯一对应 `TaskID_t` 枚举中的一个任务。当一个任务调用 `TaskMonitor_CheckIn()` 时，
 *        它对应的比特位就会被置为1。
 *      - `s_all_tasks_ok_mask`: 这是一个在初始化时根据 `TaskID_t` 枚举的任务总数自动
 *        计算出的"完成状态掩码"。如果系统中有N个任务被监控，那么这个掩码的值就是N个1
 *        （例如，3个任务时，值为 `0b111`）。
 * 
 *      `TaskMonitor_FeedDogIfAllOk()` 的核心逻辑就是一句简单的整数比较：
 *      `if (s_check_in_mask == s_all_tasks_ok_mask)`。
 *      这个操作极其快速，确保了监控逻辑本身不会成为系统的性能瓶颈。
 * 
 *      为了确保在读-改-写 `s_check_in_mask` 时的原子性，防止在多任务环境下被中断打断
 *      导致状态不一致，所有的写操作都使用了开关全局中断 (`__disable_irq`/`__enable_irq`)
 *      进行临界区保护。
 */

#include "task_monitor.h"
#include "stm32u5xx_hal.h"
#include "main.h" // For HAL_IWDG_Refresh
#include "cmsis_os2.h"
#include <string.h>
#include <stdio.h>

// IWDG 句柄在 main.c 中定义，此处进行外部声明
extern IWDG_HandleTypeDef hiwdg;

// 使用一个静态的32位无符号整型作为"签到板" (bitmask)
// 每个比特位代表一个任务的签到状态 (1: 已签到, 0: 未签到)
// volatile 关键字确保编译器不会对此变量的读写操作进行过度优化，保证每次访问都从内存中读取。
static volatile uint32_t s_check_in_mask = 0;

// 此掩码用于校验。当 s_check_in_mask 的值与它相等时，表明所有被监控的任务均已签到。
static uint32_t s_all_tasks_ok_mask = 0;

/**
 * @brief 初始化任务监控系统。
 */
void TaskMonitor_Init(void)
{
    // 根据在 .h 文件中 TaskID_t 枚举的任务总数，自动计算出"全员签到"状态对应的掩码值。
    // 例如，如果 TASK_MONITOR_COUNT 是 3, 此掩码将是 0b111 (即7)。
    // 这种设计使得在增删被监控任务时，此处代码无需任何修改。
    for (int i = 0; i < TASK_MONITOR_COUNT; i++) {
        s_all_tasks_ok_mask |= (1 << i);
    }
    printf("[Debug][TaskMonitor] Initialized. Required check-in mask: 0x%08X\r\n", s_all_tasks_ok_mask);

    // 初始状态下，清空签到板，没有任何任务签到。
    s_check_in_mask = 0;
}

/**
 * @brief 任务签到函数。
 */
void TaskMonitor_CheckIn(TaskID_t task_id)
{
    // 基本的边界检查，防止无效的task_id访问数组越界（虽然此处是位操作，但仍是好习惯）
    if (task_id < TASK_MONITOR_COUNT) {
        // [CRITICAL SECTION]
        // 通过"或"运算，将对应任务的比特位置为1。这是一个"读-改-写"操作。
        // 为防止在执行此操作时被中断打断，导致数据不一致（例如，另一个任务也同时签到），
        // 此处使用开关中断来保护这个关键的临界区，确保操作的原子性。
        __disable_irq();
        s_check_in_mask |= (1 << task_id);
        __enable_irq();
    }
}

/**
 * @brief 检查所有任务是否都已签到，如果全部签到则喂狗。
 */
void TaskMonitor_FeedDogIfAllOk(void)
{
    // [DEBUG] 打印当前签到状态和目标状态，用于调试
    printf("[Debug][TaskMonitor] Checking... Current mask: 0x%08X, Required mask: 0x%08X\r\n", 
           s_check_in_mask, s_all_tasks_ok_mask);

    // 核心逻辑: 检查"签到板"上的状态是否和"全员签到"状态完全一致。
    if (s_check_in_mask == s_all_tasks_ok_mask) {
        // 所有关键任务都健康，喂狗！
        printf("[Debug][TaskMonitor] All tasks OK. Feeding the dog.\r\n");
        HAL_IWDG_Refresh(&hiwdg);
        
        // [CRITICAL SECTION]
        // 喂狗后，必须清空签到板，为下一个监督周期做准备。
        // 此处同样是关键临界区，需要用开关中断保护。
        __disable_irq();
        s_check_in_mask = 0;
        __enable_irq();
    }
    else {
        // [DEBUG] 如果检查失败，明确打印出来
        printf("[Debug][TaskMonitor] Check failed! Not feeding dog.\r\n");
    }
    // else: 如果有任务没来签到，s_check_in_mask 的值就不会等于 s_all_tasks_ok_mask。
    // 在这种情况下，我们什么都不做。物理看门狗IWDG将因为得不到刷新而超时，
    // 最终安全地复位整个系统，从而实现从任务卡死的故障中自动恢复。
}

// 注意：您之前添加的 Register 函数和相关数组逻辑与本文件核心的位掩码机制不符，
// 且在 app_main.c 中并未调用 Register, 因此我将它们移除以保持文件逻辑一致性。
// 如果需要基于超时的监控，则需要对本文件进行更完整的重构。 