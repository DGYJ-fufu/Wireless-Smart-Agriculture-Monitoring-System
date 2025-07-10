/**
 * @file at_handler.h
 * @author Your Name
 * @brief 高性能、双缓冲、线程安全的AT命令处理器
 * @version 3.0
 * @date 2025-06-09
 *
 * @copyright Copyright (c) 2025
 *
 * @par V3.0 (2025-06-09)
 *      - 升级为软件双缓冲DMA接收机制，实现无缝数据接收，极大提升高波特率下的稳定性。
 *      - 全面适配STM32U5系列HAL库，使用 `HAL_UARTEx_ReceiveToIdle_DMA` API。
 *      - 优化了解析逻辑，兼容多种AT响应格式，增强了驱动的通用性和健壮性。
 * 
 * @par 模块功能:
 *      - 基于FreeRTOS，提供完全线程安全的AT命令处理框架。
 *      - 使用 **双缓冲DMA** 和 **UART空闲中断** 高效接收数据，CPU负载极低。
 *      - 支持同步阻塞命令（发送并等待响应）和异步非阻塞命令。
 *      - 支持URC（主动上报信息）的动态注册和回调处理。
 *      - 内置环形缓冲区（Ring Buffer）来解耦高速的DMA数据接收与较慢的RTOS任务解析。
 */

#ifndef __AT_HANDLER_H
#define __AT_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32u5xx_hal.h"
#include "cmsis_os2.h"

// --- Public Configuration ---

// 定义用于接收 AT 响应的静态缓冲区大小
// 需要足够大以容纳任何单条 AT 响应行
#define AT_RESPONSE_LINE_BUFFER_SIZE    512
#define AT_TX_BUFFER_SIZE               1024

// --- Public Enums and Structs ---

/**
 * @brief AT 命令执行状态码
 */
typedef enum {
    AT_OK = 0,      // 命令成功执行，并收到 "OK"
    AT_ERROR,       // 命令执行失败，并收到 "ERROR" 或类似错误码
    AT_TIMEOUT,     // 在指定时间内未收到最终响应
    AT_BUFFER_FULL, // 提供的用于存储响应的缓冲区已满
    AT_UART_ERROR   // UART 发送或接收配置错误
} AT_Status_t;

/**
 * @brief AT模块的主句柄结构体
 * @note  此结构体由 AT_Init 函数进行全自动初始化，用户无需直接操作其成员。
 */
typedef struct {
    UART_HandleTypeDef* huart;             // 指向用于 AT 通信的 UART 句柄
    DMA_HandleTypeDef*  hdma_rx;           // 指向 UART RX 的 DMA 句柄
    DMA_HandleTypeDef*  hdma_tx;           // 指向 UART TX 的 DMA 句柄

    osMutexId_t         cmd_mutex;         // 命令发送互斥锁
    osSemaphoreId_t     response_sem;      // 同步响应信号量
    osSemaphoreId_t     tx_cplt_sem;       // DMA发送完成信号量
    osThreadId_t        rx_task_handle;    // AT 接收解析任务句柄

    // --- DMA 发送机制 ---
    uint8_t*            tx_buffer;         // DMA 发送缓冲区
    uint16_t            tx_buffer_size;    // DMA 发送缓冲区大小

    // --- 双缓冲DMA接收机制 ---
    uint8_t*            dma_rx_buffer_a;   // DMA 接收缓冲区 A
    uint8_t*            dma_rx_buffer_b;   // DMA 接收缓冲区 B
    uint8_t*            current_dma_buffer; // 指向当前正在被DMA使用的缓冲区
    uint16_t            dma_rx_buffer_size;  // 单个DMA接收缓冲区大小
    
    // --- 内部环形缓冲区，用于解耦 ---
    uint8_t*            ring_buffer;       // 环形缓冲区指针
    uint16_t            ring_buffer_size;    // 环形缓冲区大小
    volatile uint16_t   ring_buffer_head;  // 环形缓冲区头指针（写入）
    volatile uint16_t   ring_buffer_tail;  // 环形缓冲区尾指针（读取）
    
    // 响应处理
    char*               p_response_buf;    // 指向用户提供的用于存储响应的缓冲区
    uint16_t            response_buf_size; // 用户缓冲区的总大小
    uint16_t            response_len;      // 当前已存入用户缓冲区的响应长度
    AT_Status_t         last_status;       // 上一个命令的执行状态

    // URC (Unsolicited Result Code) 处理
    const void*         urc_table;         // 指向 URC 回调函数表的指针
    uint8_t             urc_table_size;    // URC 表的大小

} AT_Handler_t;

/**
 * @brief URC（主动上报信息）回调函数指针类型
 * @param urc_line 模块上报的完整URC字符串
 */
typedef void (*urc_callback_t)(const char* urc_line);

/**
 * @brief URC回调注册表条目结构体
 */
