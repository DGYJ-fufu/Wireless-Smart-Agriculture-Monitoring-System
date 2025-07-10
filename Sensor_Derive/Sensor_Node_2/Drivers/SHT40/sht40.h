#ifndef __SHT40_H__
#define __SHT40_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "i2c.h"

// --- SHT40 I2C ��ַ ---
// SHT40��7λI2C��ַ�̶�Ϊ0x44��
#define SHT40_ADDRESS (0x44 << 1) // 8λд��ַ

// --- SHT40 ��� ---
#define SHT40_CMD_MEASURE_HPM       0xFD // ������ʪ�� - �߾���ģʽ (High Precision Measurement)
#define SHT40_CMD_MEASURE_MPM       0xF6 // ������ʪ�� - �о���ģʽ (Medium Precision Measurement)
#define SHT40_CMD_MEASURE_LPM       0xE0 // ������ʪ�� - �;���ģʽ (Low Precision Measurement)
#define SHT40_CMD_READ_SERIAL       0x89 // ��ȡ��������Ψһ���к�
#define SHT40_CMD_HEATER_200MW_1S   0x39 // ������������200mW������1��
#define SHT40_CMD_HEATER_200MW_01S  0x32 // ������������200mW������0.1��
#define SHT40_CMD_HEATER_110MW_1S   0x2F // ������������110mW������1��
#define SHT40_CMD_HEATER_110MW_01S  0x24 // ������������110mW������0.1��
#define SHT40_CMD_HEATER_20MW_1S    0x1E // ������������20mW������1��
#define SHT40_CMD_HEATER_20MW_01S   0x15 // ������������20mW������0.1��
#define SHT40_CMD_RESET             0x94 // ��λ

/**
 * @brief  ��SHT40��������ȡ�¶Ⱥ�ʪ�����ݡ�
 * @param  temperature: ָ�����ڴ洢�¶�ֵ��double���ͱ�����ָ�� (��λ: ��C)��
 * @param  humidity: ָ�����ڴ洢���ʪ��ֵ��double���ͱ�����ָ�� (��λ: %RH)��
 * @retval 0  - ��ȡ��У��ɹ���
 * @retval -1 - I2Cͨ��ʧ�ܻ�CRCУ��ʧ�ܡ�
 */
uint8_t SHT40_Read_RHData(double *temperature,double *humidity);


#ifdef __cplusplus
}
#endif

#endif /* __SHT40_H__ */
	
