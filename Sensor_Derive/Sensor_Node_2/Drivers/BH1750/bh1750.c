#include "bh1750.h"

// --- BH1750 指令集 (Opecode) ---
#define BH1750_POWER_ON         0x01 // 上电指令
#define BH1750_POWER_OFF        0x00 // 掉电指令
#define BH1750_RESET            0x07 // 复位指令
#define BH1750_CONT_H_RES_MODE  0x10 // 连续高分辨率模式 (1 lux)
#define BH1750_CONT_H_RES_MODE2 0x11 // 连续高分辨率模式2 (0.5 lux)
#define BH1750_CONT_L_RES_MODE  0x13 // 连续低分辨率模式 (4 lux)
#define BH1750_ONETIME_H_RES_MODE  0x20 // 单次高分辨率模式
// ... 其他模式可以根据需要添加 ...

/**
 * @brief  通过I2C向BH1750发送指令。
 * @param  pData 指向要发送的指令字节的指针。
 * @param  size  指令的长度（通常为1）。
 * @return HAL_StatusTypeDef HAL库的I2C传输状态。
 */
static HAL_StatusTypeDef I2C_BH1750_Opecode_Write(uint8_t* pData, uint16_t size) {
	return HAL_I2C_Master_Transmit(&hi2c2, BH1750_ADDRESS, pData, size, HAL_MAX_DELAY);	
}

/**
 * @brief  通过I2C从BH1750读取数据。
 * @param  pData 指向用于存储接收数据的缓冲区的指针。
 * @param  size  要读取的数据长度。
 * @return HAL_StatusTypeDef HAL库的I2C接收状态。
 */
static HAL_StatusTypeDef I2C_BH1750_Data_Read(uint8_t* pData, uint16_t size){
	// I2C读操作的地址位(R/W bit)是由HAL库自动处理的，此处不应加1。
	return HAL_I2C_Master_Receive(&hi2c2, BH1750_ADDRESS, pData, size, HAL_MAX_DELAY);	
}

int Init_BH1750(void) {
	uint8_t opecode = BH1750_POWER_ON; // 发送上电指令
	if(I2C_BH1750_Opecode_Write(&opecode, 1) == HAL_OK ){
		HAL_Delay(5); // 等待上电稳定
		opecode = BH1750_RESET; // 发送复位指令
		if(I2C_BH1750_Opecode_Write(&opecode, 1) == HAL_OK){
			return 0; // 成功
		}
	}
	return -1; // 失败
}

int BH1750_GetDate(uint16_t *light){
	uint8_t opecode;
	
	// 1. 发送测量指令（高精度模式）
	opecode = BH1750_ONETIME_H_RES_MODE;
	if (I2C_BH1750_Opecode_Write(&opecode, 1) != HAL_OK){
		return -1; // 指令发送失败
	}
		
	// 2. 等待测量完成
	//    根据数据手册，高精度模式下最长测量时间为180ms。
	HAL_Delay(180); 
	
	// 3. 读取2字节的测量结果
	uint8_t data_buf[2] = {0};
	if (I2C_BH1750_Data_Read(data_buf, 2) != HAL_OK){
		return -1; // 读取数据失败
	}
		
	// 4. 合成数据并进行转换
	uint16_t raw_data = (data_buf[0] << 8) | data_buf[1];
	
	// 根据数据手册，最终的光照强度值 (lux) = 原始数据 / 1.2
	*light = (uint16_t)(raw_data / 1.2f);
	
	return 0; // 成功
}
