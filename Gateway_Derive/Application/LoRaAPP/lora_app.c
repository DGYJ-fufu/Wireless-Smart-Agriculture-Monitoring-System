/**
 * @file      lora_app.c
 * @author    Your Name
 * @brief     LoRa 应用层核心任务
 *
 * @par 模块设计思想:
 *      本模块是LoRa通信的应用层，它充当了底层LoRa硬件驱动和上层应用逻辑之间的桥梁。
 *      其核心是一个事件驱动的后台任务(`LoRa_APP_Task`)，该任务的设计遵循了低功耗和高响应性的原则：
 *
 *      1.  **中断驱动**: 任务平时处于挂起状态(阻塞于信号量)，不消耗CPU资源。当LoRa芯片通过
 *          `DIO0`引脚触发硬件中断时，中断服务程序(ISR)会释放一个信号量，以此来精确地唤醒任务。
 *
 *      2.  **异步处理**: 中断服务程序本身只做最少的工作（释放信号量），将耗时的数据读取和协议解析
 *          工作全部转移到`LoRa_APP_Task`中执行。这遵循了实时系统中中断处理的黄金法则，
 *          确保了系统的实时响应能力。
 *
 *      3.  **任务存活监控**: `LoRa_APP_Task`被设计为系统中的一个关键任务，因此它必须周期性地
 *          向`TaskMonitor`模块"签到"（Check-In）。这使得主任务能够监控其健康状况。
 *          即使在没有LoRa信号的情况下，任务也会因信号量超时而被唤醒并执行签到，从而向系统
 *          证明自己并未"卡死"。
 */

#include "lora_app.h"
#include "cmsis_os2.h"
#include "LoRa.h"
#include "lora_protocol.h"
#include "device_manager.h"
#include <stdio.h>
#include <string.h>
#include "../../Middlewares/TaskMonitor/task_monitor.h"

// 1. SPI Handle
//    请将 &hspi1 替换为您用于连接 LoRa 模块的 SPI 外设句柄
extern SPI_HandleTypeDef hspi2;
#define LORA_SPI_HANDLE (&hspi2)

// 2. LoRa Reset Pin
#define LORA_RESET_PORT LORA_RESET_GPIO_Port
#define LORA_RESET_PIN LORA_RESET_Pin

// 3. LoRa Chip Select (CS) Pin
#define LORA_CS_PORT LORA_NSS_GPIO_Port
#define LORA_CS_PIN LORA_NSS_Pin

// 4. LoRa DIO0 Interrupt Pin
//    !! 重要 !!: 请确保此引脚在 CubeMX 中已配置为外部中断(EXTI)模式
#define LORA_DIO0_PORT LORA_DIO0_GPIO_Port
#define LORA_DIO0_PIN LORA_DIO0_Pin

// ============================================================================
// Private Defines & Variables
// ============================================================================

#define LORA_TASK_STACK_SIZE 4096 
#define LORA_TASK_PRIORITY osPriorityNormal
#define LORA_TX_QUEUE_MSG_COUNT 8 // 发送队列能缓存的消息数量
#define LORA_TX_QUEUE_MSG_SIZE sizeof(lora_tx_request_t) // 每个消息的大小

// Event Flags for LoRa Task
#define EVT_FLAG_LORA_RX_DONE (1U << 0) // 接收完成标志
#define EVT_FLAG_LORA_TX_REQ  (1U << 1) // 发送请求标志

// 发送请求消息结构体
typedef struct {
    uint8_t buffer[LORA_MAX_PAYLOAD_SIZE];
    uint8_t length;
} lora_tx_request_t;

osThreadId_t s_lora_app_task_handle; 
static osMutexId_t s_lora_access_mutex;       // LoRa 硬件访问互斥锁
static osEventFlagsId_t s_lora_event_flags;   // 用于唤醒任务的事件标志组
static osMessageQueueId_t s_lora_tx_queue;    // LoRa 发送消息队列

static LoRa s_lora_handle; // LoRa 驱动句柄

// LoRa 数据接收缓冲区
static uint8_t s_lora_rx_buffer[LORA_MAX_RAW_PACKET];

// ============================================================================
// Private Function Prototypes
// ============================================================================

static void LoRa_APP_Task(void *argument);
static void process_received_packet(uint8_t *data, uint8_t len);
static bool lora_send_packet(const uint8_t* data, uint8_t len);

// ============================================================================
// Public Function Implementations
// ============================================================================

/**
 * @brief 初始化 LoRa 硬件
 */
