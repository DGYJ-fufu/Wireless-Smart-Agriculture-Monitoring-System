/**
 * @file gps.c
 * @brief GP-02 GPS模块驱动程序的实现。
 * @note  本驱动通过UART中断接收NMEA语句，并解析其中的RMC和GGA语句，
 *        提取时间、位置、状态等关键信息。
 */
#include "gps.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define GPS_BUFFER_SIZE 256

// 用于HAL库进行UART单字节接收的缓冲区
uint8_t gps_rx_byte; 
// 指向当前使用的UART句柄的指针
static UART_HandleTypeDef *gps_huart;

// 用于存储一条完整NMEA语句的缓冲区
static uint8_t gps_buffer[GPS_BUFFER_SIZE];
// gps_buffer的索引
static uint16_t gps_buffer_index = 0;

// GPS数据结构体实例
GPS_t gps_data;

// 私有函数原型
static double nmea_to_decimal(double nmea_val, char indicator);
static void parse_rmc(char *line);
static void parse_gga(char *line);


void GPS_Init(UART_HandleTypeDef *huart)
{
    // 保存传入的UART句柄以供后续使用
    gps_huart = huart;

    // 清空主数据结构体和接收缓冲区
    memset(&gps_data, 0, sizeof(gps_data));
    memset(gps_buffer, 0, GPS_BUFFER_SIZE);
    gps_buffer_index = 0;
    
    // --- 关键修复：清除可能存在的UART错误标志 ---
    // 在系统启动期间，GPS模块可能已经开始发送数据，而此时MCU还未准备好接收，
    // 这可能导致ORE（覆写）或FE（帧）等错误。
    // 在启动接收前，我们必须清除这些"陈旧"的错误标志，否则接收中断将无法正常触发。
    __HAL_UART_CLEAR_IT(gps_huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF);
    
    // 启动中断，开始接收一个字节。
    // 如果成功，每次接收完成时都会触发 HAL_UART_RxCpltCallback
    if (HAL_UART_Receive_IT(gps_huart, &gps_rx_byte, 1) != HAL_OK)
    {
        // 如果启动失败，可以在这里添加错误处理
        printf("GPS_Init: Failed to start UART reception\r\n");
    }
}

void GPS_Pause(void)
{
    // 中断正在进行的IT接收。
    // 这是停止UART产生中断的正确方式。
    if (gps_huart != NULL)
    {
        HAL_UART_AbortReceive_IT(gps_huart);
    }
}

void GPS_Resume(void)
{
    // 恢复接收与初始化本质上是相同的：清除旧状态并重新开始接收。
    if (gps_huart != NULL)
    {
        // 重新初始化接收逻辑，
        // 这会清除标志位并重启HAL_UART_Receive_IT。
        GPS_Init(gps_huart);
    }
}

void GPS_UART_ErrorHandler(void)
{
    // 简单地重新启动接收
    if (HAL_UART_Receive_IT(gps_huart, &gps_rx_byte, 1) != HAL_OK)
    {
        // 错误处理
    }
}

void GPS_ReceiveData(void)
{
    // 检查是否接收到换行符，这标志着一条语句的结束
    if (gps_rx_byte == '\n' && gps_buffer_index > 0)
    {
        // 在缓冲区末尾添加字符串结束符
        gps_buffer[gps_buffer_index] = '\0';
        
        // 解析接收到的NMEA语句
        GPS_Parse((uint8_t*)gps_buffer);
        
        // 为下一条语句重置缓冲区索引
        gps_buffer_index = 0;
    }
    else
    {
        // 如果缓冲区未满，则将接收到的字节添加到缓冲区
        if (gps_buffer_index < (GPS_BUFFER_SIZE - 1))
        {
            // 忽略回车符
            if (gps_rx_byte != '\r')
            {
                gps_buffer[gps_buffer_index++] = gps_rx_byte;
            }
        }
    }
    
    // 重新使能UART接收，以接收下一个字节
    if (HAL_UART_Receive_IT(gps_huart, &gps_rx_byte, 1) != HAL_OK)
    {
        // 错误处理
    }
}

void GPS_Parse(uint8_t* line)
{
    // 我们主要关心 RMC (推荐最小定位信息) 和 GGA (GPS定位数据) 语句
    // 同时检查 GN (多星座系统) 和 GP (仅GPS) 的语句头
    if (strstr((char*)line, "RMC"))
    {
        parse_rmc((char*)line);
    }
    else if (strstr((char*)line, "GGA"))
    {
        parse_gga((char*)line);
    }
}

