#include "iap_main.h"

#include "usart.h"
#include "can.h"
#include "string.h"

/* CAN中断接收缓冲区 - 增大以支持完整的Ymodem帧 */
volatile can_buffer_s ymodem_can_rx_buffer = {
    .can_rx_head = 0,
    .can_rx_tail = 0,
    .can_rx_frame_ready = 0,
    .can_rx_frame_length = 0,
};

/**
 * @brief CAN初始化
 */
void IAP_CAN_Init(void)
{

/* 请根据具体硬件平台配置CAN参数和GPIO */
#if MCU_TYPE == USE_STM32
    MX_CAN1_Init();

    /* 启用接收中断和空闲中断 */
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE); /* 接收中断 */
#elif MCU_TYPE == USE_GD32
    MX_CAN_INIT();
#endif
}

/**
 * @brief CAN去初始化
 */
void IAP_CAN_DeInit(void)
{
/* 禁用所有CAN中断和外设 */
#if MCU_TYPE == USE_STM32
    /* 禁用所有UART中断 */
    __HAL_UART_DISABLE_IT(&huart1, UART_IT_RXNE);
    __HAL_UART_DISABLE_IT(&huart1, UART_IT_IDLE);
    __HAL_UART_DISABLE_IT(&huart1, UART_IT_TXE);
    
    /* 去初始化UART */
    HAL_UART_DeInit(&huart1);
#elif MCU_TYPE == USE_GD32
    /* 禁用CAN中断 */
    // usart_interrupt_disable(IAP_UART, USART_INT_RBNE);
    // usart_interrupt_disable(IAP_UART, USART_INT_IDLE);
    // usart_interrupt_disable(IAP_UART, USART_INT_TBE);

    can_interrupt_disable(CAN0, CAN_INT_RFNE1);
    
    // /* 禁用UART */
    // usart_disable(IAP_UART);
#endif
}

/**
 * @brief 从缓存中接收一个字节（非阻塞）
 * @param data 用于存储接收数据的指针
 * @return 0:成功, 1:无数据可用
 */
uint8_t IAP_CAN_ReceiveByte(uint8_t *data)
{
    /* 检查缓存中是否有数据 */
    if (ymodem_can_rx_buffer.can_rx_head != ymodem_can_rx_buffer.can_rx_tail)
    {
        *data = ymodem_can_rx_buffer.can_rx_buffer[ymodem_can_rx_buffer.can_rx_tail];
        ymodem_can_rx_buffer.can_rx_tail = (ymodem_can_rx_buffer.can_rx_tail + 1) % CAN_RX_BUFFER_SIZE;
        
        /* 如果读取了数据且缓存为空，清除帧就绪标志 */
        if (ymodem_can_rx_buffer.can_rx_head == ymodem_can_rx_buffer.can_rx_tail)
        {
            ymodem_can_rx_buffer.can_rx_frame_ready = 0;
            ymodem_can_rx_buffer.can_rx_frame_length = 0;
        }
        
        return 0; /* 成功 */
    }
    
    return 1; /* 无数据可用 */
}

/**
 * @brief 从缓存中接收一个字节并返回状态（兼容旧接口）
 * @param data 用于存储接收数据的指针
 * @param timeout_ms 超时时间(毫秒) - 已弃用，保持接口兼容性
 * @return 0:成功, 1:无数据可用
 * @note 此函数现在为非阻塞模式，timeout_ms参数被忽略
 */
uint8_t IAP_CAN_ReceiveByteWithStatus(uint8_t *data, uint32_t timeout_ms)
{
    (void)timeout_ms; /* 忽略超时参数 */
    return IAP_CAN_ReceiveByte(data);
}

/**
 * @brief 发送一个字节
 * @param data 要发送的字节数据
 *
 */
void IAP_CAN_SendByte(uint8_t data)
{
#if MCU_TYPE == USE_STM32
    HAL_UART_Transmit(&huart1, &data, 1, HAL_MAX_DELAY);
#elif MCU_TYPE == USE_GD32
    // usart_data_transmit(IAP_UART, data);
    CAN_send(CAN_ID_BOOTLOADER_REQUEST, &data, 1);

    // while (RESET == usart_flag_get(IAP_UART, USART_FLAG_TBE))
    //     ; // 等待发送完成
#endif
}

/**
 * @brief 发送数据
 * @param data 要发送的数据缓冲区
 * @param length 要发送的数据长度
 */
