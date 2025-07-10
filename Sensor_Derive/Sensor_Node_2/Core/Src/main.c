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
#include "adc.h"
#include "crc.h"
#include "dma.h"
#include "i2c.h"
#include "usart.h"
#include "rtc.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "stdint.h"
#include "stdlib.h"
#include "LoRa.h"
#include "bh1750.h"
#include "bmp280.h"
#include "gps.h"
#include "w25qxx.h"
#include "sht40.h"
#include "device_properties.h"
#include "lora_protocol.h"
#include "battery.h"
#include "config_manager.h"
#include "key_handler.h"
#include "state_manager.h"
#include "cli_manager.h"
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

/* USER CODE BEGIN PV */
// LoRa配置及发送缓冲区
static LoRa myLoRa;
static uint8_t lora_send_buffer[35];
// 传感器数据结构体 (volatile确保在中断和主循环间安全访问)
static volatile ExternalSensorProperties_t sensor_data;

// -- 系统状态机变量 --
static volatile SystemState_t g_SystemState = STATE_NORMAL_OPERATION;

// -- 低功耗工作循环相关变量 --
static uint8_t lora_transmission_count = 0;  // 当前工作周期内已发送的次数
static uint32_t last_transmission_time = 0; // 上次发送的时间戳
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/** @brief 初始化所有应用层和驱动层外设 */
void Peripherals_Init(void);
/** @brief 反初始化所有应用层和驱动层外设，为进入低功耗模式做准备 */
void Peripherals_DeInit(void);
/** @brief 执行一次完整的数据采集、打包和LoRa发送流程 */
void Perform_Sensor_Transmission(void);

// --- 按键事件的回调函数 ---
void on_key_long_press(void);
void on_key_short_press(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void print_hex(char *buffer, int len){
	int i;
	printf("******************start code**********************************\n");
	for(i = 1; i <= len; i++){
		printf("0x%02X ",buffer[i-1]);					
		if(i % 16 == 0){
			printf("\n");
		}
	}
	printf("\n");
	printf("********************end code************************************\n");
}

/**
 * @brief 长按按键的回调函数实现
 */
void on_key_long_press(void)
{
    // 仅在正常工作模式下响应，防止在配置模式下重复进入
    if (g_SystemState == STATE_NORMAL_OPERATION)
    {
        g_SystemState = STATE_CONFIGURATION;
        GPS_Pause(); // 暂停GPS处理以节省CPU资源
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET); // 打开LED作为状态指示
        printf("\r\n--- 按键长按: 进入配置模式 ---\r\n");
    }
}

/**
 * @brief 短按按键的回调函数实现
 */