/**
 * @brief 将NMEA的DDMM.MMMM或DDDMM.MMMM格式转换为十进制度。
 * @param nmea_val 从NMEA语句中直接提取的浮点数值。
 * @param indicator 方向指示符 ('N', 'S', 'E', 'W')。
 * @return double 转换后的十进制度坐标值 (南纬和西经为负数)。
 */
static double nmea_to_decimal(double nmea_val, char indicator)
{
    if (nmea_val == 0.0)
    {
        return 0.0;
    }
    
    // NMEA 格式是 (D)DDMM.MMMM (度分)
    // 获取度部分 (例如, 11714.22101 -> 117)
    int degrees = (int)(nmea_val / 100);
    // 获取分部分 (例如, 11714.22101 -> 14.22101)
    double minutes = nmea_val - (double)(degrees * 100);
    // 将分转换为度，并与度部分相加
    double decimal_val = (double)degrees + (minutes / 60.0);

    // 根据方向指示符，为南半球和西半球的坐标应用负号
    if (indicator == 'S' || indicator == 'W')
    {
        decimal_val = -decimal_val;
    }

    return decimal_val;
}

static void parse_rmc(char *line)
{
    // 示例: $GNRMC,153001.000,A,2921.19673,N,11714.22101,E,0.00,221.19,080625,,,A*0C
    // strtok是不可重入的，它会修改传入的字符串。在多线程环境下应避免使用，
    // 但在当前单片机的顺序执行流程中是可接受的。
    char *token;
    int token_index = 0;
		// 使用临时变量保存解析出的值，直到确认整条语句有效再更新全局结构体
		double temp_lat = 0, temp_lon = 0;
		char temp_lat_ind = ' ', temp_lon_ind = ' ';

    token = strtok(line, ",");
    while (token != NULL)
    {
        switch (token_index)
        {
            case 1: // 时间 (HHMMSS.sss)
                if (strlen(token) >= 6)
                {
                    sscanf(token, "%2hhu%2hhu%2hhu", &gps_data.utc_hour, &gps_data.utc_minute, &gps_data.utc_second);
                }
                break;
            case 2: // 状态 ('A' 或 'V')
                gps_data.status = token[0];
                break;
            case 3: // 纬度 (DDMM.MMMM)
                temp_lat = atof(token);
                break;
            case 4: // 纬度半球 ('N' 或 'S')
                temp_lat_ind = token[0];
                break;
            case 5: // 经度 (DDDMM.MMMM)
                temp_lon = atof(token);
                break;
            case 6: // 经度半球 ('E' 或 'W')
                temp_lon_ind = token[0];
                break;
            case 7: // 对地速度 (节)
                gps_data.speed_knots = atof(token);
                break;
            case 8: // 对地航向 (度)
                gps_data.course = atof(token);
                break;
            case 9: // 日期 (DDMMYY)
                if (strlen(token) == 6)
                {
                    uint16_t year_temp;
                    sscanf(token, "%2hhu%2hhu%2hu", &gps_data.utc_day, &gps_data.utc_month, &year_temp);
                    gps_data.utc_year = year_temp < 2000 ? year_temp + 2000 : year_temp;
                }
                break;
        }
        token_index++;
        token = strtok(NULL, ",");
    }

    // 在所有字段解析完成后，如果定位状态是有效的('A')，
    // 才进行坐标格式转换并更新全局数据结构。
    if (gps_data.status == 'A')
    {
        gps_data.latitude = nmea_to_decimal(temp_lat, temp_lat_ind);
				gps_data.lat_indicator = temp_lat_ind;
        gps_data.longitude = nmea_to_decimal(temp_lon, temp_lon_ind);
				gps_data.lon_indicator = temp_lon_ind;
        gps_data.new_data_flag = 1; // 发出信号，表示有新的有效数据可用
    }
}


static void parse_gga(char *line)
{
    // 示例: $GNGGA,153002.000,2921.19624,N,11714.22086,E,1,16,3.1,68.5,M,-1.6,M,,*6F
    char *token;
    int token_index = 0;

    token = strtok(line, ",");
    while (token != NULL)
    {
        switch (token_index)
        {
            // 注意: RMC语句已提供了核心定位信息 (时间、经纬度、日期)。
            // 我们只从GGA语句中获取RMC所不包含的补充信息，如定位质量和卫星数。
            case 6: // 定位质量
                gps_data.fix_quality = atoi(token);
                break;
            case 7: // 使用中的卫星数量
                gps_data.satellites_in_use = atoi(token);
                break;
            case 8: // HDOP
                gps_data.hdop = atof(token);
                break;
            case 9: // 海拔高度
                gps_data.altitude = atof(token);
                break;
        }
        token_index++;
        token = strtok(NULL, ",");
    }
} 