bool LoRa_HW_Init(void)
{
    // 1. 初始化 LoRa 硬件句柄
    s_lora_handle = newLoRa();
    s_lora_handle.hSPIx = LORA_SPI_HANDLE;
    s_lora_handle.CS_port = LORA_CS_PORT;
    s_lora_handle.CS_pin = LORA_CS_PIN;
    s_lora_handle.reset_port = LORA_RESET_PORT;
    s_lora_handle.reset_pin = LORA_RESET_PIN;
    s_lora_handle.DIO0_port = LORA_DIO0_PORT;
    s_lora_handle.DIO0_pin = LORA_DIO0_PIN;

    // 2. 设置 LoRa 通信参数
    s_lora_handle.frequency = 433;          // 433 MHz
    s_lora_handle.spredingFactor = SF_7;    // 扩频因子 7
    s_lora_handle.bandWidth = BW_125KHz;    // 带宽 125 KHz
    s_lora_handle.crcRate = CR_4_5;         // 编码率 4/5

    // 3. 复位并初始化 LoRa 芯片
    LoRa_reset(&s_lora_handle);
    uint16_t lora_status = LoRa_init(&s_lora_handle);
    if (lora_status != LORA_OK) {
        printf("LoRa HW Init Failed! Status: %d\r\n", lora_status);
        return false;
    }
    
    // 打印版本号确认通信正常
    printf("LoRa HW Init OK, Version: 0x%02X\r\n", LoRa_read(&s_lora_handle, RegVersion));
    return true;
}

/**
 * @brief 将一个数据包放入 LoRa 发送队列
 */
bool LoRa_APP_Send(const uint8_t *data, uint8_t len)
{
    if (data == NULL || len == 0 || len > LORA_MAX_PAYLOAD_SIZE) {
        return false;
    }

    lora_tx_request_t tx_req;
    memcpy(tx_req.buffer, data, len);
    tx_req.length = len;

    // 将消息放入队列。0超时表示如果队列已满则立即返回失败。
    if (osMessageQueuePut(s_lora_tx_queue, &tx_req, 0, 0) != osOK) {
        printf("[LoRa] TX Queue full!\r\n");
        return false;
    }

    // 设置事件标志，通知 LoRa 任务有数据需要发送
    osEventFlagsSet(s_lora_event_flags, EVT_FLAG_LORA_TX_REQ);
    
    return true;
}

/**
 * @brief 初始化 LoRa 应用层 OS 对象
 */
void LoRa_APP_Init(void)
{
    printf("LoRa APP Init\r\n");

    // 创建互斥锁，用于保护对 LoRa 硬件的访问
    s_lora_access_mutex = osMutexNew(NULL);
    if (s_lora_access_mutex == NULL) {
        printf("LoRa APP Mutex Create Failed\r\n");
        return;
    }
    printf("LoRa APP Mutex Create OK\r\n");

    // 创建事件标志组
    s_lora_event_flags = osEventFlagsNew(NULL);
    if (s_lora_event_flags == NULL) {
        printf("LoRa APP Event Flags Create Failed\r\n");
        return;
    }
    printf("LoRa APP Event Flags Create OK\r\n");

    // 创建发送消息队列
    s_lora_tx_queue = osMessageQueueNew(LORA_TX_QUEUE_MSG_COUNT, LORA_TX_QUEUE_MSG_SIZE, NULL);
    if (s_lora_tx_queue == NULL) {
        printf("LoRa APP TX Queue Create Failed\r\n");
        return;
    }
    printf("LoRa APP TX Queue Create OK\r\n");

    // 创建 LoRa 应用任务
    const osThreadAttr_t task_attributes = {
        .name = "LoRaAppTask",
        .stack_size = LORA_TASK_STACK_SIZE,
        .priority = (osPriority_t) LORA_TASK_PRIORITY,
    };
    s_lora_app_task_handle = osThreadNew(LoRa_APP_Task, NULL, &task_attributes);
    if (s_lora_app_task_handle == NULL) {
        printf("LoRa APP Task Create Failed\r\n");
    }
    printf("LoRa APP Task Create OK\r\n");
}

/**
 * @brief LoRa DIO0 引脚的外部中断服务函数 (EXTI ISR)
 * @details 当 LoRa 芯片成功接收到一个数据包后，会通过 DIO0 引脚触发此中断。
 *          此函数在中断上下文中执行，必须尽可能快地完成。
 * @param GPIO_Pin 触发中断的引脚号。
 *
 * @note 此函数的唯一职责就是释放信号量以唤醒 `LoRa_APP_Task`。所有耗时操作都应
 *       在任务上下文中进行，以保证系统的实时性。
 */
