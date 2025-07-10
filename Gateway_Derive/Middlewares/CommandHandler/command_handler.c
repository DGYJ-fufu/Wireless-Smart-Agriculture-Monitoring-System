#include "command_handler.h"
#include "string.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdlib.h>

// Module-level variable to hold the queue handle
static osMessageQueueId_t response_queue_handle = NULL;

void CommandHandler_Init(osMessageQueueId_t queue_handle) {
    response_queue_handle = queue_handle;
}

void CommandHandler_ProcessUrc(const char* urc_line) {
    if (response_queue_handle == NULL) {
        printf("[CMD_HANDLER] Error: Not initialized.\r\n");
        return;
    }

    // --- 采纳您的两步判断法 ---
    // 第一步：粗略判断是否为平台下发数据
    // 第二步：精确判断是否为需要响应的"命令"
    if (strstr(urc_line, "+HMREC:") != NULL && strstr(urc_line, "/sys/commands/request_id=") != NULL) {
        printf("[CMD_HANDLER] Confirmed command URC. Processing...\r\n");

        // --- Safely extract request_id ---
        const char* request_id_tag = "request_id=";
        char* request_id_start = strstr(urc_line, request_id_tag);
        if (!request_id_start) {
            return; 
        }
        
        request_id_start += strlen(request_id_tag);
        
        char* request_id_end = strchr(request_id_start, '"');
        if (!request_id_end) {
            return; // Malformed URC
        }

        int request_id_len = request_id_end - request_id_start;
        
        if (request_id_len <= 0 || request_id_len >= 48) {
            printf("[CMD_HANDLER] Error: Invalid request_id length: %d\r\n", request_id_len);
            return;
        }

        char request_id[48] = {0};
        strncpy(request_id, request_id_start, request_id_len);

        // --- Safely construct and queue the response command ---
        char* response_cmd = pvPortMalloc(256);
        if (response_cmd == NULL) {
            printf("[CMD_HANDLER] Error: Failed to allocate memory for response command.\r\n");
            return;
        }

        char response_topic[200];
        int topic_len = snprintf(response_topic, sizeof(response_topic), "$oc/devices/Gateway_1/sys/commands/response/request_id=%s", request_id);

        if (topic_len < 0 || topic_len >= sizeof(response_topic)) {
            printf("[CMD_HANDLER] Error: Response topic would be truncated.\r\n");
            vPortFree(response_cmd);
            return;
        }

        const char* response_payload = "{\\\"result_code\\\":0}";
        int payload_len = 17;
        int cmd_len = snprintf(response_cmd, 256, "AT+HMPUB=1,\"%s\",%d,\"%s\"", response_topic, payload_len, response_payload);

        if (cmd_len < 0 || cmd_len >= 256) {
            printf("[CMD_HANDLER] Error: Response AT command would be truncated.\r\n");
            vPortFree(response_cmd);
            return;
        }

        if (osMessageQueuePut(response_queue_handle, &response_cmd, 0U, 100) != osOK) {
            printf("[CMD_HANDLER] Error: Failed to queue response command. Freeing memory.\r\n");
            vPortFree(response_cmd);
        } else {
            printf("[STAGE 4] Command Queued via CommandHandler.\r\n");
        }
    }
}

// 一个示例函数，用于发送命令到队列
void send_response_to_queue(const char* command_string) {
    if (response_queue_handle != NULL) {
        // ... 构建命令结构体 ...
        // 假设 response_cmd 是一个指向动态分配内存的指针
        void* response_cmd = malloc(sizeof(char) * (strlen(command_string) + 1));
        if (response_cmd) {
            strcpy(response_cmd, command_string);
            // 使用 v2 API: osMessageQueuePut
            if (osMessageQueuePut(response_queue_handle, &response_cmd, 0U, 100) != osOK) {
                // 队列发送失败，释放内存
                free(response_cmd);
            }
        }
    }
} 
