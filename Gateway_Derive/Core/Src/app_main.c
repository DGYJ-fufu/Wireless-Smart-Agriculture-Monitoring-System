/**
 * @file      app_main.c
 * @author    Your Name
 * @brief     物联网网关应用核心逻辑与状态机
 * @version   3.0
 * @date      2025-06-22
 *
 * @copyright Copyright (c) 2025
 *
 * @par V3.0 (2025-06-22)
 *      1. [BUGFIX] 修复了看门狗(IWDG)在初始化阶段超时导致系统反复重启的致命问题。
 *         通过在 `WAIT_FOR_MODULE` 和 `INITIALIZING` 状态的关键路径中添加手动喂狗
 *         (`HAL_IWDG_Refresh`)，确保了冗长的初始化流程不会被中断。
 *      2. [DOC] 对整个文件进行了全面的注释重构，提升了代码的可读性和可维护性。
 *
 * @par V2.0 (2025-06-09)
 *      1. [FEATURE] 实现了工业级的启动健壮性：通过主动轮询`AT+CPIN?`和被动监听`+SIM READY` URC
 *         相结合的方式，解决了单片机单独复位而模组未复位时，系统无法启动的致命缺陷。
 *      2. [REFACTOR] 重构了系统状态机，增加了`SYS_STATE_WAIT_FOR_MODULE`状态，使启动流程更清晰可靠。
 *
 * @par 模块设计思想:
 *      此模块是整个应用的心脏，其核心是一个强大的状态机(`App_Main_Task`)。该状态机负责管理
 *      设备的完整生命周期，从上电启动、初始化通信模组、连接云平台，到正常运行期间的数据上报
 *      和任务监控，以及在检测到异常时的断线重连。
 *
 *      特别地，为了保证系统在无人值守环境下的长期稳定运行，本模块的设计中融入了两个关键的
 *      高可用性策略：
 *      1.  **鲁棒的启动流程**: 结合主动探测和被动等待，确保无论MCU和模组以何种顺序、
 *          何种状态启动，系统都能最终进入正常工作状态。
 *      2.  **分阶段的看门狗管理**:
 *          - 在**初始化阶段**，由本模块根据代码执行路径显式地喂狗，保证启动过程不被中断。
 *          - 在**运行阶段**，喂狗的责任转移给 `TaskMonitor` 模块，通过监控所有关键业务
 *            任务的存活状态，实现对整个系统的全面监督。
 */

#include "main.h"
#include "cmsis_os2.h"
#include "app_main.h"
#include "at_handler.h"
#include "huawei_iot_app.h"
#include "device_manager.h"
#include "lora_app.h"
#include <string.h>
#include "stdio.h"
#include "task_monitor.h"

/* Private typedef -----------------------------------------------------------*/
/**
 * @brief 系统全局网络状态枚举
 * @details 用于驱动 `App_Main_Task` 中的主状态机。每个状态都代表了设备生命周期中的一个明确阶段。
 */
typedef enum {
    SYS_STATE_START,                ///< 0: 系统启动。此状态负责初始化AT处理器，并注册URC回调，为与模组通信做准备。
    SYS_STATE_WAIT_FOR_MODULE,      ///< 1: 等待模组就绪。此为鲁棒性设计的核心，结合主动轮询(`AT+CPIN?`)与被动监听(`+SIM READY`)，确保模组可用。
    SYS_STATE_INITIALIZING,         ///< 2: 模组已就绪。此状态执行一系列配置，完成网络注册和云平台连接。
    SYS_STATE_RUNNING,              ///< 3: 已成功连接，系统正常运行。此状态下，主要负责周期性地上报数据和监控各任务状态。
    SYS_STATE_RECONNECTING,         ///< 4: 检测到连接中断。此状态负责清理资源，然后切换回 `SYS_STATE_START` 以尝试重新建立连接。
} SystemState_t;

/* Private variables ---------------------------------------------------------*/
// 外部硬件句柄，由 main.c 初始化并提供
extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef handle_GPDMA1_Channel0; // RX DMA
extern DMA_HandleTypeDef handle_GPDMA1_Channel1; // TX DMA
extern IWDG_HandleTypeDef hiwdg;

// 应用模块内部变量
AT_Handler_t g_at_handle; // 全局句柄，以 'g_' 为前缀
static volatile SystemState_t g_system_state = SYS_STATE_START; ///< 全局系统状态变量

// --- Private Function Prototypes ---
static void Cloud_UploadDataTask(void);

// --- URC (主动上报信息) 处理 ---

