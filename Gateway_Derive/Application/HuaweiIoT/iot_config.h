/**
 * @file iot_config.h
 * @author Your Name
 * @brief 物联网应用核心配置文件
 * @version 1.0
 * @date 2025-06-08
 *
 * @copyright Copyright (c) 2025
 *
 * @par 使用说明:
 *      此文件集中管理所有与云平台和子设备相关的配置信息。
 *      修改此文件中的宏定义和数组，即可改变设备连接的目标平台和拓扑结构，
 *      无需改动任何业务逻辑代码。
 */

#ifndef IOT_CONFIG_H
#define IOT_CONFIG_H

#include "lora_protocol.h"

//==============================================================================
// 1. 云平台连接配置 (华为云物联网平台)
//==============================================================================

/**
 * @brief MQTT服务器地址
 *        从华为云物联网平台获取。
 */
#define IOT_SERVER_ADDRESS      "xxxxxxxxxxxxxxxxxxxx"

/**
 * @brief MQTT服务器端口
 *        标准MQTT端口为 "1883"。
 */
#define IOT_SERVER_PORT         "1883"

/**
 * @brief 网关设备ID
 *        在华为云平台上创建的网关设备的ID。
 */
#define IOT_DEVICE_ID           "xxxxxxxx"

/**
 * @brief 网关设备密钥
 *        对应设备ID的连接密钥。
 */
#define IOT_DEVICE_PASSWORD     "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"


//==============================================================================
// 2. 子设备配置中心
//==============================================================================
#include "device_properties.h" // 引入设备类型枚举

/**
 * @brief 设备配置信息结构体
 * @details 将一个设备的所有关键ID和类型信息绑定在一起。
 */
typedef struct {
    const char*   cloud_id;      ///< 云平台上的设备ID (例如 "Internal_Sensor_1")
    uint16_t      lora_id;       ///< LoRa网络中的内部ID (例如 0x01)
    DeviceType_e  type;          ///< 设备类型 (例如 DEVICE_TYPE_INTERNAL_SENSOR)
} device_config_t;

/**
 * @brief 全局设备配置表
 * @details
 *        这是整个系统的设备"真理之源"。所有模块都将从此获取设备信息。
 *        要添加、删除或修改一个设备，只需修改此表即可。
 *        - cloud_id: 必须与您在华为云平台上创建的子设备ID完全一致。
 *        - lora_id:  必须与您在LoRa节点设备固件中定义的ID一致。
 */
static const device_config_t DEVICE_CONFIG_TABLE[] = {
    {.cloud_id = "Internal_Sensor_1", .lora_id = DEVICE_TYPE_SENSOR_Internal, .type = DEVICE_TYPE_INTERNAL_SENSOR},
    {.cloud_id = "Control_Node_1",    .lora_id = DEVICE_TYPE_CONTROL, .type = DEVICE_TYPE_CONTROL_NODE},
    {.cloud_id = "External_Sensor_1", .lora_id = DEVICE_TYPE_SENSOR_External, .type = DEVICE_TYPE_EXTERNAL_SENSOR}, // 示例：可以轻松添加更多设备
};

/**
 * @brief 子设备总数
 * @details 此宏自动计算配置表中的设备数量，无需手动修改。
 */
#define DEVICE_CONFIG_COUNT (sizeof(DEVICE_CONFIG_TABLE) / sizeof(DEVICE_CONFIG_TABLE[0]))


#endif // IOT_CONFIG_H 
 