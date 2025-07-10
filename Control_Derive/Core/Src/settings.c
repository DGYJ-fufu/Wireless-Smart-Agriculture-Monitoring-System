#include "settings.h"
#include "w25qxx.h"
#include "main.h"    // 引入以使用CRC句柄 hcrc
#include <string.h>  // 引入以使用 memcpy
#include "stdio.h"   // 引入以使用 printf

// --- 外部变量声明 ---
extern CRC_HandleTypeDef hcrc;

// --- 私有全局变量 ---
// 用于在内存中缓存一份当前的设置，所有操作都先修改它，然后才保存到Flash
static settings_t g_settings;

// --- 私有函数原型 ---
static W25QXX_Status_t prv_Settings_Load(void);       // 从Flash加载设置到g_settings
static void prv_Settings_LoadDefaults(void);          // 加载默认设置到g_settings
static uint16_t prv_Settings_CalculateCRC16(settings_t* settings); // 计算设置结构体的CRC16

// ======================================================================================
// --- 公共API函数实现 ---
// ======================================================================================

/**
 * @brief  获取一个指向全局内存设置结构体的指针
 */
settings_t* Settings_Get(void)
{
    return &g_settings;
}

/**
 * @brief  初始化设置模块
 */
W25QXX_Status_t Settings_Init(void)
{
    printf("Initializing Settings Module...\r\n");
    // 1. 初始化Flash硬件
    if (W25QXX_Init() != W25QXX_OK) {
        printf("ERROR: W25QXX hardware init failed!\r\n");
        return W25QXX_ERROR;
    }

    // 2. 尝试从Flash加载设置
    if (prv_Settings_Load() != W25QXX_OK) {
        // 如果加载失败 (比如第一次上电，或Flash数据损坏)，则加载并保存一套默认值
        printf("NOTICE: No valid settings found in flash. Loading defaults.\r\n");
        prv_Settings_LoadDefaults();
        if (Settings_Save() != W25QXX_OK) {
            printf("ERROR: Failed to save default settings to flash!\r\n");
            return W25QXX_ERROR;
        }
    } else {
        printf("SUCCESS: Settings loaded from flash.\r\n");
    }
    return W25QXX_OK;
}

/**
 * @brief  将当前内存中的设置保存到QSPI Flash
 */
W25QXX_Status_t Settings_Save(void)
{
    printf("Saving settings to flash...\r\n");
    
    // 1. 在保存前，计算并更新CRC校验码
    g_settings.crc16 = prv_Settings_CalculateCRC16(&g_settings);

    // 2. 先擦除将要写入的扇区
    if (W25QXX_Erase_Sector(SETTINGS_FLASH_ADDRESS) != W25QXX_OK) {
        printf("ERROR: Failed to erase sector for settings!\r\n");
        return W25QXX_ERROR;
    }

    // 3. 写入整个设置结构体
    if (W25QXX_Write((uint8_t*)&g_settings, SETTINGS_FLASH_ADDRESS, sizeof(settings_t)) != W25QXX_OK) {
        printf("ERROR: Failed to write settings to flash!\r\n");
        return W25QXX_ERROR;
    }
    
    printf("Save successful.\r\n");
    return W25QXX_OK;
}

// ======================================================================================
// --- 私有辅助函数实现 ---
// ======================================================================================

/**
 * @brief  从Flash中读取设置并进行校验
 * @return W25QXX_Status_t: 如果数据有效则返回 W25QXX_OK
 */
static W25QXX_Status_t prv_Settings_Load(void)
{
    settings_t temp_settings; // 创建一个临时结构体来接收Flash数据
    
    // 1. 从Flash读取整个设置结构体大小的数据
    if (W25QXX_Read((uint8_t*)&temp_settings, SETTINGS_FLASH_ADDRESS, sizeof(settings_t)) != W25QXX_OK) {
        return W25QXX_ERROR; // 读取失败
    }

    // 2. 校验魔术字，这是最快、最基本的检查
    if (temp_settings.magic_number != SETTINGS_MAGIC_NUMBER) {
        printf("DEBUG: Magic number mismatch.\r\n");
        return W25QXX_ERROR; // 魔术字不对，说明不是有效的设置数据
    }

    // 3. 校验CRC，确保数据在存储过程中没有被意外损坏
    uint16_t calculated_crc = prv_Settings_CalculateCRC16(&temp_settings);
    if (calculated_crc != temp_settings.crc16) {
        printf("DEBUG: CRC-16 mismatch.\r\n");
        return W25QXX_ERROR; // CRC不对，说明数据已损坏
    }

    // 4. 如果所有校验都通过，则将从Flash读取的有效数据拷贝到全局设置变量中
    memcpy(&g_settings, &temp_settings, sizeof(settings_t));
    
    return W25QXX_OK; // 加载成功
}

/**
 * @brief  将一套硬编码的默认值加载到内存中的g_settings变量
 */
static void prv_Settings_LoadDefaults(void)
{
    g_settings.magic_number     = SETTINGS_MAGIC_NUMBER;
    g_settings.device_id        = DEFAULT_DEVICE_ID;
    g_settings.lora_frequency   = DEFAULT_LORA_FREQ;
    g_settings.fan_status       = false;
    g_settings.pump_status      = false;
    g_settings.light_status     = false;
    g_settings.fan_speed        = DEFAULT_FAN_SPEED;
    g_settings.pump_speed       = DEFAULT_PUMP_SPEED;
    // 注意: CRC校验码在此处不计算，它会在调用 Settings_Save() 时自动计算并填充
}

/**
 * @brief  使用STM32硬件CRC外设计算设置结构体的CRC16值 (字节输入)
 */
static uint16_t prv_Settings_CalculateCRC16(settings_t* settings)
{
    // 计算整个结构体的CRC，但是要排除最后2个字节的CRC字段本身
    uint32_t len_bytes = sizeof(settings_t) - sizeof(uint16_t);
    uint8_t* p_data = (uint8_t*)settings;

    // 复位CRC计算单元，确保从初始值开始
    __HAL_CRC_DR_RESET(&hcrc);

    // 以字节为单位，将数据送入硬件CRC计算单元
    for(uint32_t i = 0; i < len_bytes; i++)
    {
        // 强制类型转换为8位指针，并写入DR寄存器的8位地址
        // 这可以确保每次只写入一个字节，适配硬件CRC的字节输入模式
        *(__IO uint8_t*)(&(hcrc.Instance->DR)) = p_data[i];
    }

    // 从DR寄存器读取最终的16位CRC结果
    return (uint16_t)(hcrc.Instance->DR);
} 