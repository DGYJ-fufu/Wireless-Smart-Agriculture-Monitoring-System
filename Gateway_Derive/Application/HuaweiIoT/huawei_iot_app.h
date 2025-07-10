/**
 * @file huawei_iot_app.h
 * @author Your Name
 * @brief 华为云物联网应用层协议封装头文件
 * @version 2.0
 * @date 2025-06-08
 *
 * @copyright Copyright (c) 2025
 *
 * @par 模块功能:
 *      - 封装连接到华为云物联网平台所需的业务流程。
 *      - 封装上报所有子设备状态的业务逻辑。
 *      - 提供一个命令分发器，用于解析云端下发的命令并调用相应的处理函数。
 *      - 提供cJSON库的内存管理钩子初始化。
 */

#ifndef __HUAWEI_IOT_APP_H
#define __HUAWEI_IOT_APP_H

#include "at_handler.h"
#include "cJSON.h"
#include "iot_config.h"
#include "device_properties.h"

/**
 * @brief 命令处理函数的函数指针类型
 * @param paras 指向cJSON对象的指针，该对象包含了从云端下发的命令参数。
 */
typedef void (*command_handler_t)(const cJSON *paras);

/**
 * @brief 华为云应用层函数返回状态码
 * @note  目前此枚举未使用，但为未来扩展保留。
 */
typedef enum {
    HUAWEI_IOT_OK = 0,
    HUAWEI_IOT_ERROR,
    HUAWEI_IOT_MALLOC_FAILED,
    HUAWEI_IOT_JSON_ERROR
} HuaweiIoT_StatusTypeDef;

/*
 * Note: The main URC handling logic and initialization (AppInit)
 * has been moved to main.c in the user's project.
 */

// --- Public Function Prototypes ---

/**
 * @brief 初始化华为云物联网应用模块
 * @details
 *        此函数为cJSON库配置基于FreeRTOS的内存管理函数(pvPortMalloc/vPortFree)。
 *        它必须在系统启动初期、在调用任何其他华为云应用函数之前被调用一次。
 */
void HuaweiIoT_Init(void);

/**
 * @brief 执行连接云平台的完整业务序列
 * @details
 *        此函数包含了检查网络、激活IP、连接MQTT等一系列操作。
 *        它被设计为可重入的，用于系统首次连接及后续的断线重连。
 * @param at_handler AT处理器实例指针
 * @return AT_Status_t AT命令执行的状态
 */
AT_Status_t HuaweiIoT_ConnectCloud(AT_Handler_t *at_handler);

/**
 * @brief 上报所有在 iot_config.h 中定义的子设备为"ONLINE"状态
 * @details
 *        此函数会动态构建一个符合华为云物模型规范的JSON负载，
 *        然后通过AT命令将其发布到云平台。
 * @param at_handler AT处理器实例指针
 * @return AT_Status_t AT命令执行的状态
 */
AT_Status_t HuaweiIoT_PublishAllSubDevicesOnline(AT_Handler_t *at_handler);

/**
 * @brief 解析并处理来自模块的 "+HMREC" (云端下发命令) URC
 * @details
 *        此函数被注册为URC回调，专门用于处理华为云的命令下发。
 *        它会解析URC，提取命令名称和参数，然后通过内部的命令分发器
 *        找到并执行对应的处理函数，最后调用响应函数将执行结果返回给云平台。
 * @param at_handler AT处理器实例指针
 * @param hprec_str  模块上报的完整URC字符串
 */
void HuaweiIoT_ParseHMREC(AT_Handler_t *at_handler, const char *hprec_str);

/**
 * @brief (内部使用) 构建并发送对云端命令的响应
 * @details
 *        此函数由 HuaweiIoT_ParseHMREC 内部调用。它使用非阻塞的
 *        "发后即忘" 方式发送响应，以避免阻塞URC处理流程。
 * @param at_handler AT处理器实例指针
 * @param request_id 从云端命令中提取的请求ID
 * @param escaped_payload 已经转义好的JSON负载 (例如 "{\\\"result_code\\\":0}")
 * @param logical_len JSON负载的逻辑长度 (未转义前的长度)
 * @return AT_Status_t AT命令发送的状态
 */
AT_Status_t HuaweiIoT_PublishCommandResponse(AT_Handler_t *at_handler, const char *request_id, const char *escaped_payload, uint16_t logical_len);

/**
 * @brief 构建并上报一个包含多个子设备属性的聚合报告 (网关模式)
 * @details
 *        此函数是网关数据上报的核心。它会：
 *        1. 在 DeviceManager 中查找所有带有"脏"标记的子设备。
 *        2. 如果找到，它会构建一个符合华为云网关上报规范的复杂JSON对象。
 *        3. 将所有脏设备的数据填充到JSON中。
 *        4. 发送包含此JSON的AT命令到正确的网关Topic。
 *        5. 如果发送成功，则清除所有已上报设备的"脏"标记。
 * @param handler AT处理器实例指针
 * @return AT_Status_t
 *         - AT_OK: 成功发送上报，或没有需要上报的数据。
 *         - AT_ERROR: 发送失败。
 *         - AT_MALLOC_FAILED: JSON对象内存分配失败。
 */
AT_Status_t HuaweiIoT_PublishGatewayReport(AT_Handler_t *handler);

#endif /* __HUAWEI_IOT_APP_H */ 
