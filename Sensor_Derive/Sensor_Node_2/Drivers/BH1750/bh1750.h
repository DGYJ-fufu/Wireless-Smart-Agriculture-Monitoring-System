#ifndef __BH1750_H__
#define __BH1750_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "i2c.h"
#include "main.h"

// BH1750 I2C �ӻ���ַ (8λ��ʽ)
// ALT ADDRESS ���Žӵ�ʱ��7λ��ַΪ 0x23��8λ��ַΪ 0x46��
// ALT ADDRESS ���Ž�VCCʱ��7λ��ַΪ 0x5C��8λ��ַΪ 0xB8��
#define	BH1750_ADDRESS 0xB8

/**
 * @brief  ��ʼ��BH1750���մ�������
 * @retval 0  - ��ʼ���ɹ���
 * @retval -1 - ��ʼ��ʧ�ܡ�
 */
int Init_BH1750(void);

/**
 * @brief  ��BH1750��ȡ��ǰ�Ĺ���ǿ��ֵ��
 * @param  light: ָ�����ڴ洢����ǿ��ֵ�ı�����ָ�� (��λ: lux)��
 * @retval 0  - ��ȡ�ɹ���
 * @retval -1 - ��ȡʧ�ܡ�
 */
int BH1750_GetDate(uint16_t *light);


#ifdef __cplusplus
}
#endif

#endif /* __BH1750_H__ */
