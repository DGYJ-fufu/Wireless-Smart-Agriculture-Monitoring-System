#include "huawei_iot_app.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "cmsis_os2.h"
#include "main.h"              // 包含 main.h 以便使用 GPIO 定义 (LED_Pin, LED_GPIO_Port)
#include "iot_config.h"        // 包含新的配置文件
#include "cJSON.h"             // 引入 cJSON 库
#include "device_properties.h" // 1. 引入新创建的设备属性库
#include "FreeRTOS.h"
#include "device_manager.h" // 需要访问设备管理器
#include "lora_app.h"       // 引入LoRa应用层，用于发送数据
#include "lora_protocol.h"  // 引入LoRa协议层，用于封装数据帧

// The URC handling logic (callback table, init function) has been moved to main.c,
// as the user has a more advanced implementation there.
// This file will now only contain the helper functions for building and parsing Huawei IoT commands.

// --- Device Properties Instance ---

// --- Hardware Handles ---
extern RNG_HandleTypeDef hrng; // 声明外部RNG句柄

/**
 * @brief 控制节点的属性实例
 * @details
 *        这是一个静态全局变量，用于存储控制节点所有属性的当前状态。
 *        它是设备在本地的"数字孪生"。云端命令会修改它，而硬件状态会根据它来更新。
 */
static ControlNodeProperties_t g_controlNodeProps = {
    .fanStatus = false,
    .growLightStatus = false,
    .pumpStatus = false,
    .fanSpeed = 0,
    .pumpSpeed = 0,
};

/* Private Type Definitions --------------------------------------------------*/

static uint8_t s_lora_tx_buffer[LORA_MAX_RAW_PACKET] = {0}; // LoRa发送缓冲区

// --- Command Dispatcher Implementation ---

// 定义命令处理函数的函数指针类型 (修正：使用 const cJSON*)
typedef void (*command_handler_t)(const cJSON *paras);

/* Private Type Definitions --------------------------------------------------*/

/**
 * @brief 命令表条目结构体
 *        用于将云端下发的命令名称字符串与本地的处理函数进行绑定。
 */
typedef struct
{
    const char *command_name;  ///< 命令名称 (来自云端物模型定义)
    command_handler_t handler; ///< 指向该命令处理函数的指针
} command_entry_t;

/* Private Function Prototypes ---------------------------------------------*/
// 为保持代码可读性，所有私有函数在使用前都进行了定义，此处无需前置声明。

/* Private Functions ---------------------------------------------------------*/

/**
 * @brief 构建并发送一个LoRa指令数据包
 * @details
 *        这是一个核心辅助函数，它封装了构建和发送LoRa帧的完整流程。
 *        1. 使用硬件RNG生成一个随机的序列号(seq_num)，用于追踪或调试。
 *        2. 调用协议层的 `generate_lora_frame` 函数将应用数据打包成LoRa帧。
 *        3. 调用 `LoRa_APP_Send` 将打包好的帧放入发送队列。
 * @param command_payload 指向要发送的指令负载的指针
 * @param payload_length 指令负载的长度
 */
static void send_lora_command(const uint8_t *command_payload, uint8_t payload_length)
{
    if (command_payload == NULL || payload_length == 0)
    {
        printf("[LoRa] Invalid command data to send.\r\n");
        return;
    }

    uint32_t random_seq_num = 0;
    if (HAL_RNG_GenerateRandomNumber(&hrng, &random_seq_num) != HAL_OK)
    {
        // 如果RNG失败，使用一个基于时间的伪随机数作为备用
        random_seq_num = osKernelGetTickCount();
        printf("[LoRa] WARN: RNG failed, using Tick as SeqNum.\r\n");
    }

    // 调用协议层函数生成LoRa帧
    int frame_len = generate_lora_frame(
        DEVICE_TYPE_CONTROL,
        LORA_HOST_ADDRESS,
        MSG_TYPE_CMD_SET_CONFIG, // 消息类型: 设置命令
        (uint8_t)random_seq_num, // 序列号
        command_payload,         // 指令负载
        payload_length,
        s_lora_tx_buffer, // 输出缓冲区
        sizeof(s_lora_tx_buffer));

    if (frame_len > 0)
    {
        // LoRa_APP_Send 是线程安全的，可以直接调用
        if (LoRa_APP_Send(s_lora_tx_buffer, frame_len))
        {
            // 打印原始LoRa数据以供调试
            printf("[LoRa CMD] Sent %d bytes (Seq: %u): ", frame_len, (unsigned int)random_seq_num);
            for (int i = 0; i < frame_len; i++)
            {
                printf("%02X ", s_lora_tx_buffer[i]);
            }
            printf("\r\n");
        }
        else
        {
            printf("[LoRa CMD] Send failed. TX queue might be full.\r\n");
        }

        // [FIX] 关键修复：添加延时以避免总线竞争
        // 在请求LoRa发送后，当前任务（AT命令处理任务）会立刻尝试通过UART发送AT响应。
        // 而此时LoRa任务可能正在通过SPI与芯片通信。这会导致底层DMA或总线资源冲突。
        // 此延时给予LoRa任务足够的时间来完成SPI操作，从而错开总线访问高峰。
        osDelay(200);
    }
    else
    {
        printf("[LoRa CMD] Frame generation failed.\r\n");
    }
}