void IAP_CAN_SendData(uint8_t *data, uint16_t length)
{
    /* 用户需要在此实现发送多字节数据的代码 */
    /* 可以循环调用IAP_UART_SendByte，或使用DMA等高效方式 */
    uint16_t i;
    uint16_t length_to_send;

    length_to_send = length / 8;  /* 每次发送8字节 */
    /* 循环发送数据 */

    for (i = 0; i < length_to_send; i++)
    {
        // IAP_CAN_SendByte(data[i]);
        CAN_send(CAN_ID_BOOTLOADER_REQUEST, &data[i * 8], 8);
    }
    if (length % 8 != 0)
    {
        CAN_send(CAN_ID_BOOTLOADER_REQUEST, &data[length_to_send * 8], length % 8);
    }
    
}

/**
 * @brief 检查缓存中是否有数据可读
 * @return 1-有数据可读，0-无数据
 */
uint8_t IAP_CAN_DataAvailable(void)
{
    /* 检查环形缓存中是否有数据 */
    return (ymodem_can_rx_buffer.can_rx_head != ymodem_can_rx_buffer.can_rx_tail) ? 1 : 0;
}

/**
 * @brief 获取缓冲区中的可用数据字节数
 * @return 可用数据字节数
 */
uint16_t IAP_CAN_GetAvailableBytes(void)
{
    uint16_t head = ymodem_can_rx_buffer.can_rx_head;
    uint16_t tail = ymodem_can_rx_buffer.can_rx_tail;
    
    /* 计算环形缓存中的可用字节数 */
    if (head >= tail)
    {
        return head - tail;
    }
    else
    {
        return (UART_RX_BUFFER_SIZE - tail) + head;
    }
}

/**
 * @brief 检查是否有完整的帧数据准备好
 * @return 1-有完整帧，0-无完整帧
 */
uint8_t IAP_CAN_IsFrameReady(void)
{
    return ymodem_can_rx_buffer.can_rx_frame_ready;
}

/**
 * @brief 从缓存中读取多个字节（非阻塞）
 * @param buffer 用于存储数据的缓冲区
 * @param length 要读取的字节数
 * @return 实际读取的字节数
 */
uint16_t IAP_CAN_ReadBytes(uint8_t *buffer, uint16_t length)
{
    uint16_t bytes_read = 0;
    uint8_t data;
    
    /* 读取指定数量的字节或直到缓存为空 */
    while (bytes_read < length && IAP_CAN_ReceiveByte(&data) == 0)
    {
        buffer[bytes_read] = data;
        bytes_read++;
    }
    
    return bytes_read;
}

/**
 * @brief 查看缓存中的下一个字节但不移除（非阻塞）
 * @param data 用于存储查看到的数据
 * @return 0:成功, 1:无数据可用
 */
uint8_t IAP_CAN_PeekByte(uint8_t *data)
{
    /* 检查缓存中是否有数据 */
    if (ymodem_can_rx_buffer.can_rx_head != ymodem_can_rx_buffer.can_rx_tail)
    {
        *data = ymodem_can_rx_buffer.can_rx_buffer[ymodem_can_rx_buffer.can_rx_tail];
        return 0; /* 成功 */
    }
    
    return 1; /* 无数据可用 */
}

/**
 * @brief 丢弃缓存中指定数量的字节
 * @param count 要丢弃的字节数
 * @return 实际丢弃的字节数
 */
uint16_t IAP_CAN_DiscardBytes(uint16_t count)
{
    uint16_t discarded = 0;
    uint8_t dummy;
    
    /* 丢弃指定数量的字节或直到缓存为空 */
    while (discarded < count && IAP_CAN_ReceiveByte(&dummy) == 0)
    {
        discarded++;
    }
    
    return discarded;
}

/**
 * @brief 清空接收缓冲区
 *
 */