void LoRa_DIO0_ISR(uint16_t GPIO_Pin)
{
    // 确认是来自 DIO0 的中断
    if (GPIO_Pin == LORA_DIO0_PIN) {
        // [CRITICAL] 移除在中断服务程序中的 printf 调用。
        // 在ISR中调用 printf 是非重入和不安全的，可能导致死锁，是导致系统重启的根源。
        // printf("[LoRa-DBG] ISR: DIO0 Triggered!\r\n");
        
        // 设置接收完成标志位，唤醒 LoRa 任务进行数据处理
        // 此函数在中断上下文中是安全的
        osEventFlagsSet(s_lora_event_flags, EVT_FLAG_LORA_RX_DONE);
    }
}

// ============================================================================
// Private Function Implementations
// ============================================================================

/**
 * @brief LoRa 应用主任务。
 * @details
 *      此任务是 LoRa 数据收发的核心。它采用事件驱动模型，可以被两种事件唤醒：
 *      1.  **接收完成 (RX_DONE)**: 由 `LoRa_DIO0_ISR` 在接收到数据包后设置事件标志。
 *      2.  **发送请求 (TX_REQ)**:  由 `LoRa_APP_Send` 在向队列中添加新消息后设置事件标志。
 *
 *      为了保证发送和接收操作不会相互干扰（LoRa芯片是半双工），任务使用一个互斥锁 `s_lora_access_mutex`
 *      来保护所有对 LoRa 硬件的直接访问。
 * 
 *      任务会优先处理发送请求。
 *
 * @param argument RTOS 传入的参数，未使用。
 */
static void LoRa_APP_Task(void *argument)
{
    (void)argument; // suppress unused parameter warning

    printf("LoRa APP Task Started\r\n");

    // 1. 将 LoRa 设置为连续接收模式
    LoRa_startReceiving(&s_lora_handle);

    // 2. 任务主循环
    for (;;) {
        // [EVENT] 等待接收完成或发送请求事件。
        // [FIX] 超时时间必须小于App_Main_Task的监控周期(2000ms)，以确保签到总能及时进行。
        // 设置为1800ms，为任务的其他操作留下200ms的余量。
        uint32_t flags = osEventFlagsWait(s_lora_event_flags, EVT_FLAG_LORA_RX_DONE | EVT_FLAG_LORA_TX_REQ, osFlagsWaitAny, 1800);

        // [LoRa-DBG] Task Woken Up
        printf("[LoRa-DBG] Task Woken Up. Flags: 0x%X\r\n", flags);

        if ((flags & (EVT_FLAG_LORA_RX_DONE | EVT_FLAG_LORA_TX_REQ)) != 0)
        {
            // 获取 LoRa 硬件访问权限
            if (osMutexAcquire(s_lora_access_mutex, osWaitForever) == osOK)
            {
                printf("[LoRa-DBG] Mutex Acquired.\r\n");
                // --- 优先处理发送 ---
                if ((flags & EVT_FLAG_LORA_TX_REQ) || (osMessageQueueGetCount(s_lora_tx_queue) > 0))
                {
                    lora_tx_request_t tx_req;
                    printf("[LoRa-DBG] TX branch entered. Queue count: %u\r\n", osMessageQueueGetCount(s_lora_tx_queue));
                    // 循环处理所有待发送的消息，直到队列为空
                    while (osMessageQueueGet(s_lora_tx_queue, &tx_req, NULL, 0) == osOK)
                    {
                        lora_send_packet(tx_req.buffer, tx_req.length);
                    }
                }

                // --- 处理接收 ---
                if (flags & EVT_FLAG_LORA_RX_DONE)
                {
                    printf("[LoRa-DBG] RX branch entered. Calling LoRa_receive...\r\n");
                    uint8_t received_len = LoRa_receive(&s_lora_handle, s_lora_rx_buffer, LORA_MAX_RAW_PACKET);
                    printf("[LoRa-DBG] LoRa_receive returned %d bytes.\r\n", received_len);

                    if (received_len > 0)
                    {
                        // --- [调试代码] 打印原始 LoRa 数据包 ---
                        printf("[LoRa RAW] Received %d bytes: ", received_len);
                        for(int i = 0; i < received_len; i++) {
                            printf("%02X ", s_lora_rx_buffer[i]);
                        }
                        printf("\r\n");
                        // ------------------------------------

                        process_received_packet(s_lora_rx_buffer, received_len);
                    }
                }

                // 释放 LoRa 硬件访问权限
                osMutexRelease(s_lora_access_mutex);
                printf("[LoRa-DBG] Mutex Released.\r\n");
            }
        }
        
        // [WATCHDOG] 无论是否有 LoRa 事件，都必须进行签到。
        TaskMonitor_CheckIn(TASK_ID_LORA_APP);
    }
}

