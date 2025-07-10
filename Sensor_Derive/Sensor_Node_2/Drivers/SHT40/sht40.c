/**
 * @file sht40.c
 * @brief SHT40温湿度传感器驱动程序的实现。
 * @note  本驱动实现了通过I2C读取温湿度数据，并包含CRC8校验功能以确保数据可靠性。
 */
#include "sht40.h"

// SHT4x系列使用的CRC8校验多项式
#define SHT40_CRC8_POLYNOMIAL 0x31

/**
 * @brief  使用SHT40的CRC8算法校验数据。
 * @param  data: 指向待校验数据缓冲区的指针。
 * @param  len: 数据长度（不包括CRC字节）。
 * @param  checksum: 从传感器接收到的CRC校验字节。
 * @return 0 - CRC校验成功, -1 - CRC校验失败。
 */
static int8_t SHT40_CheckCrc(uint8_t *data, uint8_t len, uint8_t checksum)
{
	uint8_t crc = 0xFF; // CRC初始值
	
	for(uint8_t i = 0; i < len; i++) 
	{
		crc ^= data[i];
		for(uint8_t j = 8; j > 0; --j) 
		{
			if(crc & 0x80)
				crc = (crc << 1) ^ SHT40_CRC8_POLYNOMIAL;
			else
				crc = (crc << 1);
		}
	}
	
	if(crc == checksum)
		return 0; // 校验成功
	else
		return -1; // 校验失败
}

/**
 * @brief  从SHT40传感器读取温度和湿度数据。
 * @param  temperature: 指向用于存储温度值的double类型变量的指针 (单位: °C)。
 * @param  humidity: 指向用于存储相对湿度值的double类型变量的指针 (单位: %RH)。
 * @retval 0  - 读取和校验成功。
 * @retval -1 - I2C通信失败或CRC校验失败。
 */
uint8_t SHT40_Read_RHData(double *temperature, double *humidity)
{
	uint8_t write_cmd[1] = {SHT40_CMD_MEASURE_HPM}; // 使用高精度测量命令
	uint8_t read_data[6] = {0}; // 2字节温度 + 1字节CRC + 2字节湿度 + 1字节CRC
	uint16_t temp_raw, hum_raw;

	// 1. 发送测量指令
	if(HAL_I2C_Master_Transmit(&hi2c3, SHT40_ADDRESS, write_cmd, 1, 1000) != HAL_OK)
	{
		return -1; // I2C发送失败
	}
	
	// 2. 等待测量完成 (高精度模式下最大需要8.5ms)
	HAL_Delay(10); 
	
	// 3. 读取6个字节的数据
	if(HAL_I2C_Master_Receive(&hi2c3, SHT40_ADDRESS, read_data, 6, 1000) != HAL_OK)
	{
		return -1; // I2C接收失败
	}
	
	// 4. 校验温度数据 (前两个字节)
	if(SHT40_CheckCrc(&read_data[0], 2, read_data[2]) != 0)
	{
		return -1; // 温度数据CRC校验失败
	}
	
	// 5. 校验湿度数据 (第4、5字节)
	if(SHT40_CheckCrc(&read_data[3], 2, read_data[5]) != 0)
	{
		return -1; // 湿度数据CRC校验失败
	}

	// 6. 组合原始数据并根据官方公式进行转换
	temp_raw = (read_data[0] << 8) | read_data[1];
	// 温度(°C) = -45 + 175 * (S_T / 65535)
	*temperature = -45.0f + 175.0f * (double)temp_raw / 65535.0f;
	
	hum_raw = (read_data[3] << 8) | read_data[4];
	// 相对湿度(%RH) = -6 + 125 * (S_RH / 65535)
	*humidity = -6.0f + 125.0f * (double)hum_raw / 65535.0f;
	
	// 钳位操作，确保湿度值在0-100范围内
	if(*humidity > 100.0f) *humidity = 100.0f;
	if(*humidity < 0.0f)   *humidity = 0.0f;

	return 0; // 成功
}
