#include "at_handler.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "FreeRTOS.h"

/* Private Defines -----------------------------------------------------------*/
#define AT_DMA_RX_BUFFER_SIZE 512    // 为获得最佳性能，此大小应能容纳最常见的AT响应
#define AT_RING_BUFFER_SIZE   2048   // 环形缓冲区，应足够大以应对突发数据
#define AT_RX_TASK_STACK_SIZE 2048   // AT接收解析任务的堆栈大小
#define AT_RX_TASK_PRIORITY   (osPriorityHigh) // AT接收解析任务的优先级

/* Private Function Prototypes -----------------------------------------------*/
static void at_rx_task(void *argument);
static void process_line(AT_Handler_t *handle, const char *line);

/* Public Functions ----------------------------------------------------------*/

osStatus_t AT_Init(AT_Handler_t *handle, UART_HandleTypeDef *huart_at, DMA_HandleTypeDef *hdma_uart_rx, DMA_HandleTypeDef *hdma_uart_tx)
{
    if (!handle || !huart_at || !hdma_uart_rx || !hdma_uart_tx)
    {
        return osErrorParameter;
    }

    memset(handle, 0, sizeof(AT_Handler_t));
    handle->huart = huart_at;
    handle->hdma_rx = hdma_uart_rx;
    handle->hdma_tx = hdma_uart_tx;

    // 1. 为DMA缓冲区、环形缓冲区和发送缓冲区分配内存
    handle->dma_rx_buffer_a = pvPortMalloc(AT_DMA_RX_BUFFER_SIZE);
    handle->dma_rx_buffer_b = pvPortMalloc(AT_DMA_RX_BUFFER_SIZE);
    handle->ring_buffer = pvPortMalloc(AT_RING_BUFFER_SIZE);
    handle->tx_buffer = pvPortMalloc(AT_TX_BUFFER_SIZE);

    if (!handle->dma_rx_buffer_a || !handle->dma_rx_buffer_b || !handle->ring_buffer || !handle->tx_buffer)
    {
        if (handle->dma_rx_buffer_a) vPortFree(handle->dma_rx_buffer_a);
        if (handle->dma_rx_buffer_b) vPortFree(handle->dma_rx_buffer_b);
        if (handle->ring_buffer) vPortFree(handle->ring_buffer);
        if (handle->tx_buffer) vPortFree(handle->tx_buffer);
        return osErrorNoMemory;
    }
    handle->dma_rx_buffer_size = AT_DMA_RX_BUFFER_SIZE;
    handle->ring_buffer_size = AT_RING_BUFFER_SIZE;
    handle->tx_buffer_size = AT_TX_BUFFER_SIZE;

    // 2. 创建RTOS对象
    handle->cmd_mutex = osMutexNew(NULL);
    handle->response_sem = osSemaphoreNew(1, 0, NULL); // 命令响应信号量, 初始为0
    handle->tx_cplt_sem = osSemaphoreNew(1, 1, NULL);  // 发送完成信号量, 初始为1 (可用)

    if (!handle->cmd_mutex || !handle->response_sem || !handle->tx_cplt_sem)
    {
        // 创建失败
        vPortFree(handle->dma_rx_buffer_a);
        vPortFree(handle->dma_rx_buffer_b);
        vPortFree(handle->ring_buffer);
        vPortFree(handle->tx_buffer);
        if (handle->cmd_mutex) osMutexDelete(handle->cmd_mutex);
        if (handle->response_sem) osSemaphoreDelete(handle->response_sem);
        if (handle->tx_cplt_sem) osSemaphoreDelete(handle->tx_cplt_sem);
        return osErrorResource;
    }

    const osThreadAttr_t at_rx_task_attributes = {
        .name = "at_rx_task",
        .stack_size = AT_RX_TASK_STACK_SIZE,
        .priority = AT_RX_TASK_PRIORITY,
    };
    handle->rx_task_handle = osThreadNew(at_rx_task, handle, &at_rx_task_attributes);

    if (!handle->rx_task_handle)
    {
        vPortFree(handle->dma_rx_buffer_a);
        vPortFree(handle->dma_rx_buffer_b);
        vPortFree(handle->ring_buffer);
        vPortFree(handle->tx_buffer);
        if (handle->cmd_mutex) osMutexDelete(handle->cmd_mutex);
        if (handle->response_sem) osSemaphoreDelete(handle->response_sem);
        if (handle->tx_cplt_sem) osSemaphoreDelete(handle->tx_cplt_sem);
        return osErrorResource;
    }
    
    // 3. 启动双缓冲DMA接收
    __HAL_UART_CLEAR_IT(handle->huart, UART_CLEAR_IDLEF); // 清除可能存在的旧标志
    
    // 首次启动DMA接收，使用缓冲区A
    handle->current_dma_buffer = handle->dma_rx_buffer_a; 
    HAL_UARTEx_ReceiveToIdle_DMA(handle->huart, handle->current_dma_buffer, handle->dma_rx_buffer_size);

    return osOK;
}

