#include "iap_main.h"
#include "tim.h"

/**
 * @brief 初始化定时器
 * @note 超时时间设置为1ms
 *
 */
void IAP_TIMER_Init(void)
{
    /* 用户需要在此实现定时器初始化代码 */
    /* 请根据具体硬件平台配置定时器参数和GPIO */
    #if MCU_TYPE == USE_STM32
    MX_TIM1_Init();
    #elif MCU_TYPE == USE_GD32
    MX_TIMER1_INIT();
    #endif

}

/**
 * @brief 定时器去初始化
 * @note 在跳转到应用程序前调用
 *
 */
void IAP_TIMER_DeInit(void)
{
    /* 停止定时器并去初始化 */
    #if MCU_TYPE == USE_STM32
    /* 禁用定时器中断 */
    __HAL_TIM_DISABLE_IT(&IAP_TIMER, TIM_IT_UPDATE);
    /* 停止定时器 */
    HAL_TIM_Base_Stop_IT(&IAP_TIMER);
    /* 去初始化定时器 */
    HAL_TIM_Base_DeInit(&IAP_TIMER);
    #elif MCU_TYPE == USE_GD32
    /* 禁用定时器中断 */
    timer_interrupt_disable(TIMER1, TIMER_INT_UP);
    /* 禁用定时器 */
    timer_disable(TIMER1);
    #endif
}

/**
 * @brief 清除定时器计数
 * @note 停止定时器并重置计数器
 *
 */
void IAP_TIMER_clean_timer_count(void)
{
    #if MCU_TYPE == USE_STM32
    /* 禁用定时器中断 */
    __HAL_TIM_DISABLE_IT(&IAP_TIMER, TIM_IT_UPDATE);

    /* 重置定时器计数器 */
    __HAL_TIM_SET_COUNTER(&IAP_TIMER, 0);

    /* 清除定时器更新标志 */
    __HAL_TIM_CLEAR_FLAG(&IAP_TIMER, TIM_FLAG_UPDATE);

    /* 重新开启定时器中断 */
    __HAL_TIM_ENABLE_IT(&IAP_TIMER, TIM_IT_UPDATE);

    /* 重新启用定时器 */
    HAL_TIM_Base_Start(&IAP_TIMER);
    #elif MCU_TYPE == USE_GD32
    /* 禁用定时器中断 */
    timer_interrupt_disable(IAP_TIMER,TIMER_INT_UP);
    /* 重置定时器计数器 */
    timer_counter_value_config(IAP_TIMER,0);
    /* 清除定时器更新标志 */
    timer_interrupt_flag_clear(IAP_TIMER, TIMER_FLAG_UP);
    /* 重新开启定时器中断 */
    timer_interrupt_enable(IAP_TIMER,TIMER_INT_UP);
    /* 重新启用定时器 */
    timer_enable(IAP_TIMER);
    #endif
}

/**
 * @brief 启动定时器
 * @note 启动定时器以开始计时
 *
 */
void IAP_TIMER_Start(void)
{
    #if MCU_TYPE == USE_STM32
    /* 启动定时器 */
    HAL_TIM_Base_Start(&IAP_TIMER);
    #elif MCU_TYPE == USE_GD32
    /* 启动定时器 */
    timer_enable(IAP_TIMER);
    #endif
}

/**
 * @brief 停止定时器
 * @note 停止定时器以停止计时
 *
 */
void IAP_TIMER_Stop(void)
{
    #if MCU_TYPE == USE_STM32
    /* 停止定时器 */
    HAL_TIM_Base_Stop(&IAP_TIMER);
    #elif MCU_TYPE == USE_GD32
    /* 停止定时器 */
    timer_disable(IAP_TIMER);
		timer_interrupt_disable(IAP_TIMER,TIMER_INT_UP);
    #endif
}

/**
 * @brief 定时器中断处理函数
 * @note 用户需要在此实现定时器中断处理逻辑
 */
void IAP_TIMER_IRQHandler(void)
{
    #if MCU_TYPE == USE_STM32
    /* 检查定时器更新中断 */
    if (__HAL_TIM_GET_FLAG(&IAP_TIMER, TIM_FLAG_UPDATE) != RESET)
    {
        /* 清除定时器更新中断标志 */
        __HAL_TIM_CLEAR_IT(&IAP_TIMER, TIM_IT_UPDATE);
        IAP_UART_RxCompletedCallback(); // 调用接收完成回调
        IAP_TIMER_Stop();               // 停止定时器
    }
    #elif MCU_TYPE == USE_GD32
    /* 检查定时器更新中断 */
    if (timer_interrupt_flag_get(IAP_TIMER, TIMER_FLAG_UP) == SET)
    {
        /* 清除定时器更新中断标志 */
        timer_interrupt_flag_clear(IAP_TIMER, TIMER_FLAG_UP);
        IAP_UART_RxCompletedCallback(); // 调用接收完成回调
        IAP_TIMER_Stop();               // 停止定时器
    }
    #endif
}
