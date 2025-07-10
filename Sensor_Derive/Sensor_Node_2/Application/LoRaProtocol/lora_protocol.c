/**
 * @file lora_protocol.c
 * @brief LoRa通信协议核心工具函数的实现
 *
 * @note
 * 本文件包含以下功能的具体实现：
 * - 使用STM32硬件外设进行CRC16-Modbus计算。
 * - 针对标准整数和浮点数类型的字节流打包/解包（小端序）。
 * - LoRa标准数据帧的构建（封包）与解析（解包）。
 * - 面向应用层的传感器数据与紧凑型LoRa载荷之间的转换。
 *
 * 本模块设计为平台无关，仅依赖于C标准库和STM32的HAL库（用于硬件CRC）。
 */

#include "lora_protocol.h" // 包含本模块的头文件
#include <string.h>        // 标准库，用于 memcpy
#include <math.h>          // 标准库，用于 fabsf/fabs (浮点数绝对值)
#include "crc.h"           // 引入 STM32 硬件 CRC 句柄，由CubeMX生成

// ============================================================================
// CRC16-Modbus 实现 (利用STM32硬件加速)
// ============================================================================
// 原有的软件CRC16查找表已被移除，以节省Flash空间。

/**
 * @brief 使用STM32硬件CRC外设计算给定数据的 CRC16-Modbus 校验码。
 * @param data 指向待计算数据的指针。
 * @param length 数据的字节长度。
 * @return uint16_t 计算得到的16位CRC值。
 * @note 本函数依赖于 `MX_CRC_Init()` 已在别处被正确调用，
 *       且 `hcrc` 句柄已根据Modbus CRC16的要求配置完毕
 *       （多项式0x8005, 初始值0xFFFF, 输入/输出数据反转）。
 */
uint16_t crc16_modbus(const uint8_t *data, size_t length)
{
    if (data == NULL || length == 0)
    {
        return 0; // 维持对无效输入的行为
    }

    // 使用 STM32 硬件 CRC 外设进行计算。
    // `HAL_CRC_Calculate` 在很多 STM32 型号上是基于 32-bit 字进行操作的，
    // 为了兼容任意字节长度的数据，最稳健的方式是逐字节写入 CRC 的数据寄存器(DR)。
    // STM32CubeMX 中的配置（输入数据反转模式：字节）确保了硬件会正确处理每个字节。

    // 1. 复位 CRC 计算单元，使其恢复到初始值 (0xFFFF)
    //    这一步对于每次新的、独立的 CRC 计算都至关重要。
    __HAL_CRC_DR_RESET(&hcrc);

    // 2. 逐字节送入数据
    for (size_t i = 0; i < length; i++)
    {
        // 通过 volatile 8位指针写入数据寄存器，确保每次写入都被执行
        // 并且只写入一个字节。
        *(__IO uint8_t *)(&(hcrc.Instance->DR)) = data[i];
    }

    // 3. 返回最终计算结果
    //    硬件已根据配置处理了输出反转，直接读取DR寄存器即可。
    return (uint16_t)READ_REG(hcrc.Instance->DR);
}

// ============================================================================
// 数据打包/解包函数 (小端序 - Little Endian)
// ============================================================================
// 这些函数用于在 C 数据类型和字节数组之间进行转换，以便进行二进制传输。
// 假设通信双方（如 ESP32 和 STM32）都使用小端序。

// --- 打包函数 (Pack: 将 C 类型值写入字节缓冲区) ---

/** @brief 将 uint8_t 值写入缓冲区 (1 字节) */
inline void lora_model_pack_u8(uint8_t *buffer, uint8_t value)
{
    if (buffer)
        buffer[0] = value;
}

/** @brief 将 int8_t 值写入缓冲区 (1 字节) */
inline void lora_model_pack_i8(uint8_t *buffer, int8_t value)
{
    // 直接类型转换即可
    if (buffer)
        buffer[0] = (uint8_t)value;
}

/** @brief 将 uint16_t 值按小端序写入缓冲区 (2 字节) */
inline void lora_model_pack_u16le(uint8_t *buffer, uint16_t value)
{
    if (buffer)
    {
        buffer[0] = (uint8_t)(value & 0xFF);        // 低字节写入第一个位置
        buffer[1] = (uint8_t)((value >> 8) & 0xFF); // 高字节写入第二个位置
    }
}