/**
 * @brief 反初始化 AT 模块句柄
 */
void AT_DeInit(AT_Handler_t *handle)
{
    if (!handle)
    {
        return;
    }

    // 停止硬件
    HAL_UART_DMAStop(handle->huart);
    __HAL_UART_DISABLE_IT(handle->huart, UART_IT_IDLE);

    // 删除RTOS对象
    if (handle->rx_task_handle)
    {
        osThreadTerminate(handle->rx_task_handle);
        handle->rx_task_handle = NULL;
    }
    if (handle->cmd_mutex)
    {
        osMutexDelete(handle->cmd_mutex);
        handle->cmd_mutex = NULL;
    }
    if (handle->response_sem)
    {
        osSemaphoreDelete(handle->response_sem);
        handle->response_sem = NULL;
    }
    if (handle->tx_cplt_sem)
    {
        osSemaphoreDelete(handle->tx_cplt_sem);
        handle->tx_cplt_sem = NULL;
    }

    // 释放内存
    if (handle->dma_rx_buffer_a)
    {
        vPortFree(handle->dma_rx_buffer_a);
        handle->dma_rx_buffer_a = NULL;
    }
    if (handle->dma_rx_buffer_b)
    {
        vPortFree(handle->dma_rx_buffer_b);
        handle->dma_rx_buffer_b = NULL;
    }
    if (handle->ring_buffer)
    {
        vPortFree(handle->ring_buffer);
        handle->ring_buffer = NULL;
    }
    if (handle->tx_buffer)
    {
        vPortFree(handle->tx_buffer);
        handle->tx_buffer = NULL;
    }

    // 清理整个句柄结构体，确保没有残留状态
    memset(handle, 0, sizeof(AT_Handler_t));
}

/**
 * @brief 发送 AT 命令并等待响应
 */
AT_Status_t AT_SendCommand(AT_Handler_t *handle, const char *cmd, uint32_t timeout_ms,
                           char *response_buf, uint16_t buf_len)
{
    if (osMutexAcquire(handle->cmd_mutex, timeout_ms) != osOK)
    {
        return AT_TIMEOUT;
    }

    // 确保上一次DMA发送已完成
    if (osSemaphoreAcquire(handle->tx_cplt_sem, timeout_ms) != osOK) {
        osMutexRelease(handle->cmd_mutex);
        return AT_TIMEOUT;
    }

    // 清理上一次的响应状态
    handle->p_response_buf = response_buf;
    handle->response_buf_size = response_buf ? buf_len : 0;
    handle->response_len = 0;
    if (handle->p_response_buf)
    {
        memset(handle->p_response_buf, 0, handle->response_buf_size);
    }
    
    // 清空响应信号量，以防有残留
    osSemaphoreAcquire(handle->response_sem, 0);

    // 格式化命令到发送缓冲区
    int cmd_len = snprintf((char*)handle->tx_buffer, handle->tx_buffer_size, "%s\r\n", cmd);
    if (cmd_len < 0 || cmd_len >= handle->tx_buffer_size) {
        osSemaphoreRelease(handle->tx_cplt_sem); // 释放TX信号量，因为我们没有启动DMA
        osMutexRelease(handle->cmd_mutex);
        return AT_BUFFER_FULL; // Command too long for buffer
    }

    // 使用DMA发送
    if (HAL_UART_Transmit_DMA(handle->huart, handle->tx_buffer, cmd_len) != HAL_OK)
    {
        osSemaphoreRelease(handle->tx_cplt_sem); // 释放TX信号量
        osMutexRelease(handle->cmd_mutex);
        return AT_UART_ERROR;
    }

    // 等待响应信号量 (由process_line释放)
    if (osSemaphoreAcquire(handle->response_sem, timeout_ms) != osOK)
    {
        handle->last_status = AT_TIMEOUT;
    }
    
    osMutexRelease(handle->cmd_mutex);
    return handle->last_status;
}

