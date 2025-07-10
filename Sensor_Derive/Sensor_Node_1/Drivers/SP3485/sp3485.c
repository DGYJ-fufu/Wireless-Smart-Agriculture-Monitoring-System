#include "sp3485.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>
#include "main.h"

// 声明由CubeMX在main.c中定义的CRC句柄
extern CRC_HandleTypeDef hcrc;

// 定义RS485控制引脚
#define RS485_CTRL_GPIO_PORT     RS485_CTRL_GPIO_Port
#define RS485_CTRL_GPIO_PIN      RS485_CTRL_Pin
#define SP3485_UART_HANDLE       hlpuart1

// Modbus功能码
#define MODBUS_FUNC_READ_HOLDING_REGISTERS  0x03

// 寄存器地址 (仅用于专用函数)
#define SOIL_REGISTER_START         0x0000
#define SOIL_REGISTER_COUNT         4 // 读取 水分、温度、电导率、PH值

// 驱动内部变量
static uint8_t sp3485_rx_buffer[SP3485_RX_BUFFER_SIZE];
static volatile uint8_t sp3485_rx_count = 0;
static uint8_t uart_rx_byte; // UART中断接收的单字节缓冲区

// 静态函数声明
static void RS485_Set_Mode(uint8_t mode);
static uint16_t CRC16_MODBUS(const uint8_t* buf, uint8_t len);
static void Print_Hex_Data(const char* title, const uint8_t* data, uint8_t len);

/**
  * @brief 初始化SP3485驱动
  */
void SP3485_Init(void)
{
    sp3485_rx_count = 0;
    memset(sp3485_rx_buffer, 0, SP3485_RX_BUFFER_SIZE);
    // 启动UART中断接收
    HAL_UART_Receive_IT(&SP3485_UART_HANDLE, &uart_rx_byte, 1);
}

/**
  * @brief UART接收完成回调函数，应在系统的HAL_UART_RxCpltCallback中调用
  * @param huart 触发中断的UART句柄
  */
void SP3485_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    // 判断是否是本驱动关心的UART实例
    if (huart->Instance == SP3485_UART_HANDLE.Instance)
    {
        if (sp3485_rx_count < SP3485_RX_BUFFER_SIZE)
        {
            sp3485_rx_buffer[sp3485_rx_count++] = uart_rx_byte;
        }
        // 重新启动UART中断接收
        HAL_UART_Receive_IT(&SP3485_UART_HANDLE, &uart_rx_byte, 1);
    }
}

/**
  * @brief  (通用) 读取一个或多个Modbus保持寄存器
  * @param  slave_addr: 从机地址
  * @param  reg_addr:   要读取的寄存器起始地址
  * @param  num_regs:   要读取的寄存器数量
  * @param  dest_buffer: 用于存放读取结果的缓冲区，大小必须 >= num_regs * 2
  * @retval 0: 成功, 其他: 失败
  */
uint8_t SP3485_Read_Holding_Registers(uint8_t slave_addr, uint16_t reg_addr, uint16_t num_regs, uint8_t* dest_buffer)
{
    uint8_t tx_buffer[8];
    uint16_t crc_calc;

    // 1. 构建问询帧
    tx_buffer[0] = slave_addr;
    tx_buffer[1] = MODBUS_FUNC_READ_HOLDING_REGISTERS;
    tx_buffer[2] = (reg_addr >> 8) & 0xFF;
    tx_buffer[3] = reg_addr & 0xFF;
    tx_buffer[4] = (num_regs >> 8) & 0xFF;
    tx_buffer[5] = num_regs & 0xFF;
    crc_calc = CRC16_MODBUS(tx_buffer, 6);
    tx_buffer[6] = crc_calc & 0xFF;
    tx_buffer[7] = (crc_calc >> 8) & 0xFF;

    // 2. 发送
    sp3485_rx_count = 0;
    memset(sp3485_rx_buffer, 0, SP3485_RX_BUFFER_SIZE);
    RS485_Set_Mode(1);
    if (SP3485_DEBUG) { Print_Hex_Data("TX", tx_buffer, sizeof(tx_buffer)); }
    HAL_UART_Transmit(&SP3485_UART_HANDLE, tx_buffer, sizeof(tx_buffer), 1000);
    while (__HAL_UART_GET_FLAG(&SP3485_UART_HANDLE, UART_FLAG_TC) == RESET);
    RS485_Set_Mode(0);

    // 3. 等待响应 (动态长度)
    uint32_t start_time = HAL_GetTick();
    uint8_t expected_len = 0;
    while(1)
    {
        if (HAL_GetTick() - start_time > 2000) { if (SP3485_DEBUG) printf("UART Receive Timeout!\r\n"); return 1; }

        if (expected_len == 0 && sp3485_rx_count >= 3) {
            if (sp3485_rx_buffer[1] == (MODBUS_FUNC_READ_HOLDING_REGISTERS | 0x80)) {
                expected_len = 5; // Modbus异常响应帧为5字节
            } else {
                expected_len = sp3485_rx_buffer[2] + 5; // 正常响应: Addr(1)+Func(1)+Len(1)+Data(N)+CRC(2)
            }
        }
        if (expected_len > 0 && sp3485_rx_count >= expected_len) {
            break; // 收到完整帧
        }
    }
    
    if (SP3485_DEBUG) { Print_Hex_Data("RX", sp3485_rx_buffer, sp3485_rx_count); }

    // 4. 校验
    if (sp3485_rx_buffer[0] != slave_addr || sp3485_rx_buffer[1] != MODBUS_FUNC_READ_HOLDING_REGISTERS) {
        if (SP3485_DEBUG) printf("Invalid slave address or function code.\r\n");
        return 2;
    }
    if (sp3485_rx_buffer[2] != (num_regs * 2)) {
        if (SP3485_DEBUG) printf("Invalid data byte count.\r\n");
        return 3;
    }
    crc_calc = CRC16_MODBUS(sp3485_rx_buffer, expected_len - 2);
    uint16_t crc_recv = (sp3485_rx_buffer[expected_len - 1] << 8) | sp3485_rx_buffer[expected_len - 2];
    if (crc_calc != crc_recv) {
        if (SP3485_DEBUG) printf("CRC check failed!\r\n");
        return 4;
    }

    // 5. 拷贝数据
    memcpy(dest_buffer, &sp3485_rx_buffer[3], num_regs * 2);

    return 0; // 成功
}