static void update_hardware_from_properties(void)
{
    // TODO: 未来应根据实际连接的硬件编写此函数
    // 例如：Fan_SetStatus(g_controlNodeProps.fanStatus);
    printf("[HARDWARE] Updating hardware based on properties...\r\n");
    printf("[HARDWARE]   - Fan Status: %s\r\n", g_controlNodeProps.fanStatus ? "ON" : "OFF");
    printf("[HARDWARE]   - Grow Light: %s\r\n", g_controlNodeProps.growLightStatus ? "ON" : "OFF");
    printf("[HARDWARE]   - Pump Status: %s\r\n", g_controlNodeProps.pumpStatus ? "ON" : "OFF");
    printf("[HARDWARE]   - Fan Speed: %d\r\n", g_controlNodeProps.fanSpeed);
    printf("[HARDWARE]   - Pump Speed: %d\r\n", g_controlNodeProps.pumpSpeed);
}

/**
 * @brief "setFanStatus" 命令的处理函数
 * @details
 *        根据云端下发的布尔值参数 `status` 控制风扇（或示例中的LED）。
 *        此函数是命令分发表 `command_table` 的一个成员。
 * @param paras cJSON对象，包含了命令的所有参数
 */
static void handle_setFanStatus(const cJSON *paras)
{
    if (!cJSON_IsObject(paras))
    {
        printf("[CMD_HANDLER] 'paras' is not an object for setFanStatus.\r\n");
        return;
    }

    const cJSON *status_item = cJSON_GetObjectItem(paras, "status");
    uint8_t cmd[2]; // 使用局部变量构建指令负载

    if (cJSON_IsTrue(status_item))
    {
        printf("[ACTION] Updating 'fanStatus' property to: ON\r\n");
        g_controlNodeProps.fanStatus = true;
        cmd[0] = CONTROLLER_DEVICE_TYPE_STATUS_FAN;
        cmd[1] = 0x01; // 1 for ON
    }
    else if (cJSON_IsFalse(status_item))
    {
        printf("[ACTION] Updating 'fanStatus' property to: OFF\r\n");
        g_controlNodeProps.fanStatus = false;
        cmd[0] = CONTROLLER_DEVICE_TYPE_STATUS_FAN;
        cmd[1] = 0x00; // 0 for OFF
    }
    else
    {
        printf("[CMD_HANDLER] 'status' not found or not a boolean in setFanStatus.\r\n");
        return; // 参数错误，不发送指令
    }

    // 将构建好的指令通过LoRa发送出去
    send_lora_command(cmd, sizeof(cmd));
}

/**
 * @brief "setGrowLightStatus" 命令的处理函数
 * @details
 *        根据云端下发的布尔值参数 `status` 控制植物生长灯。
 *        此函数是命令分发表 `command_table` 的一个成员。
 * @param paras cJSON对象，包含了命令的所有参数
 */
static void handle_setGrowLightStatus(const cJSON *paras)
{
    if (!cJSON_IsObject(paras))
    {
        printf("[CMD_HANDLER] 'paras' is not an object for setGrowLightStatus.\r\n");
        return;
    }

    const cJSON *status_item = cJSON_GetObjectItem(paras, "status");
    uint8_t cmd[2]; // 使用局部变量构建指令负载

    if (cJSON_IsTrue(status_item))
    {
        printf("[ACTION] Updating 'growLightStatus' property to: ON\r\n");
        g_controlNodeProps.growLightStatus = true;
        cmd[0] = CONTROLLER_DEVICE_TYPE_STATUS_LIGHT;
        cmd[1] = 0x01; // 1 for ON
    }
    else if (cJSON_IsFalse(status_item))
    {
        printf("[ACTION] Updating 'growLightStatus' property to: OFF\r\n");
        g_controlNodeProps.growLightStatus = false;
        cmd[0] = CONTROLLER_DEVICE_TYPE_STATUS_LIGHT;
        cmd[1] = 0x00; // 0 for OFF
    }
    else
    {
        printf("[CMD_HANDLER] 'status' not found or not a boolean in setGrowLightStatus.\r\n");
        return; // 参数错误，不发送指令
    }

    // 将构建好的指令通过LoRa发送出去
    send_lora_command(cmd, sizeof(cmd));
}

/**
 * @brief "setPumpStatus" 命令的处理函数
 * @details
 *        根据云端下发的布尔值参数 `status` 控制水泵。
 *        此函数是命令分发表 `command_table` 的一个成员。
 * @param paras cJSON对象，包含了命令的所有参数
 */
static void handle_setPumpStatus(const cJSON *paras)
{
    if (!cJSON_IsObject(paras))
    {
        printf("[CMD_HANDLER] 'paras' is not an object for setPumpStatus.\r\n");
        return;
    }

    const cJSON *status_item = cJSON_GetObjectItem(paras, "status");
    uint8_t cmd[2]; // 使用局部变量构建指令负载

    if (cJSON_IsTrue(status_item))
    {
        printf("[ACTION] Updating 'PumpStatus' property to: ON\r\n");
        g_controlNodeProps.pumpStatus = true;
        cmd[0] = CONTROLLER_DEVICE_TYPE_STATUS_PUMP;
        cmd[1] = 0x01; // 1 for ON
    }
    else if (cJSON_IsFalse(status_item))
    {
        printf("[ACTION] Updating 'PumpStatus' property to: OFF\r\n");
        g_controlNodeProps.growLightStatus = false;
        cmd[0] = CONTROLLER_DEVICE_TYPE_STATUS_PUMP;
        cmd[1] = 0x00; // 0 for OFF
    }
    else
    {
        printf("[CMD_HANDLER] 'status' not found or not a boolean in setPumpStatus.\r\n");
        return; // 参数错误，不发送指令
    }

    // 将构建好的指令通过LoRa发送出去
    send_lora_command(cmd, sizeof(cmd));
}

