#ifndef __BH1750_H__
#define __BH1750_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "i2c.h"
#include "main.h"


#define	  BH1750_ADDRESS   0xB8 //定义器件在IIC总线中的从地址,根据ALT  ADDRESS地址引脚不同修改
                              //ALT  ADDRESS引脚接地时地址为0x46，接电源时地址为0xB8

 
int Init_BH1750(void);
int BH1750_GetDate(uint16_t *light);


#ifdef __cplusplus
}
#endif

#endif /* __BH1750_H__ */
