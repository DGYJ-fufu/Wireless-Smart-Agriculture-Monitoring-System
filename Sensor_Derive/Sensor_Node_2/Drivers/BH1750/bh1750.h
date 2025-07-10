#ifndef __BH1750_H__
#define __BH1750_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "i2c.h"
#include "main.h"

// BH1750 I2C 从机地址 (8位格式)
// ALT ADDRESS 引脚接地时，7位地址为 0x23，8位地址为 0x46。
// ALT ADDRESS 引脚接VCC时，7位地址为 0x5C，8位地址为 0xB8。
#define	BH1750_ADDRESS 0xB8

/**
 * @brief  初始化BH1750光照传感器。
 * @retval 0  - 初始化成功。
 * @retval -1 - 初始化失败。
 */
int Init_BH1750(void);

/**
 * @brief  从BH1750读取当前的光照强度值。
 * @param  light: 指向用于存储光照强度值的变量的指针 (单位: lux)。
 * @retval 0  - 读取成功。
 * @retval -1 - 读取失败。
 */
int BH1750_GetDate(uint16_t *light);


#ifdef __cplusplus
}
#endif

#endif /* __BH1750_H__ */
