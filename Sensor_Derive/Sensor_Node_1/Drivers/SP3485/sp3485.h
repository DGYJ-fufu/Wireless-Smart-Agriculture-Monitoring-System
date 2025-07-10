#ifndef __SP3485_H
#define __SP3485_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

// 宏定义，用于控制是否打印调试信息
#define SP3485_DEBUG    0
#define SP3485_RX_BUFFER_SIZE 32 // 接收缓冲区大小

// 土壤传感器数据结构体
typedef struct
{
    float moisture;     // 含水率 (%)
    float temperature;  // 温度 (°C)
    uint16_t ec;        // 电导率 (μS/cm)
    float ph;           // PH值
    
    // 您可以根据需要在这里添加手册中提到的其他参数
    // ...

} Soil_Sensor_Data_t;

// 土壤传感器扩展数据结构体
typedef struct
{
    float moisture;     // 含水率 (%)
    float temperature;  // 温度 (°C)
    uint16_t ec;        // 电导率 (μS/cm)
    float ph;           // PH值
    uint16_t nitrogen;  // 氮含量
    uint16_t phosphorus;// 磷含量
    uint16_t potassium; // 钾含量
} Soil_Sensor_Extended_Data_t;

// 土壤传感器完整数据结构体 (10个值)
typedef struct
{
    float moisture;     // 含水率 (%)
    float temperature;  // 温度 (°C)
    uint16_t ec;        // 电导率 (μS/cm)
    float ph;           // PH值
    uint16_t nitrogen;  // 氮含量
    uint16_t phosphorus;// 磷含量
    uint16_t potassium; // 钾含量
    uint16_t salinity;  // 盐度
    uint16_t tds;       // 总溶解固体
    uint16_t fertility; // 肥力
} Soil_Sensor_Full_Data_t;

// 土壤传感器9参数数据结构体
typedef struct
{
    float moisture;     // 含水率 (%)
    float temperature;  // 温度 (°C)
    uint16_t ec;        // 电导率 (μS/cm)
    float ph;           // PH值
    uint16_t nitrogen;  // 氮含量
    uint16_t phosphorus;// 磷含量
    uint16_t potassium; // 钾含量
    uint16_t salinity;  // 盐度
    uint16_t tds;       // 总溶解固体
} Soil_Sensor_9_Values_Data_t;

/**
  * @brief 初始化SP3485驱动
  */
void SP3485_Init(void);

/**
  * @brief UART接收完成回调函数，应在系统的HAL_UART_RxCpltCallback中调用
  * @param huart 触发中断的UART句柄
  */
void SP3485_UART_RxCpltCallback(UART_HandleTypeDef *huart);

/**
  * @brief  (通用) 读取一个或多个Modbus保持寄存器
  * @param  slave_addr: 从机地址
  * @param  reg_addr:   要读取的寄存器起始地址
  * @param  num_regs:   要读取的寄存器数量
  * @param  dest_buffer: 用于存放读取结果的缓冲区，大小必须 >= num_regs * 2
  * @retval 0: 成功, 其他: 失败
  */
uint8_t SP3485_Read_Holding_Registers(uint8_t slave_addr, uint16_t reg_addr, uint16_t num_regs, uint8_t* dest_buffer);

/**
  * @brief  (专用) 读取土壤传感器标准数据 (水分、温度、EC、PH)
  * @param  slave_addr: 从机地址
  * @param  data:       用于存储解析后数据的结构体指针
  * @retval 0: 成功, 1: 失败
  */
uint8_t SP3485_Read_Soil_Data(uint8_t slave_addr, Soil_Sensor_Data_t *data);

/**
  * @brief  (专用) 读取土壤传感器扩展数据 (7个值)
  * @param  slave_addr: 从机地址
  * @param  data:       用于存储解析后数据的结构体指针
  * @retval 0: 成功, 1: 失败
  */
uint8_t SP3485_Read_Soil_Extended_Data(uint8_t slave_addr, Soil_Sensor_Extended_Data_t *data);

/**
  * @brief  (专用) 读取土壤传感器完整数据 (10个值)
  * @param  slave_addr: 从机地址
  * @param  data:       用于存储解析后数据的结构体指针
  * @retval 0: 成功, 1: 失败
  */
uint8_t SP3485_Read_Soil_Full_Data(uint8_t slave_addr, Soil_Sensor_Full_Data_t *data);

/**
  * @brief  (专用) 读取土壤传感器9项数据
  * @param  slave_addr: 从机地址
  * @param  data:       用于存储解析后数据的结构体指针
  * @retval 0: 成功, 1: 失败
  */
uint8_t SP3485_Read_Soil_9_Values_Data(uint8_t slave_addr, Soil_Sensor_9_Values_Data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* __SP3485_H */ 