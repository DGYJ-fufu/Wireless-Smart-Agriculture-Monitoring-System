/**
 * @file lora_protocol.c
 * @brief LoRa 通信协议核心工具函数实现
 *
 * 包含 CRC16-Modbus 计算、基本数据类型打包/解包 (小端序)、
 * 以及 LoRa 数据帧构建和解析的功能。
 * 设计为平台无关，依赖 <stdint.h>, <stddef.h>, <string.h>, <stdbool.h>。
 */

#include "lora_protocol.h" // 包含本模块的头文件（定义和函数声明）
#include <string.h>        // 标准库，用于 memcpy
#include <math.h>        // 用于 roundf, floorf, isnan, isinf
#include "stdlib.h"
#include "settings.h"

// ============================================================================
// CRC16-Modbus 实现
// ============================================================================

// CRC16 Modbus 预计算查找表 (基于多项式 0xA001 - 0x8005 的反转形式)
// 使用查表法可以提高 CRC 计算效率
static const uint16_t crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040};

/**
 * @brief 计算给定数据的 CRC16 Modbus 校验码
 * @param data 指向待计算数据的指针
 * @param length 数据的字节长度
 * @return uint16_t 计算得到的 16 位 CRC 值。
 * 注意：Modbus CRC 结果通常是低字节在前，高字节在后。
 * 在小端序系统上，这个函数返回的 uint16_t 值可以直接按字节放入内存。
 */