/**
 * @brief "setFanSpeed" 命令的处理函数
 * @details
 *        根据云端下发的整数参数 `speed` 控制风扇速度。
 *        此函数是命令分发表 `command_table` 的一个成员。
 * @param paras cJSON对象，包含了命令的所有参数
 */
static void handle_setFanSpeed(const cJSON *paras)
{
    if (!cJSON_IsObject(paras))
    {
        printf("[CMD_HANDLER] 'paras' is not an object for setFanSpeed.\r\n");
        return;
    }

    const cJSON *speed_item = cJSON_GetObjectItem(paras, "speed");
    uint8_t cmd[2]; // 使用局部变量构建指令负载

    if (cJSON_IsNumber(speed_item))
    {
        uint8_t speed = (uint8_t)speed_item->valuedouble;
        printf("[ACTION] Updating 'FanSpeed' property to: %d\r\n", speed);
        g_controlNodeProps.fanSpeed = speed;
        cmd[0] = CONTROLLER_DEVICE_TYPE_SPEED_FAN;
        cmd[1] = speed;
    }
    else
    {
        printf("[CMD_HANDLER] 'speed' not found or not a number in setFanSpeed.\r\n");
        return; // 参数错误，不发送指令
    }

    // 将构建好的指令通过LoRa发送出去
    send_lora_command(cmd, sizeof(cmd));
}

/**
 * @brief "setPumpSpeed" 命令的处理函数
 * @details
 *        根据云端下发的整数参数 `speed` 控制水泵速度。
 *        此函数是命令分发表 `command_table` 的一个成员。
 * @param paras cJSON对象，包含了命令的所有参数
 */
static void handle_setPumpSpeed(const cJSON *paras)
{
    if (!cJSON_IsObject(paras))
    {
        printf("[CMD_HANDLER] 'paras' is not an object for setPumpSpeed.\r\n");
        return;
    }

    const cJSON *speed_item = cJSON_GetObjectItem(paras, "speed");
    uint8_t cmd[2]; // 使用局部变量构建指令负载

    if (cJSON_IsNumber(speed_item))
    {
        uint8_t speed = (uint8_t)speed_item->valuedouble;
        printf("[ACTION] Updating 'PumpSpeed' property to: %d\r\n", speed);
        g_controlNodeProps.pumpSpeed = speed;
        cmd[0] = CONTROLLER_DEVICE_TYPE_SPEED_PUMP;
        cmd[1] = speed;
    }
    else
    {
        printf("[CMD_HANDLER] 'speed' not found or not a number in setPumpSpeed.\r\n");
        return; // 参数错误，不发送指令
    }

    // 将构建好的指令通过LoRa发送出去
    send_lora_command(cmd, sizeof(cmd));
}

/* Private Constants ---------------------------------------------------------*/

/**
 * @brief 命令分发表
 * @details
 *        一个静态常量数组，是命令分发机制的核心。
 *        要扩展新的云端命令，只需：
 *        1. 实现一个新的 handle_xxx 函数。
 *        2. 在此表中新增一行，将命令字符串和新的处理函数关联起来。
 */
static const command_entry_t command_table[] = {
    {"setFanStatus", handle_setFanStatus},
    {"setGrowLightStatus", handle_setGrowLightStatus},
    {"setPumpStatus", handle_setPumpStatus},
    {"setFanSpeed", handle_setFanSpeed},
    {"setPumpSpeed", handle_setPumpSpeed}
};

// 自动计算命令表的大小
#define COMMAND_TABLE_SIZE (sizeof(command_table) / sizeof(command_table[0]))

/* Private Functions (Helpers) ---------------------------------------------*/

/**
 * @brief 为cJSON库提供基于FreeRTOS的内存分配函数
 * @param size 要分配的内存大小
 * @return void* 指向分配内存的指针
 */
static void *cjson_malloc_rtos(size_t size)
{
    return pvPortMalloc(size);
}

/**
 * @brief 为cJSON库提供基于FreeRTOS的内存释放函数
 * @param ptr 指向要释放内存的指针
 */
static void cjson_free_rtos(void *ptr)
{
    vPortFree(ptr);
}

/* Public Functions ----------------------------------------------------------*/

void HuaweiIoT_Init(void)
{
    cJSON_Hooks hooks = {
        .malloc_fn = cjson_malloc_rtos,
        .free_fn = cjson_free_rtos};
    cJSON_InitHooks(&hooks);
}

// --- Main Parser and Dispatcher ---

