#ifndef __CLI_MANAGER_H
#define __CLI_MANAGER_H

/**
 * @brief  处理来自命令行的任何待处理指令。
 * @note   当系统处于接受CLI指令的状态（例如，配置模式）时，
 *         应在主循环中定期调用此函数。
 *         它会检查来自USART的新数据，进行解析并执行指令。
 */
void CLI_Process(void);


#endif // __CLI_MANAGER_H 