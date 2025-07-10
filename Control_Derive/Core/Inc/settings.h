#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include <stdbool.h>
#include "w25qxx.h"

// --- 可配置宏定义 ---
#define SETTINGS_FLASH_ADDRESS  0x000000      // 设置信息在Flash中的存储起始地址 (第0扇区)
#define SETTINGS_MAGIC_NUMBER   0xA5A5BEEF    // "魔术字"，一个唯一的数字，用于识别Flash中的数据是否有效

// --- 默认值定义 ---
#define DEFAULT_LORA_FREQ       433           // 默认LoRa频率
#define DEFAULT_DEVICE_ID       0x12          // 默认设备ID (对应 DEVICE_TYPE_CONTROL)
#define DEFAULT_FAN_SPEED       100           // 默认风扇速度
#define DEFAULT_PUMP_SPEED      100           // 默认水泵速度

/**
 * @brief  用于保存所有持久化设置的结构体
 * @note   此结构体将直接写入Flash，因此应使用packed属性确保内存对齐不会影响大小
 */
typedef struct __attribute__((packed)) {
    uint32_t magic_number;      // 魔术字 - 用于验证Flash中的数据是否为有效设置
    
    // --- 用户可配置的设置项 ---
    uint8_t  device_id;         // 本机设备ID
    uint16_t lora_frequency;    // LoRa频率 (410-525 MHz)
    
    bool     fan_status;        // 风扇开关状态
    bool     pump_status;       // 水泵开关状态
    bool     light_status;      // 补光灯开关状态
    
    uint8_t  fan_speed;         // 风扇速度
    uint8_t  pump_speed;        // 水泵速度

    // --- 数据完整性校验 ---
    uint16_t crc16;             // CRC16校验码 - 必须是最后一个成员，以简化计算
} settings_t;


/**
 * @brief  初始化设置模块
 * @detail - 初始化QSPI Flash驱动
 *         - 尝试从Flash中加载设置
 *         - 如果加载失败(没有有效设置)，则加载并保存一套默认值
 * @return W25QXX_Status_t: 成功返回 W25QXX_OK
 */
W25QXX_Status_t Settings_Init(void);

/**
 * @brief  将当前内存中的设置保存到QSPI Flash
 * @detail 此函数会先执行扇区擦除，然后再写入新的数据
 * @return W25QXX_Status_t: 成功返回 W25QXX_OK
 */
W25QXX_Status_t Settings_Save(void);

/**
 * @brief  获取一个指向全局内存设置结构体的指针
 * @return settings_t*: 指向当前设置的指针
 */
settings_t* Settings_Get(void);

#endif // SETTINGS_H 