void HuaweiIoT_ParseHMREC(AT_Handler_t *at_handler, const char *hprec_str)
{
    if (!at_handler)
    {
        printf("[ERROR] Invalid AT handler.\r\n");
        return;
    }

    printf("\r\n[URC] Parsing HMREC with dispatcher: %s\r\n", hprec_str);

    // --- 修正：将所有局部变量声明移到函数顶部 ---
    cJSON *root = NULL;
    char request_id[48] = {0};
    const cJSON *command_name_item = NULL;
    const cJSON *paras_item = NULL;
    int handler_found = 0;
    char escaped_payload[] = "{\\\"result_code\\\":0}";
    uint16_t logical_len = 17;

    // 1. 提取 Request ID
    const char *request_id_key = "request_id=";
    char *p_request_id_start = strstr(hprec_str, request_id_key);
    if (!p_request_id_start)
    { /* ... error handling ... */
        return;
    }
    p_request_id_start += strlen(request_id_key);
    char *p_request_id_end = strchr(p_request_id_start, '"');
    if (!p_request_id_end)
    { /* ... error handling ... */
        return;
    }

    int req_id_len = p_request_id_end - p_request_id_start;
    if (req_id_len <= 0 || req_id_len >= sizeof(request_id))
    { /* ... error handling ... */
        return;
    }
    strncpy(request_id, p_request_id_start, req_id_len);
    request_id[req_id_len] = '\0';
    printf("[URC] Request ID: %s\r\n", request_id);

    // 2. 定位并解析 JSON 负载
    const char *p_json_start = strchr(hprec_str, '{');
    if (!p_json_start)
    { /* ... error handling ... */
        return;
    }

    root = cJSON_Parse(p_json_start);
    if (root == NULL)
    {
        printf("[cJSON] Failed to parse JSON.\r\n");
        return;
    }

    // 3. 提取命令名称和参数
    command_name_item = cJSON_GetObjectItem(root, "command_name");
    paras_item = cJSON_GetObjectItem(root, "paras");

    if (!cJSON_IsString(command_name_item) || (command_name_item->valuestring == NULL))
    {
        printf("[cJSON] command_name not found or not a string.\r\n");
        goto end;
    }
    printf("[DISPATCHER] Received command: %s\r\n", command_name_item->valuestring);

    // 4. 在命令表中查找并执行处理函数
    for (int i = 0; i < COMMAND_TABLE_SIZE; i++)
    {
        if (strcmp(command_name_item->valuestring, command_table[i].command_name) == 0)
        {
            printf("[DISPATCHER] Found handler for '%s'. Executing...\r\n", command_table[i].command_name);
            command_table[i].handler(paras_item);
            handler_found = 1;
            // 成功处理命令后，根据新的属性值更新硬件状态
            update_hardware_from_properties();

            // [MODIFIED] 只有在成功找到并执行处理函数后，才发送响应
            printf("[ACTION] Preparing command response...\r\n");
            HuaweiIoT_PublishCommandResponse(at_handler, request_id, escaped_payload, logical_len);
            
            break;
        }
    }
    if (!handler_found)
    {
        printf("[DISPATCHER] Warning: No handler found for command '%s'.\r\n", command_name_item->valuestring);
    }

    // 5. 响应命令 (逻辑已被移至上面的循环内部，在找到并执行handler后进行)

end:
    cJSON_Delete(root);
}

/**
 * @brief 构建并发送对云端命令的响应 (非阻塞)
 * @details
 *        此函数构建一个标准的AT+HMPUB命令来响应云平台。
 *        它使用非阻塞的 AT_SendRaw 函数来发送命令，避免阻塞URC处理流程。
 * @param at_handler AT处理器实例指针
 * @param request_id 从云端命令中提取的请求ID
 * @param escaped_payload 已经转义好的JSON负载 (例如 "{\\\"result_code\\\":0}")
 * @param logical_len JSON负载的逻辑长度 (未转义前的长度)
 * @return AT_Status_t 命令发送状态
 */
AT_Status_t HuaweiIoT_PublishCommandResponse(AT_Handler_t *at_handler, const char *request_id, const char *escaped_payload, uint16_t logical_len)
{
    // 使用栈上静态缓冲区，避免动态内存分配的开销和风险
    char topic[256];
    char full_command[512];

    // 构建Topic
    snprintf(topic, sizeof(topic), "$oc/devices/%s/sys/commands/response/request_id=%s", IOT_DEVICE_ID, request_id);

    // 构建完整的AT命令
    snprintf(full_command, sizeof(full_command), "AT+HMPUB=1,\"%s\",%d,\"%s\"\r\n", topic, logical_len, escaped_payload);

    printf("[CMD-RESP] Sending (non-blocking): %s", full_command); // full_command 已包含 \r\n，故此处不用

    // --- 核心修改：使用非阻塞的 AT_SendRaw 发送命令 ---
    // AT_Status_t status = AT_SendBasicCommand(at_handler, full_command, 10000); // 旧的阻塞方式
    AT_Status_t status = AT_SendRaw(at_handler, full_command); // 新的非阻塞方式

    if (status != AT_OK)
    {
        printf("[CMD-RESP] Failed to send command response.\r\n");
    }
    // 注意：因为是非阻塞发送，这里无法知道云平台是否真正成功收到了响应。
    // 但它解决了命令处理延迟的问题。

    return status;
}

/******************************************************************************
 * Stubs for other functions to make the file compilable.
 * 在实际应用中，您需要填充这些函数的具体实现。
 ******************************************************************************/

/**
 * @brief 初始化模块
 * @details
 *        此函数初始化模块，包括检查SIM卡、信号强度、网络注册等AT命令。
 * @param at_handler AT处理器实例指针
 * @return AT_Status_t 初始化结果
 */
AT_Status_t HuaweiIoT_InitModule(AT_Handler_t *at_handler)
{
    // 此处应包含检查SIM卡、信号强度、网络注册等AT命令
    printf("[INFO] HuaweiIoT_InitModule stub called.\r\n");
    (void)at_handler;
    return AT_OK;
}

