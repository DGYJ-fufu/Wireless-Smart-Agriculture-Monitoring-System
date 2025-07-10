/**
 * @file w25qxx.c
 * @brief W25QXX系列SPI NOR Flash芯片驱动程序的实现。
 * @note  本驱动提供了对W25QXX系列Flash的初始化、ID读取、数据读写、
 *        扇区擦除和整片擦除等功能。
 */
#include "w25qxx.h"
#include "spi.h"
#include <stdio.h> // 用于 printf

/* ------------------------- 硬件和指令配置 ------------------------- */

// 1. SPI句柄 - 请根据您在CubeMX中的配置修改
#define W25QXX_SPI_HANDLE       hspi2

// 2. CS引脚 - 请根据您在CubeMX中的配置修改
#define W25QXX_CS_GPIO_PORT     FLASH_CS_GPIO_Port
#define W25QXX_CS_GPIO_PIN      FLASH_CS_Pin

// 3. 调试信息打印开关
#define W25QXX_DEBUG            1

/* W25QXX 指令集 */
#define CMD_WRITE_ENABLE		0x06 // 写使能
#define CMD_WRITE_DISABLE		0x04 // 写失能
#define CMD_READ_STATUS_REG1	0x05 // 读状态寄存器1
#define CMD_READ_STATUS_REG2	0x35 // 读状态寄存器2
#define CMD_READ_STATUS_REG3	0x15 // 读状态寄存器3
#define CMD_WRITE_STATUS_REG1	0x01 // 写状态寄存器1
#define CMD_WRITE_STATUS_REG2	0x31 // 写状态寄存器2
#define CMD_WRITE_STATUS_REG3	0x11 // 写状态寄存器3
#define CMD_READ_DATA			0x03 // 读数据
#define CMD_FAST_READ			0x0B // 快速读数据
#define CMD_PAGE_PROGRAM		0x02 // 页编程 (写)
#define CMD_SECTOR_ERASE_4K		0x20 // 4KB扇区擦除
#define CMD_BLOCK_ERASE_32K		0x52 // 32KB块擦除
#define CMD_BLOCK_ERASE_64K		0xD8 // 64KB块擦除
#define CMD_CHIP_ERASE			0xC7 // 整片擦除
#define CMD_POWER_DOWN			0xB9 // 掉电
#define CMD_RELEASE_POWER_DOWN	0xAB // 从掉电模式唤醒
#define CMD_JEDEC_ID			0x9F // 读取JEDEC ID

/* ------------------------- 静态(私有)函数声明 ------------------------- */

/** @brief 拉低CS引脚，选中芯片 */
static void W25QXX_CS_Select(void);
/** @brief 拉高CS引脚，取消选中芯片 */
static void W25QXX_CS_Deselect(void);
/** @brief 通过SPI发送一个字节并接收一个字节 */
static uint8_t W25QXX_SPI_TransmitReceive(uint8_t data);
/** @brief 发送"写使能"指令 */
static void W25QXX_WriteEnable(void);
/** @brief 等待Flash内部操作完成 (轮询BUSY位) */
static void W25QXX_WaitForWriteEnd(void);
/** @brief 向Flash的一个页写入数据 (单次写入不能超过页大小) */
static void W25QXX_Write_Page(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);

/* ------------------------- 公开函数实现 ------------------------- */

/**
  * @brief  初始化W25QXX驱动
  */
uint8_t W25QXX_Init(void)
{
    W25QXX_CS_Deselect(); // 确保CS拉高
    uint16_t id = W25QXX_Read_ID();

    if (W25QXX_DEBUG)
    {
        printf("W25QXX Init...\r\n");
        printf("W25QXX JEDEC ID: 0x%X\r\n", id);
    }
    
    if (id == W25Q32_ID || id == W25Q64_ID || id == W25Q128_ID) // 扩展支持更多型号
    {
        if (W25QXX_DEBUG) printf("W25QXX series chip detected!\r\n");
        return 0; // 成功
    }
    else
    {
        if (W25QXX_DEBUG) printf("W25QXX ID mismatch!\r\n");
        return 1; // 失败
    }
}

/**
  * @brief  读取W25QXX的JEDEC ID
  */
uint16_t W25QXX_Read_ID(void)
{
    uint8_t tx_data[1] = {CMD_JEDEC_ID};
    uint8_t rx_data[3];
    uint16_t id;

    W25QXX_CS_Select();
    HAL_SPI_Transmit(&W25QXX_SPI_HANDLE, tx_data, 1, 100);
    HAL_SPI_Receive(&W25QXX_SPI_HANDLE, rx_data, 3, 100);
    W25QXX_CS_Deselect();

    // JEDEC ID包含3个字节：制造商ID(0xEF)，设备类型(高8位)，容量(低8位)
    // 我们将后两者组合成16位ID用于识别。
    id = (rx_data[1] << 8) | rx_data[2];
    return id;
}

/**
  * @brief  读取Flash数据
  */
void W25QXX_Read_Data(uint8_t* pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead)
{
    uint8_t tx_header[4];

    // 1. 构造指令和3字节地址头
    tx_header[0] = CMD_READ_DATA;
    tx_header[1] = (ReadAddr >> 16) & 0xFF; // 地址高位 A23-A16
    tx_header[2] = (ReadAddr >> 8) & 0xFF;  // 地址中位 A15-A8
    tx_header[3] = ReadAddr & 0xFF;         // 地址低位 A7-A0

    // 2. 发送指令和地址，然后接收数据
    W25QXX_CS_Select();
    HAL_SPI_Transmit(&W25QXX_SPI_HANDLE, tx_header, 4, 100);
    HAL_SPI_Receive(&W25QXX_SPI_HANDLE, pBuffer, NumByteToRead, 2000);
    W25QXX_CS_Deselect();
}

