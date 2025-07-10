#ifndef __STATE_MANAGER_H
#define __STATE_MANAGER_H

/**
 * @brief Defines the main operational states of the device.
 */
typedef enum {
    STATE_NORMAL_OPERATION,  // Normal mode: cycling between data collection and sleep
    STATE_CONFIGURATION     // Configuration mode: stays awake, waiting for commands
} SystemState_t;


#endif // __STATE_MANAGER_H 