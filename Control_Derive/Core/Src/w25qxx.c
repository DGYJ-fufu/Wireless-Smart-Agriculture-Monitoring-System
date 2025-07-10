#include "w25qxx.h"

extern QSPI_HandleTypeDef hqspi1;

uint32_t W25QXX_Read_ID(void)
{
    QSPI_CommandTypeDef s_command;
    uint8_t id_data[3];
    uint32_t jedec_id = 0;

    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = READ_JEDEC_ID_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.NbData            = 3;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi1, &s_command, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
        return 0;
    }

    if (HAL_QSPI_Receive(&hqspi1, id_data, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
        return 0;
    }

    jedec_id = (id_data[0] << 16) | (id_data[1] << 8) | id_data[2];
    return jedec_id;
}


W25QXX_Status_t W25QXX_Write_Enable(void)
{
    QSPI_CommandTypeDef s_command;
    QSPI_AutoPollingTypeDef s_config;

    // Send Write Enable command
	s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = WRITE_ENABLE_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
	
    if (HAL_QSPI_Command(&hqspi1, &s_command, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
        return W25QXX_ERROR;
    }

    // Wait for WEL bit to be set
    s_config.Match           = 0x02; // WEL bit is bit 1
    s_config.Mask            = 0x02;
    s_config.MatchMode       = QSPI_MATCH_MODE_AND;
    s_config.StatusBytesSize = 1;
    s_config.Interval        = 0x10;
    s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    s_command.Instruction    = READ_STATUS_REG1_CMD;
    s_command.DataMode       = QSPI_DATA_1_LINE;

    if (HAL_QSPI_AutoPolling(&hqspi1, &s_command, &s_config, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
        return W25QXX_TIMEOUT;
    }
    return W25QXX_OK;
}

W25QXX_Status_t W25QXX_Wait_Busy(void)
{
    QSPI_CommandTypeDef s_command;
    QSPI_AutoPollingTypeDef s_config;

    // Configure the command for reading status register
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = READ_STATUS_REG1_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    // Configure the auto-polling for the WIP (Write In Progress) bit
    s_config.Match           = 0x00; // We want to wait until WIP bit (bit 0) is 0
    s_config.Mask            = WIP_FLAG_BIT; // Mask for WIP bit
    s_config.MatchMode       = QSPI_MATCH_MODE_AND;
    s_config.StatusBytesSize = 1;
    s_config.Interval        = 0x10; // Polling interval
    s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    if (HAL_QSPI_AutoPolling(&hqspi1, &s_command, &s_config, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
        return W25QXX_TIMEOUT;
    }
    return W25QXX_OK;
}

W25QXX_Status_t W25QXX_Reset(void)
{
    QSPI_CommandTypeDef s_command;

    // Configure the command for Reset Enable
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = RESET_ENABLE_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    // Send Reset Enable command
    if (HAL_QSPI_Command(&hqspi1, &s_command, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
        return W25QXX_ERROR;
    }

    // Configure the command for Reset Device
    s_command.Instruction = RESET_DEVICE_CMD;

    // Send Reset Device command
    if (HAL_QSPI_Command(&hqspi1, &s_command, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
        return W25QXX_ERROR;
    }
    
    // Wait for the reset to take effect (t_RST, min 30us according to datasheet)
    HAL_Delay(1); 

    return W25QXX_OK;
}

W25QXX_Status_t W25QXX_Init(void)
{
    // Note: The QSPI peripheral is already initialized by MX_QUADSPI_Init() in main.c

    // 1. Reset the QSPI flash memory to ensure it's in a known state (standard SPI mode).
    if (W25QXX_Reset() != W25QXX_OK)
    {
        return W25QXX_ERROR;
    }

    // 2. Read the JEDEC ID to verify communication.
    uint32_t id = W25QXX_Read_ID();
    if (id != W25Q32_JEDEC_ID) {
        return W25QXX_ID_ERROR;
    }

    return W25QXX_OK;
}

W25QXX_Status_t W25QXX_Erase_Sector(uint32_t SectorAddr)
{
    QSPI_CommandTypeDef s_command;

    // 1. Send Write Enable command
    if (W25QXX_Write_Enable() != W25QXX_OK)
    {
        return W25QXX_ERROR;
    }

    // 2. Send Sector Erase command
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = SECTOR_ERASE_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.Address           = SectorAddr;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi1, &s_command, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
        return W25QXX_ERROR;
    }

    // 3. Wait for the operation to complete
    if (W25QXX_Wait_Busy() != W25QXX_OK)
    {
        return W25QXX_TIMEOUT;
    }

    return W25QXX_OK;
}

W25QXX_Status_t W25QXX_Write(uint8_t* pData, uint32_t WriteAddr, uint32_t Size)
{
    QSPI_CommandTypeDef s_command;
    uint32_t end_addr, current_size, current_addr;

    current_addr = WriteAddr;
    end_addr = current_addr + Size;

    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = PAGE_PROG_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    while (current_addr < end_addr) {
        // Must write enable before each page program
        if (W25QXX_Write_Enable() != W25QXX_OK)
        {
            return W25QXX_ERROR;
        }

        // Calculate how many bytes to write in the current page
        current_size = W25Q32_PAGE_SIZE - (current_addr % W25Q32_PAGE_SIZE);
        if (current_size > (end_addr - current_addr)) {
            current_size = end_addr - current_addr;
        }

        s_command.Address = current_addr;
        s_command.NbData = current_size;

        if (HAL_QSPI_Command(&hqspi1, &s_command, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
            return W25QXX_ERROR;
        }
        if (HAL_QSPI_Transmit(&hqspi1, pData, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
            return W25QXX_ERROR;
        }

        // Wait for the write to complete
        if (W25QXX_Wait_Busy() != W25QXX_OK)
        {
            return W25QXX_TIMEOUT;
        }

        // Move to the next data chunk
        current_addr += current_size;
        pData += current_size;
    }

    return W25QXX_OK;
}


W25QXX_Status_t W25QXX_Read(uint8_t* pData, uint32_t ReadAddr, uint32_t Size)
{
    QSPI_CommandTypeDef s_command;

    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = READ_DATA_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.Address           = ReadAddr;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.NbData            = Size;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi1, &s_command, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
        return W25QXX_ERROR;
    }

    if (HAL_QSPI_Receive(&hqspi1, pData, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
        return W25QXX_ERROR;
    }

    return W25QXX_OK;
}

W25QXX_Status_t W25QXX_Enable_MemMappedMode(void)
{
    QSPI_CommandTypeDef      s_command;
    QSPI_MemoryMappedTypeDef s_mem_mapped_cfg;

    /* Configure the command for the read instruction */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;    // Instruction is on a single line
    s_command.Instruction       = 0x6B; // Fast Read Quad Output
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;      // Address is on a single line
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_4_LINES;        // Data is on four lines
    s_command.DummyCycles       = 8; // 8 dummy cycles for 0x6B command
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the memory mapped mode */
    s_mem_mapped_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
    s_mem_mapped_cfg.TimeOutPeriod     = 0;

    if (HAL_QSPI_MemoryMapped(&hqspi1, &s_command, &s_mem_mapped_cfg) != HAL_OK)
    {
        return W25QXX_ERROR;
    }

    return W25QXX_OK;
}

W25QXX_Status_t W25QXX_Enter_QuadMode(void)
{
    QSPI_CommandTypeDef s_command;
    uint8_t status_reg2;

    // Read Status Register 2
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = READ_STATUS_REG2_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.NbData            = 1;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi1, &s_command, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
        return W25QXX_ERROR;
    }
    if (HAL_QSPI_Receive(&hqspi1, &status_reg2, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
        return W25QXX_ERROR;
    }

    // Check if QE bit is already set
    if (status_reg2 & QE_FLAG_BIT) {
        return W25QXX_OK;
    }
    
    // If not, set the QE bit
    status_reg2 |= QE_FLAG_BIT;

    // Enable Write
    if (W25QXX_Write_Enable() != W25QXX_OK) {
        return W25QXX_ERROR;
    }

    // Write back the modified Status Register 2
    s_command.Instruction = WRITE_STATUS_REG2_CMD;
    s_command.NbData      = 1;

    if (HAL_QSPI_Command(&hqspi1, &s_command, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
        return W25QXX_ERROR;
    }
    if (HAL_QSPI_Transmit(&hqspi1, &status_reg2, QSPI_TIMEOUT_DEFAULT) != HAL_OK) {
        return W25QXX_ERROR;
    }

    // Wait for write to complete
    if (W25QXX_Wait_Busy() != W25QXX_OK) {
        return W25QXX_TIMEOUT;
    }

    return W25QXX_OK;
} 