void on_key_short_press(void)
{
    // 仅在配置模式下响应，用于退出配置模式
    if (g_SystemState == STATE_CONFIGURATION)
    {
        g_SystemState = STATE_NORMAL_OPERATION;
        GPS_Resume(); // 恢复GPS处理
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); // 关闭LED
        printf("\r\n--- 按键短按: 退出配置模式，恢复正常运行 ---\r\n");
    }
    else
    {
        // 在正常模式下，短按主要用于从STOP模式唤醒MCU。
        // 具体的唤醒逻辑由EXTI中断本身处理，这里可以定义其他交互功能。
        printf("\r\n--- 按键短按: (正常模式下) 可在此定义其他功能 ---\r\n");
    }
}
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
  // --- Check and handle wakeup from Stop mode ---
  if (__HAL_PWR_GET_FLAG(PWR_FLAG_WUF2) != RESET)
  {
    // Clear the RTC Wakeup flag
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF2);
    // Note: WUF1 is for the key press, we let its handler run normally.
  }

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_I2C3_Init();
  MX_SPI2_Init();
  MX_CRC_Init();
  MX_LPUART1_UART_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
  // 在启动时停用RTC唤醒定时器，因为它可能在复位后默认开启。
  // 我们只在进入休眠模式前才需要它。
  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

  // 在系统启动时，仅执行一次按键处理模块的初始化。
  Key_Init();
  Key_Register_Callbacks(on_key_short_press, on_key_long_press);

  Peripherals_Init();
	
	printf("系统启动!\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // 处理按键事件（主要负责检测长按）
    Key_Process();

    // --- 状态机逻辑 ---
    if (g_SystemState == STATE_NORMAL_OPERATION)
    {
        // 检查是否到了下一次发送的时间点。第一次(count=0)立即发送。
        if (lora_transmission_count < 4 && 
           (lora_transmission_count == 0 || HAL_GetTick() - last_transmission_time >= 5000))
        {
            Perform_Sensor_Transmission();
            lora_transmission_count++;
            last_transmission_time = HAL_GetTick(); // 发送后重置时间戳
        }

        // 如果当前周期的所有发送任务都已完成，则准备休眠
        if (lora_transmission_count >= 4)
        {
            printf("\r\n--- 工作周期完成，准备进入STOP 2模式... ---\r\n");
            Peripherals_DeInit(); // 关闭外设以降低功耗

            // 短暂延时，确保printf能完成输出
            HAL_Delay(100);

            // 挂起SysTick，防止SysTick中断唤醒MCU
            HAL_SuspendTick();

            // 设置RTC唤醒定时器，60秒后唤醒
            if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 59, RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 0) != HAL_OK)
            {
                Error_Handler();
            }
            printf("RTC唤醒定时器已设置为60秒。\r\n");

            // 进入STOP 2模式，等待中断（WFI）
            HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

            // --- MCU唤醒点 ---
            // 代码将在此处继续执行...

            // 从STOP模式唤醒后，系统时钟会重置为MSI，必须重新配置
            SystemClock_Config();
            // 恢复SysTick
            HAL_ResumeTick(); 
            printf("\r\n--- 已从STOP 2模式唤醒 ---\r\n");
            
            // 关键修复：从深度睡眠唤醒后，必须重新初始化所有HAL库级别的外设。
            // 因为在STOP模式下，它们大部分的时钟和电源都被关闭了。
            MX_GPIO_Init();
            MX_DMA_Init();
            MX_SPI1_Init();
            MX_USART1_UART_Init();
            MX_ADC1_Init();
            MX_I2C1_Init();
            MX_I2C2_Init();
            MX_I2C3_Init();
            MX_SPI2_Init();
            MX_CRC_Init();
            MX_LPUART1_UART_Init();
            // 注意：RTC在整个过程中持续运行，无需重新初始化。

            // 重置工作循环计数器，并重新初始化应用层驱动，准备开始新的工作周期
            lora_transmission_count = 0;
            Peripherals_Init();
        }
    }
    else if (g_SystemState == STATE_CONFIGURATION)
    {
        // 在配置模式下，循环处理CLI指令
        CLI_Process();
    }

    /* USER CODE END 3 */
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 14;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void Peripherals_DeInit(void)
{
    printf("De-initializing peripherals...\r\n");

    // De-init HAL drivers to save power and prevent bus issues
    HAL_SPI_DeInit(&hspi1);
    HAL_SPI_DeInit(&hspi2);
    HAL_I2C_DeInit(&hi2c1);
    HAL_I2C_DeInit(&hi2c2);
    HAL_I2C_DeInit(&hi2c3);
    HAL_ADC_DeInit(&hadc1);
    HAL_UART_DeInit(&hlpuart1);
    
    // Turn off peripheral power
    HAL_GPIO_WritePin(DEV_PWR_CTRL_GPIO_Port, DEV_PWR_CTRL_Pin, GPIO_PIN_RESET);
    printf("Peripheral power OFF.\r\n");
}

