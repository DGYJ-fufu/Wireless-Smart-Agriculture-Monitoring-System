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
#include "stdbool.h"
#include "stdio.h"
#include "string.h"
#include "stdint.h"
#include "stdlib.h"
#include "LoRa.h"
#include "bh1750.h"
#include "sgp30.h"
#include "sht40.h"
#include "w25qxx.h"
#include "sp3485.h"
#include "battery.h"
#include "device_properties.h"
#include "lora_protocol.h"
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
// LoRaé

static LoRa myLoRa;
static uint8_t lora_send_buffer[45];
// äź ćĺ¨ć°ćŽçťćä˝ (volatileçĄŽäżĺ¨ä¸­ć­ĺä¸ťĺžŞçŻé´ĺŽĺ
static volatile InternalSensorProperties_t sensor_data;

// -- çłťçťçśććşĺé --
static volatile SystemState_t g_SystemState = STATE_NORMAL_OPERATION;

// -- ä˝ĺčĺˇĽä˝ĺžŞçŻç¸ĺ
static uint32_t last_transmission_time = 0; // ä¸ćŹĄĺéçćśé´ćł
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/** @brief ĺĺ§ĺććĺşç¨ĺąĺéŠąĺ¨ĺąĺ¤čŽž */
void Peripherals_Init(void);
/** @brief ĺĺĺ§ĺććĺşç¨ĺąĺéŠąĺ¨ĺąĺ¤čŽžďźä¸şčżĺ
Ľä˝ĺčć¨Ąĺźĺĺĺ¤ */
void Peripherals_DeInit(void);
/** @brief ć§čĄä¸ćŹĄĺŽć´çć°ćŽééăćĺ
ĺLoRaĺéćľç¨ */
void Perform_Sensor_Transmission(void);

// --- ćéŽäşäťśçĺč°ĺ˝ć° ---
void on_key_long_press(void);
void on_key_short_press(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void print_hex(char *buffer, int len)
{
  int i;
  printf("******************start code**********************************\n");
  for (i = 1; i <= len; i++)
  {
    printf("0x%02X ", buffer[i - 1]);
    if (i % 16 == 0)
    {
      printf("\n");
    }
  }
  printf("\n");
  printf("********************end code************************************\n");
}
/**
 * @brief Callback function implementation for a long key press.
 */
void on_key_long_press(void)
{
    // This function is now intentionally empty.
    // Long press no longer switches modes during runtime.
}

/**
 * @brief Callback function implementation for a short key press.
 */
void on_key_short_press(void)
{
    // The primary function of a short press is to wake the device from STOP mode,
    // which is handled by the EXTI interrupt itself.
    // We can add a simple log message here for debugging if needed.
    printf("\r\n--- Key press detected (wake-up event) ---\r\n");
}

/**
 * @brief Initializes only the peripherals required for configuration mode.
 */
void Configuration_Mode_Init(void)
{
    printf("Initializing for Configuration Mode...\r\n");

    // Enable power for external peripherals (like Flash)
    HAL_GPIO_WritePin(DEV_PWR_CTRL_GPIO_Port, DEV_PWR_CTRL_Pin, GPIO_PIN_SET);
    HAL_Delay(10); // Give peripherals a moment to power up

    // Initialize Flash memory
    if (!W25QXX_Init())
    {
        printf("W25QXX Flash init OK!\r\n");
        // Try to load existing configuration
        if (Config_Load())
        {
            printf("Configuration loaded successfully from Flash.\r\n");
        }
        else
        {
            printf("No valid config in Flash. Using default values.\r\n");
            Config_SetDefault(); // Use defaults, but don't save yet.
        }
    }
    else
    {
        printf("W25QXX Flash init ERROR! Using default configuration.\r\n");
        Config_SetDefault();
    }

    // Start UART reception for CLI commands
    USART1_Start_DMA_Reception();
    printf("Configuration Mode Ready.\r\n");
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

  // Disable RTC wakeup timer on startup, as it might be enabled by default after a reset.
  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
  Key_Init();

  // Check for key press at startup to determine the operating mode
  HAL_Delay(100); // Small delay to allow holding the key during startup
  if (HAL_GPIO_ReadPin(Key_GPIO_Port, Key_Pin) == GPIO_PIN_RESET)
  {
      g_SystemState = STATE_CONFIGURATION;
      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET); // Turn on LED to indicate config mode
      Configuration_Mode_Init(); // Initialize only what's needed for config mode
  }
  else
  {
      // --- Normal Operation Startup ---
      g_SystemState = STATE_NORMAL_OPERATION;
      Peripherals_Init();
      // Start the DMA reception for USART1, which might be used for debugging.
      USART1_Start_DMA_Reception();
      printf("System Started for Normal Operation!\r\n");
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // --- State machine logic ---
    if (g_SystemState == STATE_NORMAL_OPERATION)
    {
        // Key_Process(); // No longer needed for mode switching.

        Perform_Sensor_Transmission();

        // If all transmissions for the current cycle are done, prepare to sleep.
        printf("\r\n--- Work cycle finished, preparing to enter STOP 2 mode... ---\r\n");
        Peripherals_DeInit(); // De-initialize peripherals to reduce power consumption

        // Short delay to ensure printf completes its output
        HAL_Delay(100);

        // Suspend SysTick to prevent it from waking up the MCU
        HAL_SuspendTick();

        // Set the RTC Wakeup timer to wake up after 60 seconds
        if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 29, RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 0) != HAL_OK)
        {
          Error_Handler();
        }
        printf("RTC Wakeup timer has been set to 30 seconds.\r\n");

        // Enter STOP 2 mode, wait for interrupt (WFI)
        HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

        // --- MCU Wake-up Point ---
        // Code execution will resume here...

        // After waking from STOP mode, the system clock is reset to MSI and must be reconfigured.
        SystemClock_Config();
        // Resume SysTick
        HAL_ResumeTick();
        printf("\r\n--- Woke up from STOP 2 mode ---\r\n");

        // Critical fix: After waking from deep sleep, all HAL-level peripherals must be re-initialized,
        // as their clocks and power sources were mostly turned off in STOP mode.
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
        // Note: RTC keeps running throughout the process and does not need re-initialization.

        // Reset the work cycle counter and re-initialize application-layer drivers
        // to start a new work cycle.
        Peripherals_Init();
        // Restart DMA reception for the CLI
        USART1_Start_DMA_Reception();
    }
    else if (g_SystemState == STATE_CONFIGURATION)
    {
        // In configuration mode, loop to process CLI commands.
        CLI_Process();
    }
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

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
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
  HAL_UART_DeInit(&huart1);
  HAL_UART_DeInit(&hlpuart1);
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

  if (!sgp30_init())
  {
    printf("SGP30 init ok!\r\n");
  }
  else
  {
    printf("SGP30 init err!\r\n");
  }

  if (Init_BH1750())
  {
    printf("BH1750 init ok!\r\n");
  }
  else
  {
    printf("BH1750 init err!\r\n");
  }

  SP3485_Init();
  printf("SP3485 init ok!\r\n");

  printf("sgp30 wait air for init\r\n");
  do
  {
    if (sgp30_read((uint16_t *)&sensor_data.co2Concentration, (uint16_t *)&sensor_data.vocConcentration) < 0)
    {
      printf("\r\n spg30 read failed");
    }
    HAL_Delay(200);
  } while (sensor_data.vocConcentration == 0 && sensor_data.co2Concentration == 400);
  printf("sgp30 air init End!\r\n");

  Battery_Init();

  if (!W25QXX_Init())
  {
    printf("W25QXX Flash init OK!\r\n");

    if (Config_Load())
    {
      printf("Configuration loaded successfully from Flash.\r\n");
    }
    else
    {
      printf("No valid config found in Flash. Saving default values.\r\n");
      if (!Config_Save())
      {
        printf("Error: Failed to save default configuration!\r\n");
        Error_Handler();
      }
    }
  }
  else
  {
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

  HAL_Delay(1000);
}