typedef struct {
    const char* urc_prefix;   ///< 要匹配的URC前缀, 例如 "+SIM READY"
    urc_callback_t callback;  ///< 匹配成功后要执行的回调函数
} AT_URC_t;


// --- Public Function Prototypes ---

/**
 * @brief 初始化AT命令处理器
 * @details
 *        此函数是使用本模块的第一步。它会完成所有资源的初始化，
 *        包括：分配内存、创建RTOS任务和同步对象、启动DMA和UART中断，
 *        并发送基础AT指令以确保模块通信正常。
 * @param handle 指向要初始化的 AT_Handler_t 结构体的指针
 * @param huart_at 用于AT通信的UART句柄
 * @param hdma_uart_rx 用于UART RX的DMA句柄
 * @param hdma_uart_tx 用于UART TX的DMA句柄
 * @retval osStatus 成功返回 osOK, 失败返回对应的错误码
 */
osStatus_t AT_Init(AT_Handler_t* handle, UART_HandleTypeDef* huart_at, DMA_HandleTypeDef* hdma_uart_rx, DMA_HandleTypeDef* hdma_uart_tx);

/**
 * @brief 反初始化AT命令处理器
 * @details
 *        安全地终止所有任务、删除同步对象并释放所有已分配的内存。
 *        主要用于在系统需要重启通信模块时，进行彻底的资源清理。
 * @param handle 指向要反初始化的 AT_Handler_t 结构体的指针
 */
void AT_DeInit(AT_Handler_t* handle);

/**
 * @brief 发送AT命令并阻塞等待其最终响应 (OK/ERROR)。
 * @details
 *        这是一个同步函数。它会阻塞当前任务，直到收到 "OK", "ERROR"
 *        或超时。期间收到的其他信息行将被存入 response_buf。
 * @param handle AT句柄
 * @param cmd 要发送的AT命令字符串 (无需包含 \\r\\n)
 * @param timeout_ms 等待响应的超时时间 (毫秒)
 * @param response_buf (可选, 可为NULL) 用于存储模块返回的中间响应内容的缓冲区。
 * @param buf_len response_buf 的大小
 * @retval AT_Status_t 命令执行的状态
 */
AT_Status_t AT_SendCommand(AT_Handler_t* handle, const char* cmd, uint32_t timeout_ms, 
                           char* response_buf, uint16_t buf_len);

/**
 * @brief 发送一个简单的AT命令，它只关心最终的 "OK" 或 "ERROR"。
 * @details
 *        这是 AT_SendCommand 的一个常用简化版，它不接收任何中间响应。
 * @param handle AT句柄
 * @param cmd 要发送的AT命令字符串
 * @param timeout_ms 超时时间 (毫秒)
 * @retval AT_Status_t 命令执行的状态
 */
AT_Status_t AT_SendBasicCommand(AT_Handler_t* handle, const char* cmd, uint32_t timeout_ms);

/**
 * @brief 发送一个原始AT命令，不等待任何响应（"发后即忘"）。
 * @details
 *        这是一个非阻塞函数。它会获取命令互斥锁以保证发送的原子性，
 *        然后立即释放锁并返回。主要用于不需要关心响应的场景，例如在
 *        URC处理流程中发送响应，以避免阻塞后续的URC。
 * @param handle AT句柄
 * @param cmd 要发送的完整AT命令字符串 (需要自行包含 \\r\\n)
 * @retval AT_Status_t 命令发送的状态 (只关心发送动作是否成功)
 */
AT_Status_t AT_SendRaw(AT_Handler_t* handle, const char* cmd);

/**
 * @brief 串口接收事件回调函数 (由应用层在 `HAL_UARTEx_RxEventCallback` 中调用)
 * @note  这是驱动整个模块数据接收的核心，必须被正确调用。
 * @param handle AT句柄
 * @param huart 触发中断的UART句柄
 * @param size 本次DMA传输的数据长度
 */
void AT_UartIdleCallback(AT_Handler_t* handle, UART_HandleTypeDef *huart, uint16_t size);

/**
 * @brief 串口发送完成回调函数 (由应用层在 `HAL_UART_TxCpltCallback` 中调用)
 * @note  此函数用于在DMA发送完成后释放信号量
 * @param handle AT句柄
 * @param huart 触发中断的UART句柄
 */
void AT_UartTxCpltCallback(AT_Handler_t *handle, UART_HandleTypeDef *huart);

/**
 * @brief 注册一个URC回调函数表。
 * @param handle AT句柄
 * @param table 指向 AT_URC_t 结构体数组的指针
 * @param table_size 数组中的元素数量
 */
void AT_RegisterURCCallbacks(AT_Handler_t* handle, const AT_URC_t* table, uint8_t table_size);

/*
 * This function is removed as it's incompatible with the task-based processing logic.
uint16_t AT_GetRawReceivedData(AT_Handler_t *at_handler, uint8_t *buffer, uint16_t length);
*/

#ifdef __cplusplus
}
#endif

#endif /* __AT_HANDLER_H */ 