/** @brief 将 int16_t 值按小端序写入缓冲区 (2 字节) */
inline void lora_model_pack_i16le(uint8_t *buffer, int16_t value)
{
    // 对于有符号数，先转为无符号数再打包
    if (buffer)
        lora_model_pack_u16le(buffer, (uint16_t)value);
}

/** @brief 将 uint32_t 值按小端序写入缓冲区 (4 字节) */
inline void lora_model_pack_u32le(uint8_t *buffer, uint32_t value)
{
    if (buffer)
    {
        buffer[0] = (uint8_t)(value & 0xFF); // 最低字节
        buffer[1] = (uint8_t)((value >> 8) & 0xFF);
        buffer[2] = (uint8_t)((value >> 16) & 0xFF);
        buffer[3] = (uint8_t)((value >> 24) & 0xFF); // 最高字节
    }
}

/** @brief 将 int32_t 值按小端序写入缓冲区 (4 字节) */
inline void lora_model_pack_i32le(uint8_t *buffer, int32_t value)
{
    if (buffer)
        lora_model_pack_u32le(buffer, (uint32_t)value);
}

/**
 * @brief 将 float 值按 IEEE 754 单精度格式（小端序）写入缓冲区。
 * @note 这是一个基于"类型双关"(Type Punning)的技巧。它假设编译器和CPU
 *       对 float 的内存表示遵循IEEE 754标准，且字节序为小端序。
 *       对于ESP32和常见的ARM Cortex-M核，直接内存复制是有效且高效的。
 */
inline void lora_model_pack_float_ieee754le(uint8_t *buffer, float value)
{
    if (buffer)
        memcpy(buffer, &value, sizeof(float));
}

// --- 解包函数 (Unpack: 从字节缓冲区读取 C 类型值) ---

/** @brief 从缓冲区读取 uint8_t 值 */
inline uint8_t lora_model_unpack_u8(const uint8_t *buffer)
{
    // 如果 buffer 为 NULL，返回 0 (或可以考虑返回错误码/设置 errno)
    return buffer ? buffer[0] : 0;
}

/** @brief 从缓冲区读取 int8_t 值 */
inline int8_t lora_model_unpack_i8(const uint8_t *buffer)
{
    return buffer ? (int8_t)buffer[0] : 0;
}

/** @brief 从缓冲区按小端序读取 uint16_t 值 */
inline uint16_t lora_model_unpack_u16le(const uint8_t *buffer)
{
    // 将字节组合回 uint16_t
    return buffer ? (uint16_t)(buffer[0] | (buffer[1] << 8)) : 0;
}

/** @brief 从缓冲区按小端序读取 int16_t 值 */
inline int16_t lora_model_unpack_i16le(const uint8_t *buffer)
{
    // 先按无符号解包，再转为有符号
    return buffer ? (int16_t)lora_model_unpack_u16le(buffer) : 0;
}

/** @brief 从缓冲区按小端序读取 uint32_t 值 */
inline uint32_t lora_model_unpack_u32le(const uint8_t *buffer)
{
    return buffer ? (uint32_t)(buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24)) : 0;
}

/** @brief 从缓冲区按小端序读取 int32_t 值 */
inline int32_t lora_model_unpack_i32le(const uint8_t *buffer)
{
    return buffer ? (int32_t)lora_model_unpack_u32le(buffer) : 0;
}

/**
 * @brief 从缓冲区按 IEEE 754 单精度格式（小端序）读取 float 值。
 * @note 同样是利用"类型双关"技巧，依赖于平台的内存表示。
 */
inline float lora_model_unpack_float_ieee754le(const uint8_t *buffer)
{
    float value = 0.0f;
    if (buffer)
        memcpy(&value, buffer, sizeof(float));
    return value;
}

// ============================================================================
// LoRa 帧构建与解析函数
// ============================================================================

