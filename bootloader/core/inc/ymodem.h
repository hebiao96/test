#ifndef __YMODEM_H
#define __YMODEM_H

#include "iap_types.h"

/**
 * @file ymodem.h
 * @brief Ymodem协议接口定义 - 重构版本
 * @version 2.0
 * @date 2025-08-06
 */

/* Ymodem控制字符 */
#define SOH                     0x01  /* 128字节数据包 */
#define STX                     0x02  /* 1024字节数据包 */
#define EOT                     0x04  /* 传输结束 */
#define ACK                     0x06  /* 确认 */
#define NAK                     0x15  /* 否认 */
#define CAN                     0x18  /* 取消 */
#define CTRLZ                   0x1A  /* 文件结束 */

/* =============================================================================
 * 公共接口函数
 * =============================================================================
 */

/**
 * @brief Ymodem协议初始化
 */
void Ymodem_Init(void);

/**
 * @brief 接收文件 - 主要的接收接口
 * @param file_info 文件信息结构体指针
 * @return YmodemStatus 接收状态
 */
YmodemStatus Ymodem_ReceiveFile(YmodemFileInfo *file_info);

/**
 * @brief 发送取消信号
 */
void Ymodem_SendCancel(void);

/* =============================================================================
 * 固件相关函数（用于固件验证和存储）
 * =============================================================================
 */

/**
 * @brief 解析固件包头
 * @param data 包头数据
 * @param header 固件包头结构体
 * @return FirmwareVerifyStatus 解析状态
 */
FirmwareVerifyStatus Ymodem_ParseFirmwareHeader(uint8_t *data, FirmwareHeader *header);

/**
 * @brief 验证固件数据
 * @param start_addr 固件起始地址
 * @param size 固件大小
 * @param expected_md5 期望的MD5值
 * @return FirmwareVerifyStatus 验证状态
 */
FirmwareVerifyStatus Ymodem_VerifyFirmware(uint32_t start_addr, uint32_t size, uint8_t *expected_md5);

/**
 * @brief 擦除固件数据
 * @param start_addr 起始地址
 * @param size 擦除大小
 * @return YmodemStatus 操作状态
 */
YmodemStatus Ymodem_EraseFirmware(uint32_t start_addr, uint32_t size);

/**
 * @brief 保存固件包头到指定扇区
 * @param header 固件包头指针
 * @return YmodemStatus 操作状态
 */
YmodemStatus Ymodem_SaveFirmwareHeader(const FirmwareHeader *header);

#endif /* __YMODEM_H */