/**
 * @brief 发送简单 AT 命令 (这是一个更优的实现，它包装了 AT_SendCommand)
 */
AT_Status_t AT_SendBasicCommand(AT_Handler_t *handle, const char *cmd, uint32_t timeout_ms)
{
    // 调用更底层的 AT_SendCommand, 不关心具体的响应内容 (response_buf = NULL)
    return AT_SendCommand(handle, cmd, timeout_ms, NULL, 0);
}

/**
 * @brief 发送一个原始的AT命令，不等待任何响应 (实现)
 */
AT_Status_t AT_SendRaw(AT_Handler_t* handle, const char* cmd)
{
    if (osMutexAcquire(handle->cmd_mutex, 1000) != osOK)
    {
        return AT_TIMEOUT;
    }

    // 确保上一次DMA发送已完成
    if (osSemaphoreAcquire(handle->tx_cplt_sem, 1000) != osOK) {
        osMutexRelease(handle->cmd_mutex);
        return AT_TIMEOUT;
    }
    
    // 格式化命令到发送缓冲区
    size_t cmd_len = strlen(cmd);
    if (cmd_len >= handle->tx_buffer_size) {
        osSemaphoreRelease(handle->tx_cplt_sem);
        osMutexRelease(handle->cmd_mutex);
        return AT_BUFFER_FULL;
    }
    memcpy(handle->tx_buffer, cmd, cmd_len);

    // 使用DMA发送
    if (HAL_UART_Transmit_DMA(handle->huart, handle->tx_buffer, cmd_len) != HAL_OK)
    {
        osSemaphoreRelease(handle->tx_cplt_sem);
        osMutexRelease(handle->cmd_mutex);
        return AT_UART_ERROR;
    }

    // 对于"发后即忘"的场景，我们依然需要等待本次DMA传输完成，
    // 以确保发送操作的原子性，防止后续命令干扰。
    if(osSemaphoreAcquire(handle->tx_cplt_sem, 1000) != osOK)
    {
        // 如果DMA在1秒内没有完成，这可能是一个硬件问题。
        // 我们依然释放互斥锁，但返回超时错误。
        osMutexRelease(handle->cmd_mutex);
        return AT_TIMEOUT;
    }
    
    // DMA完成后，立即将信号量还回去，让其他任务可以使用
    osSemaphoreRelease(handle->tx_cplt_sem);
    osMutexRelease(handle->cmd_mutex);
    return AT_OK;
}

/**
 * @brief 注册 URC 回调函数表
 */
void AT_RegisterURCCallbacks(AT_Handler_t *handle, const AT_URC_t *table, uint8_t table_size)
{
    handle->urc_table = table;
    handle->urc_table_size = table_size;
}

/* Private Functions ---------------------------------------------------------*/

/**
 * @brief [内部] AT 接收和解析任务
 */
static void at_rx_task(void *argument)
{
    AT_Handler_t *handle = (AT_Handler_t *)argument;
    char line_buffer[AT_RESPONSE_LINE_BUFFER_SIZE];
    uint16_t line_pos = 0;

    while (1)
    {
        if (handle->ring_buffer_head == handle->ring_buffer_tail)
        {
            // 缓冲区为空，等待
            osDelay(10);
            continue;
        }

        while (handle->ring_buffer_head != handle->ring_buffer_tail)
        {
            char c = handle->ring_buffer[handle->ring_buffer_tail];
            handle->ring_buffer_tail = (handle->ring_buffer_tail + 1) % handle->ring_buffer_size;

            if (c == '\n' || c == '\r')
            {
                if (line_pos > 0)
                {
                    line_buffer[line_pos] = '\0';
                    process_line(handle, line_buffer);
                    line_pos = 0;
                }
            }
            else
            {
                if (line_pos < sizeof(line_buffer) - 1)
                {
                    line_buffer[line_pos++] = c;
                }
            }
        }
    }
}

/**
 * @brief [内部使用] UART 空闲中断回调函数
 * @note  这个函数需要在 STM32 的 UART 空闲中断回调中被调用
 */
