#ifndef __LORA_PROTOCOL_H
#define __LORA_PROTOCOL_H

#include <stdint.h>  // 标准整数类型
#include <stddef.h>  // size_t 定义
#include <stdbool.h> // bool 类型
#include "device_properties.h" // 引入设备属性定义

// --- 地址定义 ---
#define LORA_HOST_ADDRESS 0x00      // 主机地址 (使用 0x00)
#define LORA_BROADCAST_ADDRESS 0xFF // 广播地址

// --- 帧结构常量 ---
#define LORA_HEADER_SIZE 4      // 帧头长度: Target(1)+Sender(1)+Type(1)+Seq(1)
#define LORA_CHECKSUM_SIZE 2    // 校验和长度: CRC16 = 2 bytes
#define LORA_MAX_RAW_PACKET 255 // LoRa 物理层允许的最大原始数据包长度
// 计算应用层载荷的最大长度
#define LORA_MAX_PAYLOAD_APP (LORA_MAX_RAW_PACKET - LORA_HEADER_SIZE - LORA_CHECKSUM_SIZE)

// --- 消息类型定义 ---
#define MSG_TYPE_CMD_SET_CONFIG 0x10    // Host -> Slave: 设置参数命令
#define MSG_TYPE_CMD_REPORT_CONFIG 0x11 // Slave -> Host: 控制器属性上报
// #define MSG_TYPE_CMD_GET_STATUS 0x11 // Host -> Slave: 获取状态命令
#define MSG_TYPE_REPORT_SENSOR 0x20 // Slave -> Host: 上报传感器数据
#define MSG_TYPE_REPORT_STATUS 0x21 // Slave -> Host: 上报设备状态/回复状态
#define MSG_TYPE_HEARTBEAT 0xA0     // Slave -> Host: 心跳包
// 如果未来需要 ACK/NACK
// #define MSG_TYPE_ACK_SUCCESS      0xAC
// #define MSG_TYPE_ACK_FAIL         0xAF

// 设备类型
#define DEVICE_TYPE_HOST 0x10    // 主机
#define DEVICE_TYPE_SENSOR_Internal 0x11  // 大棚环境传感器节点
#define DEVICE_TYPE_SENSOR_External 0x13  // 室外环境传感器节点
#define DEVICE_TYPE_CONTROL 0x12 // 控制节点

// --- 解析后的消息结构体 (应用层使用) ---
// 注意：此结构体不包含原始 CRC 校验字节
typedef struct
{
    uint8_t target_addr;                   // 目标地址
    uint8_t sender_addr;                   // 发送者地址
    uint8_t msg_type;                      // 消息类型
    uint8_t seq_num;                       // 消息序列号
    uint8_t payload[LORA_MAX_PAYLOAD_APP]; // 应用层数据载荷
    uint8_t payload_len;                   // 应用层载荷的实际长度
    int16_t rssi;                          // 信号强度 (由底层驱动填充)
    float snr;                             // 信噪比 (由底层驱动填充)
} lora_parsed_message_t;

// 传感器节点上报数据的数据载荷模型 (Payload)
// 设计目标是尽可能紧凑，以节省 LoRa 的空中时间 (Time on Air)。
// 它将应用层 `ExternalSensorProperties_t` 中易于使用的浮点数和字符串
// 转换为适合低带宽传输的整数格式。
typedef struct
{
    // 室外温湿度 (SHT40/BMP280)
    int8_t   temperature_int;      // 温度整数部分 (支持负数)
    uint8_t  temperature_dec;      // 温度小数部分 (e.g., 25.67 -> temp_int=25, temp_dec=67)
    uint8_t  humidity_int;         // 湿度整数部分 (0-100)
    uint8_t  humidity_dec;         // 湿度小数部分

    // 大气压 (BMP280)
    uint32_t air_pressure;         // 气压，单位: Pa

    // 光照强度 (BH1750)
    uint32_t light_intensity;      // 光照强度，单位: lux

    // GPS 数据 (GP-02)
    int16_t  altitude;             // 海拔，单位: m (支持负数)
    int32_t  latitude_e6;          // 纬度 * 1,000,000
    int32_t  longitude_e6;         // 经度 * 1,000,000
    
    // 通用设备属性
    uint8_t  battery_level;        // 电池电量 (0-100)
    uint16_t battery_voltage_x10;  // 电池电压 * 10 (例如 4.2V 存储为 42)

} __attribute__((packed)) sensor_data_payload_t; // `__attribute__((packed))` 确保编译器以最紧凑的方式存储此结构体，不进行任何字节对齐填充，这对于跨平台和精确控制载荷大小至关重要。

// --- 函数声明 ---

/**
 * @brief 计算给定数据的 CRC16 Modbus 校验码
 * @param data 指向数据缓冲区的指针
 * @param length 数据的字节长度
 * @return uint16_t 计算得到的 CRC16 值 (小端序)
 */
uint16_t crc16_modbus(const uint8_t *data, size_t length);

