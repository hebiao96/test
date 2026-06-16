#ifndef __IAP_CAN_H
#define __IAP_CAN_H

#include <stdint.h>

#define CAN_RX_BUFFER_SIZE (YMODEM_PACKET_1024 + 16)
typedef struct
{
    uint8_t can_rx_buffer[CAN_RX_BUFFER_SIZE];
    uint8_t can_rx_frame_ready;   // 标志位，表示有完整帧数据准备好
    uint16_t can_rx_head;         // 接收缓冲区头指针
    uint16_t can_rx_tail;         // 接收缓冲区头尾指针
    uint16_t can_rx_frame_length; // 当前帧长度
} can_buffer_s;

/**
 * @brief CAN初始化
 * @note 用户需要实现此函数，完成CAN硬件初始化，包括：
 *       - GPIO配置（TX/RX引脚）
 *       - CAN参数配置（波特率、数据位、停止位、校验位）
 *       - 时钟使能
 *       - 中断配置（如果使用中断方式）
 */
void IAP_CAN_Init(void);

/**
 * @brief CAN去初始化
 * @note 在跳转到应用程序前调用，完成CAN硬件去初始化，包括：
 *       - 禁用CAN中断
 *       - 关闭CAN外设
 *       - 复位GPIO引脚
 */
void IAP_CAN_DeInit(void);

/**
 * @brief 发送一个字节
 * @param data 要发送的字节数据
 * @note 用户需要实现此函数，发送一个字节到CAN
 */
void IAP_CAN_SendByte(uint8_t data);

/**
 * @brief 接收一个字节（非阻塞）
 * @param data 用于存储接收数据的指针
 * @return 0:成功, 1:无数据可用
 * @note 此函数为非阻塞模式，立即返回结果
 */
uint8_t IAP_CAN_ReceiveByte(uint8_t *data);

/**
 * @brief 发送数据
 * @param data 要发送的数据缓冲区
 * @param length 要发送的数据长度
 * @note 用户需要实现此函数，发送多个字节到CAN
 */
void IAP_CAN_SendData(uint8_t *data, uint16_t length);

/**
 * @brief 接收数据
 * @param buffer 接收数据的缓冲区
 * @param max_length 缓冲区最大长度
 * @param timeout 接收超时时间(ms)
 * @return 实际接收到的数据长度
 * @note 用户需要实现此函数，在指定超时时间内接收数据
 */
uint16_t IAP_CAN_ReceiveData(uint8_t *buffer, uint16_t max_length, uint32_t timeout);

/**
 * @brief 检查缓存中是否有数据可读
 * @return 1-有数据可读，0-无数据
 * @note 检查环形缓存中是否有数据可以读取
 */
uint8_t IAP_CAN_DataAvailable(void);

/**
 * @brief 清空接收缓冲区
 * @note 用户需要实现此函数，清空CAN接收缓冲区
 */
void IAP_CAN_ClearBuffer(void);

/**
 * @brief 接收一个字节并返回状态（兼容旧接口）
 * @param data 用于存储接收数据的指针
 * @param timeout_ms 超时时间(毫秒) - 已弃用，保持接口兼容性
 * @return 0:成功, 1:无数据可用
 * @note 此函数现在为非阻塞模式，timeout_ms参数被忽略
 */
uint8_t IAP_CAN_ReceiveByteWithStatus(uint8_t *data, uint32_t timeout_ms);

/**
 * @brief 从缓存中读取多个字节（非阻塞）
 * @param buffer 用于存储数据的缓冲区
 * @param length 要读取的字节数
 * @return 实际读取的字节数
 */
uint16_t IAP_CAN_ReadBytes(uint8_t *buffer, uint16_t length);

/**
 * @brief 查看缓存中的下一个字节但不移除（非阻塞）
 * @param data 用于存储查看到的数据
 * @return 0:成功, 1:无数据可用
 */
uint8_t IAP_CAN_PeekByte(uint8_t *data);

/**
 * @brief 丢弃缓存中指定数量的字节
 * @param count 要丢弃的字节数
 * @return 实际丢弃的字节数
 */
uint16_t IAP_CAN_DiscardBytes(uint16_t count);

/**
 * @brief 获取缓冲区中的可用数据字节数
 * @return 可用数据字节数
 */
uint16_t IAP_CAN_GetAvailableBytes(void);

/**
 * @brief 检查是否有完整的帧数据准备好
 * @return 1-有完整帧，0-无完整帧
 */
uint8_t IAP_CAN_IsFrameReady(void);

/**
 * @brief 获取帧长度
 * @return 帧长度
 */
uint16_t IAP_CAN_GetFrameLength(void);

void IAP_CAN_RxCompletedCallback(void);

#endif /* __IAP_CAN_H */