void AT_UartIdleCallback(AT_Handler_t *handle, UART_HandleTypeDef *huart, uint16_t size)
{
    if (huart->Instance != handle->huart->Instance)
    {
        return;
    }

    uint8_t* completed_buffer = handle->current_dma_buffer; // 刚刚完成接收的是当前buffer

    // 步骤1: 立即切换到另一个缓冲区并启动下一次DMA接收，确保数据流不中断
    if (handle->current_dma_buffer == handle->dma_rx_buffer_a)
    {
        handle->current_dma_buffer = handle->dma_rx_buffer_b;
    }
    else
    {
        handle->current_dma_buffer = handle->dma_rx_buffer_a;
    }
    HAL_UARTEx_ReceiveToIdle_DMA(handle->huart, handle->current_dma_buffer, handle->dma_rx_buffer_size);

    // 步骤2: 处理刚刚在 `completed_buffer` 中接收到的数据
    if (size > 0)
    {
        // 将数据从DMA缓冲区拷贝到环形缓冲区，后续由 `at_rx_task` 进行解析
        uint16_t head = handle->ring_buffer_head;
        uint16_t len_to_copy = (head + size > handle->ring_buffer_size) ? (handle->ring_buffer_size - head) : size;
        
        memcpy(&handle->ring_buffer[head], completed_buffer, len_to_copy);

        if (len_to_copy < size)
        {
            memcpy(&handle->ring_buffer[0], &completed_buffer[len_to_copy], size - len_to_copy);
        }
        
        handle->ring_buffer_head = (handle->ring_buffer_head + size) % handle->ring_buffer_size;
    }
}

/**
 * @brief  [内部使用] UART DMA 发送完成回调函数
 */
void AT_UartTxCpltCallback(AT_Handler_t *handle, UART_HandleTypeDef *huart)
{
    if (huart->Instance == handle->huart->Instance)
    {
        osSemaphoreRelease(handle->tx_cplt_sem);
    }
}

/**
 * @brief  [核心] 解析收到的单行响应或URC
 * @details 此函数是AT任务的核心处理逻辑，它按照以下顺序对传入的行进行解析：
 *          1. **URC优先**: 检查该行是否匹配URC注册表中的任何前缀。如果是，则调用对应的回调函数并立即返回。
 *             这可以确保URC事件（如模组重启）得到最优先处理，并且不会被错误地判断为其他响应。
 *          2. **最终响应**: 如果不是URC，则检查是否为命令的最终响应。此处的逻辑经过增强，可以兼容：
 *             - 标准响应: "OK", "ERROR"
 *             - 带前缀的成功响应: "+CMD OK"
 *             - 带错误码的响应: "+CME ERROR: ...", "+CMS ERROR: ..."
 *             当匹配到最终响应时，会释放命令信号量，以唤醒等待的 `AT_SendCommand` 函数。
 *          3. **中间响应**: 如果既不是URC也不是最终响应，则认为它是命令的中间信息，并将其存入用户提供的缓冲区。
 * @param handle AT处理器实例指针
 * @param line   从环形缓冲区中提取出的、不包含换行符的单行字符串
 */
static void process_line(AT_Handler_t *handle, const char *line)
{
    // 忽略前导的空白字符
    while (*line && isspace((unsigned char)*line))
    {
        line++;
    }
    if (*line == '\0') {
        return; // 是空行，直接返回
    }

    // 步骤1：优先检查是否为已注册的URC
    if (handle->urc_table)
    {
        const AT_URC_t *urc_entry = handle->urc_table;
        for (uint8_t i = 0; i < handle->urc_table_size; i++, urc_entry++)
        {
            if (strstr(line, urc_entry->urc_prefix))
            {
                if (urc_entry->callback)
                {
                    urc_entry->callback(line);
                }
                return; // URC 已处理，此行任务结束
            }
        }
    }

    // 步骤2：如果不是URC，再检查是否为最终响应
    // 兼容 "OK" 和 "+CMD OK" 这两种成功响应格式
    size_t len = strlen(line);
    if ((strcmp(line, "OK") == 0) || (len > 3 && line[0] == '+' && strcmp(line + len - 3, " OK") == 0))
    {
        handle->last_status = AT_OK;
        osSemaphoreRelease(handle->response_sem);
        return;
    }
    // 增强的错误检查：将所有包含 "ERR" 或 "ERROR" 的响应都视为最终错误响应
    if (strstr(line, "ERROR") != NULL || strstr(line, "ERR:") != NULL)
    {
        handle->last_status = AT_ERROR;
        osSemaphoreRelease(handle->response_sem);
        return;
    }
    
    // 步骤3：如果既不是URC也不是最终响应，则视为中间信息，存入缓冲区
    if (handle->p_response_buf && handle->response_len < handle->response_buf_size)
    {
        handle->response_len += snprintf(handle->p_response_buf + handle->response_len,
                                         handle->response_buf_size - handle->response_len,
                                         "%s\r\n", line);
    }
} 