/**
  * @brief  (专用) 读取土壤传感器标准数据 (水分、温度、EC、PH)
  */
uint8_t SP3485_Read_Soil_Data(uint8_t slave_addr, Soil_Sensor_Data_t *data)
{
    uint8_t raw_data[8]; // 4 registers * 2 bytes/register
    uint8_t status = SP3485_Read_Holding_Registers(slave_addr, SOIL_REGISTER_START, SOIL_REGISTER_COUNT, raw_data);

    if (status != 0) {
        return status;
    }
    
    // 解析数据
    int16_t temp_val;
    
    // 水分值
    temp_val = (int16_t)((raw_data[0] << 8) | raw_data[1]);
    data->moisture = (float)temp_val / 10.0f;
    
    // 温度值
    temp_val = (int16_t)((raw_data[2] << 8) | raw_data[3]);
    data->temperature = (float)temp_val / 10.0f;

    // 电导率
    data->ec = (uint16_t)((raw_data[4] << 8) | raw_data[5]);

    // PH值
    temp_val = (int16_t)((raw_data[6] << 8) | raw_data[7]);
    data->ph = (float)temp_val / 10.0f;
    
    return 0; // 成功
}

/**
  * @brief  (专用) 读取土壤传感器扩展数据 (7个值)
  */
uint8_t SP3485_Read_Soil_Extended_Data(uint8_t slave_addr, Soil_Sensor_Extended_Data_t *data)
{
    uint8_t raw_data[14]; // 7 registers * 2 bytes/register
    uint16_t start_addr = 0x0000;
    uint16_t num_regs = 7;
    uint8_t status = SP3485_Read_Holding_Registers(slave_addr, start_addr, num_regs, raw_data);

    if (status != 0) {
        return status;
    }

    // 解析数据
    int16_t temp_val;

    temp_val = (int16_t)((raw_data[0] << 8) | raw_data[1]);
    data->moisture = (float)temp_val / 10.0f;

    temp_val = (int16_t)((raw_data[2] << 8) | raw_data[3]);
    data->temperature = (float)temp_val / 10.0f;

    data->ec = (uint16_t)((raw_data[4] << 8) | raw_data[5]);

    temp_val = (int16_t)((raw_data[6] << 8) | raw_data[7]);
    data->ph = (float)temp_val / 10.0f;

    data->nitrogen = (uint16_t)((raw_data[8] << 8) | raw_data[9]);

    data->phosphorus = (uint16_t)((raw_data[10] << 8) | raw_data[11]);

    data->potassium = (uint16_t)((raw_data[12] << 8) | raw_data[13]);

    return 0; // 成功
}

/**
  * @brief  (专用) 读取土壤传感器完整数据 (10个值)
  */
