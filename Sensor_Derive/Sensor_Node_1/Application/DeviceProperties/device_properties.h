#ifndef DEVICE_PROPERTIES_H
#define DEVICE_PROPERTIES_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 枚举，用于标识设备类型
 */
typedef enum {
    DEVICE_TYPE_GATEWAY,
    DEVICE_TYPE_CONTROL_NODE,
    DEVICE_TYPE_EXTERNAL_SENSOR,
    DEVICE_TYPE_INTERNAL_SENSOR,
    DEVICE_TYPE_UNKNOWN // 为默认情况添加一个类型
} DeviceType_e;

/**
 * @brief 所有子设备通用的元数据
 */
typedef struct {
    uint8_t batteryLevel; // 电池电量 (0-100)
    float batteryVoltage; // 电池电压 (4.2-3.0)
} CommonDeviceProperties_t;

/**
 * @brief 网关属性
 * @note 当前没有属性，但定义结构体以备将来使用
 */
typedef struct {
    uint8_t placeholder; // 占位符以避免空的结构体
} GatewayProperties_t;

/**
 * @brief 控制节点属性
 */
typedef struct {
    bool fanStatus;             // 风扇状态
    bool growLightStatus;       // 生长灯状态
    bool pumpStatus;            // 水泵状态
    bool shadeStatus;           // 遮阳帘状态
    uint8_t fanSpeed;           // 风扇速度 (0-100)
    uint8_t pumpSpeed;          // 水泵速度 (0-100)
} ControlNodeProperties_t;

/**
 * @brief 外部传感器属性
 */
#define LOCATION_MAX_LEN 64
typedef struct {
    double outdoorTemperature;      // 室外温度
    double outdoorHumidity;         // 室外湿度
    double outdoorLightIntensity;   // 室外光照强度
    double airPressure;             // 大气压
    char location[LOCATION_MAX_LEN]; // 位置信息
    CommonDeviceProperties_t common; // 通用属性
} ExternalSensorProperties_t;

/**
 * @brief 内部传感器属性
 */
typedef struct {
    // SHT40传感器数据
    double greenhouseTemperature;   // 温室温度
    double greenhouseHumidity;      // 温室湿度
    // 土壤传感器数据
    float       soilMoisture;             // 土壤含水率 (%)
    float       soilTemperature;          // 土壤温度 (°C)
    uint16_t    soilEc;                // 土壤电导率 (μS/cm)
    float       soilPh;                   // 土壤PH值
    uint16_t    soilNitrogen;          // 土壤氮含量
    uint16_t    soilPhosphorus;        // 土壤磷含量
    uint16_t    soilPotassium;         // 土壤钾含量
    uint16_t    soilSalinity;          // 土壤盐度
    uint16_t    soilTds;               // 土壤总溶解固体
    uint16_t    soilFertility;         // 土壤肥力
    // BH1750传感器数据 
    uint32_t lightIntensity;        // 光照强度
    // SGP30传感器数据
    uint16_t vocConcentration;      // VOC浓度 (0-60000)
    uint16_t co2Concentration;      // CO2浓度 (400-60000)
    CommonDeviceProperties_t common; // 通用属性
} InternalSensorProperties_t;

#endif // DEVICE_PROPERTIES_H 
