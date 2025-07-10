#ifndef __SHT40_H__
#define __SHT40_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "i2c.h"

// --- SHT40 I2C 地址 ---
// SHT40的7位I2C地址固定为0x44。
#define SHT40_ADDRESS (0x44 << 1) // 8位写地址

// --- SHT40 命令集 ---
#define SHT40_CMD_MEASURE_HPM       0xFD // 测量温湿度 - 高精度模式 (High Precision Measurement)
#define SHT40_CMD_MEASURE_MPM       0xF6 // 测量温湿度 - 中精度模式 (Medium Precision Measurement)
#define SHT40_CMD_MEASURE_LPM       0xE0 // 测量温湿度 - 低精度模式 (Low Precision Measurement)
#define SHT40_CMD_READ_SERIAL       0x89 // 读取传感器的唯一序列号
#define SHT40_CMD_HEATER_200MW_1S   0x39 // 启动加热器：200mW，持续1秒
#define SHT40_CMD_HEATER_200MW_01S  0x32 // 启动加热器：200mW，持续0.1秒
#define SHT40_CMD_HEATER_110MW_1S   0x2F // 启动加热器：110mW，持续1秒
#define SHT40_CMD_HEATER_110MW_01S  0x24 // 启动加热器：110mW，持续0.1秒
#define SHT40_CMD_HEATER_20MW_1S    0x1E // 启动加热器：20mW，持续1秒
#define SHT40_CMD_HEATER_20MW_01S   0x15 // 启动加热器：20mW，持续0.1秒
#define SHT40_CMD_RESET             0x94 // 软复位

/**
 * @brief  从SHT40传感器读取温度和湿度数据。
 * @param  temperature: 指向用于存储温度值的double类型变量的指针 (单位: °C)。
 * @param  humidity: 指向用于存储相对湿度值的double类型变量的指针 (单位: %RH)。
 * @retval 0  - 读取和校验成功。
 * @retval -1 - I2C通信失败或CRC校验失败。
 */
uint8_t SHT40_Read_RHData(double *temperature,double *humidity);


#ifdef __cplusplus
}
#endif

#endif /* __SHT40_H__ */
	