// --- 字节流打包/解包辅助函数 (处理小端序) ---
// 这些函数用于在不同数据类型和字节数组之间进行转换，确保数据在传输和存储时遵循统一的字节序。
void lora_model_pack_u8(uint8_t *buffer, uint8_t value);
void lora_model_pack_i8(uint8_t *buffer, int8_t value);
void lora_model_pack_u16le(uint8_t *buffer, uint16_t value);
void lora_model_pack_i16le(uint8_t *buffer, int16_t value);
void lora_model_pack_u32le(uint8_t *buffer, uint32_t value);
void lora_model_pack_i32le(uint8_t *buffer, int32_t value);
void lora_model_pack_float_ieee754le(uint8_t *buffer, float value); // IEEE 754 single precision

uint8_t lora_model_unpack_u8(const uint8_t *buffer);
int8_t lora_model_unpack_i8(const uint8_t *buffer);
uint16_t lora_model_unpack_u16le(const uint8_t *buffer);
int16_t lora_model_unpack_i16le(const uint8_t *buffer);
uint32_t lora_model_unpack_u32le(const uint8_t *buffer);
int32_t lora_model_unpack_i32le(const uint8_t *buffer);
float lora_model_unpack_float_ieee754le(const uint8_t *buffer); // IEEE 754 single precision

// --- 帧处理函数声明 ---

/**
 * @brief 定义帧解析结果的状态码
 */
typedef enum
{
    LORA_FRAME_OK = 0,                   // 解析成功且 CRC 正确
    LORA_FRAME_ERR_INVALID_LEN = -1,     // 接收到的原始数据长度不足
    LORA_FRAME_ERR_INVALID_CRC = -2,     // CRC 校验失败
    LORA_FRAME_ERR_INVALID_PARAM = -3,   // 输入参数无效 (例如 buffer 为 NULL)
    LORA_FRAME_ERR_BUFFER_TOO_SMALL = -4 // 提供的 parsed_msg->payload 缓冲区不足
} lora_frame_status_t;

/**
 * @brief 生成一个完整的 LoRa 数据帧（头部 + 载荷 + CRC16）
 *
 * @param target_addr 目标设备地址
 * @param sender_addr 发送设备地址 (本机地址)
 * @param msg_type 消息类型代码
 * @param seq_num 消息序列号
 * @param payload 指向应用层数据载荷的指针
 * @param payload_len 应用层数据载荷的长度
 * @param output_buffer 指向用于存储最终 LoRa 帧的缓冲区的指针
 * @param output_buffer_max_len output_buffer 的最大长度
 * @return int 成功则返回完整的 LoRa 帧长度，失败则返回负值错误码 (见 lora_frame_status_t, 但用 int 表示)
 */
int generate_lora_frame(uint8_t target_addr, uint8_t sender_addr, uint8_t msg_type, uint8_t seq_num,
                        const uint8_t *payload, size_t payload_len,
                        uint8_t *output_buffer, size_t output_buffer_max_len);

/**
 * @brief 解析接收到的原始 LoRa 数据帧，校验 CRC16 并提取信息
 *
 * @param raw_packet 指向接收到的原始数据缓冲区的指针
 * @param raw_len 接收到的原始数据的总长度
 * @param parsed_msg 指向用于存储解析后消息信息的结构体的指针 (不包含原始 CRC)
 * @return lora_frame_status_t 返回解析状态 (OK, CRC错误, 长度错误等)
 */
lora_frame_status_t parse_lora_frame(const uint8_t *raw_packet, size_t raw_len,
                                     lora_parsed_message_t *parsed_msg);

/**
 * @brief 将高层传感器数据结构打包成紧凑的 LoRa 载荷格式
 *
 * 本函数是 lora_model_parse_sensor_data 的逆操作。它将应用层的
 * ExternalSensorProperties_t 结构体中的数据（包含浮点数和字符串）
 * 转换为 sensor_data_payload_t 结构体。
 * 转换规则:
 * - 浮点数 (如温度/湿度) 被拆分为整数和小数部分。
 * - 电压被乘以 10 并存为整数。
 * - GPS 坐标被乘以 1,000,000 并存为整数。
 * - 海拔直接转换为有符号整数，支持负值。
 *
 * @param sensor_data 指向包含源传感器数据的 ExternalSensorProperties_t 结构体 (输入)
 * @param payload 指向用于存储打包后结果的 sensor_data_payload_t 结构体 (输出)
 * @return bool 如果成功打包则返回 true，若输入参数无效则返回 false
 */
bool lora_model_create_sensor_payload(const ExternalSensorProperties_t *sensor_data,
                                      sensor_data_payload_t *payload);

/**
 * @brief 从已解析的消息中提取传感器数据并转换为应用层结构体
 *
 * 检查消息类型和载荷长度是否匹配传感器报告格式。
 * 如果匹配，则从 payload 中解包字节数据，根据打包规则执行逆操作
 * (例如，将整数和小数部分合并为浮点数，将乘以10的电压还原等)，
 * 并填充到输出的 sensor_data 结构体中。
 *
 * @param parsed_msg 指向已通过 CRC 校验并解析了头部的消息结构体 (输入)
 * @param sensor_data 指向用于存储最终应用层结果的 ExternalSensorProperties_t 结构体 (输出)
 * @return bool 如果成功解析并填充 sensor_data 则返回 true, 否则返回 false
 * (例如消息类型不匹配、载荷长度错误等)
 */
bool lora_model_parse_sensor_data(const lora_parsed_message_t *parsed_msg,
                                  ExternalSensorProperties_t *sensor_data);


#endif