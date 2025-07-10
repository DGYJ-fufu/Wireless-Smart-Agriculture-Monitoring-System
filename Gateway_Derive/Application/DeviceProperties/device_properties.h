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
    uint32_t outdoorLightIntensity;   // 室外光照强度
    double airPressure;             // 大气压
    double altitude;                // 海拔高度
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

// --- 函数声明 ---

/**
 * @brief 将经纬度数值格式化为"经度 N/S, 纬度 E/W"格式的字符串。
 *
 * @param latitude 纬度值 (十进制度)
 * @param lat_indicator 纬度方向 ('N' 或 'S')
 * @param longitude 经度值 (十进制度)
 * @param lon_indicator 经度方向 ('E' 或 'W')
 * @param buffer 指向用于存储结果字符串的缓冲区的指针
 * @param buffer_size 缓冲区的最大大小
 * @return 指向格式化后的字符串的指针 (即 buffer)，如果失败则返回 NULL。
 */
const char* format_location_string(double latitude, char lat_indicator, 
                                   double longitude, char lon_indicator,
                                   char* buffer, int buffer_size);

/**
 * @brief 从字符串（例如 "29.3532 N, 117.2370 E"）中解析出经纬度。
 *
 * @param location_string 指向包含位置信息的源字符串的指针
 * @param latitude 指向用于存储解析出的纬度值的变量的指针 (输出)
 * @param lat_indicator 指向用于存储纬度方向的变量的指针 (输出)
 * @param longitude 指向用于存储解析出的经度值的变量的指针 (输出)
 * @param lon_indicator 指向用于存储经度方向的变量的指针 (输出)
 * @return 如果成功解析了所有4个字段，则返回 true，否则返回 false。
 */
bool parse_location_string(const char* location_string, 
                           double* latitude, char* lat_indicator,
                           double* longitude, char* lon_indicator);

#endif // DEVICE_PROPERTIES_H 