/**
 * @brief 发送一个 LoRa 数据包 (内部函数)
 * @details 此函数封装了发送的完整流程：发送数据 -> 等待完成 -> 切换回接收模式
 * @note **调用此函数前必须已获取 `s_lora_access_mutex`**
 * 
 * @param data 要发送的数据指针
 * @param len 数据长度
 * @return true 发送成功
 * @return false 发送失败
 */
static bool lora_send_packet(const uint8_t* data, uint8_t len)
{
    printf("[LoRa-DBG] Enter lora_send_packet. Sending %d bytes...\r\n", len);

    // LoRa_transmit 是一个阻塞函数，它会等待发送完成
    if (LoRa_transmit(&s_lora_handle, (uint8_t*)data, len, 500))
    {
        printf("[LoRa-DBG] LoRa_transmit SUCCESS.\r\n");
    }
    else
    {
        printf("[LoRa-DBG] LoRa_transmit FAILED.\r\n");
    }

    // [CRITICAL] 发送完成后，必须立即将 LoRa 切换回接收模式，否则将错过传入的数据包
    printf("[LoRa-DBG] Switching back to RX mode...\r\n");
    LoRa_startReceiving(&s_lora_handle);
    
    return true; // 简化处理，当前版本始终返回 true
}

/**
 * @brief 处理接收到的完整 LoRa 数据包
 * @param data 指向原始数据包的指针
 * @param len 数据包的长度
 */
static void process_received_packet(uint8_t *data, uint8_t len)
{
    lora_parsed_message_t parsed_msg;
    
    printf("[LoRa-DBG] Parsing received packet...\r\n");
    // 解析数据帧
    lora_frame_status_t status = parse_lora_frame(data, len, &parsed_msg);
    if (status != LORA_FRAME_OK) {
        // 解析失败 (例如 CRC 错误)，丢弃该包
        printf("[LoRa-DBG] Packet parse failed! Status: %d\r\n", status);
        return;
    }

    printf("[LoRa-DBG] Packet parsed OK. MsgType: %d, Sender: 0x%04X\r\n", parsed_msg.msg_type, parsed_msg.sender_addr);

    // 根据消息类型，将解析后的数据更新到 DeviceManager
    switch (parsed_msg.msg_type)
    {
        case MSG_TYPE_REPORT_SENSOR:
        {
            // 当收到传感器报告时，首先查询设备类型
            managed_device_t device_info;
            if (DeviceManager_GetDevice(parsed_msg.sender_addr, &device_info))
            {
                if (device_info.device_type == DEVICE_TYPE_INTERNAL_SENSOR)
                {
                    InternalSensorProperties_t sensor_data;
                    if (lora_model_parse_sensor_data_internal(&parsed_msg, &sensor_data)) {
                       DeviceManager_UpdateInternalSensorData(parsed_msg.sender_addr, &sensor_data);
                    }
                }
                else if (device_info.device_type == DEVICE_TYPE_EXTERNAL_SENSOR)
                {
                    ExternalSensorProperties_t sensor_data;
                    if (lora_model_parse_sensor_data_external(&parsed_msg, &sensor_data)) {
                        DeviceManager_UpdateExternalSensorData(parsed_msg.sender_addr, &sensor_data);
                    }
                }
            }
            break;
        }

        case MSG_TYPE_CMD_REPORT_CONFIG:
        {
            ControlNodeProperties_t control_data;
            if (lora_model_parse_control_data(&parsed_msg, &control_data)) {
                DeviceManager_UpdateControlNodeData(parsed_msg.sender_addr, &control_data);
            }
            break;
        }

        case MSG_TYPE_HEARTBEAT:
        {
            // 收到心跳包，可以调用一个函数来更新节点的 is_online 状态和 last_seen_ts
            // 例如: DeviceManager_UpdateHeartbeat(parsed_msg.sender_addr);
            break;
        }

        default:
            // 未知消息类型，忽略
            break;
    }
} 