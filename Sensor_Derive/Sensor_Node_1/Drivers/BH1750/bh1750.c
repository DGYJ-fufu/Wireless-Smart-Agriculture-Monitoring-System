#include "bh1750.h"

uint32_t I2C_BH1750_Opecode_Write(uint8_t* pData, uint16_t size) {
	HAL_StatusTypeDef status = HAL_OK;	
	status = HAL_I2C_Master_Transmit(&hi2c2, BH1750_ADDRESS,pData, size, 1);	
	return status;	
}

int Init_BH1750(void) {
	uint8_t opecode = 0x01;
	if( I2C_BH1750_Opecode_Write(&opecode, 1) == HAL_OK ){
		return 1;
	}else {
		return 0;
	}
}

uint32_t I2C_BH1750_Data_Read(uint8_t* pData, uint16_t size){
	HAL_StatusTypeDef status = HAL_OK;	
	status = HAL_I2C_Master_Receive(&hi2c2, BH1750_ADDRESS+1,pData, size, 1);	
	return status;
}

int BH1750_GetDate(uint16_t *light){
	uint8_t opecode;
	opecode = 0x01;
	if ( I2C_BH1750_Opecode_Write(&opecode, 1) != HAL_OK)
		return 0;
	opecode = 0x10;
	if ( I2C_BH1750_Opecode_Write(&opecode, 1) != HAL_OK)
		return 0;
	uint8_t DATA_BUF[8] = {0};
	if ( I2C_BH1750_Data_Read(DATA_BUF, 2) != HAL_OK)
		return 0;
	int dis_data =(DATA_BUF[0]);
	dis_data=(dis_data<<8)+ (DATA_BUF[1]);
	*light = (uint16_t)dis_data/1.2;
	return 1;
}