/**
 * @brief 模组就绪/重启事件的回调函数
 * @details
 *        此回调函数被注册用于处理代表模组就绪的URC ("+SIM READY")。
 *        它提供了一个由模组事件驱动的"快速通道"来改变系统状态，作为对主动轮询机制的补充。
 *        - **快速启动**: 如果系统正在 `WAIT_FOR_MODULE` 状态下轮询，收到此URC可以立即进入下一状态，无需等待轮询超时。
 *        - **掉线重连**: 如果系统已在 `RUNNING` 状态，收到此URC表明模组已自行重启，触发系统进入重连流程。
 * @param urc_line 模块上报的原始URC字符串 (当前未使用)
 */
static void on_module_ready(const char *urc_line)
{
    (void)urc_line; // 避免编译器警告

    // 如果我们正在等待，这个URC就是我们继续的信号。
    if (g_system_state == SYS_STATE_WAIT_FOR_MODULE) {
        printf("\r\n[URC] Module is ready! (+SIM READY received). Starting initialization...\r\n");
        g_system_state = SYS_STATE_INITIALIZING;
    }
    // 如果我们已经在运行，这意味着模块已经自行重启了。
    else if (g_system_state == SYS_STATE_RUNNING) {
        printf("\r\n[URC] Module reboot detected (+SIM READY received). Triggering reconnection...\r\n");
        g_system_state = SYS_STATE_RECONNECTING;
    }
    // 在其他状态（初始化、重连等）下，我们忽略此URC，因为状态转换已在进行中。
}

/**
 * @brief 云端下发命令的URC回调函数
 * @param urc_line 模块上报的原始URC字符串, e.g., "+HMREC: 0,..."
 */
static void on_cloud_command(const char *urc_line)
{
    // 直接将整条URC转发给华为云应用层进行解析和分发
    HuaweiIoT_ParseHMREC(&g_at_handle, urc_line);
}

// 定义URC处理表
static const AT_URC_t urc_table[] = {
    {"+SIM READY", on_module_ready},
    {"+HMREC:", on_cloud_command} // 新增：处理华为云平台下行命令的URC
};
static const uint8_t URC_TABLE_SIZE = sizeof(urc_table) / sizeof(urc_table[0]);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief 执行只需进行一次的应用层初始化。
 */
void App_Main_Init(void)
{
    // 1. 初始化cJSON钩子
    HuaweiIoT_Init();
    
    // 2. 初始化设备管理器
    DeviceManager_Init();

    // 3. 初始化任务监控器
    TaskMonitor_Init();
}

/**
 * @brief 应用主任务，实现了系统的核心状态机。
 * @note  此任务是系统的"总指挥"，它不执行具体的业务操作（如解析JSON），而是负责
 *        在不同系统状态之间进行切换，并调用其他模块的接口来完成具体任务。
 *        看门狗的管理权责在此任务中进行划分：初始化阶段手动喂狗，运行阶段交由TaskMonitor管理。
 */
