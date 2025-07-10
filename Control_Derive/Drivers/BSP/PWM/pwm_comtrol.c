#include "pwm_control.h"

uint32_t duty = 0;

// PWM初始化
HAL_StatusTypeDef PWM_Init(void){
	if (HAL_TIM_PWM_Start(&PWM_TIMER, PUMP_PWM_CHANNEL) != HAL_OK)
	{
		return HAL_ERROR;
	}
	if (HAL_TIM_PWM_Start(&PWM_TIMER, FAN_PWM_CHANNEL) != HAL_OK)
	{
		return HAL_ERROR;
	}
	set_pump_speed(0);
  set_fan_speed(0);
	return HAL_OK;
}

/**
  * @brief  设置水泵 PWM 占空比
  * @param  duty_cycle: 占空比 (0 到 100)
  * @retval None
  */
void set_pump_speed(uint32_t duty_cycle)
{
  uint32_t pulse_value = 0;

  if (duty_cycle > 100)
  {
    duty_cycle = 100; // 限制最大占空比为 100%
  }

  // 将 0-100 的占空比映射到 0-PWM_MAX_DUTY 的脉冲值
  pulse_value = (uint32_t)(((float)duty_cycle / 100.0f) * (PWM_MAX_DUTY + 1));

  // 处理边界情况，确保 pulse_value 不超过 ARR 值
  if (pulse_value > PWM_MAX_DUTY) {
      pulse_value = PWM_MAX_DUTY;
  }
  // 如果占空比为0，确保脉冲为0
  if (duty_cycle == 0) {
      pulse_value = 0;
  }
  // 如果占空比为100，确保脉冲为 ARR+1 (或 ARR，取决于具体实现，通常设置为 ARR+1 获得完全 100%)
  // 但是 HAL 库通常期望 CCR 的值 <= ARR，所以设置为 PWM_MAX_DUTY 即可达到接近 100%
  // 如果需要精确的 100%，可能需要调整 PWM 模式或 ARR 值
  if (duty_cycle == 100) {
      pulse_value = PWM_MAX_DUTY + 1; // 对于标准 PWM 模式，设置 CCR > ARR 会产生 100% 占空比
                                      // 但为了兼容性，通常限制在 ARR 内，即 PWM_MAX_DUTY
      // pulse_value = PWM_MAX_DUTY; // 更安全的做法
  }


  // 设置 PWM 比较值 (CCR - Capture Compare Register)
  __HAL_TIM_SET_COMPARE(&PWM_TIMER, PUMP_PWM_CHANNEL, pulse_value);
}

/**
  * @brief  设置风扇 PWM 占空比
  * @param  duty_cycle: 占空比 (0 到 100)
  * @retval None
  */
void set_fan_speed(uint32_t duty_cycle)
{
  uint32_t pulse_value = 0;

  if (duty_cycle > 100)
  {
    duty_cycle = 100;
  }

  pulse_value = (uint32_t)(((float)duty_cycle / 100.0f) * (PWM_MAX_DUTY + 1));

  if (pulse_value > PWM_MAX_DUTY) {
      pulse_value = PWM_MAX_DUTY;
  }
   if (duty_cycle == 0) {
      pulse_value = 0;
  }
  if (duty_cycle == 100) {
      pulse_value = PWM_MAX_DUTY + 1;
      // pulse_value = PWM_MAX_DUTY; // 更安全的做法
  }

  __HAL_TIM_SET_COMPARE(&PWM_TIMER, FAN_PWM_CHANNEL, pulse_value);
}

