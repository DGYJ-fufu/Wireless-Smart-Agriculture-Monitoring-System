#ifndef __CONFIG_MANAGER_H
#define __CONFIG_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "lora_protocol.h"

// 定义在W25Q32 Flash中用于存储配置的特定地址。
// 我们使用第一个扇区的起始位置，地址为0。
#define CONFIG_STORAGE_ADDRESS 0x000000

#define CONFIG_MAGIC_NUMBER 0x5A5A5A5A // “魔数”，用于验证Flash中的数据是否有效

// 默认配置值
#define DEFAULT_LORA_FREQUENCY 433U          								 // 默认LoRa频率：433 MHz
#define DEFAULT_DEVICE_ID      DEVICE_TYPE_SENSOR_Internal   // 默认设备ID

/**
 * @brief  设备配置数据结构体。
 * @note   此结构体总大小为12字节。
 */
typedef struct {
    uint32_t magic_number;    // 4字节：用于验证结构体有效性的“魔数”
    uint32_t lora_frequency;  // 4字节：LoRa频率，单位MHz (例如: 433)
    uint16_t device_id;       // 2字节：设备ID (范围 0-65535)
    uint16_t crc16;           // 2字节：针对前10个字节计算的 CRC16-Modbus 校验和
} DeviceConfig_t;

// 全局变量，用于在整个应用程序中保存和访问当前的设备配置
extern DeviceConfig_t g_DeviceConfig;

/**
 * @brief  从W25Q32 Flash中加载配置到全局变量 g_DeviceConfig。
 * @retval true  - 如果成功加载了有效的配置。
 * @retval false - 如果Flash中的数据无效。在这种情况下，g_DeviceConfig 会被填充为默认值，
 *                 之后应调用 Config_Save() 将默认值保存。
 */
bool Config_Load(void);

/**
 * @brief  将当前的 g_DeviceConfig 保存到W25Q32 Flash中。
 *         此函数会自动计算CRC校验和，并写入整个结构体。
 * @retval true  - 如果配置保存成功。
 * @retval false - 其他情况。
 */
bool Config_Save(void);

/**
 * @brief 使用硬编码的默认值来填充 g_DeviceConfig。
 */
void Config_SetDefault(void);

#endif // __CONFIG_MANAGER_H 