/**
 * @brief 连接到华为云平台
 * @details
 *        此函数实现了一套健壮的、包含完整错误处理的连接流程。
 *        1. **环境清理**: 首先检查GPRS(IP)的连接状态。如果已连接或状态未知，则会先尝试断开MQTT，
 *           然后强制去激活GPRS，确保后续的连接在一个干净的环境中进行。
 *        2. **IP激活 (轮询机制)**: 发送 `AT+MIPCALL=1` 指令后，并不会依赖其返回值来判断结果。
 *           而是采用轮询机制，在15秒内每秒发送一次 `AT+MIPCALL?` 来主动查询。
 *           直到查询结果中包含一个合法的IP地址，才认为GPRS激活成功。这种方式可以完美避开时序问题。
 *        3. **MQTT连接**: GPRS激活成功后，最后才使用 `AT+HMCON` 指令发起MQTT连接。
 * @param at_handler AT处理器实例指针
 * @return AT_Status_t 最终连接成功返回 AT_OK, 否则返回 AT_ERROR 或其他错误码。
 */
AT_Status_t HuaweiIoT_ConnectCloud(AT_Handler_t *at_handler)
{
    char cmd_buf[512];
    char res_buf[128];

    // 1. 确保网络环境干净
    printf("\r\n--- Checking IP Status ---\r\n");
    if (AT_SendCommand(at_handler, "AT+MIPCALL?", 5000, res_buf, sizeof(res_buf)) == AT_OK && strstr(res_buf, "+MIPCALL: 0") == NULL)
    {
        printf("  > IP context active. Deactivating for a clean start...\r\n");
        AT_SendBasicCommand(at_handler, "AT+HMDIS", 5000);     // 先断开MQTT
        AT_SendBasicCommand(at_handler, "AT+MIPCALL=0", 8000); // 再关闭PDP
    }

    // 2. 激活PDP，获取IP
    printf("\r\n--- Activating IP Connection ---\r\n");
    if (AT_SendBasicCommand(at_handler, "AT+MIPCALL=1", 8000) != AT_OK)
    {
        printf("  > Module failed to accept IP activation command.\r\n");
        return AT_ERROR;
    }
    // 轮询确认IP地址
    bool ip_obtained = false;
    for (int i = 0; i < 15; i++)
    {
        osDelay(1000);
        if (AT_SendCommand(at_handler, "AT+MIPCALL?", 2000, res_buf, sizeof(res_buf)) == AT_OK && strstr(res_buf, ".") != NULL && strstr(res_buf, "0.0.0.0") == NULL)
        {
            printf("  > IP Address Obtained: %s\r\n", res_buf);
            ip_obtained = true;
            break;
        }
    }
    if (!ip_obtained)
    {
        printf("  > Failed to obtain IP Address. Halting.\r\n");
        return AT_ERROR;
    }

    // 3. 连接到华为云 (使用单一的 HMCON 命令)
    printf("\r\n--- Connecting to Huawei Cloud ---\r\n");
    snprintf(cmd_buf, sizeof(cmd_buf), "AT+HMCON=0,60,\"%s\",\"%s\",\"%s\",\"%s\",0",
             IOT_SERVER_ADDRESS,
             IOT_SERVER_PORT,
             IOT_DEVICE_ID,
             IOT_DEVICE_PASSWORD);

    if (AT_SendBasicCommand(at_handler, cmd_buf, 30000) != AT_OK)
    {
        printf("  > Failed to connect to Huawei Cloud.\r\n");
        return AT_ERROR;
    }

    printf("\r\n--- Successfully connected to Huawei Cloud! ---\r\n");
    return AT_OK;
}

AT_Status_t HuaweiIoT_DisconnectFromCloud(AT_Handler_t *at_handler)
{
    printf("[INFO] Disconnecting from cloud...\r\n");
    return AT_SendBasicCommand(at_handler, "AT+HMDIS", 5000);
}

// [BUGFIX] 使用健壮的cJSON生成逻辑，并修复错误处理
AT_Status_t HuaweiIoT_PublishAllSubDevicesOnline(AT_Handler_t *at_handler)
{
    if (at_handler == NULL)
        return AT_ERROR;

    cJSON *root = cJSON_CreateObject();
    if (!root)
        return AT_ERROR;

    cJSON *services = cJSON_AddArrayToObject(root, "services");
    cJSON *service = cJSON_CreateObject();
    cJSON_AddItemToArray(services, service);
    cJSON_AddStringToObject(service, "service_id", "$sub_device_manager");
    cJSON_AddStringToObject(service, "event_type", "sub_device_update_status");
    cJSON *paras = cJSON_AddObjectToObject(service, "paras");
    cJSON *device_statuses = cJSON_AddArrayToObject(paras, "device_statuses");

    // 从配置中心读取子设备列表并构建JSON
    for (int i = 0; i < DEVICE_CONFIG_COUNT; i++)
    {
        cJSON *dev = cJSON_CreateObject();
        if (dev)
        {
            cJSON_AddStringToObject(dev, "device_id", DEVICE_CONFIG_TABLE[i].cloud_id);
            cJSON_AddStringToObject(dev, "status", "ONLINE");
            cJSON_AddItemToArray(device_statuses, dev);
        }
    }

    char *logical_payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!logical_payload)
        return AT_ERROR;

    // --- 步骤: 手动转义JSON payload ---
    size_t logical_len = strlen(logical_payload);
    char *escaped_payload = pvPortMalloc(logical_len * 2 + 1); // 为转义分配足够空间
    if (escaped_payload == NULL)
    {
        vPortFree(logical_payload);
        return AT_ERROR;
    }
    char *p_in = logical_payload;
    char *p_out = escaped_payload;
    while (*p_in)
    {
        if (*p_in == '"' || *p_in == '\\')
        {
            *p_out++ = '\\';
        }
        *p_out++ = *p_in++;
    }
    *p_out = '\0';

    // --- 步骤: 构建并发送 AT+HMPUB 命令 ---
    char at_cmd[1024];
    snprintf(at_cmd, sizeof(at_cmd), "AT+HMPUB=1,\"$oc/devices/%s/sys/events/up\",%d,\"%s\"",
             IOT_DEVICE_ID, logical_len, escaped_payload);

    AT_Status_t status = AT_SendCommand(at_handler, at_cmd, 15000, NULL, 0);

    vPortFree(logical_payload);
    vPortFree(escaped_payload);
    return status;
}