void App_Main_Task(void)
{
    // --- 主状态机循环 ---
    for (;;)
    {
        switch (g_system_state)
        {
        case SYS_STATE_START:
            printf("\r\n--- [STATE] Starting AT Processor ---\r\n");
            // 步骤1: 初始化AT句柄，启动监听任务
            if (AT_Init(&g_at_handle, &huart3, &handle_GPDMA1_Channel0, &handle_GPDMA1_Channel1) != osOK) {
                printf("[FATAL] AT Processor Initialization Failed. System Halted.\r\n");
                // 此处发生错误通常是内存不足，是严重问题，直接卡住
                while(1) { osDelay(10000); }
            }
            // 步骤2: 注册URC回调
            AT_RegisterURCCallbacks(&g_at_handle, urc_table, URC_TABLE_SIZE);
            g_system_state = SYS_STATE_WAIT_FOR_MODULE; // 初始化完成，进入等待模组就绪状态
            break;

        case SYS_STATE_WAIT_FOR_MODULE:
            printf("--- [STATE] Waiting for module ready (Polling AT+CPIN?)...\r\n");

            // [BUGFIX] 在执行可能耗时很长的AT指令前喂狗，防止在初始化阶段被IWDG复位。
            // 此时喂狗责任由本任务承担，进入RUNNING状态后将移交给TaskMonitor。
            HAL_IWDG_Refresh(&hiwdg);

            // [鲁棒性设计] 主动发送 `AT+CPIN?` 轮询SIM卡状态。
            // 这是为了解决"单片机复位陷阱"：即MCU因看门狗或电源波动而复位，但通信模组可能并未复位。
            char cpin_response[64] = {0};
            AT_Status_t status;
            
            // [鲁棒性设计] 主动发送 `AT+CPIN?` 轮询SIM卡状态。
            // 这是为了解决"单片机复位陷阱"：即MCU因看门狗或电源波动而复位，但通信模组已就绪。
            // 在此场景下，模组已就绪，不会再发送 `+SIM READY` URC。如果系统只被动等待该URC，将陷入死锁。
            // 主动轮询确保了即使错过了URC，系统也能通过自身探测恢复正常。
            status = AT_SendCommand(&g_at_handle, "AT+CPIN?", 5000, cpin_response, sizeof(cpin_response));
            
            // 检查响应是否包含 "+CPIN: READY"，这是SIM卡已识别并准备就绪的明确信号。
            if (status == AT_OK && strstr(cpin_response, "+CPIN: READY") != NULL)
            {
                printf("  > Polling successful: SIM card is READY!\r\n");
                g_system_state = SYS_STATE_INITIALIZING;
            }
            else
            {
                // 轮询失败或SIM卡未就绪，则等待3秒后重试。
                // 在此等待期间，`on_module_ready` URC回调（被动监听）仍可能被触发，从而提供一个更快的启动路径。
                osDelay(3000); 
            }
            break;

        case SYS_STATE_INITIALIZING:
            printf("\r\n--- [STATE] Initializing System ---\r\n");
            
            // [BUGFIX] 在初始化序列的开始和各个关键步骤之间喂狗，确保流程不被中断。
            HAL_IWDG_Refresh(&hiwdg);
            osDelay(500); // 在收到 "+SIM READY" 后，再稍等一下，确保模块内部各功能（如TCP/IP协议栈）完全准备好。

            // 步骤1: 配置模块 (AT, ATE0)
            if (AT_SendBasicCommand(&g_at_handle, "AT", 2000) != AT_OK) {
                 printf("[ERROR] Module not responding to AT. Reconnecting...\r\n");
                 g_system_state = SYS_STATE_RECONNECTING;
                 break;
            }
             if (AT_SendBasicCommand(&g_at_handle, "ATE0", 2000) != AT_OK) {
                 printf("[ERROR] Failed to set ATE0. Reconnecting...\r\n");
                 g_system_state = SYS_STATE_RECONNECTING;
                 break;
            }
            printf("Module Configured Successfully.\r\n");

            // 步骤2: 连接到云平台
            HAL_IWDG_Refresh(&hiwdg);
            printf("\r\n--- Cloud Connection Sequence ---\r\n");
            if (HuaweiIoT_ConnectCloud(&g_at_handle) != AT_OK) {
                printf("[ERROR] Failed to connect to cloud. Reconnecting...\r\n");
                g_system_state = SYS_STATE_RECONNECTING;
                break;
            }

            // 步骤3: 上报所有子设备状态为在线
            HAL_IWDG_Refresh(&hiwdg);
            if (HuaweiIoT_PublishAllSubDevicesOnline(&g_at_handle) != AT_OK) {
                printf("[ERROR] Failed to publish sub-device status. Reconnecting...\r\n");
                g_system_state = SYS_STATE_RECONNECTING;
                break; 
            }
            
            // 步骤4: 通知设备管理器，云平台已在线
            DeviceManager_SetCloudOnlineStatus(true);

            // 所有初始化步骤成功，进入正常运行状态
            g_system_state = SYS_STATE_RUNNING;
            printf("\r\n--- [STATE] System Running ---\r\n");
            break;

        case SYS_STATE_RUNNING:
            // 进入正常运行状态后，喂狗的责任完全移交给 TaskMonitor 模块。
            // TaskMonitor 会检查所有被监控任务的"签到"状态，只有在所有任务都存活时才喂狗。
            // 步骤1: 作为监督者，检查所有关键任务是否都已签到。
            // 如果是，此函数会喂狗。如果否，则不喂狗，系统将最终被复位。
            TaskMonitor_FeedDogIfAllOk();

            // 步骤2: 主任务自己签到，表明自己在本轮循环中是存活的。
            TaskMonitor_CheckIn(TASK_ID_APP_MAIN);
            
            // 步骤3: 调用批量上报函数
            HuaweiIoT_PublishGatewayReport(&g_at_handle);
            
            // 步骤4: 短暂休眠，定义监督周期
            // [TIMING] 此延时必须小于 (看门狗超时 - 上报函数最大耗时)
            // 4.2s - 1.5s = 2.7s. 为保证裕量，取 2s.
            osDelay(2000); 
            break;

        case SYS_STATE_RECONNECTING:
            printf("\r\n--- [STATE] Reconnecting ---\r\n");
            
            // 步骤1: 通知设备管理器，云平台已离线
            DeviceManager_SetCloudOnlineStatus(false);

            // 核心步骤: 彻底清理旧的会话资源（如AT任务、缓冲区），为全新启动做准备。
            AT_DeInit(&g_at_handle); 
            osDelay(3000);          // 等待模组稳定或网络环境恢复
            g_system_state = SYS_STATE_START; // 重新进入启动状态，开始全新流程
            break;
        }
    }
} 