void IAP_CAN_ClearBuffer(void)
{
#if MCU_TYPE == USE_STM32
    /* 禁用UART中断，避免竞态条件 */
    __HAL_UART_DISABLE_IT(&huart1, UART_IT_RXNE);

    /* 清空中断接收缓冲区 */
    ymodem_can_rx_buffer.uart_rx_head = ymodem_can_rx_buffer.uart_rx_tail = 0;
    ymodem_can_rx_buffer.uart_rx_frame_ready = 0;
    ymodem_can_rx_buffer.uart_rx_frame_length = 0;
    /* 清除UART硬件缓冲区 */
    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != RESET)
    {
        /* 读取并丢弃硬件缓冲区中的数据 */
        (void)(huart1.Instance->DR);
    }

    /* 清除UART错误标志 */
    __HAL_UART_CLEAR_PEFLAG(&huart1);   /* 清除奇偶校验错误标志 */
    __HAL_UART_CLEAR_FEFLAG(&huart1);   /* 清除帧错误标志 */
    __HAL_UART_CLEAR_NEFLAG(&huart1);   /* 清除噪声错误标志 */
    __HAL_UART_CLEAR_OREFLAG(&huart1);  /* 清除溢出错误标志 */
    __HAL_UART_CLEAR_IDLEFLAG(&huart1); /* 清除空闲线路检测标志 */

    /* 重新启用UART中断 */
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
#elif MCU_TYPE == USE_GD32
    can_receive_message_struct rx_message;

    /* 禁用CAN中断，避免竞态条件 */
    can_interrupt_disable(CAN0, CAN_INT_RFNE1);
    /* 清空中断接收缓冲区 */
    ymodem_can_rx_buffer.can_rx_head = ymodem_can_rx_buffer.can_rx_tail = 0;
    ymodem_can_rx_buffer.can_rx_frame_ready = 0;
    ymodem_can_rx_buffer.can_rx_frame_length = 0;

    // /* 清除CAN硬件缓冲区 */
    // while (can_flag_get(CAN0, CAN_FLAG_RFL) != RESET)
    // {
    //     /* 读取并丢弃硬件缓冲区中的数据 */
    //     usart_data_receive(IAP_UART);
    // }

    /* 清除CAN硬件缓冲区 */
    while ( can_receive_message_length_get(CAN0, CAN_FIFO1) > 0) 
    {
        can_message_receive(CAN0, CAN_FIFO1, &rx_message);
    }
    /* 清除错误状态标志 */
    can_flag_clear(CAN0, CAN_FLAG_ERRIF);   /* 错误标志 */
    can_flag_clear(CAN0, CAN_FLAG_MTE0);    /* 邮箱传输错误标志 */
    can_flag_clear(CAN0, CAN_FLAG_MAL0);    /* 邮箱仲裁丢失标志 */

    /* 清除接收FIFO溢出标志 */
    can_flag_clear(CAN0, CAN_FLAG_RFO1);    /* FIFO1溢出 */
    can_flag_clear(CAN0, CAN_FLAG_RFF1);    /* FIFO1满 */

    /* 重新启用CAN中断 */
    can_interrupt_enable(CAN0, CAN_INT_RFNE1);
#endif
}

/**
 * @brief 获取帧长度
 * @return 帧长度
 */
uint16_t IAP_CAN_GetFrameLength(void)
{
    return ymodem_can_rx_buffer.can_rx_frame_length;
}

#define CAN_RX_DATA_SIZE 8

/**
 * @brief CAN接收回调函数
 */
static void IAP_CAN_RxCallback(void)
{
    uint16_t next_head;
    
	can_receive_message_struct RxMsg; // can接受数据结构体

    can_message_receive(CAN0, CAN_FIFO1, &RxMsg);

    /* 计算下一个头指针位置 */
    next_head = (ymodem_can_rx_buffer.can_rx_head + CAN_RX_DATA_SIZE) % CAN_RX_BUFFER_SIZE;
    
    /* 检查缓存是否溢出 */
    if (next_head != ymodem_can_rx_buffer.can_rx_tail)
    {
        /* 缓存未满，存储数据 */
        for (int i = 0; i < CAN_RX_DATA_SIZE; i++)
        {
            ymodem_can_rx_buffer.can_rx_buffer[ymodem_can_rx_buffer.can_rx_head + i] = RxMsg.rx_data[i];
            ymodem_can_rx_buffer.can_rx_frame_length++;
        }
        ymodem_can_rx_buffer.can_rx_head = next_head;
    }
    /* 如果缓存满了，则丢弃新数据（静默处理溢出） */
    
    IAP_TIMER_clean_timer_count(); // 清除定时器计数
}

/**
 * @brief 串口接收完成回调函数
 * @note 调用后表示完成了一帧数据的接收
 */
void IAP_CAN_RxCompletedCallback(void)
{
    // 有数据，设置帧准备好标志
    ymodem_can_rx_buffer.can_rx_frame_ready = 1;
}

/**
 * @brief 串口中断处理函数
 * @note 用户需要在此实现UART中断处理逻辑
 */
void CAN0_RX1_IRQHandler(void)
{
#if MCU_TYPE == USE_STM32
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != RESET)
    {
        /* 调用接收中断处理函数 */
        IAP_UART_RxCallback();
    }
#elif MCU_TYPE == USE_GD32
    // if (usart_flag_get(IAP_UART, USART_FLAG_RBNE) != RESET)
    // {
    //     /* 调用接收中断处理函数 */
    //     IAP_UART_RxCallback();
    // }
    IAP_CAN_RxCallback();
#endif
}