/**
 * @brief 生成一个完整的 LoRa 数据帧（头部 + 载荷 + CRC16）
 * @param target_addr   目标设备地址 (1 byte)
 * @param sender_addr   发送设备地址 (1 byte, 通常是本机地址)
 * @param msg_type      消息类型 (1 byte, 见 lora_protocol_defs.h)
 * @param seq_num       消息序列号 (1 byte)
 * @param payload       指向应用层数据载荷的指针 (可以为 NULL 如果 payload_len 为 0)
 * @param payload_len   应用层数据载荷的长度 (字节)
 * @param output_buffer 指向用于存储最终 LoRa 帧的缓冲区的指针
 * @param output_buffer_max_len output_buffer 的最大可用长度
 * @return int 成功则返回完整的 LoRa 帧的总长度 (字节),
 * 失败则返回负值错误码 (例如 -1: 载荷超长, -2: 输出 buffer 不足, -3: 输出 buffer 为 NULL)
 */
int generate_lora_frame(uint8_t target_addr, uint8_t sender_addr, uint8_t msg_type, uint8_t seq_num,
                        const uint8_t *payload, size_t payload_len,
                        uint8_t *output_buffer, size_t output_buffer_max_len)
{
    // 1. 检查应用层载荷是否超过协议定义的最大值
    if (payload_len > LORA_MAX_PAYLOAD_APP)
    {
        // 载荷对于协议定义来说太大了
        return LORA_FRAME_ERR_INVALID_LEN;
    }

    // 计算包含头部、载荷和CRC的总帧长
    const size_t total_len = LORA_HEADER_SIZE + payload_len + LORA_CHECKSUM_SIZE;

    // 2. 检查输出缓冲区是否足够大
    if (total_len > output_buffer_max_len)
    {
        // 输出缓冲区太小，无法容纳整个帧
        return LORA_FRAME_ERR_BUFFER_TOO_SMALL;
    }
    // 3. 检查输出缓冲区指针是否有效
    if (output_buffer == NULL)
    {
        return LORA_FRAME_ERR_INVALID_PARAM;
    }

    // --- 按顺序填充数据帧 ---
    // a. 填充协议头 (4 字节)
    lora_model_pack_u8(&output_buffer[0], target_addr);
    lora_model_pack_u8(&output_buffer[1], sender_addr);
    lora_model_pack_u8(&output_buffer[2], msg_type);
    lora_model_pack_u8(&output_buffer[3], seq_num);

    // b. 填充应用层数据载荷
    if (payload != NULL && payload_len > 0)
    {
        memcpy(&output_buffer[LORA_HEADER_SIZE], payload, payload_len);
    }

    // c. 计算 CRC16 (校验范围: 从 TargetAddr 到 Payload 的最后一个字节)
    uint16_t crc = crc16_modbus(output_buffer, LORA_HEADER_SIZE + payload_len);

    // d. 填充 CRC16 到帧尾 (小端序: 低字节在前)
    lora_model_pack_u16le(&output_buffer[LORA_HEADER_SIZE + payload_len], crc);

    // 成功，返回完整的帧长度
    return (int)total_len;
}

/**
 * @brief 解析接收到的 LoRa 原始数据帧，校验 CRC16 并提取信息
 * @param raw_packet 指向接收到的原始数据缓冲区的指针
 * @param raw_len 接收到的原始数据的总长度 (字节)
 * @param parsed_msg 指向用于存储解析后消息信息的结构体的指针。
 * 函数成功时，会填充此结构体 (payload, payload_len, header fields)。
 * 注意：RSSI 和 SNR 需要由调用者根据底层 LoRa 驱动信息另行填充。
 * @return lora_frame_status_t 返回解析状态码 (LORA_FRAME_OK 表示成功)。
 */
