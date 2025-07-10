/**
 * @file device_properties.c
 * @brief 设备属性库的实现文件
 * @date 2025-06-08
 *
 * 该文件未来可以用于实现与设备属性相关的具体功能，
 * 例如：初始化、序列化、反序列化等。
 */

#include "device_properties.h"
#include <stdio.h>  // 用于 snprintf 和 sscanf
#include <string.h> // 用于 strlen

// 未来可以在此处添加函数实现 

const char* format_location_string(double latitude, char lat_indicator, 
                                   double longitude, char lon_indicator,
                                   char* buffer, int buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return NULL;
    }

    // 使用 snprintf 保证安全，防止缓冲区溢出
    int result = snprintf(buffer, buffer_size, "%.4f %c, %.4f %c", 
                          latitude, lat_indicator, 
                          longitude, lon_indicator);

    // 检查 snprintf 是否成功并且没有截断
    if (result > 0 && result < buffer_size) {
        return buffer;
    }

    // 如果失败或空间不足，返回NULL
    return NULL;
}

bool parse_location_string(const char* location_string, 
                           double* latitude, char* lat_indicator,
                           double* longitude, char* lon_indicator)
{
    if (location_string == NULL || latitude == NULL || lat_indicator == NULL || 
        longitude == NULL || lon_indicator == NULL) {
        return false;
    }
		
		// `sscanf`格式字符串中的空格可以匹配一个或多个任意的空白字符（如空格、制表符、换行符）。
		// 在 `%c` 格式说明符前加上一个空格是关键技巧，
		// 它可以消耗掉数值（如29.3532）和方向字符（如'N'）之间的所有空白符，
		// 确保 `%c` 能准确地读到非空白的方向字符。
    int items_parsed = sscanf(location_string, "%lf %c, %lf %c", 
                              latitude, lat_indicator, 
                              longitude, lon_indicator);

    // sscanf 应该成功解析出4个项目
    return (items_parsed == 4);
} 