void Perform_Sensor_Transmission(void)
{
  BH1750_GetDate((uint16_t *)&sensor_data.lightIntensity);
  sgp30_read((uint16_t *)&sensor_data.co2Concentration, (uint16_t *)&sensor_data.vocConcentration);
  SHT40_Read_RHData((double *)&sensor_data.greenhouseTemperature, (double *)&sensor_data.greenhouseHumidity);
  Soil_Sensor_Full_Data_t soil_data;
  SP3485_Read_Soil_Full_Data(0x01, &soil_data);
  sensor_data.soilMoisture = soil_data.moisture;
  sensor_data.soilTemperature = soil_data.temperature;
  sensor_data.soilEc = soil_data.ec;
  sensor_data.soilPh = soil_data.ph;
  sensor_data.soilNitrogen = soil_data.nitrogen;
  sensor_data.soilPhosphorus = soil_data.phosphorus;
  sensor_data.soilPotassium = soil_data.potassium;
  sensor_data.soilSalinity = soil_data.salinity;
  sensor_data.soilTds = soil_data.tds;
  sensor_data.soilFertility = soil_data.fertility;
  sensor_data.common.batteryVoltage = Battery_GetVoltage();
  sensor_data.common.batteryLevel = Battery_GetPercentage(sensor_data.common.batteryVoltage);

  printf("    Read Success!   \r\n");

  printf("Moisture:    %.1f %%\r\n", sensor_data.soilMoisture);
  printf("Temperature: %.1f C\r\n", sensor_data.soilTemperature);
  printf("EC:          %d uS/cm\r\n", sensor_data.soilEc);
  printf("PH:          %.1f\r\n", sensor_data.soilPh);
  printf("Nitrogen:    %d\r\n", sensor_data.soilNitrogen);
  printf("Phosphorus:  %d\r\n", sensor_data.soilPhosphorus);
  printf("Potassium:   %d\r\n", sensor_data.soilPotassium);
  printf("Salinity:    %d\r\n", sensor_data.soilSalinity);
  printf("TDS:         %d\r\n", sensor_data.soilTds);
  printf("Fertility:   %d\r\n", sensor_data.soilFertility);
  printf("CO2:         %d ppm\r\n", sensor_data.co2Concentration);
  printf("VOC:         %d ppb\r\n", sensor_data.vocConcentration);
  printf("Temperature: %.1f C\r\n", sensor_data.greenhouseTemperature);
  printf("Humidity:    %.1f %%\r\n", sensor_data.greenhouseHumidity);
  printf("Light:       %d lux\r\n", sensor_data.lightIntensity);
  printf("BatteryLvel: %d%%\r\n", sensor_data.common.batteryLevel);

  sensor_data_payload_t sensor_lora_payload;
  if (lora_model_create_sensor_payload((const InternalSensorProperties_t *)&sensor_data, &sensor_lora_payload))
  {
    int lora_data_len = generate_lora_frame(LORA_HOST_ADDRESS, DEVICE_TYPE_SENSOR_Internal, MSG_TYPE_REPORT_SENSOR, 0, (const uint8_t *)&sensor_lora_payload, sizeof(sensor_lora_payload), lora_send_buffer, sizeof(lora_send_buffer));
    printf("lora_data_len:%d\r\n", lora_data_len);
    printf("\r\n");
    print_hex((char *)lora_send_buffer, lora_data_len);
    printf("lora send status:%d\r\n", LoRa_transmit(&myLoRa, lora_send_buffer, lora_data_len, 3000));
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