/**
  * @brief  向Flash写入数据 (自动处理翻页)
  */
void W25QXX_Write_Data(uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite)
{
    uint32_t first_page_remain; // 当前页剩余可写空间
    uint32_t page_offset;       // 写入地址在当前页的偏移
    uint32_t bytes_to_write;    // 本次循环要写入的字节数

    page_offset = WriteAddr % W25Q32_PAGE_SIZE;
    first_page_remain = W25Q32_PAGE_SIZE - page_offset;
    
    // 判断要写入的数据是否会跨页
    if (NumByteToWrite <= first_page_remain)
    {
        // 不会跨页，一次性写入
        bytes_to_write = NumByteToWrite;
    }
    else
    {
        // 会跨页，先写满当前页
        bytes_to_write = first_page_remain;
    }

    // 循环处理，直到所有数据都被写入
    while (NumByteToWrite > 0)
    {
        W25QXX_Write_Page(pBuffer, WriteAddr, bytes_to_write);
        
        // 更新剩余字节数、缓冲区指针和写入地址
        NumByteToWrite -= bytes_to_write;
        pBuffer += bytes_to_write;
        WriteAddr += bytes_to_write;

        // 为下一次循环计算要写入的字节数
        if (NumByteToWrite > W25Q32_PAGE_SIZE)
        {
            bytes_to_write = W25Q32_PAGE_SIZE; // 下次写满一整页
        }
        else
        {
            bytes_to_write = NumByteToWrite; // 下次写入剩余的所有数据
        }
    }
}

/**
  * @brief  擦除整个芯片
  */
void W25QXX_Erase_Chip(void)
{
    uint8_t tx_data[1] = {CMD_CHIP_ERASE};

    W25QXX_WriteEnable(); // 擦除前必须先写使能
    
    W25QXX_CS_Select();
    HAL_SPI_Transmit(&W25QXX_SPI_HANDLE, tx_data, 1, 100);
    W25QXX_CS_Deselect();
    
    W25QXX_WaitForWriteEnd(); // 整片擦除耗时很长，必须等待
}

/**
  * @brief  擦除一个扇区
  */
void W25QXX_Erase_Sector(uint32_t SectorAddr)
{
    uint8_t tx_header[4];
    // 将扇区号转换为字节地址
    uint32_t address = SectorAddr * W25Q32_SECTOR_SIZE;

    tx_header[0] = CMD_SECTOR_ERASE_4K;
    tx_header[1] = (address >> 16) & 0xFF;
    tx_header[2] = (address >> 8) & 0xFF;
    tx_header[3] = address & 0xFF;
    
    W25QXX_WriteEnable(); // 擦除前必须先写使能
    
    W25QXX_CS_Select();
    HAL_SPI_Transmit(&W25QXX_SPI_HANDLE, tx_header, 4, 100);
    W25QXX_CS_Deselect();
    
    W25QXX_WaitForWriteEnd(); // 扇区擦除需要时间，必须等待
}

/* ------------------------- 静态(私有)函数实现 ------------------------- */

static void W25QXX_CS_Select(void)
{
    HAL_GPIO_WritePin(W25QXX_CS_GPIO_PORT, W25QXX_CS_GPIO_PIN, GPIO_PIN_RESET);
}

static void W25QXX_CS_Deselect(void)
{
    HAL_GPIO_WritePin(W25QXX_CS_GPIO_PORT, W25QXX_CS_GPIO_PIN, GPIO_PIN_SET);
}

static uint8_t W25QXX_SPI_TransmitReceive(uint8_t data)
{
    uint8_t rx_data;
    HAL_SPI_TransmitReceive(&W25QXX_SPI_HANDLE, &data, &rx_data, 1, 100);
    return rx_data;
}

static void W25QXX_WriteEnable(void)
{
    uint8_t tx_data[1] = {CMD_WRITE_ENABLE};
    W25QXX_CS_Select();
    HAL_SPI_Transmit(&W25QXX_SPI_HANDLE, tx_data, 1, 100);
    W25QXX_CS_Deselect();
}

static void W25QXX_WaitForWriteEnd(void)
{
    uint8_t status_reg1 = 0;
    uint8_t tx_cmd[1] = {CMD_READ_STATUS_REG1};

    W25QXX_CS_Select();
    HAL_SPI_Transmit(&W25QXX_SPI_HANDLE, tx_cmd, 1, 100);
    do
    {
        HAL_SPI_Receive(&W25QXX_SPI_HANDLE, &status_reg1, 1, 100);
        HAL_Delay(1); // 让出CPU
    } while ((status_reg1 & 0x01) == 0x01); // BUSY bit is 1
    W25QXX_CS_Deselect();
}

static void W25QXX_Write_Page(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    uint8_t tx_header[4];

    if (NumByteToWrite > W25Q32_PAGE_SIZE)
    {
        // 错误：写入的数据超出单页大小
        return;
    }
    
    tx_header[0] = CMD_PAGE_PROGRAM;
    tx_header[1] = (WriteAddr >> 16) & 0xFF;
    tx_header[2] = (WriteAddr >> 8) & 0xFF;
    tx_header[3] = WriteAddr & 0xFF;
    
    W25QXX_WriteEnable();
    W25QXX_CS_Select();

    HAL_SPI_Transmit(&W25QXX_SPI_HANDLE, tx_header, 4, 100);
    HAL_SPI_Transmit(&W25QXX_SPI_HANDLE, pBuffer, NumByteToWrite, 2000);

    W25QXX_CS_Deselect();
    W25QXX_WaitForWriteEnd();
} 