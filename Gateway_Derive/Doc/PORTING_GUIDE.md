# STM32 物联网网关应用快速移植指南

本指南将指导您如何将已完成的物联网网关应用逻辑快速、无缝地移植到一个全新的、由 `STM32CubeMX` 生成的标准工程中。

### **阶段一：源文件准备**

1.  **复制核心驱动与应用文件**:
    在您的旧工程目录下，找到以下文件夹和文件，并将它们完整地复制到新工程的对应位置：

    *   `Drivers/AT_Handler` -> `(新工程)/Drivers/AT_Handler`
    *   `Drivers/Huawei_IoT` -> `(新工程)/Drivers/Huawei_IoT`
    *   `Drivers/cJSON` -> `(新工程)/Drivers/cJSON`
    *   `Core/Inc/app_main.h` -> `(新工程)/Core/Inc/app_main.h`
    *   `Core/Src/app_main.c` -> `(新工程)/Core/Src/app_main.c`

### **阶段二：STM32CubeMX 配置**

打开新工程的 `.ioc` 文件，完成以下硬件和中间件配置：

1.  **系统核心 (SYS)**:
    *   `Debug`: 设置为 `Serial Wire`。
    *   `Timebase Source`: 若使用 FreeRTOS，建议选择一个基本定时器 (如 `TIM13`) 作为 HAL 库的时间基准，以避免与 FreeRTOS 的 Systick 冲突。

2.  **FreeRTOS**:
    *   `Interface`: 启用 `CMSIS_V1`。
    *   `Tasks and Queues`: CubeMX 会自动创建一个 `defaultTask`，保持即可。我们后续会修改它的内容。
    *   `Config parameters`: 建议将 `TOTAL_HEAP_SIZE` 增加到 `8192` 或更高，以满足 AT 处理器和 cJSON 的内存需求。

3.  **AT 通信串口 (例如 USART1)**:
    *   `Mode`: 设置为 `Asynchronous`。
    *   `Parameter Settings`:
        *   `Baud Rate`: `115200`
        *   `Word Length`: `8 Bits`
        *   `Parity`: `None`
        *   `Stop Bits`: `1`
    *   `DMA Settings`:
        *   点击 `Add`，添加 `USART1_RX` 的 DMA 请求，模式设为 `Normal`。
    *   `NVIC Settings`:
        *   **必须启用** `USART1 global interrupt`。

4.  **日志打印串口 (例如 USART3)**:
    *   `Mode`: 设置为 `Asynchronous`。
    *   `Parameter Settings`: 配置与 USART1 相同（波特率等）。此串口无需 DMA。

5.  **LED 指示灯 GPIO (例如 PB0)**:
    *   将对应引脚配置为 `GPIO_Output`。
    *   在 `System Core > GPIO` 中，给该引脚设置一个用户标签，如 `LED`。

6.  **时钟配置 (Clock Configuration)**:
    *   根据您的外部晶振（如 8MHz HSE），将系统时钟（HCLK）配置为最大频率（如 168MHz）。CubeMX 会自动解算其他时钟。

7.  **生成代码**:
    *   完成以上配置后，点击右上角的 `GENERATE CODE`。

### **阶段三：IDE (Keil/CubeIDE) 配置**

1.  **添加文件到工程**:
    *   在 IDE 的工程文件树中，手动添加我们复制过来的所有 `.c` 文件（`at_handler.c`, `huawei_iot_app.c`, `cJSON.c`, `app_main.c` 等）。

2.  **添加头文件路径**:
    *   在工程的编译选项中（如 Keil 的 `Options for Target > C/C++ > Include Paths`），添加以下路径：
        *   `../Drivers/AT_Handler/Inc` (如果您的.h在Inc中) 或 `../Drivers/AT_Handler`
        *   `../Drivers/Huawei_IoT/Inc` 或 `../Drivers/Huawei_IoT`
        *   `../Drivers/cJSON/Inc` 或 `../Drivers/cJSON`
        *   `../Core/Inc` (这个通常已存在)

### **阶段四：代码整合 (修改 `main.c`)**

打开新生成的 `main.c` 文件，进行以下修改：

1.  **包含头文件**:
    在 `/* USER CODE BEGIN Includes */` 区域，添加：
    ```c
    #include "stdio.h"
    #include "at_handler.h"
    #include "app_main.h"
    ```

2.  **添加 printf 重定向**:
    在 `/* USER CODE BEGIN 0 */` 区域，添加 `fputc` 实现（假设使用 `huart3` 打印）：
    ```c
    int fputc(int ch, FILE *f)
    {
      HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
      return ch;
    }
    ```

3.  **声明外部 AT 句柄**:
    在 `/* USER CODE BEGIN PV */` 区域，添加：
    ```c
    extern AT_Handler_t at_handler;
    ```

4.  **实现 UART 空闲中断回调**:
    在 `/* USER CODE BEGIN 4 */` 区域，添加 `HAL_UARTEx_RxEventCallback` 函数。**注意**：CubeMX F4 V1.27.0 及以后版本可能已在 `stm32f4xx_it.c` 中生成了此函数的弱定义，您需要找到并替换它，或者直接在 `main.c` 中实现这个强定义。
    ```c
    void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
    {
        // 确保是 AT 模块所在的串口
        if (huart->Instance == USART1)
        {
            AT_UartIdleCallback(&at_handler, huart);
        }
    }
    ```

5.  **修改 `StartDefaultTask`**:
    这是最关键的一步。将 `StartDefaultTask` 函数的内容完全替换为对我们应用层的调用：
    ```c
    /* USER CODE BEGIN Header_StartDefaultTask */
    /**
    * @brief Function implementing the defaultTask thread.
    * @param argument: Not used
    * @retval None
    */
    /* USER CODE END Header_StartDefaultTask */
    void StartDefaultTask(void const * argument)
    {
      /* USER CODE BEGIN 5 */
      // 执行一次性应用初始化
      App_Main_Init();

      // 启动应用主任务 (此函数是死循环，永不返回)
      App_Main_Task();
      /* USER CODE END 5 */
    }
    ```
    **注意**: `huart1` 和 `hdma_usart1_rx` 等句柄由 CubeMX 生成为 `main.c` 中的全局变量，`app_main.c` 中已通过 `extern` 引用，无需额外操作。

### **阶段五：编译与运行**

完成以上所有步骤后，**重新编译整个工程**。如果没有错误，烧录到您的新硬件上。打开串口监视器，您应该能看到与旧工程完全一致的启动、连接和运行日志。

至此，移植完成！ 