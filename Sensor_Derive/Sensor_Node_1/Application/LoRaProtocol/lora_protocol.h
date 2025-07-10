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

// 控制器类型
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

// 传感器节点上报数据的数据载荷模型 (Payload)
// 设计目标是尽可能紧凑，以节省 LoRa 的空中时间 (Time on Air)。
// 浮点数被拆分为整数和小数部分进行传输，以避免直接传输4字节的 float。
typedef struct
{
    // SHT40 温湿度传感器数据
    int8_t   greenhouse_temp_int;    // 温室温度整数部分 (支持负数)
    uint8_t  greenhouse_temp_dec;    // 温室温度小数部分 (e.g., 25.67 -> temp_int=25, temp_dec=67)
    uint8_t  greenhouse_humid_int;   // 温室湿度整数部分 (0-100)
    uint8_t  greenhouse_humid_dec;   // 温室湿度小数部分

    // SP3485 土壤传感器数据
    int8_t   soil_moisture_int;      // 土壤含水率整数部分
    uint8_t  soil_moisture_dec;      // 土壤含水率小数部分
    int8_t   soil_temp_int;          // 土壤温度整数部分
    uint8_t  soil_temp_dec;          // 土壤温度小数部分
    uint16_t soil_ec;                // 土壤电导率 (μS/cm)
    uint8_t  soil_ph_int;            // 土壤PH值整数部分
    uint8_t  soil_ph_dec;            // 土壤PH值小数部分
    uint16_t soil_nitrogen;          // 土壤氮含量 (mg/kg)
    uint16_t soil_phosphorus;        // 土壤磷含量 (mg/kg)
    uint16_t soil_potassium;         // 土壤钾含量 (mg/kg)
    uint16_t soil_salinity;          // 土壤盐度 (mg/L)
    uint16_t soil_tds;               // 土壤总溶解固体 (ppm)
    uint16_t soil_fertility;         // 土壤肥力

    // BH1750 光照传感器数据
    uint32_t light_intensity;        // 光照强度 (lux)

    // SGP30 空气质量传感器数据
    uint16_t voc_concentration;      // VOC 浓度 (ppb)
    uint16_t co2_concentration;      // CO2 浓度 (ppm)
    
    // 通用设备属性
    uint8_t  battery_level;           // 电池电量 (0-100)
    uint8_t  battery_voltage_x10;     // 电池电压 * 10 (e.g. 4.2V -> 42)

} __attribute__((packed)) sensor_data_payload_t;

// control_data_payload_t 与 device_properties.h 中的 ControlNodeProperties_t 结构几乎一致
// 我们可以直接使用 ControlNodeProperties_t，并确保其打包
typedef struct {
    bool fanStatus;             // 风扇状态
    bool growLightStatus;       // 生长灯状态
    bool pumpStatus;            // 水泵状态
    bool shadeStatus;           // 遮阳帘状态
    uint8_t fanSpeed;           // 风扇速度 (0-100)
    uint8_t pumpSpeed;          // 水泵速度 (0-100)
} __attribute__((packed)) control_data_payload_t;

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

/**
 * @brief 将高层传感器数据结构打包成紧凑的 LoRa 载荷格式
 *
 * 本函数是 lora_model_parse_sensor_data 的逆操作。它将应用层的
 * InternalSensorProperties_t 结构体中的数据（包含浮点数）
 * 转换为 sensor_data_payload_t 结构体，该结构体使用整数和小数部分
 * 来表示浮点数，以及直接使用整型，以优化网络传输。
 *
 * @param sensor_data 指向包含源传感器数据的 InternalSensorProperties_t 结构体 (输入)
 * @param payload 指向用于存储打包后结果的 sensor_data_payload_t 结构体 (输出)
 * @return bool 如果成功打包则返回 true，若输入参数无效则返回 false
 */
bool lora_model_create_sensor_payload(const InternalSensorProperties_t *sensor_data,
                                      sensor_data_payload_t *payload);

/**
 * @brief 从已解析的消息中提取传感器数据并转换为浮点值
 *
 * 检查消息类型和载荷长度是否匹配传感器报告格式，
 * 如果匹配，则从 payload 中解包字节数据，计算出浮点值，
 * 并填充到输出的 sensor_data 结构体中。
 *
 * @param parsed_msg 指向已通过 CRC 校验并解析了头部的消息结构体 (输入)
 * @param sensor_data 指向用于存储最终浮点结果的 InternalSensorProperties_t 结构体 (输出)
 * @return bool 如果成功解析并填充 sensor_data 则返回 true, 否则返回 false
 * (例如消息类型不匹配、载荷长度错误等)
 */
bool lora_model_parse_sensor_data(const lora_parsed_message_t *parsed_msg,
                                  InternalSensorProperties_t *sensor_data);

/**
 * @brief 从已解析的消息中提取控制器配置报告数据 (类型 0x11)
 *
 * @param parsed_msg 指向已解析的消息结构体 (输入)
 * @param control_data 指向用于存储最终配置/控制数据的 ControlNodeProperties_t 结构体 (输出)
 * @return bool 如果成功解析并填充 control_data 则返回 true, 否则返回 false
 */
bool lora_model_parse_control_data(const lora_parsed_message_t *parsed_msg,
                                   ControlNodeProperties_t *control_data);

#endif