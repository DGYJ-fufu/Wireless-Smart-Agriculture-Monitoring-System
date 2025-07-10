#include "sht40.h"

uint8_t SHT40_Read_RHData(double *temperature,double *humidity)
{
	uint8_t writeData[1] = {0xFD};
	uint8_t readData[6] = {0};
	uint32_t tempData = 0;
	if(HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)SHT40_Write, (uint8_t *)writeData, 1, 1000) != HAL_OK)
	{
		return ERROR;
	}
  HAL_Delay(10);
	HAL_I2C_Master_Receive(&hi2c1, (uint16_t)SHT40_Read, (uint8_t *)readData, 6, 1000);
	
	tempData = readData[0]<<8 | readData[1];
	*temperature = (tempData * 175.0f) / 65535.0f - 45;
	
	tempData = readData[3]<<8 | readData[4];
  *humidity 	 = (tempData * 125.0f) / 65535.0f - 6;
	return SUCCESS;
}