AT_Status_t HuaweiIoT_PublishSubDeviceStatus(AT_Handler_t *at_handler, const char *sub_device_id, const char *status)
{
    printf("[INFO] HuaweiIoT_PublishSubDeviceStatus stub called.\r\n");
    (void)at_handler;
    (void)sub_device_id;
    (void)status;
    return AT_OK;
}

AT_Status_t HuaweiIoT_Subscribe(AT_Handler_t *at_handler, const char *topic)
{
    printf("[INFO] HuaweiIoT_Subscribe stub called.\r\n");
    (void)at_handler;
    (void)topic;
    return AT_OK;
}

/**
 * @brief (内部私有) 根据设备信息构建对应的 a service cJSON 对象
 */
static int build_service_json(const managed_device_t *device, cJSON *services_array)
{
    cJSON *service = cJSON_CreateObject();
    if (service == NULL)
        return NULL;

    cJSON *properties = cJSON_CreateObject();
    if (properties == NULL)
    {
        cJSON_Delete(service);
        return NULL;
    }
    cJSON_AddItemToObject(service, "properties", properties);

    switch (device->device_type)
    {
    case DEVICE_TYPE_INTERNAL_SENSOR:
    {
        cJSON_AddStringToObject(service, "service_id", "sensor"); // 对应您物模型中的服务ID
        const InternalSensorProperties_t *data = &device->properties.internal_sensor;
        // Greenhouse Env
        cJSON_AddNumberToObject(properties, "greenhouseTemperature", data->greenhouseTemperature);
        cJSON_AddNumberToObject(properties, "greenhouseHumidity", data->greenhouseHumidity);
        // Soil
        cJSON_AddNumberToObject(properties, "soilMoisture", round(data->soilMoisture * 10.0) / 10.0);
        cJSON_AddNumberToObject(properties, "soilTemperature", round(data->soilTemperature * 10.0) / 10.0);
        cJSON_AddNumberToObject(properties, "soilPh", round(data->soilPh * 10.0) / 10.0);
        cJSON_AddNumberToObject(properties, "soilEc", data->soilEc);
        cJSON_AddNumberToObject(properties, "soilNitrogen", data->soilNitrogen);
        cJSON_AddNumberToObject(properties, "soilPhosphorus", data->soilPhosphorus);
        cJSON_AddNumberToObject(properties, "soilPotassium", data->soilPotassium);
        cJSON_AddNumberToObject(properties, "soilSalinity", data->soilSalinity);
        cJSON_AddNumberToObject(properties, "soilTds", data->soilTds);
        cJSON_AddNumberToObject(properties, "soilFertility", data->soilFertility);
        // Light
        cJSON_AddNumberToObject(properties, "lightIntensity", data->lightIntensity);
        // Air
        cJSON_AddNumberToObject(properties, "vocConcentration", data->vocConcentration);
        cJSON_AddNumberToObject(properties, "co2Concentration", data->co2Concentration);
        break;
    }
    case DEVICE_TYPE_EXTERNAL_SENSOR:
    {
        cJSON_AddStringToObject(service, "service_id", "sensor"); // 对应您物模型中的服务ID
        const ExternalSensorProperties_t *data = &device->properties.external_sensor;
        cJSON_AddNumberToObject(properties, "outdoorTemperature", data->outdoorTemperature);
        cJSON_AddNumberToObject(properties, "outdoorHumidity", data->outdoorHumidity);
        cJSON_AddNumberToObject(properties, "outdoorLightIntensity", data->outdoorLightIntensity);
        cJSON_AddNumberToObject(properties, "airPressure", data->airPressure);
        cJSON_AddNumberToObject(properties, "altitude", data->altitude);
        cJSON_AddStringToObject(properties, "location", data->location);
        break;
    }
    case DEVICE_TYPE_CONTROL_NODE:
    {
        cJSON_AddStringToObject(service, "service_id", "control"); // 对应您物模型中的服务ID
        const ControlNodeProperties_t *data = &device->properties.control;
        cJSON_AddBoolToObject(properties, "fanStatus", data->fanStatus);
        cJSON_AddBoolToObject(properties, "growLightStatus", data->growLightStatus);
        cJSON_AddBoolToObject(properties, "pumpStatus", data->pumpStatus);
        cJSON_AddNumberToObject(properties, "fanSpeed", data->fanSpeed);
        cJSON_AddNumberToObject(properties, "pumpSpeed", data->pumpSpeed);
        break;
    }
    default:
    {
        // 未知类型，释放内存并返回NULL
        cJSON_Delete(service);
        return NULL;
    }
    }

    cJSON_AddItemToArray(services_array, service);

    // 只有传感器节点有电池信息，为它们添加 "device" service
    // device->device_type == DEVICE_TYPE_INTERNAL_SENSOR || 
    if (device->device_type == DEVICE_TYPE_EXTERNAL_SENSOR)
    {
        service = cJSON_CreateObject();
        if (service == NULL)
            return NULL;

        properties = cJSON_CreateObject();
        if (properties == NULL)
        {
            cJSON_Delete(service);
            return NULL;
        }
        cJSON_AddItemToObject(service, "properties", properties);

        switch (device->device_type)
        {
        case DEVICE_TYPE_INTERNAL_SENSOR:
        {
            cJSON_AddStringToObject(service, "service_id", "device"); // 对应您物模型中的服务ID
            const InternalSensorProperties_t *data = &device->properties.internal_sensor;
            cJSON_AddNumberToObject(properties, "batteryLevel", data->common.batteryLevel);
            cJSON_AddNumberToObject(properties, "batteryVoltage", round(data->common.batteryVoltage * 10.0) / 10.0);
            break;
        }
        case DEVICE_TYPE_EXTERNAL_SENSOR:
        {
            cJSON_AddStringToObject(service, "service_id", "device"); // 对应您物模型中的服务ID
            const ExternalSensorProperties_t *data = &device->properties.external_sensor;
            cJSON_AddNumberToObject(properties, "batteryLevel", data->common.batteryLevel);
            cJSON_AddNumberToObject(properties, "batteryVoltage", round(data->common.batteryVoltage * 10.0) / 10.0);
            break;
        }
        default:
        {
            // 未知类型，释放内存并返回NULL
            cJSON_Delete(service);
            return NULL;
        }
        }
        cJSON_AddItemToArray(services_array, service);
    }
    return 1;
}

