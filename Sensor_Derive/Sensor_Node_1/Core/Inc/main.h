/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define PWR_Pin GPIO_PIN_0
#define PWR_GPIO_Port GPIOF
#define BATVOL_CTRL_Pin GPIO_PIN_1
#define BATVOL_CTRL_GPIO_Port GPIOF
#define CHRG_PIN_Pin GPIO_PIN_2
#define CHRG_PIN_GPIO_Port GPIOF
#define Key_Pin GPIO_PIN_0
#define Key_GPIO_Port GPIOA
#define Key_EXTI_IRQn EXTI0_1_IRQn
#define RS485_CTRL_Pin GPIO_PIN_1
#define RS485_CTRL_GPIO_Port GPIOA
#define BATVOL_PIN_Pin GPIO_PIN_4
#define BATVOL_PIN_GPIO_Port GPIOA
#define NSS_Pin GPIO_PIN_0
#define NSS_GPIO_Port GPIOB
#define RES_Pin GPIO_PIN_1
#define RES_GPIO_Port GPIOB
#define DIO0_Pin GPIO_PIN_2
#define DIO0_GPIO_Port GPIOB
#define DIO0_EXTI_IRQn EXTI2_3_IRQn
#define FLASH_CS_Pin GPIO_PIN_12
#define FLASH_CS_GPIO_Port GPIOB
#define DEV_PWR_CTRL_Pin GPIO_PIN_8
#define DEV_PWR_CTRL_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