uint8_t SP3485_Read_Soil_Full_Data(uint8_t slave_addr, Soil_Sensor_Full_Data_t *data)
{
    uint8_t status;

    // 第1步: 读取前9个连续的寄存器 (0x0000 - 0x0008)
    uint8_t raw_data_part1[18];
    status = SP3485_Read_Holding_Registers(slave_addr, 0x0000, 9, raw_data_part1);
    if (status != 0) {
        return status;
    }

    // 第2步: 单独读取肥力寄存器 (0x000C)
    uint8_t raw_data_part2[2];
    status = SP3485_Read_Holding_Registers(slave_addr, 0x000C, 1, raw_data_part2);
    if (status != 0) {
        return status;
    }

    // 全部成功，开始解析
    int16_t temp_val;

    temp_val = (int16_t)((raw_data_part1[0] << 8) | raw_data_part1[1]);
    data->moisture    = (float)temp_val / 10.0f;
    temp_val = (int16_t)((raw_data_part1[2] << 8) | raw_data_part1[3]);
    data->temperature = (float)temp_val / 10.0f;
    data->ec          = (uint16_t)((raw_data_part1[4] << 8) | raw_data_part1[5]);
    temp_val = (int16_t)((raw_data_part1[6] << 8) | raw_data_part1[7]);
    data->ph          = (float)temp_val / 10.0f;
    data->nitrogen    = (uint16_t)((raw_data_part1[8] << 8) | raw_data_part1[9]);
    data->phosphorus  = (uint16_t)((raw_data_part1[10] << 8) | raw_data_part1[11]);
    data->potassium   = (uint16_t)((raw_data_part1[12] << 8) | raw_data_part1[13]);
    data->salinity    = (uint16_t)((raw_data_part1[14] << 8) | raw_data_part1[15]);
    data->tds         = (uint16_t)((raw_data_part1[16] << 8) | raw_data_part1[17]);
    data->fertility   = (uint16_t)((raw_data_part2[0] << 8) | raw_data_part2[1]);

    return 0; // 成功
}

/**
  * @brief  (专用) 读取土壤传感器9项数据
  */
uint8_t SP3485_Read_Soil_9_Values_Data(uint8_t slave_addr, Soil_Sensor_9_Values_Data_t *data)
{
    uint8_t raw_data[18]; // 9 registers * 2 bytes/register
    uint16_t start_addr = 0x0000;
    uint16_t num_regs = 9;
    uint8_t status = SP3485_Read_Holding_Registers(slave_addr, start_addr, num_regs, raw_data);

    if (status != 0) {
        return status;
    }

    // 解析数据
    int16_t temp_val;

    temp_val = (int16_t)((raw_data[0] << 8) | raw_data[1]);
    data->moisture = (float)temp_val / 10.0f;

    temp_val = (int16_t)((raw_data[2] << 8) | raw_data[3]);
    data->temperature = (float)temp_val / 10.0f;

    data->ec = (uint16_t)((raw_data[4] << 8) | raw_data[5]);

    temp_val = (int16_t)((raw_data[6] << 8) | raw_data[7]);
    data->ph = (float)temp_val / 10.0f;

    data->nitrogen = (uint16_t)((raw_data[8] << 8) | raw_data[9]);

    data->phosphorus = (uint16_t)((raw_data[10] << 8) | raw_data[11]);

    data->potassium = (uint16_t)((raw_data[12] << 8) | raw_data[13]);

    data->salinity = (uint16_t)((raw_data[14] << 8) | raw_data[15]);

    data->tds = (uint16_t)((raw_data[16] << 8) | raw_data[17]);

    return 0;
}


/*******************************************************************************
 * PRIVATE FUNCTIONS
 ******************************************************************************/

/**
  * @brief  设置RS485的模式 (发送/接收)
  * @param  mode: 0=接收, 1=发送
  */
static void RS485_Set_Mode(uint8_t mode)
{
    HAL_GPIO_WritePin(RS485_CTRL_GPIO_PORT, RS485_CTRL_GPIO_PIN, (mode == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
  * @brief  打印十六进制数据 (用于调试)
  */
static void Print_Hex_Data(const char* title, const uint8_t* data, uint8_t len)
{
    if (!SP3485_DEBUG) return;
    printf("%s[%d]: ", title, len);
    for(uint8_t i=0; i<len; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\r\n");
}

/**
  * @brief  使用STM32硬件CRC计算Modbus CRC16
  * @note   需要确保CubeMX中CRC外设已开启，并按以下或兼容Modbus标准的方式配置:
  *         - Generating polynomial: 0x8005 (CRC-16-MODBUS)
  *         - Initial CRC value: 0xFFFF
  *         - Input data format: Bytes
  *         - Input data reversal: Byte-wise reversal
  *         - Output data reversal: Bit-wise reversal (Enabled)
  * @param  buf: 数据缓冲区指针
  * @param  len: 数据长度 (字节)
  * @retval CRC16校验值
  */
static uint16_t CRC16_MODBUS(const uint8_t* buf, uint8_t len)
{
    // 重置CRC计算单元，开始新的计算
    __HAL_CRC_DR_RESET(&hcrc);

    // 逐字节喂入数据到硬件CRC计算单元
    for (uint8_t i = 0; i < len; i++)
    {
        *(__IO uint8_t*)(&hcrc.Instance->DR) = buf[i];
    }
    
    // 从数据寄存器读出最终的16位CRC结果
    return (uint16_t)hcrc.Instance->DR;
} 