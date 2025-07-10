#ifndef __W25QXX_H
#define __W25QXX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

// W25QXX系列芯片信息 (设备类型 + 容量ID)
#define W25Q80_ID   0x4014
#define W25Q16_ID   0x4015
#define W25Q32_ID   0x4016
#define W25Q64_ID   0x4017
#define W25Q128_ID  0x4018
#define W25Q256_ID  0x4019

// W25Q32JV 参数
#define W25Q32_PAGE_SIZE            256     // 256字节/页
#define W25Q32_SECTOR_SIZE          4096    // 4K字节/扇区
#define W25Q32_BLOCK_SIZE           65536   // 64K字节/块
#define W25Q32_PAGE_COUNT           16384   // 总页数
#define W25Q32_SECTOR_COUNT         1024    // 总扇区数
#define W25Q32_BLOCK_COUNT          64      // 总块数
#define W25Q32_CHIP_CAPACITY        4194304 // 总容量 (字节)

/**
  * @brief  初始化W25QXX驱动
  * @retval 0: 成功, 1: 失败 (ID不匹配)
  */
uint8_t W25QXX_Init(void);

/**
  * @brief  读取W25QXX的JEDEC ID
  * @retval 16位的设备ID (例如 0xEF15)
  */
uint16_t W25QXX_Read_ID(void);

/**
  * @brief  读取Flash数据
  * @param  pBuffer:     数据存储区
  * @param  ReadAddr:    读取地址 (0 ~ W25Q32_CHIP_CAPACITY-1)
  * @param  NumByteToRead: 要读取的字节数
  */
void W25QXX_Read_Data(uint8_t* pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead);

/**
  * @brief  向Flash写入数据 (自动处理翻页)
  * @note   此函数在写入前【不会】自动擦除扇区，请确保目标区域已被擦除
  * @param  pBuffer:       数据缓冲区
  * @param  WriteAddr:     写入地址 (0 ~ W25Q32_CHIP_CAPACITY-1)
  * @param  NumByteToWrite: 要写入的字节数
  */
void W25QXX_Write_Data(uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite);

/**
  * @brief  擦除整个芯片
  * @note   此操作耗时较长，请谨慎使用
  */
void W25QXX_Erase_Chip(void);

/**
  * @brief  擦除一个扇区 (4KB)
  * @param  SectorAddr: 扇区地址 (0 ~ W25Q32_SECTOR_COUNT-1)
  */
void W25QXX_Erase_Sector(uint32_t SectorAddr);


#ifdef __cplusplus
}
#endif

#endif /* __W25QXX_H */ 