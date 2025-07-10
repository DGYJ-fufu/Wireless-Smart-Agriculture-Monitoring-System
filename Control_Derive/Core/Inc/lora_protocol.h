#ifndef LORA_PROTOCOL_H
#define LORA_PROTOCOL_H

#include <stdint.h>  // 标准整数类型
#include <stddef.h>  // size_t 定义
#include <stdbool.h> // bool 类型
#include "LoRa.h"

extern CRC_HandleTypeDef hcrc;

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
#define MSG_TYPE_CMD_SET_CONFIG 0x10 // Host -> Slave: 设置参数命令
#define MSG_TYPE_CMD_REPORT_CONFIG 0x11 // Slave -> Host: 控制器属性上报
#define MSG_TYPE_REPORT_SENSOR 0x20  // Slave -> Host: 上报传感器数据
#define MSG_TYPE_REPORT_STATUS 0x21  // Slave -> Host: 上报设备状态/回复状态
#define MSG_TYPE_HEARTBEAT 0xA0      // Slave -> Host: 心跳包
// 如果未来需要 ACK/NACK
// #define MSG_TYPE_ACK_SUCCESS      0xAC
// #define MSG_TYPE_ACK_FAIL         0xAF

// 设备类型
#define DEVICE_TYPE_HOST 0x10    // 主机
#define DEVICE_TYPE_SENSOR 0x11  // 传感器节点
#define DEVICE_TYPE_CONTROL 0x12 // 控制节点

// 消息类型
#define MSG_TYPE_CMD_SET_CONFIG 0x10 // 设置参数
#define MSG_TYPE_CMD_GET_STATUS 0x11 // 执行动作
#define MSG_TYPE_REPORT_SENSOR 0x20  // 上报传感器属性
#define MSG_TYPE_REPORT_STATUS 0x21  // 上报设备属性
#define MSG_TYPE_HEARTBEA 0xA0       // 心跳包

#define CONTROLLER_DEVICE_TYPE_STATUS_FAN 0x01   // 风扇状态
#define CONTROLLER_DEVICE_TYPE_SPEED_FAN 0x02    // 风扇工作速度
#define CONTROLLER_DEVICE_TYPE_STATUS_PUMP 0x03  // 水泵状态
#define CONTROLLER_DEVICE_TYPE_SPEED_PUMP 0x04   // 水泵工作速度
#define CONTROLLER_DEVICE_TYPE_STATUS_LIGHT 0x05 // 补光灯状态

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

// 传感器节点数据模型
typedef struct
{
    int8_t temp_int;           // 温度整数部分
    uint8_t temp_dec;          // 温度小数部分
    uint8_t humid_int;         // 湿度整数部分
    uint8_t humid_dec;         // 湿度小数部分
    uint16_t light_int;         // 光照强度整数部分
    uint8_t light_dec;         // 光照强度小数部分
    int8_t soil_moisture_int;  // 土壤湿度整数部分
    uint8_t soil_moisture_dec; // 土壤湿度小数部分
} __attribute__((packed)) sensor_data_payload_t;

// 控制节点数据模型

typedef struct
{
    bool fanStatus;             // 风扇状态
    bool growLightStatus;       // 生长灯状态
    bool pumpStatus;            // 水泵状态
    uint8_t fanSpeed;           // 风扇速度 (0-100)
    uint8_t pumpSpeed;          // 水泵速度 (0-100)
} __attribute__((packed)) control_data_payload_t;

// 控制器数据模型
typedef struct
{
    uint8_t device_code;  // 设备类型
    uint8_t device_state; // 设备状态
} __attribute__((packed)) controller_data_payload_t;

// --- 函数声明 ---

/**
 * @brief 计算给定数据的 CRC16 Modbus 校验码
 * @param data 指向数据缓冲区的指针
 * @param length 数据的字节长度
 * @return uint16_t 计算得到的 CRC16 值 (小端序)
 */
uint16_t crc16_modbus(const uint8_t *data, size_t length);

// --- 打包/解包函数声明 (小端序) ---
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

bool pack_sensor_data_payload(sensor_data_payload_t *payload_struct,
                              float temp, float hum, float light, float soil_moisture);

/**
 * @brief 将控制节点数据打包到字节缓冲区中 (生成二进制载荷)
 *
 * @param output_buffer 指向用于存储打包后数据的目标缓冲区的指针
 * @param buffer_len 目标缓冲区的最大长度 (至少应为 2 字节)
 * @param dev_code 要打包的设备类型代码
 * @param dev_state 要打包的设备状态
 * @return bool true 表示打包成功, false 表示失败 (例如缓冲区为空或太小)
 */
bool pack_controller_data_payload(controller_data_payload_t *payload_struct,uint8_t dev_code, uint8_t dev_state);

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
                               uint8_t light_stat);

void controller_data_update(void);

#endif