void Peripherals_Init(void)
{
  // This function centralizes all peripheral initializations
  // that need to be re-done after waking from certain low-power modes.

  // NOTE: Key handler is initialized only once at startup, not here.

  if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  
  printf("Drivers power init start...\r\n");
  HAL_GPIO_WritePin(DEV_PWR_CTRL_GPIO_Port, DEV_PWR_CTRL_Pin, GPIO_PIN_SET);
  HAL_Delay(500); // Give peripherals time to power up
  printf("Drivers power init ok!\r\n");

	if(Init_BH1750()){
		printf("bh1750 init ok!\r\n");
	}else{
		printf("bh1750 init err!\r\n");
	}
	
	if(!BMP280_Init()){
		printf("bmp280 init ok!\r\n");
	}else{
		printf("bmp280 init err!\r\n");
	}
	
	GPS_Init(&hlpuart1);
	printf("gp-02 init ok!\r\n");

	Battery_Init();
	
	if(!W25QXX_Init()){
		printf("W25QXX Flash init OK!\r\n");

    if (Config_Load()) {
        printf("Configuration loaded successfully from Flash.\r\n");
    } else {
        printf("No valid config found in Flash. Saving default values.\r\n");
        if (!Config_Save()) {
            printf("Error: Failed to save default configuration!\r\n");
            Error_Handler();
        }
    }
	}else{
		printf("W25QXX Flash init ERROR!\r\n");
    Config_SetDefault();
    printf("Using default configuration as Flash is not available.\r\n");
	}
	
  printf("----------------------------------------\r\n");
  printf("--- Device Configuration ---\r\n");
  printf("   Device ID:      0x%X\r\n", g_DeviceConfig.device_id);
  printf("   LoRa Frequency: %u MHz\r\n", g_DeviceConfig.lora_frequency);
  printf("----------------------------------------\r\n\r\n");
	
	myLoRa = newLoRa();
  myLoRa.CS_port = NSS_GPIO_Port;
  myLoRa.CS_pin = NSS_Pin;
  myLoRa.reset_port = RES_GPIO_Port;
  myLoRa.reset_pin = RES_Pin;
  myLoRa.DIO0_port = DIO0_GPIO_Port;
  myLoRa.DIO0_pin = DIO0_Pin;
  myLoRa.hSPIx = &hspi1;

  myLoRa.frequency = g_DeviceConfig.lora_frequency;

  uint16_t LoRa_status = LoRa_init(&myLoRa);
  if (LoRa_status == LORA_OK)
  {
    printf("lora init ok!\r\n");
  }
  else
  {
    printf("lora init err!\r\n");
  }
	
	HAL_Delay(2000);
}

void Perform_Sensor_Transmission(void)
{
    printf("\r\n--- Sensor Data Report (%d/4) ---\r\n", lora_transmission_count + 1);

    BMP280_t bmp280_data;
    BMP280_ReadData(&bmp280_data);
    sensor_data.airPressure = bmp280_data.pressure;
    SHT40_Read_RHData((double *)&sensor_data.outdoorTemperature,(double *)&sensor_data.outdoorHumidity);
    BH1750_GetDate((uint16_t *)&sensor_data.outdoorLightIntensity);
    
    sensor_data.common.batteryVoltage = Battery_GetVoltage();
    sensor_data.common.batteryLevel = Battery_GetPercentage(sensor_data.common.batteryVoltage);

    if (gps_data.new_data_flag)
    {
        gps_data.new_data_flag = 0;
        if (gps_data.status == 'A')
        {
          sensor_data.altitude = gps_data.altitude;
          format_location_string(gps_data.latitude, gps_data.lat_indicator, 
                   gps_data.longitude, gps_data.lon_indicator,(char *)&sensor_data.location,sizeof(sensor_data.location));
        }
    }
  
    printf("  Temperature:      %.2f C\r\n", sensor_data.outdoorTemperature);
    printf("  Humidity:         %.2f %%\r\n", sensor_data.outdoorHumidity);
    printf("  Light Intensity:  %d lux\r\n", sensor_data.outdoorLightIntensity);
    printf("  Air Pressure:     %.2f\r\n", sensor_data.airPressure);
    printf("  GPS Altitude:     %.1f m\r\n", sensor_data.altitude);
    printf("  GPS Location:     %s\r\n", sensor_data.location);
    printf("  Battery:          %u %% (%.2f V)\r\n", sensor_data.common.batteryLevel, sensor_data.common.batteryVoltage);
    printf("--------------------------\r\n");
  
    sensor_data_payload_t sensor_lora_payload;
    if(lora_model_create_sensor_payload((const ExternalSensorProperties_t *)&sensor_data,&sensor_lora_payload)){
      int lora_data_len = generate_lora_frame(LORA_HOST_ADDRESS,g_DeviceConfig.device_id,MSG_TYPE_REPORT_SENSOR,0,(const uint8_t*)&sensor_lora_payload,sizeof(sensor_lora_payload),lora_send_buffer,sizeof(lora_send_buffer));
      printf("lora_data_len:%d\r\n",lora_data_len);
      print_hex((char *)lora_send_buffer, lora_data_len);
      printf("lora send status:%d\r\n",LoRa_transmit(&myLoRa,lora_send_buffer,lora_data_len,3000));
    }

    if (lora_transmission_count < 3)
    {
        printf("--- Waiting 5 seconds for next transmission... ---\r\n");
    }
}
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
	HAL_NVIC_SystemReset();
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
