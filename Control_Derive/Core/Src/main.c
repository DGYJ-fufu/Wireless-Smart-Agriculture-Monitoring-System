/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <LowLevelIOInterface.h>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "pwm_control.h"
#include "oled.h"
#include "lora_protocol.h"
#include "w25qxx.h"
#include "settings.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

I2C_HandleTypeDef hi2c1;

QSPI_HandleTypeDef hqspi1;

RNG_HandleTypeDef hrng;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
uint8_t show_mode = 0;
int set_item_id = 0;
bool fan_status = false,pump_status = false,light_status = false;
uint8_t fan_speed = 100,pump_speed = 100;
static char oled_show_buffer[8];

LoRa myLoRa;
static lora_parsed_message_t lora_msg;
static uint8_t received_data[LORA_MAX_RAW_PACKET];
int lora_rx_tag = 0;

uint16_t lora_frequency = 433;
uint8_t device_id = 0x12;

int page_code = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_I2C1_Init(void);
static void MX_QUADSPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_RNG_Init(void);
/* USER CODE BEGIN PFP */
size_t __write(int handle, const unsigned char *buffer, size_t size) {
  if (buffer == 0) {
    return 0;
  }
  if (handle != _LLIO_STDOUT && handle != _LLIO_STDERR) {
    return _LLIO_ERROR;
  }
  HAL_UART_Transmit(&huart1, (uint8_t *)buffer, size, HAL_MAX_DELAY);
  return size;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_I2C1_Init();
  MX_QUADSPI1_Init();
  MX_SPI2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_RNG_Init();
  /* USER CODE BEGIN 2 */
  if (Settings_Init() != W25QXX_OK) {
    printf("FATAL: Settings module failed to initialize!\r\n");
    Error_Handler();
  }

  settings_t* p_settings = Settings_Get();

  fan_speed = p_settings->fan_speed;
  pump_speed = p_settings->pump_speed;
  fan_status = p_settings->fan_status;
  pump_status = p_settings->pump_status;
  light_status = p_settings->light_status;
  lora_frequency = p_settings->lora_frequency;
  device_id = p_settings->device_id;
  
  myLoRa = newLoRa();
  myLoRa.CS_port = NSS_GPIO_Port;
  myLoRa.CS_pin = NSS_Pin;
  myLoRa.reset_port = RST_GPIO_Port;
  myLoRa.reset_pin = RST_Pin;
  myLoRa.hSPIx = &hspi2;
  myLoRa.frequency = lora_frequency;
  uint16_t LoRa_status = LoRa_init(&myLoRa);
  
  char send_data[200];
  memset(send_data, NULL, 200);
  if (LoRa_status == LORA_OK)
  {
    snprintf(send_data, sizeof(send_data), "\n\r LoRa is running... :) \n\r");
    HAL_UART_Transmit(&huart1, (uint8_t *)send_data, 200, 200);
  }
  else
  {
    snprintf(send_data, sizeof(send_data), "\n\r LoRa failed :( \n\r Error code: %d \n\r", LoRa_status);
    HAL_UART_Transmit(&huart1, (uint8_t *)send_data, 200, 200);
  }
  PWM_Init();
  
  if(fan_status) set_fan_speed(fan_speed); else set_fan_speed(0);
  if(pump_status) set_pump_speed(pump_speed); else set_pump_speed(0);
  HAL_GPIO_WritePin(LIGHT_PWR_CTRL_GPIO_Port, LIGHT_PWR_CTRL_Pin, light_status ? GPIO_PIN_SET : GPIO_PIN_RESET);
  
  controller_data_update();
  OLED_Init();
  OLED_Display_On();
  OLED_Clear();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (lora_rx_tag)
    {
      uint8_t packet_size = 0;
      packet_size = LoRa_receive(&myLoRa, received_data, sizeof(received_data));
      if (packet_size > 0)
      {
        parse_lora_frame(received_data, packet_size, &lora_msg);
        printf("[LoRa CMD] Sent %d bytes (Seq: %u): ", packet_size, (unsigned int)lora_msg.seq_num);
        for (int i = 0; i < packet_size; i++)
        {
          printf("%02X ", received_data[i]);
        }
        printf("\r\n");
        
        if (lora_msg.target_addr ==  device_id && lora_msg.sender_addr == LORA_HOST_ADDRESS && lora_msg.msg_type == MSG_TYPE_CMD_SET_CONFIG)
        {
          switch (lora_msg.payload[0])
          {
          case CONTROLLER_DEVICE_TYPE_STATUS_FAN:
            if(lora_msg.payload[1])
              set_fan_speed(fan_speed);
            else
              set_fan_speed(0);
            fan_status = lora_msg.payload[1];
            controller_data_update();
            break;
          case CONTROLLER_DEVICE_TYPE_SPEED_FAN:
            fan_speed = lora_model_unpack_u8(&lora_msg.payload[1]);
            set_fan_speed(fan_speed);
            controller_data_update();
            break;
          case CONTROLLER_DEVICE_TYPE_STATUS_PUMP:
            if(lora_msg.payload[1])
              set_pump_speed(pump_speed);
            else
              set_pump_speed(0);
            pump_status = lora_msg.payload[1];
            controller_data_update();
            break;
          case CONTROLLER_DEVICE_TYPE_SPEED_PUMP:
            pump_speed = lora_model_unpack_u8(&lora_msg.payload[1]);
            set_pump_speed(pump_speed);
            controller_data_update();
            break;
          case CONTROLLER_DEVICE_TYPE_STATUS_LIGHT:
            if(lora_msg.payload[1])
              HAL_GPIO_WritePin(LIGHT_PWR_CTRL_GPIO_Port,LIGHT_PWR_CTRL_Pin,GPIO_PIN_SET);
            else
              HAL_GPIO_WritePin(LIGHT_PWR_CTRL_GPIO_Port,LIGHT_PWR_CTRL_Pin,GPIO_PIN_RESET);
            light_status = lora_msg.payload[1];
            controller_data_update();
            break;
          }
        }
      }
      lora_rx_tag = 0;
    }
    
    if(set_item_id==3)
      page_code=1;
    else
      page_code=0;
    
    uint8_t page[2][6] = {
      {5,6,7,8,26,27},
      {7,8,26,27,30,31}
    };
    
    switch(show_mode){
    case 0:
      OLED_ShowCHinese(64-16*2,0,1,0);
      OLED_ShowCHinese(64-16*1,0,2,0);
      OLED_ShowCHinese(64,0,3,0);
      OLED_ShowCHinese(64+16,0,4,0);
      
      OLED_ShowCHinese(0,2,5,0);
      OLED_ShowCHinese(16,2,6,0);
      OLED_ShowChar(32,2,':',16,0);
      if(fan_status){
        if(fan_status)
          set_fan_speed(fan_speed);
        else
          set_fan_speed(0);
        OLED_ShowCHinese(40,2,11,0);
        OLED_ShowCHinese(40+16,2,12,0);
        sprintf(oled_show_buffer,"%d%%  ",fan_speed);
        OLED_ShowString(40+16+16+8,2,oled_show_buffer,16,0);
        memset(oled_show_buffer,0,sizeof(oled_show_buffer));
      }else{
        OLED_ShowCHinese(40,2,13,0);
        OLED_ShowCHinese(40+16,2,14,0);
        OLED_ShowString(40+16+16+8,2,"      ",16,0);
      }
      
      OLED_ShowCHinese(0,4,7,0);
      OLED_ShowCHinese(16,4,8,0);
      OLED_ShowChar(32,4,':',16,0);
      if(pump_status){
        OLED_ShowCHinese(40,4,11,0);
        OLED_ShowCHinese(40+16,4,12,0);
        sprintf(oled_show_buffer,"%d%%  ",pump_speed);
        OLED_ShowString(40+16+16+8,4,oled_show_buffer,16,0);
        memset(oled_show_buffer,0,sizeof(oled_show_buffer));
      }else{
        OLED_ShowCHinese(40,4,13,0);
        OLED_ShowCHinese(40+16,4,14,0);
        OLED_ShowString(40+16+16+8,4,"      ",16,0);
      }
      
      OLED_ShowCHinese(0,6,9,0);
      OLED_ShowCHinese(16,6,10,0);
      OLED_ShowChar(32,6,':',16,0);
      if(light_status){
        OLED_ShowCHinese(40,6,11,0);
        OLED_ShowCHinese(40+16,6,12,0);
      }else{
        OLED_ShowCHinese(40,6,13,0);
        OLED_ShowCHinese(40+16,6,14,0);
      }
      break;
    case 1:
      switch(set_item_id){
      case 0:
        OLED_ShowString(40+8*5,4,"    ",16,0);
        OLED_ShowString(40+8*5,6,"    ",16,0);
        OLED_ShowCHinese(40+8*5,2,21,0);
        break;
      case 1:
        OLED_ShowString(40+8*5,2,"    ",16,0);
        OLED_ShowString(40+8*5,6,"    ",16,0);
        OLED_ShowCHinese(40+8*5,4,21,0);
        break;
      case 2:
        OLED_ShowString(40+8*5,2,"    ",16,0);
        OLED_ShowString(40+8*5,4,"    ",16,0);
        OLED_ShowCHinese(40+8*5,6,21,0);
        break;
      case 3:
        OLED_ShowString(40+8*5,2,"    ",16,0);
        OLED_ShowString(40+8*5,4,"    ",16,0);
        OLED_ShowCHinese(40+8*5,6,21,0);
        break;
      }
      OLED_ShowCHinese(64-16*2,0,17,0);
      OLED_ShowCHinese(64-16*1,0,18,0);
      OLED_ShowCHinese(64,0,19,0);
      OLED_ShowCHinese(64+16,0,20,0);
      
      OLED_ShowCHinese(0,2,page[page_code][0],0);
      OLED_ShowCHinese(16,2,page[page_code][1],0);
      OLED_ShowChar(32,2,':',16,0);
      if(!page_code){
        OLED_ShowNum(40,2,fan_speed,3,16,0);
        OLED_ShowChar(40+8*3,2,'%',16,0);
      }else{
        OLED_ShowNum(40,2,pump_speed,3,16,0);
        OLED_ShowChar(40+8*3,2,'%',16,0);
      }
      
      OLED_ShowCHinese(0,4,page[page_code][2],0);
      OLED_ShowCHinese(16,4,page[page_code][3],0);
      OLED_ShowChar(32,4,':',16,0);
      if(!page_code){
        OLED_ShowNum(40,4,pump_speed,3,16,0);
        OLED_ShowChar(40+8*3,4,'%',16,0);
      }else{
        OLED_ShowNum(40+8,4,lora_frequency,3,16,0);
      }
      
      OLED_ShowCHinese(0,6,page[page_code][4],0);
      OLED_ShowCHinese(16,6,page[page_code][5],0);
      OLED_ShowChar(32,6,':',16,0);
      OLED_ShowChar(32+8,6,' ',16,0);
      if(!page_code){
        OLED_ShowNum(40+8,6,lora_frequency,3,16,0);
      }else{
        OLED_ShowNum(40+8,6,device_id,3,16,0);
      }
      break;
    case 2:
      switch(set_item_id){
      case 0:
        if(!page_code)
          OLED_ShowNum(40,2,fan_speed,3,16,0);
        else
          OLED_ShowNum(40,2,pump_speed,3,16,0);
        OLED_ShowString(40+8*5,2,"    ",16,0);
        OLED_ShowCHinese(40+8*5,2,21,0);
        break;
      case 1:
        if(!page_code)
          OLED_ShowNum(40,4,pump_speed,3,16,0);
        else
          OLED_ShowNum(40+8,4,lora_frequency,3,16,0);
        OLED_ShowString(40+8*5,4,"    ",16,0);
        OLED_ShowCHinese(40+8*5,4,21,0);
        break;
      case 2:
        if(!page_code)
          OLED_ShowNum(40+8,6,lora_frequency,3,16,0);
        else
          OLED_ShowNum(40+8,6,device_id,3,16,0);
        OLED_ShowString(40+8*5,6,"    ",16,0);
        OLED_ShowCHinese(40+8*5,6,21,0);
        break;
      case 3:
        OLED_ShowNum(40+8,6,device_id,3,16,0);
        OLED_ShowString(40+8*5,6,"    ",16,0);
        OLED_ShowCHinese(40+8*5,6,21,0);
        break;
      }
      break;
    }
    HAL_Delay(50);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_DISABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_DISABLE;
  hcrc.Init.GeneratingPolynomial = 32773;
  hcrc.Init.CRCLength = CRC_POLYLENGTH_16B;
  hcrc.Init.InitValue = 0xFFFF;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_BYTE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_ENABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x4052060F;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }

  /** I2C Fast mode Plus enable
  */
  HAL_I2CEx_EnableFastModePlus(I2C_FASTMODEPLUS_I2C1);
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief QUADSPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_QUADSPI1_Init(void)
{

  /* USER CODE BEGIN QUADSPI1_Init 0 */

  /* USER CODE END QUADSPI1_Init 0 */

  /* USER CODE BEGIN QUADSPI1_Init 1 */

  /* USER CODE END QUADSPI1_Init 1 */
  /* QUADSPI1 parameter configuration*/
  hqspi1.Instance = QUADSPI;
  hqspi1.Init.ClockPrescaler = 1;
  hqspi1.Init.FifoThreshold = 4;
  hqspi1.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
  hqspi1.Init.FlashSize = 22;
  hqspi1.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_2_CYCLE;
  hqspi1.Init.ClockMode = QSPI_CLOCK_MODE_0;
  hqspi1.Init.FlashID = QSPI_FLASH_ID_1;
  hqspi1.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
  if (HAL_QSPI_Init(&hqspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN QUADSPI1_Init 2 */

  /* USER CODE END QUADSPI1_Init 2 */

}

/**
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  hrng.Init.ClockErrorDetection = RNG_CED_ENABLE;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 170-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 99;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, NSS_Pin|RST_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LIGHT_PWR_CTRL_GPIO_Port, LIGHT_PWR_CTRL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : Key1_Pin Key2_Pin Key3_Pin Key4_Pin */
  GPIO_InitStruct.Pin = Key1_Pin|Key2_Pin|Key3_Pin|Key4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : NSS_Pin RST_Pin */
  GPIO_InitStruct.Pin = NSS_Pin|RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : DIO0_Pin */
  GPIO_InitStruct.Pin = DIO0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(DIO0_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LIGHT_PWR_CTRL_Pin */
  GPIO_InitStruct.Pin = LIGHT_PWR_CTRL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LIGHT_PWR_CTRL_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 15, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 15, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 15, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 15, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 10, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
