/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32g4xx_it.c
 * @brief   Interrupt Service Routines.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32g4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pwm_control.h"
#include "oled.h"
#include "lora_protocol.h"
#include "settings.h"
#ifndef bool
#include <stdbool.h>
#endif
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
#define DEBOUNCE_DELAY_MS 300 // 防抖延迟时间（毫秒），根据需要调整
#ifndef VALUE_STEP
#define VALUE_STEP 10 // 如果未在其他地方定义，则定义速度调整的步长
#endif

// --- 外部变量 ---
// 这些变量应在其他文件中定义
extern uint8_t show_mode;                          // 显示模式
extern int set_item_id;                            // 当前选择的项目ID（使用int以便在检查时允许临时负值）
extern bool fan_status, pump_status, light_status; // 风扇、水泵、灯的状态
extern uint8_t fan_speed, pump_speed;              // 风扇和水泵的速度
extern LoRa myLoRa;
extern int lora_rx_tag;
extern uint16_t lora_frequency;
extern uint8_t device_id;

// --- 用于防抖的静态变量 ---
// 存储每个按键最后一次有效按下的时间戳
static uint32_t last_key1_press_time = 0;
static uint32_t last_key2_press_time = 0;
static uint32_t last_key3_press_time = 0;
static uint32_t last_key4_press_time = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern RNG_HandleTypeDef hrng;
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32G4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32g4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line0 interrupt.
  */
void EXTI0_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI0_IRQn 0 */

  /* USER CODE END EXTI0_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(Key1_Pin);
  /* USER CODE BEGIN EXTI0_IRQn 1 */

  /* USER CODE END EXTI0_IRQn 1 */
}

/**
  * @brief This function handles EXTI line1 interrupt.
  */
void EXTI1_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI1_IRQn 0 */

  /* USER CODE END EXTI1_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(Key2_Pin);
  /* USER CODE BEGIN EXTI1_IRQn 1 */

  /* USER CODE END EXTI1_IRQn 1 */
}

/**
  * @brief This function handles EXTI line2 interrupt.
  */
void EXTI2_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI2_IRQn 0 */

  /* USER CODE END EXTI2_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(Key3_Pin);
  /* USER CODE BEGIN EXTI2_IRQn 1 */

  /* USER CODE END EXTI2_IRQn 1 */
}

/**
  * @brief This function handles EXTI line3 interrupt.
  */
void EXTI3_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI3_IRQn 0 */

  /* USER CODE END EXTI3_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(Key4_Pin);
  /* USER CODE BEGIN EXTI3_IRQn 1 */

  /* USER CODE END EXTI3_IRQn 1 */
}

/**
  * @brief This function handles EXTI line[9:5] interrupts.
  */
void EXTI9_5_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI9_5_IRQn 0 */

  /* USER CODE END EXTI9_5_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(DIO0_Pin);
  /* USER CODE BEGIN EXTI9_5_IRQn 1 */

  /* USER CODE END EXTI9_5_IRQn 1 */
}

/**
  * @brief This function handles RNG global interrupt.
  */
void RNG_IRQHandler(void)
{
  /* USER CODE BEGIN RNG_IRQn 0 */

  /* USER CODE END RNG_IRQn 0 */
  HAL_RNG_IRQHandler(&hrng);
  /* USER CODE BEGIN RNG_IRQn 1 */

  /* USER CODE END RNG_IRQn 1 */
}

/* USER CODE BEGIN 1 */

