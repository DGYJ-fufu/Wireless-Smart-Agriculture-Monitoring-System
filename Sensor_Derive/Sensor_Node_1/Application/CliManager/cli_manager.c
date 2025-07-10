#include "cli_manager.h"
#include "usart.h"
#include "config_manager.h"
#include "main.h" // For HAL_NVIC_SystemReset

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern DeviceConfig_t g_DeviceConfig;

void CLI_Process(void)
{
    if (!g_usart1_new_data_flag)
    {
        return; // 没有新数据，直接返回
    }

    // --- 添加一个保护，防止接收到的数据长度为零 ---
    if (g_usart1_rx_len == 0)
    {
        g_usart1_new_data_flag = false; // 清除旧标志位
        memset(g_usart1_rx_buffer, 0, USART1_RX_BUFFER_SIZE); // 清空缓冲区
        USART1_Start_DMA_Reception(); // 重新启动DMA接收
        return; // 忽略此次事件
    }

    // 从接收缓冲区创建一个以空字符结尾的字符串
    char cmd_buffer[USART1_RX_BUFFER_SIZE + 1];
    memcpy(cmd_buffer, g_usart1_rx_buffer, g_usart1_rx_len);
    cmd_buffer[g_usart1_rx_len] = '\0';

    // 立即清除标志位
    g_usart1_new_data_flag = false;

    // --- 在重新启动DMA前，手动清空缓冲区 ---
    memset(g_usart1_rx_buffer, 0, USART1_RX_BUFFER_SIZE);

    printf("Received command: %s\r\n", cmd_buffer);

    // --- 解析并执行指令 ---
    if (strncmp(cmd_buffer, "AT+FREQ=", 8) == 0)
    {
        uint32_t new_freq = atoi(cmd_buffer + 8);
        if (new_freq >= 137 && new_freq <= 1020)
        {
            g_DeviceConfig.lora_frequency = new_freq;
            printf("OK: Set LoRa Frequency to %d MHz.\r\n", g_DeviceConfig.lora_frequency);
        }
        else
        {
            printf("ERROR: Invalid frequency. Must be between 137 and 1020 MHz.\r\n");
        }
    }
    else if (strncmp(cmd_buffer, "AT+ID=", 6) == 0)
    {
        // 使用 strtol 函数将字符串（从第6个字符开始）解析为16进制无符号整数
        uint16_t new_id = (uint16_t)strtol(cmd_buffer + 6, NULL, 16);
        g_DeviceConfig.device_id = new_id;
        printf("OK: Set Device ID to 0x%X.\r\n", g_DeviceConfig.device_id);
    }
    else if (strncmp(cmd_buffer, "AT+SAVE", 7) == 0)
    {
        if (Config_Save())
        {
            printf("OK: Configuration saved to Flash.\r\n");
        }
        else
        {
            printf("ERROR: Failed to save configuration to Flash.\r\n");
        }
    }
    else if (strncmp(cmd_buffer, "AT+RESET", 8) == 0)
    {
        printf("OK: System will reset now.\r\n");
        HAL_Delay(100); // 留出一点时间，确保printf能完成输出
        HAL_NVIC_SystemReset();
    }
    else if (strncmp(cmd_buffer, "AT+CONFIG?", 10) == 0)
    {
        printf("Current Config -> ID: 0x%X, Freq: %d MHz\r\n", g_DeviceConfig.device_id, g_DeviceConfig.lora_frequency);
    }
    else
    {
        printf("ERROR: Unknown command.\r\n");
    }

    // --- 为下一次指令接收，重新启动DMA ---
    USART1_Start_DMA_Reception();
} 