lora_frame_status_t parse_lora_frame(const uint8_t *raw_packet, size_t raw_len,
                                     lora_parsed_message_t *parsed_msg)
{
    // 检查输入参数有效性
    if (raw_packet == NULL || parsed_msg == NULL)
    {
        return LORA_FRAME_ERR_INVALID_PARAM;
    }

    size_t header_len = LORA_HEADER_SIZE;
    size_t crc_len = LORA_CHECKSUM_SIZE;
    size_t min_frame_len = header_len + crc_len; // 最小帧长度 (没有载荷时)

    // 1. 检查接收到的长度是否至少包含头部和 CRC
    if (raw_len < min_frame_len)
    {
        // 数据包太短，无法包含有效信息和校验码
        return LORA_FRAME_ERR_INVALID_LEN;
    }

    // 2. 提取帧尾部的 CRC16 值 (小端序)
    uint16_t received_crc = lora_model_unpack_u16le(&raw_packet[raw_len - crc_len]);

    // 3. 计算接收到的数据部分 (除 CRC 外的所有字节) 的 CRC16
    size_t data_len_for_crc = raw_len - crc_len;
    uint16_t calculated_crc = crc16_modbus(raw_packet, data_len_for_crc);

    // 4. 比较计算出的 CRC 与接收到的 CRC
    if (received_crc != calculated_crc)
    {
        // CRC 校验失败，数据可能已损坏
        return LORA_FRAME_ERR_INVALID_CRC;
    }

    // 5. CRC 校验通过，解析数据帧内容
    parsed_msg->target_addr = lora_model_unpack_u8(&raw_packet[0]);
    parsed_msg->sender_addr = lora_model_unpack_u8(&raw_packet[1]);
    parsed_msg->msg_type = lora_model_unpack_u8(&raw_packet[2]);
    parsed_msg->seq_num = lora_model_unpack_u8(&raw_packet[3]);
    parsed_msg->payload_len = data_len_for_crc - header_len; // 计算实际的应用层载荷长度

    // 检查解析出的载荷长度是否超过目标结构体缓冲区的大小
    if (parsed_msg->payload_len > sizeof(parsed_msg->payload))
    {
        // 这是一个内部错误，理论上不应发生，因为 LORA_MAX_PAYLOAD_APP 限制了
        parsed_msg->payload_len = 0; // 清空长度表示错误
        return LORA_FRAME_ERR_BUFFER_TOO_SMALL;
    }

    // 6. 复制应用层载荷到目标结构体
    if (parsed_msg->payload_len > 0)
    {
        memcpy(parsed_msg->payload, &raw_packet[header_len], parsed_msg->payload_len);
    }

    // 注意：RSSI 和 SNR 不在此函数中填充，应由调用者（例如 my_lora 组件）
    // 从底层 LoRa 驱动获取并填充到 parsed_msg 结构体中。
    // 可以选择在这里初始化为无效值。
    parsed_msg->rssi = -999; // 无效值示例
    parsed_msg->snr = 0.0f;

    // 所有步骤成功
    return LORA_FRAME_OK;
}

/**
 * @brief 将高层传感器数据结构打包成紧凑的 LoRa 载荷格式
 *
 * @param sensor_data 指向包含源传感器数据的 ExternalSensorProperties_t 结构体 (输入)
 * @param payload 指向用于存储打包后结果的 sensor_data_payload_t 结构体 (输出)
 * @return bool 如果成功打包则返回 true，若输入参数无效则返回 false
 */
bool lora_model_create_sensor_payload(const ExternalSensorProperties_t *sensor_data,
                                      sensor_data_payload_t *payload)
{
    if (sensor_data == NULL || payload == NULL)
    {
        return false;
    }

    // 清零 payload 结构体，确保所有字段都有一个已知的初始值
    memset(payload, 0, sizeof(sensor_data_payload_t));

    // --- 打包温湿度和光照强度 ---
    double temp = sensor_data->outdoorTemperature;
    int8_t temp_int = (int8_t)temp;
    payload->temperature_int = temp_int;
    payload->temperature_dec = (uint8_t)(fabs(temp - (double)temp_int) * 100.0);

    double humid = sensor_data->outdoorHumidity;
    uint8_t humid_int = (uint8_t)humid;
    payload->humidity_int = humid_int;
    payload->humidity_dec = (uint8_t)(fabs(humid - (double)humid_int) * 100.0);
    
    payload->light_intensity = sensor_data->outdoorLightIntensity;

    // --- 打包气压和海拔 ---
    payload->air_pressure = (uint32_t)sensor_data->airPressure;
    // 直接类型转换可以正确处理正负海拔值
    payload->altitude = (int16_t)sensor_data->altitude;

    // --- 打包GPS位置 ---
    // 从 location 字符串反向解析出经纬度数值
    double lat_d = 0.0, lon_d = 0.0;
    char lat_c = 'N', lon_c = 'E';
    if (parse_location_string(sensor_data->location, &lat_d, &lat_c, &lon_d, &lon_c))
    {
        // 转换为乘以 1,000,000 的整数
        payload->latitude_e6 = (int32_t)(lat_d * 1000000.0);
        payload->longitude_e6 = (int32_t)(lon_d * 1000000.0);
        // 如果是南纬或西经，则转为负数
        if (lat_c == 'S') payload->latitude_e6 *= -1;
        if (lon_c == 'W') payload->longitude_e6 *= -1;
    }

    // --- 打包电池信息 ---
    // 将浮点电压乘以10后存为整数，例如 4.2V -> 42
    payload->battery_level = sensor_data->common.batteryLevel;
    payload->battery_voltage_x10 = (uint16_t)(sensor_data->common.batteryVoltage * 10.0f);

    return true;
}