uint16_t crc16_modbus(const uint8_t *data, size_t length)
{
    if (data == NULL || length == 0)
        return 0;          // 对无效输入返回 0
    uint16_t crc = 0xFFFF; // Modbus CRC 初始值为 0xFFFF
    for (size_t i = 0; i < length; i++)
    {
        // 取当前 CRC 低字节与当前数据字节异或，结果作为查表索引
        uint8_t index = (crc ^ data[i]) & 0xFF;
        // CRC 右移 8 位（高字节移到低字节），然后与查表得到的 CRC 值异或
        crc = (crc >> 8) ^ crc16_table[index];
    }
    // Modbus CRC 计算完成，不需要最终异或操作
    return crc;
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
 * @brief 将 float 值按 IEEE 754 单精度格式写入缓冲区 (小端序)
 * @note 依赖于编译器和 CPU 架构对 float 的内存表示。
 * 对于 ESP32 和常见的 ARM Cortex-M (小端序)，memcpy 通常有效。
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
 * @brief 从缓冲区按 IEEE 754 单精度格式读取 float 值 (小端序)
 * @note 同样依赖于平台的内存表示。
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
    // 检查应用层载荷是否超过协议定义的最大值
    if (payload_len > LORA_MAX_PAYLOAD_APP)
    {
        // Payload too large for the defined protocol limits
        return LORA_FRAME_ERR_INVALID_LEN; // 使用负值表示错误类型
    }

    size_t header_len = LORA_HEADER_SIZE;
    size_t crc_len = LORA_CHECKSUM_SIZE;
    // 计算包含头部、载荷和 CRC 的总帧长
    size_t total_len = header_len + payload_len + crc_len;

    // 检查输出缓冲区是否足够大
    if (total_len > output_buffer_max_len)
    {
        // Output buffer is too small to hold the entire frame
        return LORA_FRAME_ERR_BUFFER_TOO_SMALL;
    }
    // 检查输出缓冲区指针是否有效
    if (output_buffer == NULL)
    {
        return LORA_FRAME_ERR_INVALID_PARAM;
    }

    // --- 按顺序填充数据帧 ---
    // 1. 填充协议头 (4 字节)
    lora_model_pack_u8(&output_buffer[0], target_addr);
    lora_model_pack_u8(&output_buffer[1], sender_addr);
    lora_model_pack_u8(&output_buffer[2], msg_type);
    lora_model_pack_u8(&output_buffer[3], seq_num);

    // 2. 填充应用层数据载荷
    if (payload != NULL && payload_len > 0)
    {
        memcpy(&output_buffer[header_len], payload, payload_len);
    }

    // 3. 计算 CRC16 (校验范围: 从 TargetAddr 到 Payload 的最后一个字节)
//    uint16_t crc = crc16_modbus(output_buffer, header_len + payload_len);
		// 计算需要校验的数据长度
		size_t data_len_to_crc = header_len + payload_len;
		// 使用硬件 CRC 计算
		uint32_t crc_result_32 = HAL_CRC_Calculate(&hcrc, (uint32_t*)output_buffer, data_len_to_crc);
		uint16_t crc = (uint16_t)crc_result_32;

    // 4. 填充 CRC16 到帧尾 (小端序: 低字节在前)
    lora_model_pack_u16le(&output_buffer[header_len + payload_len], crc);

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
//    uint16_t calculated_crc = crc16_modbus(raw_packet, data_len_for_crc);
		// 使用硬件 CRC 计算 (确保 hcrc 可访问)
		// 第三个参数 data_len_for_crc 是字节数，因为我们在 CubeMX 中配置了 Input Data Format 为 Bytes
		uint32_t crc_result_32 = HAL_CRC_Calculate(&hcrc, (uint32_t*)raw_packet, data_len_for_crc);
		uint16_t calculated_crc = (uint16_t)crc_result_32; // 硬件 CRC 返回的是 32 位寄存器的值，取低 16 位

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
 * @brief 将浮点传感器数据打包到 sensor_data_payload_t 结构体中 (生成二进制载荷)
 *
 * @param payload_struct 指向要填充的目标 sensor_data_payload_t 结构体的指针
 * @param temp 温度值
 * @param hum 湿度值
 * @param light 光照强度值
 * @param volt 电压值 (伏特)
 * @return bool true 表示打包成功, false 表示输入数据无效 (例如 NaN 或 Inf)
 */
bool pack_sensor_data_payload(sensor_data_payload_t *payload_struct, // 新函数名 V2
                                 float temp, float hum, float light, float volt)
{
    // 检查输入有效性
    if (payload_struct == NULL || isnan(temp) || isinf(temp) || isnan(hum) || isinf(hum) ||
        isnan(light) || isinf(light) || isnan(volt) || isinf(volt))
    {
        return false;
    }

    uint8_t *buffer = (uint8_t *)payload_struct; // 获取结构体起始地址，方便使用 pack 函数
    size_t offset = 0;

    // --- 处理温度 (Temp) - 2字节 ---
    // 限制范围 [-128, 127.99]
    if (temp > 127.99f) temp = 127.99f;
    if (temp < -128.0f) temp = -128.0f;
    int16_t temp_scaled = (int16_t)roundf(temp * 100.0f);
    lora_model_pack_i8(&buffer[offset], (int8_t)(temp_scaled / 100)); // 整数部分 (int8_t)
    offset += 1;
    lora_model_pack_u8(&buffer[offset], (uint8_t)(abs(temp_scaled % 100))); // 小数部分 (uint8_t)
    offset += 1;

    // --- 处理湿度 (Hum) - 2字节 ---
    // 限制范围 [0, 255.99]
    if (hum < 0.0f) hum = 0.0f;
    if (hum > 255.99f) hum = 255.99f;
    uint16_t hum_scaled = (uint16_t)roundf(hum * 100.0f);
    lora_model_pack_u8(&buffer[offset], (uint8_t)(hum_scaled / 100)); // 整数部分 (uint8_t)
    offset += 1;
    lora_model_pack_u8(&buffer[offset], (uint8_t)(hum_scaled % 100)); // 小数部分 (uint8_t)
    offset += 1;

    // --- 处理光照 (Light) - 3字节 ---
    // 限制范围 [0, 65535.99] (uint16_t)
    if (light < 0.0f) light = 0.0f;
    if (light > 65535.99f) light = 65535.99f;
    // 使用 uint32_t 避免 scaled 值溢出
    uint32_t light_scaled = (uint32_t)roundf(light * 100.0f);
    // **使用 pack_u16le 打包整数部分**
    lora_model_pack_u16le(&buffer[offset], (uint16_t)(light_scaled / 100)); // 整数部分 (uint16_t)
    offset += 2; // 整数部分占 2 字节
    lora_model_pack_u8(&buffer[offset], (uint8_t)(light_scaled % 100)); // 小数部分 (uint8_t)
    offset += 1;

    // --- 处理电压 (Volt) - 2字节 ---
    // 限制范围 [0, 255.99]
    if (volt < 0.0f) volt = 0.0f;
    if (volt > 255.99f) volt = 255.99f;
    uint16_t volt_scaled = (uint16_t)roundf(volt * 100.0f);
    lora_model_pack_u8(&buffer[offset], (uint8_t)(volt_scaled / 100)); // 整数部分 (uint8_t)
    offset += 1;
    lora_model_pack_u8(&buffer[offset], (uint8_t)(volt_scaled % 100)); // 小数部分 (uint8_t)
    offset += 1;

    // 检查最终偏移量是否等于结构体大小
    if (offset != sizeof(sensor_data_payload_t)) {
         // 这是一个内部逻辑错误
         // ESP_LOGE(TAG, "打包逻辑错误，最终偏移量 %d != 结构体大小 %d", offset, sizeof(sensor_data_payload_t));
         return false;
    }

    return true; // 打包成功
}

/**
 * @brief 将控制命令打包到 control_data_payload_t 结构体中 (生成二进制载荷)
 *
 * @param payload_struct 指向要填充的目标 control_data_payload_t 结构体的指针
 * @param fan_stat 风扇状态
 * @param fan_spd 风扇速度
 * @param pump_stat 水泵状态
 * @param pump_spd 水泵速度
 * @param light_stat 灯光状态
 * @return bool true 表示打包成功, false 表示输入指针无效
 */
bool pack_control_data_payload(control_data_payload_t *payload_struct,
                               bool fan_stat, uint8_t fan_spd,
                               bool pump_stat, uint8_t pump_spd,
                               uint8_t light_stat)
{
    // 检查输入有效性
    if (payload_struct == NULL)
    {
        return false; // 输入指针无效
    }

    payload_struct->fanStatus = fan_stat;
    payload_struct->fanSpeed = fan_spd;
    payload_struct->pumpStatus = pump_stat;
    payload_struct->pumpSpeed = pump_spd;
    payload_struct->growLightStatus = light_stat;

    return true; // 打包成功
}

/**
 * @brief 将控制节点数据打包到字节缓冲区中 (生成二进制载荷)
 *
 * @param output_buffer 指向用于存储打包后数据的目标缓冲区的指针
 * @param buffer_len 目标缓冲区的最大长度 (至少应为 2 字节)
 * @param dev_code 要打包的设备类型代码
 * @param dev_state 要打包的设备状态
 * @return bool true 表示打包成功, false 表示失败 (例如缓冲区为空或太小)
 */
bool pack_controller_data_payload(controller_data_payload_t *payload_struct,uint8_t dev_code, uint8_t dev_state)
{
    // 检查输入有效性
    if (payload_struct == NULL || isnan(dev_code) || isnan(dev_state))
    {
        return false;
    }

    uint8_t *buffer = (uint8_t *)payload_struct; // 获取结构体起始地址，方便使用 pack 函数
    size_t offset = 0;
		
    lora_model_pack_i8(&buffer[offset], dev_code);
    offset += 1;
    lora_model_pack_u8(&buffer[offset], dev_state);
    offset += 1;

    // 检查最终偏移量是否等于结构体大小
    if (offset != sizeof(sensor_data_payload_t)) {
         // 这是一个内部逻辑错误
         // ESP_LOGE(TAG, "打包逻辑错误，最终偏移量 %d != 结构体大小 %d", offset, sizeof(sensor_data_payload_t));
         return false;
    }

    return true; // 打包成功
}

extern bool fan_status,pump_status,light_status;
extern uint8_t fan_speed,pump_speed;
extern LoRa myLoRa;
extern uint8_t device_id;

static uint8_t transmit_data[LORA_MAX_RAW_PACKET];

void controller_data_update(void){
	control_data_payload_t control_data;
	pack_control_data_payload(&control_data,fan_status,fan_speed,pump_status,pump_speed,light_status);
	uint8_t frame_len;
	frame_len = generate_lora_frame(
								LORA_HOST_ADDRESS,                 			// 目标地址：主机
								 device_id,                  	// 源地址：本机
								MSG_TYPE_CMD_REPORT_CONFIG,            	// 消息类型：传感器报告
								0,                           						// 序列号
								(const uint8_t*)&control_data,   				// **将结构体指针强制转换为 uint8_t 指针作为载荷**
								sizeof(control_data_payload_t),     		// **载荷长度就是结构体的大小**
								transmit_data,                 					// 输出缓冲区
								sizeof(transmit_data)          					// 输出缓冲区大小
							);
	LoRa_transmit(&myLoRa,transmit_data,frame_len,200);
	LoRa_startReceiving(&myLoRa);
}
