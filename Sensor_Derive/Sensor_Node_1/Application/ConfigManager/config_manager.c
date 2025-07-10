#include "config_manager.h"
#include "w25qxx.h"
#include "crc.h"
#include <string.h> // 用于 memcpy 和 memcmp

// 全局配置实例在此文件中定义
DeviceConfig_t g_DeviceConfig;

// 外部的HAL CRC句柄，在 crc.c 中定义
extern CRC_HandleTypeDef hcrc;

// --- 私有函数 ---

/**
 * @brief  为给定的配置结构体计算 CRC16-Modbus 校验和。
 * @param  config: 指向配置结构体的指针。
 * @retval 计算出的CRC16值。
 */
static uint16_t Config_CalculateCRC(DeviceConfig_t* config)
{
    // 需要进行校验和计算的数据是结构体的前10个字节。
    // (magic_number, lora_frequency, device_id)
    const size_t data_len = sizeof(uint32_t) * 2 + sizeof(uint16_t);
    uint8_t *p_data = (uint8_t*)config;

    // 将CRC计算单元的初值复位 (对于CRC16-Modbus，初值为0xFFFF)
    __HAL_CRC_DR_RESET(&hcrc);

    // 将数据逐字节喂给CRC外设进行计算
    for (size_t i = 0; i < data_len; i++)
    {
        // 直接向数据寄存器(DR)写入8位数据
        *(__IO uint8_t *)(&(hcrc.Instance->DR)) = p_data[i];
    }
    
    // 硬件会自动执行在CubeMX中配置的输出反转。
    // 直接读取数据寄存器(DR)即可获得最终的CRC结果。
    return (uint16_t)READ_REG(hcrc.Instance->DR);
}


// --- 公有函数 ---

void Config_SetDefault(void)
{
    g_DeviceConfig.magic_number = CONFIG_MAGIC_NUMBER;
    g_DeviceConfig.lora_frequency = DEFAULT_LORA_FREQUENCY;
    g_DeviceConfig.device_id = DEFAULT_DEVICE_ID;
    g_DeviceConfig.crc16 = 0; // CRC值会在调用 Config_Save() 保存前自动计算
}

bool Config_Load(void)
{
    DeviceConfig_t tempConfig;

    // 1. 从W25QXX Flash中读取配置数据
    W25QXX_Read_Data((uint8_t*)&tempConfig, CONFIG_STORAGE_ADDRESS, sizeof(DeviceConfig_t));

    // 2. 检查"魔数"，判断Flash中的数据是否有效或已被初始化
    if (tempConfig.magic_number != CONFIG_MAGIC_NUMBER)
    {
        // 数据无效或Flash为空，使用默认值填充全局配置
        Config_SetDefault();
        return false;
    }

    // 3. 校验CRC，确保数据完整性
    uint16_t calculated_crc = Config_CalculateCRC(&tempConfig);
    if (calculated_crc != tempConfig.crc16)
    {
        // CRC不匹配，说明数据已损坏。使用默认值填充全局配置
        Config_SetDefault();
        return false;
    }

    // 4. 配置有效，将其从临时缓冲复制到全局结构体中
    memcpy(&g_DeviceConfig, &tempConfig, sizeof(DeviceConfig_t));
    return true;
}

bool Config_Save(void)
{
    // 1. 确保"魔数"已被设置
    g_DeviceConfig.magic_number = CONFIG_MAGIC_NUMBER;

    // 2. 计算并更新当前全局配置的CRC值
    g_DeviceConfig.crc16 = Config_CalculateCRC(&g_DeviceConfig);

    // 3. 在写入前，擦除目标Flash扇区。
    //    W25QXX_Erase_Sector 需要的是扇区编号，而不是字节地址。
    //    对于W25Q32，扇区大小为4096字节。
    uint32_t sector_num = CONFIG_STORAGE_ADDRESS / 4096;
    W25QXX_Erase_Sector(sector_num);

    // 4. 将整个配置结构体写入Flash
    W25QXX_Write_Data((uint8_t*)&g_DeviceConfig, CONFIG_STORAGE_ADDRESS, sizeof(DeviceConfig_t));

    // 5. 强烈建议：读回刚刚写入的数据，并与源数据进行比对，以校验写入操作是否真正成功
    DeviceConfig_t verifyConfig;
    W25QXX_Read_Data((uint8_t*)&verifyConfig, CONFIG_STORAGE_ADDRESS, sizeof(DeviceConfig_t));

    if (memcmp(&g_DeviceConfig, &verifyConfig, sizeof(DeviceConfig_t)) == 0)
    {
        return true; // 保存成功
    }
    else
    {
        return false; // 保存失败 (读回的数据与源数据不一致)
    }
} 