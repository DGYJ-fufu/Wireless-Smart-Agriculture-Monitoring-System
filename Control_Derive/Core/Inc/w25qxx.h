#ifndef W25QXX_H
#define W25QXX_H

#include "main.h"
#include <stdbool.h>

/* W25Qxx JEDEC ID */
#define W25Q32_JEDEC_ID         0xEF4016

/* W25Qxx Instruction Set */
#define WRITE_ENABLE_CMD        0x06
#define WRITE_DISABLE_CMD       0x04
#define READ_STATUS_REG1_CMD    0x05
#define READ_STATUS_REG2_CMD    0x35
#define WRITE_STATUS_REG1_CMD   0x01
#define WRITE_STATUS_REG2_CMD   0x31
#define READ_DATA_CMD           0x03
#define PAGE_PROG_CMD           0x02
#define SECTOR_ERASE_CMD        0x20
#define BLOCK_ERASE_64K_CMD     0xD8
#define CHIP_ERASE_CMD          0xC7
#define READ_JEDEC_ID_CMD       0x9F
#define RESET_ENABLE_CMD        0x66
#define RESET_DEVICE_CMD        0x99
#define ENABLE_QPI_CMD          0x38
#define EXIT_QPI_CMD            0xFF

/* W25Qxx Status Register Bits */
#define WIP_FLAG_BIT            0x01  // Write In Progress (WIP) flag
#define QE_FLAG_BIT             0x02  // Quad Enable (QE) flag

/* W25Qxx Physical Characteristics */
#define W25Q32_PAGE_SIZE        256     // 256 bytes per page
#define W25Q32_SECTOR_SIZE      4096    // 4k bytes per sector (also smallest erasable unit)
#define W25Q32_BLOCK_SIZE       65536   // 64k bytes per block
#define W25Q32_NUM_PAGES        16384   // 16384 pages
#define W25Q32_NUM_SECTORS      1024    // 1024 sectors
#define W25Q32_NUM_BLOCKS       64      // 64 blocks

/* QSPI HAL Timeout */
#define QSPI_TIMEOUT_DEFAULT    5000

typedef enum {
    W25QXX_OK = 0,
    W25QXX_ERROR,
    W25QXX_BUSY,
    W25QXX_TIMEOUT,
    W25QXX_ID_ERROR
} W25QXX_Status_t;

// --- High Level Functions ---
W25QXX_Status_t W25QXX_Init(void);
W25QXX_Status_t W25QXX_Reset(void);
W25QXX_Status_t W25QXX_Erase_Sector(uint32_t SectorAddr);
W25QXX_Status_t W25QXX_Erase_Chip(void);
W25QXX_Status_t W25QXX_Write(uint8_t* pData, uint32_t WriteAddr, uint32_t Size);
W25QXX_Status_t W25QXX_Read(uint8_t* pData, uint32_t ReadAddr, uint32_t Size);
W25QXX_Status_t W25QXX_Enable_MemMappedMode(void);
W25QXX_Status_t W25QXX_Enter_QuadMode(void);

// --- Low Level Functions ---
uint32_t W25QXX_Read_ID(void);
W25QXX_Status_t W25QXX_Write_Enable(void);
W25QXX_Status_t W25QXX_Wait_Busy(void);

#endif // W25QXX_H 