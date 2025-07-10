#ifndef __SHT40_H__
#define __SHT40_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "i2c.h"

/**************************I2C��ַ****************************/
#define SHT40_Write (0x44<<1)   		//д���ַ
#define SHT40_Read  ((0x44<<1)+1)   //������ַ,HAL�������������ﻹ�Ǽ��Ϸ�ֹ��ֲ����оƬ������
/**************************SHT40����****************************/
#define SHT40_MEASURE_TEMPERATURE_HUMIDITY 	0xFD  				//�߾��ȶ�ȡ��ʪ������
#define SHT40_READ_SERIAL_NUMBER 						0x89          //��ȡΨһ���к�����
#define SHT40_HEATER_200mW_1s 							0x39          //200mW����1������

uint8_t SHT40_Read_RHData(double *temperature,double *humidity);


#ifdef __cplusplus
}
#endif

#endif /* __SHT40_H__ */
	
