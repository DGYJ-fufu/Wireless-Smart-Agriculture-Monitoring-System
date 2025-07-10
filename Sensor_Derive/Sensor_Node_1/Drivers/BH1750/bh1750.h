#ifndef __BH1750_H__
#define __BH1750_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "i2c.h"
#include "main.h"


#define	  BH1750_ADDRESS   0xB8 //����������IIC�����еĴӵ�ַ,����ALT  ADDRESS��ַ���Ų�ͬ�޸�
                              //ALT  ADDRESS���Žӵ�ʱ��ַΪ0x46���ӵ�Դʱ��ַΪ0xB8

 
int Init_BH1750(void);
int BH1750_GetDate(uint16_t *light);


#ifdef __cplusplus
}
#endif

#endif /* __BH1750_H__ */