AT_Status_t HuaweiIoT_PublishGatewayReport(AT_Handler_t *handler)
{
    if (handler == NULL)
        return AT_ERROR;

    int search_index = 0;
    managed_device_t current_device;
    uint16_t dirty_device_ids[MAX_MANAGED_DEVICES];
    uint8_t dirty_count = 0;
    AT_Status_t final_status = AT_OK; // 用于跟踪整个上报周期的最终状态

    // 步骤 1: 查找所有脏设备
    while ((search_index = DeviceManager_FindNextDirtyDevice(search_index, &current_device)) != -1)
    {
        dirty_device_ids[dirty_count++] = current_device.lora_id;
        search_index++;
    }

    if (dirty_count == 0)
    {
        return AT_OK;
    }
    
    printf("[Upload] Found %d dirty devices to report.\r\n", dirty_count);

    // 步骤 2: 遍历脏设备，根据类型选择上报策略
    for (uint8_t i = 0; i < dirty_count; i++)
    {
        managed_device_t device_data;
        if (!DeviceManager_GetDevice(dirty_device_ids[i], &device_data))
        {
            continue;
        }

        // --- 策略分歧点 ---
        if (device_data.device_type == DEVICE_TYPE_INTERNAL_SENSOR)
        {
            // --- 策略A: 针对内部传感器，分两次上报 ---
            printf("[Upload] Internal Sensor requires 2-part report for %s\r\n", device_data.cloud_device_id);
            AT_Status_t status1 = AT_ERROR, status2 = AT_ERROR;
            
            // --- Packet 1: 主要环境数据 ---
            {
                cJSON *root = cJSON_CreateObject();
                cJSON *devices = cJSON_AddArrayToObject(root, "devices");
                cJSON *device = cJSON_CreateObject();
                cJSON_AddItemToArray(devices, device);
                cJSON_AddStringToObject(device, "device_id", device_data.cloud_device_id);
                cJSON *services = cJSON_AddArrayToObject(device, "services");
                cJSON *service = cJSON_CreateObject();
                cJSON_AddItemToArray(services, service);
                cJSON_AddStringToObject(service, "service_id", "sensor");
                cJSON *properties = cJSON_AddObjectToObject(service, "properties");
                
                const InternalSensorProperties_t *data = &device_data.properties.internal_sensor;
                cJSON_AddNumberToObject(properties, "greenhouseTemperature", data->greenhouseTemperature);
                cJSON_AddNumberToObject(properties, "greenhouseHumidity", data->greenhouseHumidity);
                cJSON_AddNumberToObject(properties, "soilMoisture", data->soilMoisture);
                cJSON_AddNumberToObject(properties, "lightIntensity", data->lightIntensity);
                cJSON_AddNumberToObject(properties, "soilTemperature", round(data->soilTemperature * 10.0) / 10.0);
                cJSON_AddNumberToObject(properties, "vocConcentration", data->vocConcentration);
                cJSON_AddNumberToObject(properties, "co2Concentration", data->co2Concentration);

                char *payload = cJSON_PrintUnformatted(root);
                cJSON_Delete(root);
                
                if (payload) {
                    size_t logical_len = strlen(payload);
                    char *escaped_payload = pvPortMalloc(logical_len * 2 + 1);
                    if (escaped_payload) {
                        char *p_in = payload;
                        char *p_out = escaped_payload;
                        while (*p_in) {
                            if (*p_in == '"' || *p_in == '\\') *p_out++ = '\\';
                            *p_out++ = *p_in++;
                        }
                        *p_out = '\0';
                        
                        char at_cmd[2048];
                        snprintf(at_cmd, sizeof(at_cmd), "AT+HMPUB=1,\"$oc/devices/%s/sys/gateway/sub_devices/properties/report\",%d,\"%s\"", IOT_DEVICE_ID, logical_len, escaped_payload);
                        status1 = AT_SendCommand(handler, at_cmd, 15000, NULL, 0);
                        vPortFree(escaped_payload);
                    }
                    vPortFree(payload);
                }
            }

            osDelay(500);

            // --- Packet 2: 土壤详细数据和电池状态 ---
            {
                cJSON *root = cJSON_CreateObject();
                cJSON *devices = cJSON_AddArrayToObject(root, "devices");
                cJSON *device = cJSON_CreateObject();
                cJSON_AddItemToArray(devices, device);
                cJSON_AddStringToObject(device, "device_id", device_data.cloud_device_id);
                cJSON *services = cJSON_AddArrayToObject(device, "services");

                // Service 1: sensor (soil properties)
                cJSON *svc_sensor = cJSON_CreateObject();
                cJSON_AddItemToArray(services, svc_sensor);
                cJSON_AddStringToObject(svc_sensor, "service_id", "sensor");
                cJSON *props_sensor = cJSON_AddObjectToObject(svc_sensor, "properties");
                const InternalSensorProperties_t *data = &device_data.properties.internal_sensor;
                cJSON_AddNumberToObject(props_sensor, "soilPh", round(data->soilPh * 10.0) / 10.0);
                cJSON_AddNumberToObject(props_sensor, "soilEc", data->soilEc);
                cJSON_AddNumberToObject(props_sensor, "soilNitrogen", data->soilNitrogen);
                cJSON_AddNumberToObject(props_sensor, "soilPhosphorus", data->soilPhosphorus);
                cJSON_AddNumberToObject(props_sensor, "soilPotassium", data->soilPotassium);
                cJSON_AddNumberToObject(props_sensor, "soilSalinity", data->soilSalinity);
                cJSON_AddNumberToObject(props_sensor, "soilTds", data->soilTds);
                cJSON_AddNumberToObject(props_sensor, "soilFertility", data->soilFertility);

                // Service 2: device (battery properties)
                cJSON *svc_device = cJSON_CreateObject();
                cJSON_AddItemToArray(services, svc_device);
                cJSON_AddStringToObject(svc_device, "service_id", "device");
                cJSON *props_device = cJSON_AddObjectToObject(svc_device, "properties");
                cJSON_AddNumberToObject(props_device, "batteryLevel", data->common.batteryLevel);
                cJSON_AddNumberToObject(props_device, "batteryVoltage", round(data->common.batteryVoltage * 10.0) / 10.0);

                char *payload = cJSON_PrintUnformatted(root);
                cJSON_Delete(root);

                if (payload) {
                    size_t logical_len = strlen(payload);
                    char *escaped_payload = pvPortMalloc(logical_len * 2 + 1);
                    if (escaped_payload) {
                        char *p_in = payload;
                        char *p_out = escaped_payload;
                        while (*p_in) {
                            if (*p_in == '"' || *p_in == '\\') *p_out++ = '\\';
                            *p_out++ = *p_in++;
                        }
                        *p_out = '\0';

                        char at_cmd[2048];
                        snprintf(at_cmd, sizeof(at_cmd), "AT+HMPUB=1,\"$oc/devices/%s/sys/gateway/sub_devices/properties/report\",%d,\"%s\"", IOT_DEVICE_ID, logical_len, escaped_payload);
                        status2 = AT_SendCommand(handler, at_cmd, 15000, NULL, 0);
                        vPortFree(escaped_payload);
                    }
                    vPortFree(payload);
                }
            }

            // --- 统一处理结果 ---
            if (status1 == AT_OK && status2 == AT_OK) {
                printf("[Upload] SUCCESS for 2-part report: %s.\r\n", device_data.cloud_device_id);
                DeviceManager_ClearDirtyFlag(device_data.lora_id);
            } else {
                printf("[Upload] FAILED for 2-part report: %s (Status1: %d, Status2: %d). Will retry.\r\n", device_data.cloud_device_id, status1, status2);
                final_status = AT_ERROR;
            }
        }
        else 
        {
            // --- 策略B: 其他设备，单次全量上报 ---
            printf("[Upload] Standard report for device: %s\r\n", device_data.cloud_device_id);
            cJSON *root = cJSON_CreateObject();
            cJSON *devices = cJSON_AddArrayToObject(root, "devices");
            cJSON *device = cJSON_CreateObject();
            cJSON_AddItemToArray(devices, device);
            cJSON_AddStringToObject(device, "device_id", device_data.cloud_device_id);
            cJSON *services = cJSON_AddArrayToObject(device, "services");
            
            build_service_json(&device_data, services);

            char *payload = cJSON_PrintUnformatted(root);
            cJSON_Delete(root);

            if (payload) {
                size_t logical_len = strlen(payload);
                char *escaped_payload = pvPortMalloc(logical_len * 2 + 1);
                if (escaped_payload) {
                    char *p_in = payload;
                    char *p_out = escaped_payload;
                    while (*p_in) {
                        if (*p_in == '"' || *p_in == '\\') *p_out++ = '\\';
                        *p_out++ = *p_in++;
                    }
                    *p_out = '\0';

                    char at_cmd[2048];
                    snprintf(at_cmd, sizeof(at_cmd), "AT+HMPUB=1,\"$oc/devices/%s/sys/gateway/sub_devices/properties/report\",%d,\"%s\"", IOT_DEVICE_ID, logical_len, escaped_payload);
                    AT_Status_t status = AT_SendCommand(handler, at_cmd, 15000, NULL, 0);
                    
                    if (status == AT_OK) {
                        printf("[Upload] SUCCESS for device %s.\r\n", device_data.cloud_device_id);
                        DeviceManager_ClearDirtyFlag(device_data.lora_id);
                    } else {
                        printf("[Upload] FAILED for device %s. Will retry.\r\n", device_data.cloud_device_id);
                        final_status = AT_ERROR;
                    }
                    vPortFree(escaped_payload);
                }
                vPortFree(payload);
            }
        }
        
        // 在两次上报之间短暂延时，避免冲击模组
        osDelay(500);
    }
    
    return final_status;
}