/**
 * @brief 从已解析的消息中提取传感器数据并转换为应用层结构体 (ExternalSensorProperties_t)
 *
 * @param parsed_msg 指向已通过 CRC 校验并解析了头部的消息结构体 (输入)
 * @param sensor_data 指向用于存储最终结果的 ExternalSensorProperties_t 结构体 (输出)
 * @return bool 如果成功解析并填充 sensor_data 则返回 true, 否则返回 false
 */
bool lora_model_parse_sensor_data(const lora_parsed_message_t *parsed_msg,
                                  ExternalSensorProperties_t *sensor_data)
{
    // 1. 输入参数检查
    if (parsed_msg == NULL || sensor_data == NULL)
    {
        return false;
    }

    // 2. 检查消息类型
    if (parsed_msg->msg_type != MSG_TYPE_REPORT_SENSOR)
    {
        return false;
    }

    // 3. 检查载荷长度是否与定义的传输结构体完全匹配
    if (parsed_msg->payload_len != sizeof(sensor_data_payload_t))
    {
        return false;
    }

    // 将裸 payload 缓冲区指针强制转换为 const sensor_data_payload_t 结构体指针，以便安全访问
    const sensor_data_payload_t *payload = (const sensor_data_payload_t *)parsed_msg->payload;

    // 在填充之前，将目标结构体清零
    memset(sensor_data, 0, sizeof(ExternalSensorProperties_t));

    // --- 解包温湿度和光照 ---
    if (payload->temperature_int < 0) {
        sensor_data->outdoorTemperature = (double)payload->temperature_int - ((double)payload->temperature_dec / 100.0);
    } else {
        sensor_data->outdoorTemperature = (double)payload->temperature_int + ((double)payload->temperature_dec / 100.0);
    }
    sensor_data->outdoorHumidity = (double)payload->humidity_int + ((double)payload->humidity_dec / 100.0);
    sensor_data->outdoorLightIntensity = payload->light_intensity;

    // --- 解包气压和海拔 ---
    sensor_data->airPressure = (double)payload->air_pressure;
    // int16_t 到 double 的转换会自动处理符号，正确还原负海拔
    sensor_data->altitude = (double)payload->altitude;

    // --- 解包GPS位置并格式化为字符串 ---
    double lat_d = (double)payload->latitude_e6 / 1000000.0;
    double lon_d = (double)payload->longitude_e6 / 1000000.0;
    char lat_c = (lat_d < 0) ? 'S' : 'N';
    char lon_c = (lon_d < 0) ? 'W' : 'E';
    
    // 使用 format_location_string 函数填充 location 字符串
    format_location_string(fabs(lat_d), lat_c, fabs(lon_d), lon_c,
                           sensor_data->location, sizeof(sensor_data->location));
    
    // --- 解包电池信息 ---
    // 将收到的整数值除以10，还原为浮点电压值
    sensor_data->common.batteryLevel = payload->battery_level;
    sensor_data->common.batteryVoltage = (float)payload->battery_voltage_x10 / 10.0f;

    return true; // 解析成功
}

