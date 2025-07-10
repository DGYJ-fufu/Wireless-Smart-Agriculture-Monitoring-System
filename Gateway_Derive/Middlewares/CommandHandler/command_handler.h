#ifndef __COMMAND_HANDLER_H
#define __COMMAND_HANDLER_H

#include "cmsis_os2.h" // For osMessageQueueId_t

/**
 * @brief Initializes the Command Handler module.
 * @param queue_handle The handle to the message queue for sending response commands.
 */
void CommandHandler_Init(osMessageQueueId_t queue_handle);

/**
 * @brief Processes a URC line to handle cloud commands.
 *        This function will parse the line, and if it's a known command,
 *        it will construct and queue a response.
 * @param urc_line The URC string received from the AT module.
 */
void CommandHandler_ProcessUrc(const char* urc_line);

#endif // __COMMAND_HANDLER_H 
