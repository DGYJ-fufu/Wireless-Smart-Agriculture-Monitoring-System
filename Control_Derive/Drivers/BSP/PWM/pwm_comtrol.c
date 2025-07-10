#include "pwm_control.h"

uint32_t duty = 0;

// PWM��ʼ��
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
  * @brief  ����ˮ�� PWM ռ�ձ�
  * @param  duty_cycle: ռ�ձ� (0 �� 100)
  * @retval None
  */
void set_pump_speed(uint32_t duty_cycle)
{
  uint32_t pulse_value = 0;

  if (duty_cycle > 100)
  {
    duty_cycle = 100; // �������ռ�ձ�Ϊ 100%
  }

  // �� 0-100 ��ռ�ձ�ӳ�䵽 0-PWM_MAX_DUTY ������ֵ
  pulse_value = (uint32_t)(((float)duty_cycle / 100.0f) * (PWM_MAX_DUTY + 1));

  // ����߽������ȷ�� pulse_value ������ ARR ֵ
  if (pulse_value > PWM_MAX_DUTY) {
      pulse_value = PWM_MAX_DUTY;
  }
  // ���ռ�ձ�Ϊ0��ȷ������Ϊ0
  if (duty_cycle == 0) {
      pulse_value = 0;
  }
  // ���ռ�ձ�Ϊ100��ȷ������Ϊ ARR+1 (�� ARR��ȡ���ھ���ʵ�֣�ͨ������Ϊ ARR+1 �����ȫ 100%)
  // ���� HAL ��ͨ������ CCR ��ֵ <= ARR����������Ϊ PWM_MAX_DUTY ���ɴﵽ�ӽ� 100%
  // �����Ҫ��ȷ�� 100%��������Ҫ���� PWM ģʽ�� ARR ֵ
  if (duty_cycle == 100) {
      pulse_value = PWM_MAX_DUTY + 1; // ���ڱ�׼ PWM ģʽ������ CCR > ARR ����� 100% ռ�ձ�
                                      // ��Ϊ�˼����ԣ�ͨ�������� ARR �ڣ��� PWM_MAX_DUTY
      // pulse_value = PWM_MAX_DUTY; // ����ȫ������
  }


  // ���� PWM �Ƚ�ֵ (CCR - Capture Compare Register)
  __HAL_TIM_SET_COMPARE(&PWM_TIMER, PUMP_PWM_CHANNEL, pulse_value);
}

/**
  * @brief  ���÷��� PWM ռ�ձ�
  * @param  duty_cycle: ռ�ձ� (0 �� 100)
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
      // pulse_value = PWM_MAX_DUTY; // ����ȫ������
  }

  __HAL_TIM_SET_COMPARE(&PWM_TIMER, FAN_PWM_CHANNEL, pulse_value);
}

