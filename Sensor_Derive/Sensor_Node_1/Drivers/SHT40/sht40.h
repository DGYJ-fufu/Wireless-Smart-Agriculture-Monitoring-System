#ifndef __SHT40_H__
#define __SHT40_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "i2c.h"

/**************************I2C地址****************************/
#define SHT40_Write (0x44<<1)   		//写入地址
#define SHT40_Read  ((0x44<<1)+1)   //读出地址,HAL库有做处理，这里还是加上防止移植其他芯片出问题
/**************************SHT40命令****************************/
#define SHT40_MEASURE_TEMPERATURE_HUMIDITY 	0xFD  				//高精度读取温湿度命令
#define SHT40_READ_SERIAL_NUMBER 						0x89          //读取唯一序列号命令
#define SHT40_HEATER_200mW_1s 							0x39          //200mW加热1秒命令

uint8_t SHT40_Read_RHData(double *temperature,double *humidity);


#ifdef __cplusplus
}
#endif

#endif /* __SHT40_H__ */
	
