/**
 * @file bmp280.c
 * @brief BMP280气压和温度传感器驱动程序的实现。
 * @note  本驱动基于I2C通信，并实现了官方数据手册中的数据补偿算法。
 */
#include "bmp280.h"
#include "i2c.h"
#include <string.h>

// 当SDO引脚接地时，BMP280的7位I2C地址为0x76，8位地址为0xEC
#define BMP280_I2C_ADDR (0x76 << 1)

// BMP280寄存器地址定义
#define BMP280_REG_ID           0xD0 // 芯片ID寄存器
#define BMP280_REG_RESET        0xE0 // 软件复位寄存器
#define BMP280_REG_STATUS       0xF3 // 状态寄存器
#define BMP280_REG_CTRL_MEAS    0xF4 // 测量控制寄存器
#define BMP280_REG_CONFIG       0xF5 // 配置寄存器
#define BMP280_REG_PRESS_MSB    0xF7 // 气压测量值的MSB
#define BMP280_REG_CALIB_START  0x88 // 校准参数的起始地址

// 期望从ID寄存器读出的芯片ID
#define BMP280_CHIP_ID          0x58

// 用于存储从传感器PROM中读取的校准参数
typedef struct
{
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} bmp280_calib_param_t;

static bmp280_calib_param_t calib_params;
static int32_t t_fine; // 存储由温度计算出的一个中间值，用于压力补偿

// --- 私有函数 ---

/**
 * @brief  根据传感器原始ADC值和校准参数，计算补偿后的温度。
 * @param  adc_T: 温度的原始ADC读数。
 * @return int32_t: 补偿后的温度值，单位为0.01摄氏度。例如，2534代表25.34°C。
 * @note   此函数是直接从Bosch官方数据手册移植的。
 */
// 补偿温度
static int32_t compensate_temperature(int32_t adc_T)
{
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)calib_params.dig_T1 << 1))) * ((int32_t)calib_params.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib_params.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib_params.dig_T1))) >> 12) * ((int32_t)calib_params.dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

// 补偿压力
static uint32_t compensate_pressure(int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib_params.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib_params.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib_params.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib_params.dig_P3) >> 8) + ((var1 * (int64_t)calib_params.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib_params.dig_P1) >> 33;

    if (var1 == 0)
    {
        return 0; // 避免除以零
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib_params.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib_params.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib_params.dig_P7) << 4);
    return (uint32_t)p;
}

// --- 公共函数 ---

uint8_t BMP280_Init(void)
{
    uint8_t chip_id = 0;
    
    // 1. 检查I2C设备是否就绪
    if (HAL_I2C_IsDeviceReady(&hi2c1, BMP280_I2C_ADDR, 2, 100) != HAL_OK)
    {
        return 1; // 设备未就绪
    }

    // 2. 读取芯片ID
    if (HAL_I2C_Mem_Read(&hi2c1, BMP280_I2C_ADDR, BMP280_REG_ID, 1, &chip_id, 1, 100) != HAL_OK)
    {
        return 1;
    }

    if (chip_id != BMP280_CHIP_ID)
    {
        return 1; // ID不匹配
    }

    // 3. 读取校准参数
    uint8_t calib_data[24];
    if (HAL_I2C_Mem_Read(&hi2c1, BMP280_I2C_ADDR, BMP280_REG_CALIB_START, 1, calib_data, 24, 1000) != HAL_OK)
    {
        return 1;
    }
    
    // 解析校准数据 (小端格式)
    calib_params.dig_T1 = (uint16_t)(calib_data[1] << 8 | calib_data[0]);
    calib_params.dig_T2 = (int16_t)(calib_data[3] << 8 | calib_data[2]);
    calib_params.dig_T3 = (int16_t)(calib_data[5] << 8 | calib_data[4]);
    calib_params.dig_P1 = (uint16_t)(calib_data[7] << 8 | calib_data[6]);
    calib_params.dig_P2 = (int16_t)(calib_data[9] << 8 | calib_data[8]);
    calib_params.dig_P3 = (int16_t)(calib_data[11] << 8 | calib_data[10]);
    calib_params.dig_P4 = (int16_t)(calib_data[13] << 8 | calib_data[12]);
    calib_params.dig_P5 = (int16_t)(calib_data[15] << 8 | calib_data[14]);
    calib_params.dig_P6 = (int16_t)(calib_data[17] << 8 | calib_data[16]);
    calib_params.dig_P7 = (int16_t)(calib_data[19] << 8 | calib_data[18]);
    calib_params.dig_P8 = (int16_t)(calib_data[21] << 8 | calib_data[20]);
    calib_params.dig_P9 = (int16_t)(calib_data[23] << 8 | calib_data[22]);

    // 4. 配置传感器
    uint8_t ctrl_meas_val = (0x05 << 5) | (0x05 << 2) | 0x03; // Temp/Press x16 oversampling, Normal mode
    uint8_t config_val = (0x04 << 5) | (0x04 << 2);           // Standby 500ms, IIR filter coeff 16
    
    HAL_I2C_Mem_Write(&hi2c1, BMP280_I2C_ADDR, BMP280_REG_CTRL_MEAS, 1, &ctrl_meas_val, 1, 100);
    HAL_I2C_Mem_Write(&hi2c1, BMP280_I2C_ADDR, BMP280_REG_CONFIG, 1, &config_val, 1, 100);

    return 0; // 初始化成功
}

uint8_t BMP280_ReadData(BMP280_t *bmp280_data)
{
    uint8_t raw_data[6];

    // 从寄存器 0xF7 开始，连续读取6个字节 (压力+温度)
    if (HAL_I2C_Mem_Read(&hi2c1, BMP280_I2C_ADDR, BMP280_REG_PRESS_MSB, 1, raw_data, 6, 100) != HAL_OK)
    {
        return 1;
    }
    
    // 组合原始数据 (20位)
    int32_t adc_P = (int32_t)((((uint32_t)(raw_data[0])) << 12) | (((uint32_t)(raw_data[1])) << 4) | ((uint32_t)raw_data[2] >> 4));
    int32_t adc_T = (int32_t)((((uint32_t)(raw_data[3])) << 12) | (((uint32_t)(raw_data[4])) << 4) | ((uint32_t)raw_data[5] >> 4));

    if(adc_P == 0x80000 || adc_T == 0x80000)
    {
        // 无效数据
        return 1;
    }

    // 计算补偿后的值
    bmp280_data->temperature = (float)compensate_temperature(adc_T) / 100.0f;
    bmp280_data->pressure = (float)compensate_pressure(adc_P) / 256.0f;
    
    return 0;
} 