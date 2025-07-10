#include "key_handler.h"
#include "main.h" // 用于获取按键和LED的引脚定义及HAL库函数

// --- 私有宏定义 ---
#define DEBOUNCE_TIME_MS        50     // 按键去抖时间，单位：毫秒

// --- 私有变量 ---

// 该结构体用于保存按键的所有状态信息
typedef struct {
    uint32_t press_start_time;       // 按键按下的起始时间戳 (来自 HAL_GetTick())
    uint32_t last_interrupt_time;    // 上次中断的时间戳，用于去抖
    bool is_pressed;                 // 按键当前的逻辑状态 (如果被按下则为true)
    bool long_press_triggered;       // 长按事件是否已触发的标志，防止重复触发
    Key_EventCallback_t on_short_press; // 短按事件的回调函数
    Key_EventCallback_t on_long_press;  // 长按事件的回调函数
} Key_State_t;

static Key_State_t s_key_state; // 按键状态的静态实例


// --- 公有函数实现 ---

void Key_Init(void)
{
    // 将按键状态结构体初始化为其默认（松开）状态
    s_key_state.press_start_time = 0;
    s_key_state.last_interrupt_time = 0;
    s_key_state.is_pressed = false;
    s_key_state.long_press_triggered = false;
    s_key_state.on_short_press = NULL;
    s_key_state.on_long_press = NULL;
}

void Key_Register_Callbacks(Key_EventCallback_t short_press_callback, Key_EventCallback_t long_press_callback)
{
    s_key_state.on_short_press = short_press_callback;
    s_key_state.on_long_press = long_press_callback;
}

void Key_Process(void)
{
    // --- 长按检测 (仅在按键被持续按下时进行) ---
    if (s_key_state.is_pressed && !s_key_state.long_press_triggered)
    {
        uint32_t press_duration = HAL_GetTick() - s_key_state.press_start_time;
        if (press_duration >= LONG_PRESS_TIME_MS)
        {
            // 已达到长按时间阈值
            s_key_state.long_press_triggered = true; // 标记为已触发，以避免重复执行回调
            if (s_key_state.on_long_press != NULL)
            {
                s_key_state.on_long_press();
            }
        }
    }
}

void Key_EXTI_Callback(uint16_t GPIO_Pin)
{
    // 1. 检查是否是我们的按键引脚触发的中断
    if (GPIO_Pin != Key_Pin)
    {
        return;
    }

    // 2. 按键去抖处理
    uint32_t current_time = HAL_GetTick();
    if (current_time - s_key_state.last_interrupt_time < DEBOUNCE_TIME_MS)
    {
        return; // 忽略在去抖时间窗口内的所有后续中断（毛刺）
    }
    s_key_state.last_interrupt_time = current_time;

    // 3. 读取引脚电平，判断是按下还是松开
    GPIO_PinState current_pin_state = HAL_GPIO_ReadPin(Key_GPIO_Port, Key_Pin);

    if (current_pin_state == GPIO_PIN_RESET) // 按键被按下 (低电平触发)
    {
        // 记录按键刚刚被按下的状态
        s_key_state.is_pressed = true;
        s_key_state.long_press_triggered = false; // 重置长按标志
        s_key_state.press_start_time = current_time;
    }
    else // 按键被松开 (高电平触发)
    {
        // 仅当长按事件未被触发时，才有可能触发短按
        if (s_key_state.is_pressed && !s_key_state.long_press_triggered)
        {
            uint32_t press_duration = current_time - s_key_state.press_start_time;
            
            // 一次有效的短按必须持续超过去抖时间
            if (press_duration >= SHORT_PRESS_TIME_MS) 
            {
                 if (s_key_state.on_short_press != NULL)
                 {
                     s_key_state.on_short_press();
                 }
            }
        }
        // 无论如何，只要按键松开，就重置按下状态
        s_key_state.is_pressed = false;
    }
} 