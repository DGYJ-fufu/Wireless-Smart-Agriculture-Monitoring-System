/**
 * @file device_manager.c
 * @brief 设备属性的集中式、线程安全管理器
 * @details
 *  - 使用一个静态数组作为所有子设备的数据库。
 *  - 使用 FreeRTOS 互斥锁来保护对数据库的并发访问。
 *  - 为上层应用（LoRa, Cloud, GUI）提供统一的数据访问接口。
 */

#include "device_manager.h"
#include "cmsis_os2.h"
#include "iot_config.h" // 引入配置中心
#include <string.h>

// --- Private Variables ---

// 设备列表 "数据库"
static managed_device_t g_device_list[MAX_MANAGED_DEVICES];
static uint8_t g_registered_device_count = 0;

// 用于保护设备列表的互斥锁
static osMutexId_t g_device_list_mutex;

// 云平台在线状态
static bool g_is_cloud_online = false;


// --- Private Function Prototypes ---
static int find_device_index(uint16_t lora_id);

// --- Public Function Implementations ---

/**
 * @brief 初始化设备管理器
 */
void DeviceManager_Init(void)
{
    // 1. 创建互斥锁
    const osMutexAttr_t mutex_attributes = {
        .name = "DeviceListMutex",
        .attr_bits = osMutexRecursive | osMutexPrioInherit,
        .cb_mem = NULL,
        .cb_size = 0U
    };
    g_device_list_mutex = osMutexNew(&mutex_attributes);
    if (g_device_list_mutex == NULL) {
        // 错误处理：互斥锁创建失败
        return;
    }
    
    // 2. 清空设备列表
    memset(g_device_list, 0, sizeof(g_device_list));
    g_registered_device_count = 0;

    // 3. 从配置中心加载并注册所有设备
    if (DEVICE_CONFIG_COUNT > MAX_MANAGED_DEVICES) {
        // 错误处理：配置的设备数超过了管理器容量
        return;
    }

    for (uint8_t i = 0; i < DEVICE_CONFIG_COUNT; i++) {
        g_device_list[i].lora_id = DEVICE_CONFIG_TABLE[i].lora_id;
        g_device_list[i].cloud_device_id = DEVICE_CONFIG_TABLE[i].cloud_id;
        g_device_list[i].device_type = DEVICE_CONFIG_TABLE[i].type;
        g_device_list[i].is_online = false;
        g_device_list[i].is_dirty = false;
    }
    g_registered_device_count = DEVICE_CONFIG_COUNT;
}

/**
 * @brief 更新内部传感器节点的数据
 */
bool DeviceManager_UpdateInternalSensorData(uint16_t lora_id, const InternalSensorProperties_t* data)
{
    bool success = false;
    osMutexAcquire(g_device_list_mutex, osWaitForever);

    int index = find_device_index(lora_id);
    if (index != -1 && g_device_list[index].device_type == DEVICE_TYPE_INTERNAL_SENSOR) {
        g_device_list[index].properties.internal_sensor = *data;
        g_device_list[index].is_online = true;
        g_device_list[index].last_seen_ts = osKernelGetTickCount();
        if (g_is_cloud_online) {
            g_device_list[index].is_dirty = true; // 如果云在线，则标记为待上报
        }
        success = true;
    }

    osMutexRelease(g_device_list_mutex);
    return success;
}

/**
 * @brief 更新控制节点的数据
 */
bool DeviceManager_UpdateControlNodeData(uint16_t lora_id, const ControlNodeProperties_t* data)
{
    bool success = false;
    osMutexAcquire(g_device_list_mutex, osWaitForever);

    int index = find_device_index(lora_id);
    if (index != -1 && g_device_list[index].device_type == DEVICE_TYPE_CONTROL_NODE) {
        g_device_list[index].properties.control = *data;
        g_device_list[index].is_online = true;
        g_device_list[index].last_seen_ts = osKernelGetTickCount();
        if (g_is_cloud_online) {
            g_device_list[index].is_dirty = true;
        }
        success = true;
    }

    osMutexRelease(g_device_list_mutex);
    return success;
}

/**
 * @brief 更新外部传感器节点的数据
 */
bool DeviceManager_UpdateExternalSensorData(uint16_t lora_id, const ExternalSensorProperties_t* data)
{
    bool success = false;
    osMutexAcquire(g_device_list_mutex, osWaitForever);

    int index = find_device_index(lora_id);
    if (index != -1 && g_device_list[index].device_type == DEVICE_TYPE_EXTERNAL_SENSOR) {
        g_device_list[index].properties.external_sensor = *data;
        g_device_list[index].is_online = true;
        g_device_list[index].last_seen_ts = osKernelGetTickCount();
        if (g_is_cloud_online) {
            g_device_list[index].is_dirty = true;
        }
        success = true;
    }

    osMutexRelease(g_device_list_mutex);
    return success;
}

/**
 * @brief 获取指定设备的完整信息
 */
bool DeviceManager_GetDevice(uint16_t lora_id, managed_device_t* out_device)
{
    bool success = false;
    if (out_device == NULL) return false;

    osMutexAcquire(g_device_list_mutex, osWaitForever);
    
    int index = find_device_index(lora_id);
    if (index != -1) {
        *out_device = g_device_list[index];
        success = true;
    }

    osMutexRelease(g_device_list_mutex);
    return success;
}

/**
 * @brief 设置云平台的在线状态
 */
void DeviceManager_SetCloudOnlineStatus(bool is_online)
{
    osMutexAcquire(g_device_list_mutex, osWaitForever);
    g_is_cloud_online = is_online;
    // 如果云平台刚刚上线，可以将所有在线设备都标记为 dirty，以强制全量上报一次
    if (is_online) {
        for (int i = 0; i < g_registered_device_count; i++) {
            if (g_device_list[i].is_online) {
                g_device_list[i].is_dirty = true;
            }
        }
    }
    osMutexRelease(g_device_list_mutex);
}

/**
 * @brief 查找需要上报数据的设备
 */
int DeviceManager_FindNextDirtyDevice(int start_index, managed_device_t* out_device)
{
    int found_index = -1;
    if (out_device == NULL || start_index < 0) return -1;

    osMutexAcquire(g_device_list_mutex, osWaitForever);

    for (int i = start_index; i < g_registered_device_count; i++) {
        if (g_device_list[i].is_dirty) {
            *out_device = g_device_list[i];
            found_index = i;
            break;
        }
    }

    osMutexRelease(g_device_list_mutex);
    return found_index;
}


/**
 * @brief 清除指定设备的脏标记
 */
void DeviceManager_ClearDirtyFlag(uint16_t lora_id)
{
    osMutexAcquire(g_device_list_mutex, osWaitForever);
    
    int index = find_device_index(lora_id);
    if (index != -1) {
        g_device_list[index].is_dirty = false;
    }

    osMutexRelease(g_device_list_mutex);
}


// --- Private Function Implementations ---

/**
 * @brief 根据设备 LoRa ID 查找其在列表中的索引 (内部函数，无锁)
 * @return int - >=0: 索引; -1: 未找到
 */
static int find_device_index(uint16_t lora_id)
{
    for (int i = 0; i < g_registered_device_count; i++) {
        if (g_device_list[i].lora_id == lora_id) {
            return i;
        }
    }
    return -1;
} 