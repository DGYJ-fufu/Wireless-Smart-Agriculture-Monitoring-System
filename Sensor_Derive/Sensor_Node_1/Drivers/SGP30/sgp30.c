#include "sgp30.h"
#include "i2c.h"


#define SPG30_IIC_USE_HARDWARE 
#define SGP30_IIC_Handle (&hi2c3)


static void sgp30_delay_ms(uint32_t nms)
{
	HAL_Delay(nms);
}


static int sgp30_iic_write(uint8_t addr, const uint8_t* buf, uint32_t len)
{
    if (HAL_I2C_Master_Transmit(SGP30_IIC_Handle, addr, (uint8_t *)buf, len, 100) == HAL_OK)
        return 0;
    return -1;
}

static int sgp30_iic_read(uint8_t addr, uint8_t* buf, uint32_t len)
{
    if (HAL_I2C_Master_Receive(SGP30_IIC_Handle, addr, (uint8_t *)buf, len, 100) == HAL_OK)
        return 0;
    return -1;
}

static uint8_t sgp30_checksum(const uint8_t* buf, uint32_t len);

int sgp30_get_serial_id(uint8_t id[6])
{
    uint8_t buf[32];
    uint8_t crc[3];

    buf[0] = (SGP30_CMD_GET_SERIAL_ID & 0XFF00) >> 8;
    buf[1] = (SGP30_CMD_GET_SERIAL_ID & 0X00FF);

    if (sgp30_iic_write(SGP30_ADDR_WRITE, buf, 2) < 0)
        return -1;

    if (sgp30_iic_read(SGP30_ADDR_READ, buf, 9) < 0)
        return -2;

    crc[0] = buf[2];
    crc[1] = buf[5];
    crc[2] = buf[8];

    id[0]  = buf[0];
    id[1]  = buf[1];
    id[2]  = buf[3];
    id[3]  = buf[4];
    id[4]  = buf[6];
    id[5]  = buf[7];

    if(
        sgp30_checksum(&id[0], 2) != crc[0] ||
        sgp30_checksum(&id[2], 2) != crc[1] ||
        sgp30_checksum(&id[4], 2) != crc[2]
    )
        return -3;

    return 0;
}

static uint8_t sgp30_checksum(const uint8_t* buf, uint32_t len)
{
	const uint8_t Polynomial = 0x31;
	uint8_t Initialization = 0XFF;
    uint8_t i = 0, k = 0;
	while(i < len)
	{
		Initialization ^= buf[i++];
		for(k = 0; k < 8; k++)
		{
			if(Initialization & 0X80)
				Initialization = (Initialization << 1) ^ Polynomial;
			else
				Initialization = (Initialization << 1);
		}
	}
	return Initialization;
}

int sgp30_soft_reset(void)
{
    uint8_t cmd = 0X06;
    return sgp30_iic_write(0X00, &cmd, 1);
}

int sgp30_init(void)
{
    uint8_t buf[2];

    if (sgp30_soft_reset() < 0)
        return -2;

    sgp30_delay_ms(50);

    buf[0] = (SGP30_CMD_INIT_AIR_QUALITY & 0XFF00) >> 8;
    buf[1] = (SGP30_CMD_INIT_AIR_QUALITY & 0X00FF);

    if (sgp30_iic_write(SGP30_ADDR_WRITE, buf, 2) < 0)
        return -3;

    return 0;
}

int sgp30_read(uint16_t* CO2, uint16_t* TVOC)
{
    uint8_t buf[8] = {0};

    buf[0] = (SGP30_CMD_MEASURE_AIR_QUALITY & 0XFF00) >> 8;
    buf[1] = (SGP30_CMD_MEASURE_AIR_QUALITY & 0X00FF);

    if (sgp30_iic_write(SGP30_ADDR_WRITE, buf, 2) < 0)
        return -1;

    sgp30_delay_ms(10);

    if (sgp30_iic_read(SGP30_ADDR_READ, buf, 6) < 0)
        return -2;

    if (sgp30_checksum(&buf[3], 2) != buf[5])
        return -3;

    if (CO2 != NULL)    *CO2  = (buf[0] << 8) | buf[1];
    if (TVOC != NULL)   *TVOC = (buf[3] << 8) | buf[4];

    return 0;
}

