#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "device_properties.h"
#include <stdbool.h>
#include <stdint.h>

// --- Public Constants ---
#define MAX_MANAGED_DEVICES 10 // 定义网关可管理的最大子设备数量

// --- Public Data Structures ---

/**
 * @brief 统一的设备属性联合体
 * @note 使用 union 可以节省内存，因为一个设备实体只有一种属性。
 */
typedef union {
    GatewayProperties_t         gateway;
    ControlNodeProperties_t     control;
    ExternalSensorProperties_t  external_sensor;
    InternalSensorProperties_t  internal_sensor;
} DeviceProperties_u;

/**
 * @brief 单个设备的描述符结构体
 */
typedef struct {
    uint16_t            lora_id;        // 设备的LoRa网络ID (例如 0x01)
    const char*         cloud_device_id;// 设备在云平台的字符串ID (例如 "Internal_Sensor_1")
    DeviceType_e        device_type;    // 设备类型
    DeviceProperties_u  properties;     // 设备的具体属性
    bool                is_online;      // 设备是否在线 (通过心跳或数据更新)
    bool                is_dirty;       // 数据脏标记 (true 表示数据已更新但未上报云端)
    uint32_t            last_seen_ts;   // 设备最后一次通信的时间戳
} managed_device_t;


// --- Public Function Prototypes ---

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化设备管理器
 * @details
 *  - 初始化设备列表
 *  - 创建并初始化用于保护设备列表的互斥锁
 * @param None
 * @return None
 */
void DeviceManager_Init(void);

/**
 * @brief 更新内部传感器节点的数据
 * @note  这是一个线程安全的函数。
 * @param lora_id   要更新的设备的LoRa ID
 * @param data      指向包含新数据的 InternalSensorProperties_t 结构体的指针
 * @return bool - true: 更新成功; false: 设备ID未找到或类型不匹配
 */
bool DeviceManager_UpdateInternalSensorData(uint16_t lora_id, const InternalSensorProperties_t* data);

/**
 * @brief 更新控制节点的数据
 * @note  这是一个线程安全的函数。
 * @param lora_id   要更新的设备的LoRa ID
 * @param data      指向包含新数据的 ControlNodeProperties_t 结构体的指针
 * @return bool - true: 更新成功; false: 设备ID未找到或类型不匹配
 */
bool DeviceManager_UpdateControlNodeData(uint16_t lora_id, const ControlNodeProperties_t* data);

/**
 * @brief 更新外部传感器节点的数据
 * @note  这是一个线程安全的函数。
 * @param lora_id   要更新的设备的LoRa ID
 * @param data      指向包含新数据的 ExternalSensorProperties_t 结构体的指针
 * @return bool - true: 更新成功; false: 设备ID未找到或类型不匹配
 */
bool DeviceManager_UpdateExternalSensorData(uint16_t lora_id, const ExternalSensorProperties_t* data);

/**
 * @brief 获取指定设备的完整信息
 * @note  这是一个线程安全的函数。
 * @param lora_id 要查询的设备的LoRa ID
 * @param out_device 指向用于存储设备信息的 managed_device_t 结构体的指针
 * @return bool - true: 查找成功; false: 设备ID未找到
 */
bool DeviceManager_GetDevice(uint16_t lora_id, managed_device_t* out_device);

/**
 * @brief 设置云平台的在线状态
 * @param is_online true 表示云平台已连接，false 表示离线
 */
void DeviceManager_SetCloudOnlineStatus(bool is_online);

/**
 * @brief 查找需要上报数据的设备
 * @details
 *  - 遍历设备列表，查找 is_dirty 标记为 true 的设备。
 *  - 此函数通常由负责上报云端的任务调用。
 * @param start_index   开始搜索的索引 (用于循环遍历，初始调用时应为0)
 * @param out_device    如果找到，用于存储设备信息的指针
 * @return int - >=0: 找到的设备的索引; -1: 没有找到需要上报的设备
 */
int DeviceManager_FindNextDirtyDevice(int start_index, managed_device_t* out_device);

/**
 * @brief 清除指定设备的脏标记
 * @note  上报云端任务在上报成功后，应调用此函数。
 * @param lora_id 要清除脏标记的设备的LoRa ID
 */
void DeviceManager_ClearDirtyFlag(uint16_t lora_id);


#ifdef __cplusplus
}
#endif

#endif // DEVICE_MANAGER_H 