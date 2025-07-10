#ifndef __GPS_H
#define __GPS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief GPS 数据结构体，用于存储解析后的信息
 */
typedef struct {
    // RMC语句解析出的数据
    uint8_t utc_hour;       // UTC时间 - 时
    uint8_t utc_minute;     // UTC时间 - 分
    uint8_t utc_second;     // UTC时间 - 秒
    uint8_t utc_day;        // UTC日期 - 日
    uint8_t utc_month;      // UTC日期 - 月
    uint16_t utc_year;      // UTC日期 - 年
    char status;            // 定位状态: 'A' = 有效, 'V' = 无效
    double latitude;        // 纬度 (十进制度)
    char lat_indicator;     // 纬度半球: 'N' (北) 或 'S' (南)
    double longitude;       // 经度 (十进制度)
    char lon_indicator;     // 经度半球: 'E' (东) 或 'W' (西)
    float speed_knots;      // 对地速度 (节)
    float course;           // 对地航向 (度)

    // GGA语句解析出的数据
    uint8_t fix_quality;        // 定位质量: 0=无效, 1=GPS, 2=DGPS 等
    uint8_t satellites_in_use;  // 正在使用的卫星数量
    float hdop;                 // 水平精度因子
    float altitude;             // 海拔高度 (米)
    
    // 自定义标志位，用于指示是否有新的有效数据被解析
    uint8_t new_data_flag;

} GPS_t;

// 声明一个全局的GPS数据结构体实例
extern GPS_t gps_data;
// 声明一个供外部（例如it.c）访问的接收字节变量
extern uint8_t gps_rx_byte;

/**
 * @brief 初始化GPS驱动并开始监听
 * @param huart 指向用于GPS通信的UART句柄 (例如 &hlpuart1)
 * @note  该函数会启动UART中断接收
 */
void GPS_Init(UART_HandleTypeDef *huart);

/**
 * @brief 解析一条完整的NMEA语句
 * @param line: 指向包含NMEA语句的、以null结尾的字符串的指针
 */
void GPS_Parse(uint8_t *line);

/**
 * @brief GPS数据处理函数，在UART接收中断回调中被调用
 * @note  此函数负责拼装NMEA语句并调用解析器
 */
void GPS_ReceiveData(void);

/**
 * @brief GPS UART 错误处理函数
 * @note  在 HAL_UART_ErrorCallback 中调用，用于从错误中恢复并重启接收
 */
void GPS_UART_ErrorHandler(void);

/**
 * @brief 暂停GPS接收
 * @note  此函数用于暂停GPS接收，通常在需要暂停接收时调用
 */
void GPS_Pause(void);

/**
 * @brief 恢复GPS接收
 * @note  此函数用于恢复GPS接收，通常在需要恢复接收时调用
 */
void GPS_Resume(void);

#ifdef __cplusplus
}
#endif

#endif /* __GPS_H */ 