static settings_t* p_settings;
/**
 * @brief  外部中断线检测回调函数。
 * @param  GPIO_Pin: 指定连接到相应 EXTI 线上的端口引脚。
 * @retval None
 */
   void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
   {
     uint32_t current_time = HAL_GetTick(); // 获取当前系统时间（毫秒）
     
     // --- 防抖检查和逻辑处理 ---
     switch (GPIO_Pin)
     {
     case Key1_Pin: // 按键1 处理
       // 检查自上次有效按下以来是否已过足够时间
       if ((current_time - last_key1_press_time) > DEBOUNCE_DELAY_MS)
       {
         last_key1_press_time = current_time; // 更新最后一次有效按下的时间
         
         // --- 按键1 的原始逻辑 ---
         switch (show_mode)
         {
         case 0:                     // 主控制模式
           p_settings = Settings_Get(); 
           fan_status = !fan_status; // 切换风扇状态
           // printf("FanSwitch:%d\r\n", fan_status);
           // 如果开启，立即应用速度；否则关闭PWM
           if (fan_status)
           {
             set_fan_speed(fan_speed);
           }
           else
           {
             set_fan_speed(0); // 假设速度为0表示关闭
           }
           controller_data_update();
           p_settings->fan_status = fan_status;
           if (Settings_Save() == W25QXX_ERROR) {
             printf("ERROR: Failed to save settings!\r\n");
           }
           break;
         case 1: // 项目选择模式
           // printf("按键1 按下 (选择上一个)\r\n");
           set_item_id--;
           if (set_item_id < 0)
           {                  // 循环选择
             set_item_id = 0; // 假设项目为 0 (风扇) 和 1 (水泵)
           }
           break;
         case 2: // 数值调整模式
           // printf("按键1 按下 (增加数值)\r\n");
           switch (set_item_id)
           {
           case 0: // 调整风扇速度
             if (fan_speed <= (100 - VALUE_STEP))
             {
               fan_speed += VALUE_STEP;
             }
             else
             {
               fan_speed = 100; // 限制最大值
             }
             // printf("风扇目标速度: %d\r\n", fan_speed);
             break;
           case 1: // 调整水泵速度
             if (pump_speed <= (100 - VALUE_STEP))
             {
               pump_speed += VALUE_STEP;
             }
             else
             {
               pump_speed = 100; // 限制最大值
             }
             // printf("水泵目标速度: %d\r\n", pump_speed);
             break;
           case 2: // 调整LoRa频率
             if (lora_frequency <= (525 - 1))
             {
               lora_frequency += 1;
             }
             else
             {
               lora_frequency = 525;
             }
             break;
           case 3: // 调整设备ID
             if (device_id <= (0xFF - 1))
             {
               device_id += 1;
             }
             else
             {
               device_id = 0xFF;
             }
             break;
           }
           break;
         }
       }
       break; // 结束 Key1_Pin 情况
       
     case Key2_Pin: // 按键2 处理
       // 防抖检查
       if ((current_time - last_key2_press_time) > DEBOUNCE_DELAY_MS)
       {
         last_key2_press_time = current_time; // 更新时间
         
         // --- 按键2 的原始逻辑 ---
         switch (show_mode)
         {
         case 0:                       // 主控制模式
           p_settings = Settings_Get(); 
           pump_status = !pump_status; // 切换水泵状态
           // printf("水泵开关:%d\r\n", pump_status);
           // 立即应用速度或关闭
           if (pump_status)
           {
             set_pump_speed(pump_speed);
           }
           else
           {
             set_pump_speed(0); // 假设速度为0表示关闭
           }
           controller_data_update();
           p_settings->pump_status = pump_status;
           if (Settings_Save() == W25QXX_ERROR) {
             printf("ERROR: Failed to save settings!\r\n");
           }
           break;
         case 1: // 项目选择模式
           // printf("按键2 按下 (退出选择模式)\r\n");
           OLED_Clear();    // 退出模式时清除屏幕
           show_mode = 0;   // 返回主模式
           set_item_id = 0; // 重置选择
           break;
         case 2: // 数值调整模式
           // printf("按键2 按下 (退出调整模式)\r\n");
           show_mode--; // 返回选择模式 (模式 1)
           // 在此模式下，数值不会在此处应用，仅在按下按键4时应用
           break;
         }
       }
       break; // 结束 Key2_Pin 情况
       
     case Key3_Pin: // 按键3 处理
       // 防抖检查
       if ((current_time - last_key3_press_time) > DEBOUNCE_DELAY_MS)
       {
         last_key3_press_time = current_time; // 更新时间
         
         // --- 按键3 的原始逻辑 ---
         switch (show_mode)
         {
         case 0: // 主控制模式
           p_settings = Settings_Get(); 
           // printf("按键3 按下 (切换灯光)\r\n");
           light_status = !light_status; // 切换灯光状态
           // 使用 GPIO_PIN_SET 和 GPIO_PIN_RESET 以提高清晰度
           HAL_GPIO_WritePin(LIGHT_PWR_CTRL_GPIO_Port, LIGHT_PWR_CTRL_Pin, light_status ? GPIO_PIN_SET : GPIO_PIN_RESET);
           controller_data_update();
           p_settings->light_status = light_status;
           if (Settings_Save() == W25QXX_ERROR) {
             printf("ERROR: Failed to save settings!\r\n");
           }
           // printf("灯光状态:%d\r\n", light_status);
           break;
         case 1: // 项目选择模式
           // printf("按键3 按下 (选择下一个)\r\n");
           set_item_id++;
           if (set_item_id > 3)
           {                  // 循环选择
             set_item_id = 3; // 假设项目为 0 (风扇) 和 1 (水泵)
           }
           break;
         case 2: // 数值调整模式
           // printf("按键3 按下 (减少数值)\r\n");
           switch (set_item_id)
           {
           case 0: // 调整风扇速度
             if (fan_speed >= (0 + VALUE_STEP))
             { // 使用 0 作为最小参考值
               fan_speed -= VALUE_STEP;
             }
             else
             {
               fan_speed = 0; // 限制最小值（如果0表示关闭，则为0；否则为1）
             }
             // 原始代码: if(fan_speed<100) fan_speed = 1; - 这可能是错误的逻辑
             // 修正逻辑: 限制在最小值 (例如 0 或 1)
             if (fan_speed < 1 && fan_speed != 0)
             { // 如果希望最小运行速度为 1
               fan_speed = 1;
             }
             // printf("风扇目标速度: %d\r\n", fan_speed);
             break;
           case 1: // 调整水泵速度
             if (pump_speed >= (0 + VALUE_STEP))
             {
               pump_speed -= VALUE_STEP;
             }
             else
             {
               pump_speed = 0; // 限制最小值
             }
             // 修正逻辑: 限制在最小值 (例如 0 或 1)
             if (pump_speed < 1 && pump_speed != 0)
             { // 如果希望最小运行速度为 1
               pump_speed = 1;
             }
             // printf("水泵目标速度: %d\r\n", pump_speed);
             break;
           case 2: // 调整LoRa频率
             if (lora_frequency >= (410 + 1))
             {
               lora_frequency -= 1;
             }
             else
             {
               lora_frequency = 410;
             }
             break;
           case 3: // 调整设备ID
             if(device_id >= (0x00 + 1))
             {
               device_id -= 1;
             }
             else
             {
               device_id = 0x00;
             }
             break;
           }
           break;
         }
       }
       break; // 结束 Key3_Pin 情况
       
     case Key4_Pin: // 按键4 处理
       // 防抖检查
       if ((current_time - last_key4_press_time) > DEBOUNCE_DELAY_MS)
       {
         last_key4_press_time = current_time; // 更新时间
         
         // --- 按键4 的原始逻辑 ---
         switch (show_mode)
         {
         case 0: // 主控制模式 -> 进入选择模式
           // printf("按键4 按下 (进入选择模式)\r\n");
           OLED_Clear();    // 为新模式显示清除屏幕
           show_mode = 1;   // 移动到项目选择模式
           set_item_id = 0; // 默认选择第一个项目
           break;
         case 1: // 项目选择模式 -> 进入调整模式
           // printf("按键4 按下 (进入调整模式)\r\n");
           show_mode = 2; // 移动到数值调整模式
           // 保留当前的 set_item_id
           break;
         case 2: // 数值调整模式 -> 应用数值
           // printf("按键4 按下 (应用数值)\r\n");
           p_settings = Settings_Get(); 
           switch (set_item_id)
           {
           case 0: // 应用风扇速度
             if (fan_status)
             { // 仅当风扇开启时应用
               set_fan_speed(fan_speed);
               // printf("已应用风扇速度: %d\r\n", fan_speed);
             }
             else
             {
               // printf("风扇已关闭，速度 %d 未应用。\r\n", fan_speed);
             }
             controller_data_update();
             p_settings->fan_speed = fan_speed;
             if (Settings_Save() == W25QXX_ERROR) {
               printf("ERROR: Failed to save settings!\r\n");
             }
             break;
           case 1: // 应用水泵速度
             if (pump_status)
             { // 仅当水泵开启时应用
               set_pump_speed(pump_speed);
               // printf("已应用水泵速度: %d\r\n", pump_speed);
             }
             else
             {
               // printf("水泵已关闭，速度 %d 未应用。\r\n", pump_speed);
             }
             controller_data_update();
             p_settings->pump_speed = pump_speed;
             if (Settings_Save() == W25QXX_ERROR) {
               printf("ERROR: Failed to save settings!\r\n");
             }
             break;
           case 2:
             p_settings->lora_frequency = lora_frequency;
             if (Settings_Save() == W25QXX_OK) {
               HAL_NVIC_SystemReset();
             } else {
               printf("ERROR: Failed to save settings!\r\n");
             }
             break;
           case 3:
             p_settings->device_id = lora_frequency;
             if (Settings_Save() == W25QXX_ERROR) 
               printf("ERROR: Failed to save settings!\r\n");
             break;
           }
           break;
         }
       }
       break; // 结束 Key4_Pin 情况
     case DIO0_Pin:
       lora_rx_tag = 1;
       break;
     default:
       
       break;
     }
   }
/* USER CODE END 1 */
