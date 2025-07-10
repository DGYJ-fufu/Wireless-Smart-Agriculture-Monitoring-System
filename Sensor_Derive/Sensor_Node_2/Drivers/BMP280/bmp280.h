#ifndef __BMP280_H
#define __BMP280_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief BMP280传感器数据结构体
 */
typedef struct
{
    float temperature;  // 温度 (°C)
    float pressure;     // 气压 (Pa)
} BMP280_t;


/**
 * @brief 初始化BMP280传感器
 * @note  此函数会检查传感器ID，读取校准参数，并配置传感器进入正常工作模式。
 * @retval 0: 成功
 * @retval 1: 失败 (传感器未找到或ID不匹配)
 */
uint8_t BMP280_Init(void);

/**
 * @brief 从BMP280读取温度和气压数据
 * @param[out] bmp280_data: 指向 BMP280_t 结构体的指针，用于存储读取到的数据
 * @retval 0: 成功
 * @retval 1: 失败
 */
uint8_t BMP280_ReadData(BMP280_t *bmp280_data);


#ifdef __cplusplus
}
#endif

#endif /* __BMP280_H */ 