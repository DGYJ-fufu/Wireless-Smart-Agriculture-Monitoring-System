/**
 * @file      task_monitor.h
 * @author    Your Name
 * @brief     多任务看门狗监控系统 - 头文件
 * @version   1.1
 * @date      2025-06-22
 * 
 * @copyright Copyright (c) 2025
 *
 * @par V1.1 (2025-06-22)
 *      1. [DOC] 添加了详细的 Doxygen 注释，阐明模块设计思想和使用方法。
 * 
 * @par 设计思想:
 *      在一个多任务的嵌入式系统中，只由一个主任务来喂狗是不安全的。因为主任务本身运行正常，
 *      并不能保证其他关键的业务任务（如LoRa接收任务）没有陷入死锁或卡死。
 * 
 *      本模块实现了一个专业的"多任务签到"看门狗机制，解决了上述问题。其核心思想是：
 *      - **权责分离**:
 *          - **被监督者 (Workers)**: 所有被认为对系统至关重要的任务（如 app_main, lora_app）
 *            都是被监督者。它们有**义务**在自己的主循环中，周期性地调用 `TaskMonitor_CheckIn()`
 *            来"报平安"。
 *          - **监督者 (Supervisor)**: 一个最高优先级的任务（通常是 app_main）作为监督者。
 *            它有**权力**周期性地调用 `TaskMonitor_FeedDogIfAllOk()` 来检查所有"工人"是否
 *            都已按时报到。
 *      - **喂狗条件**: 监督者**只有在确认所有工人都在本轮检查周期内报过平安后**，才会去喂狗。
 *        假如有任何一个工人任务没有按时签到，监督者便会拒绝喂狗。这将导致看门狗(IWDG)最终
 *        超时，并安全地将整个系统复位，从而实现从任务死锁中恢复。
 * 
 * @par 如何使用:
 *      1. **定义任务**: 在 `TaskID_t` 枚举中，添加所有你需要监控的关键任务的ID。
 *      2. **实现签到**: 在每个被监控任务的主循环中，确保 `TaskMonitor_CheckIn(TASK_ID_XXX)`
 *         被周期性地调用。
 *      3. **实现监督**: 在你的主任务（或任何可靠的监督任务）的主循环中，周期性地调用
 *         `TaskMonitor_FeedDogIfAllOk()`。
 */

#ifndef TASK_MONITOR_H
#define TASK_MONITOR_H

/**
 * @brief 定义所有被监控的关键任务的ID。
 * @note  这是一个可扩展的设计。当未来向系统中添加新的关键后台任务时，
 *        只需在此枚举的 `TASK_MONITOR_COUNT` 之前添加一个新的任务ID即可。
 *        整个监控系统会自动适应新的任务数量，无需修改其他代码。
 */
typedef enum {
    TASK_ID_APP_MAIN,   ///< 应用主任务
    TASK_ID_LORA_APP,   ///< LoRa 应用任务
    // 未来可在此处添加其他关键任务
    TASK_MONITOR_COUNT  ///< 特殊成员：自动计算被监控任务的总数，必须保留在最后。
} TaskID_t;

/**
 * @brief 初始化任务监控系统。
 * @details 在系统启动时由 `App_Main_Init()` 调用一次，以清零内部的签到状态记录。
 */
void TaskMonitor_Init(void);

/**
 * @brief 关键任务使用此函数进行"签到"或"报平安"。
 * @details 每个被监控的关键任务都需要在其主循环中周期性地调用此函数，
 *          以向监督者表明自己当前正存活且未卡死。
 * @param task_id 调用此函数的任务的ID (来自 `TaskID_t` 枚举)。
 */
void TaskMonitor_CheckIn(TaskID_t task_id);

/**
 * @brief 监督者任务使用此函数检查所有任务的签到状态，并在满足条件时喂狗。
 * @details 此函数应由一个可靠的"监督者"任务（如`App_Main_Task`）周期性调用。
 *          其内部逻辑如下：
 *          - 检查内部的签到记录表，判断是否所有任务都已在本轮监控周期内调用过 `TaskMonitor_CheckIn()`。
 *          - **如果是**：说明系统健康。则调用 `HAL_IWDG_Refresh()` 喂狗，并清除所有签到记录，为下一个监控周期做准备。
 *          - **如果否**：说明有任务异常。则**不**执行任何操作，这将最终导致IWDG超时并安全地复位系统。
 */
void TaskMonitor_FeedDogIfAllOk(void);

#endif // TASK_MONITOR_H 