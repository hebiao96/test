/**
 * @file ymodem.c
 * @brief Ymodem协议实现 - 重构版本
 * @version 2.0
 * @date 2025-08-06
 * 
 * 本文件实现了Ymodem协议的接收功能，支持固件传输和验证。
 * 主要特性：
 * - 状态机驱动的接收流程
 * - 自动传输恢复机制
 * - 固件包头验证
 * - MD5校验
 * - 模块化设计
 */

#include "iap_main.h"
#include "string.h"
#include <stdlib.h>

extern volatile uint8_t g_ymodem_busy;

#if IAP_LOG_PRINT_ENABLE
static const char *TAG = "Ymodem";
#endif

/* =============================================================================
 * 私有常量定义
 * =============================================================================
 */

/* CRC16表 - CCITT标准 (x^16 + x^12 + x^5 + 1) */
static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

/* =============================================================================
 * 私有函数声明
 * =============================================================================
 */

/* 基础通信函数 */
static YmodemStatus Ymodem_ReceiveChar(uint8_t *c, uint32_t timeout);
static void Ymodem_SendChar(uint8_t c);
static uint16_t Ymodem_CRC16(const uint8_t *data, uint16_t length);
static void Ymodem_FlushInput(void);

/* 协议处理函数 */
static YmodemStatus Ymodem_ReceivePacket(YmodemContext *ctx);
static YmodemStatus Ymodem_ProcessHeaderPacket(YmodemContext *ctx, YmodemFileInfo *file_info);
static YmodemStatus Ymodem_ProcessDataPacket(YmodemContext *ctx, YmodemFileInfo *file_info);
static YmodemStatus Ymodem_ProcessEOT(YmodemContext *ctx);

/* 文件处理函数 */
static YmodemStatus Ymodem_ParseFileInfo(uint8_t *packet, YmodemFileInfo *file_info);
static YmodemStatus Ymodem_ProcessFirmwareData(YmodemContext *ctx, YmodemFileInfo *file_info, uint16_t data_length);
static YmodemStatus Ymodem_VerifyAndSaveFirmware(YmodemFileInfo *file_info, uint32_t firmware_data_received);

/* 状态管理函数 */
static void Ymodem_InitContext(YmodemContext *ctx);
static void Ymodem_ResetContext(YmodemContext *ctx);
static YmodemStatus Ymodem_HandleTimeout(YmodemContext *ctx, YmodemFileInfo *file_info);
static YmodemStatus Ymodem_HandleError(YmodemContext *ctx, YmodemFileInfo *file_info);

/* =============================================================================
 * 公共接口实现
 * =============================================================================
 */

/**
 * @brief Ymodem协议初始化
 */
void Ymodem_Init(void)
{
    
}

/**
 * @brief 发送取消信号
 */
void Ymodem_SendCancel(void)
{
    Ymodem_SendChar(CAN);
    Ymodem_SendChar(CAN);
    LOGI(TAG, "Cancel signal sent");
}

/**
 * @brief 主要的文件接收函数 - 状态机驱动
 * 
 * @param file_info 文件信息结构体指针
 * @return YmodemStatus 接收状态
 */
YmodemStatus Ymodem_ReceiveFile(YmodemFileInfo *file_info)
{
    YmodemContext ctx;
    YmodemStatus status;
    
    LOGI(TAG, "Starting Ymodem file reception...");
    LOGI(TAG, "Application start address: 0x%08X", APPLICATION_START_ADDR);
    LOGI(TAG, "Max application size: %u bytes", APPLICATION_MAX_SIZE);
    
    /* 初始化上下文和文件信息 */
    Ymodem_InitContext(&ctx);
    file_info->received_size = 0;
    file_info->packet_count = 0;
    file_info->has_firmware_header = 0;
    
    /* 开始传输 - 发送'C'启动CRC模式，禁printf释放CPU */
    LOGI(TAG, "Sending 'C' to start CRC mode transmission...");
    g_ymodem_busy = 1;
    IAP_UART_ClearBuffer();
    Ymodem_SendChar('C');
    ctx.state = YMODEM_STATE_RECEIVING;
    
    /* 主状态机循环 */
    while (ctx.state != YMODEM_STATE_COMPLETE && ctx.state != YMODEM_STATE_ERROR) {
        /* 接收数据包 */
        status = Ymodem_ReceivePacket(&ctx);
        
        switch (status) {
            case YMODEM_OK:
                /* 根据包序号处理数据包 */
                if (ctx.packet_number == 0) {
                    status = Ymodem_ProcessHeaderPacket(&ctx, file_info);
                } else if (ctx.packet_number == ctx.expected_packet) {
                    status = Ymodem_ProcessDataPacket(&ctx, file_info);
                } else if (ctx.packet_number == (ctx.expected_packet - 1)) {
                    /* 重复包，发送ACK */
                    LOGD(TAG, "Duplicate packet %d, sending ACK", ctx.packet_number);
                    Ymodem_FlushInput();
                    Ymodem_SendChar(ACK);
                    status = YMODEM_OK;
                } else {
                    /* 包序号错误 */
                    LOGE(TAG, "Wrong packet number: got %d, expected %d", 
                         ctx.packet_number, ctx.expected_packet);
                    Ymodem_FlushInput();
                    Ymodem_SendChar(NAK);
                    status = YMODEM_ERROR;
                }
                
                /* 处理成功则重置重试计数 */
                if (status == YMODEM_OK) {
                    ctx.retry_count = 0;
                }
                break;
                
            case YMODEM_EOT:
                status = Ymodem_ProcessEOT(&ctx);
                break;
                
            case YMODEM_TIMEOUT:
                status = Ymodem_HandleTimeout(&ctx, file_info);
                break;
                
            case YMODEM_CANCEL:
                LOGE(TAG, "Transmission cancelled by sender");
                g_ymodem_busy = 0;
                return YMODEM_CANCEL;
                
            default:
                status = Ymodem_HandleError(&ctx, file_info);
                break;
        }
        
        /* 检查是否发生错误 */
        if (status == YMODEM_ERROR && ctx.retry_count >= YMODEM_MAX_RETRIES) {
            ctx.state = YMODEM_STATE_ERROR;
        }
    }
    
    /* 返回最终状态 */
    g_ymodem_busy = 0;
    if (ctx.state == YMODEM_STATE_COMPLETE) {
        LOGI(TAG, "Ymodem transmission completed successfully!");
        return YMODEM_OK;
    } else {
        LOGE(TAG, "Ymodem transmission failed!");
        return YMODEM_ERROR;
    }
}

/* =============================================================================
 * 私有函数实现 - 基础通信
 * =============================================================================
 */

/**
 * @brief 发送单个字符
 */
static void Ymodem_SendChar(uint8_t c)
{
    IAP_UART_SendByte(c);
}

/**
 * @brief 接收单个字符
 */
static YmodemStatus Ymodem_ReceiveChar(uint8_t *c, uint32_t timeout)
{
    uint32_t start_time = IAP_Core_GetTick();
    while ((IAP_Core_GetTick() - start_time) < timeout) {
        if (IAP_UART_DataAvailable()) 
        {
            if (IAP_UART_ReceiveByteWithStatus(c, timeout) == 0) 
            {
                return YMODEM_OK;
            } else {
                return YMODEM_TIMEOUT;
            }
        }
    }
    return YMODEM_TIMEOUT;
}

/**
 * @brief 计算CRC16校验值
 */
static uint16_t Ymodem_CRC16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0;
    
    while (length--) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ *data++) & 0xFF];
    }
    
    return crc;
}

/**
 * @brief 清空接收缓冲区
 */
static void Ymodem_FlushInput(void)
{
    IAP_UART_ClearBuffer();
}

/* =============================================================================
 * 私有函数实现 - 协议处理
 * =============================================================================
 */

/**
 * @brief 接收Ymodem数据包
 */
static YmodemStatus Ymodem_ReceivePacket(YmodemContext *ctx)
{
    uint8_t header, pkt_num, pkt_num_comp;
    uint16_t crc_received, crc_calculated;
    YmodemStatus status;
    uint16_t i;
    
    /* 接收包头，跳过0x00噪声字节 */
    {
        uint32_t hdr_start = IAP_Core_GetTick();
        do {
            status = Ymodem_ReceiveChar(&header, 100);
            if (status != YMODEM_OK) {
                if ((IAP_Core_GetTick() - hdr_start) >= YMODEM_CHAR_TIMEOUT) {
                    LOGE(TAG, "Header reception timeout");
                    return YMODEM_TIMEOUT;
                }
                continue;
            }
            if (header == EOT) {
                LOGI(TAG, "Received EOT (End of Transmission)");
                return YMODEM_EOT;
            } else if (header == CAN) {
                LOGE(TAG, "Received CAN (Cancel)");
                return YMODEM_CANCEL;
            } else if (header == SOH || header == STX) {
                break;  /* 有效包头 */
            }
            /* 0x00或其他噪声，跳过继续等 */
        } while (1);
    }
    
    /* 确定包大小 */
    ctx->packet_size = (header == SOH) ? YMODEM_PACKET_128 : YMODEM_PACKET_1024;
    
    /* 接收包序号 */
    status = Ymodem_ReceiveChar(&pkt_num, 500);
    if (status != YMODEM_OK) {
        LOGE(TAG, "Packet number reception failed");
        return status;
    }
    
    status = Ymodem_ReceiveChar(&pkt_num_comp, 500);
    if (status != YMODEM_OK) {
        LOGE(TAG, "Packet number complement reception failed");
        return status;
    }
    
    /* 验证包序号 */
    if (pkt_num != (uint8_t)(~pkt_num_comp)) {
        LOGE(TAG, "Packet number mismatch: pkt=%d, comp=%d", pkt_num, pkt_num_comp);
        return YMODEM_ERROR;
    }
    
    ctx->packet_number = pkt_num;
    
    /* 接收数据 */
    for (i = 0; i < ctx->packet_size; i++) {
        status = Ymodem_ReceiveChar(&ctx->packet_data[i], 200);
        if (status != YMODEM_OK) {
            LOGE(TAG, "Data reception failed at byte %d", i);
            return status;
        }
    }
    
    /* 接收CRC */
    uint8_t crc_h, crc_l;
    status = Ymodem_ReceiveChar(&crc_h, 500);
    if (status != YMODEM_OK) {
        LOGE(TAG, "CRC high byte reception failed");
        return status;
    }
    
    status = Ymodem_ReceiveChar(&crc_l, 500);
    if (status != YMODEM_OK) {
        LOGE(TAG, "CRC low byte reception failed");
        return status;
    }
    
    crc_received = (crc_h << 8) | crc_l;
    
    /* 验证CRC */
    crc_calculated = Ymodem_CRC16(ctx->packet_data, ctx->packet_size);
    if (crc_received != crc_calculated) {
        LOGE(TAG, "CRC mismatch: received=0x%04X, calculated=0x%04X", 
             crc_received, crc_calculated);
        return YMODEM_ERROR;
    }
    
    LOGD(TAG, "Packet #%d OK: size=%d, CRC=0x%04X", pkt_num, ctx->packet_size, crc_received);
    return YMODEM_OK;
}

/**
 * @brief 处理文件头包
 */
static YmodemStatus Ymodem_ProcessHeaderPacket(YmodemContext *ctx, YmodemFileInfo *file_info)
{
    YmodemStatus status;
    
    LOGI(TAG, "Processing file header packet...");
    status = Ymodem_ParseFileInfo(ctx->packet_data, file_info);
    
    if (status == YMODEM_COMPLETE) {
        /* 传输完成 */
        LOGI(TAG, "Transmission completed (empty filename)");
        //IAP_CAN_ClearBuffer();
        IAP_UART_ClearBuffer();
        Ymodem_SendChar(ACK);
        ctx->state = YMODEM_STATE_COMPLETE;
        return YMODEM_OK;
    } else if (status == YMODEM_OK) {
        /* 文件信息解析成功，准备接收数据 */
        LOGI(TAG, "File header accepted, erasing flash and preparing for data...");
        Ymodem_FlushInput();
        
        /* 擦除Flash */
        LOGI(TAG, "Erasing application flash area...");
        uint32_t end_addr = APPLICATION_START_ADDR + file_info->filesize;
        if (IAP_Flash_ErasePages(APPLICATION_START_ADDR, end_addr) != IAP_FLASH_OK) {
            LOGE(TAG, "Flash erase failed");
            Ymodem_FlushInput();
            Ymodem_SendChar(NAK);
            return YMODEM_ERROR;
        }
        LOGI(TAG, "Flash erase completed: %u bytes", file_info->filesize);
        
        /* 重置状态 */
        file_info->received_size = 0;
        file_info->packet_count = 0;
        file_info->has_firmware_header = 0;
        ctx->write_addr = APPLICATION_START_ADDR;
        ctx->firmware_data_received = 0;
        
        Ymodem_FlushInput();
        Ymodem_SendChar(ACK);
        Ymodem_SendChar('C');
        ctx->expected_packet = 1;
        return YMODEM_OK;
    } else {
        LOGE(TAG, "File header parsing failed");
        Ymodem_FlushInput();
        Ymodem_SendChar(NAK);
        return YMODEM_ERROR;
    }
}

/**
 * @brief 处理数据包
 */
static YmodemStatus Ymodem_ProcessDataPacket(YmodemContext *ctx, YmodemFileInfo *file_info)
{
    uint16_t data_length = (file_info->received_size + ctx->packet_size > file_info->filesize) ?
                          (file_info->filesize - file_info->received_size) : ctx->packet_size;
    
    LOGD(TAG, "Processing data packet %d: data_length=%d", ctx->packet_number, data_length);
    
    /* 处理固件数据 */
    YmodemStatus status = Ymodem_ProcessFirmwareData(ctx, file_info, data_length);
    if (status != YMODEM_OK) {
        return status;
    }
    
    /* 更新接收状态 */
    file_info->received_size += data_length;
    file_info->packet_count++;
    ctx->expected_packet++;
    
    LOGD(TAG, "Packet processed: received=%u/%u bytes, packets=%u",
         file_info->received_size, file_info->filesize, file_info->packet_count);
    
    Ymodem_FlushInput();
    Ymodem_SendChar(ACK);
    
    /* 检查是否接收完成 */
    if (file_info->received_size >= file_info->filesize) {
        return Ymodem_VerifyAndSaveFirmware(file_info, ctx->firmware_data_received);
    }
    
    return YMODEM_OK;
}

/**
 * @brief 处理EOT信号
 */
static YmodemStatus Ymodem_ProcessEOT(YmodemContext *ctx)
{
    LOGI(TAG, "Received EOT, sending ACK and waiting for final packet");
    Ymodem_FlushInput();
    Ymodem_SendChar(ACK);
    Ymodem_SendChar('C');
    ctx->expected_packet = 0;
    ctx->retry_count = 0;
    return YMODEM_OK;
}

/* =============================================================================
 * 私有函数实现 - 文件处理
 * =============================================================================
 */

/**
 * @brief 解析文件头信息
 */
static YmodemStatus Ymodem_ParseFileInfo(uint8_t *packet, YmodemFileInfo *file_info)
{
    char *filename_ptr = (char*)packet;
    char *filesize_ptr;
    
    LOGD(TAG, "Parsing file info packet...");
    
    /* 检查是否为空文件名（传输结束） */
    if (strlen(filename_ptr) == 0) {
        LOGI(TAG, "Empty filename - transmission complete");
        return YMODEM_COMPLETE;
    }
    
    /* 解析文件名 */
    strncpy(file_info->filename, filename_ptr, sizeof(file_info->filename) - 1);
    file_info->filename[sizeof(file_info->filename) - 1] = '\0';
    LOGI(TAG, "Filename: %s", file_info->filename);
    
    /* 解析文件大小 */
    filesize_ptr = filename_ptr + strlen(filename_ptr) + 1;
    LOGI(TAG, "Filesize str: '%s'", filesize_ptr);
    
    /* 手动解析文件大小，避免 atol 在嵌入式环境中的问题 */
    file_info->filesize = 0;
    {
        char *p = filesize_ptr;
        while (*p >= '0' && *p <= '9') {
            file_info->filesize = file_info->filesize * 10 + (uint32_t)(*p - '0');
            p++;
        }
    }
    LOGI(TAG, "File size: %u bytes", file_info->filesize);
    
    /* 验证文件大小 */
    if (file_info->filesize == 0) {
        LOGE(TAG, "Invalid file size: 0");
        return YMODEM_ERROR;
    }
    
    if (file_info->filesize > APPLICATION_MAX_SIZE) {
        LOGE(TAG, "File too large: %u > %u bytes", 
             file_info->filesize, APPLICATION_MAX_SIZE);
        return YMODEM_ERROR;
    }
    
    /* 初始化接收状态 */
    file_info->received_size = 0;
    file_info->packet_count = 0;
    
    LOGI(TAG, "File info parsed successfully");
    return YMODEM_OK;
}

/**
 * @brief 处理固件数据
 */
static YmodemStatus Ymodem_ProcessFirmwareData(YmodemContext *ctx, YmodemFileInfo *file_info, uint16_t data_length)
{
    /* 检查第一个数据包是否包含固件包头 */
    if (ctx->packet_number == 1 && !file_info->has_firmware_header) {
        LOGD(TAG, "First data packet - checking firmware header...");
        
        if (data_length < FIRMWARE_HEADER_SIZE) {
            LOGE(TAG, "Packet too small for firmware header: %d < %d", 
                 data_length, FIRMWARE_HEADER_SIZE);
            Ymodem_SendChar(CAN);
            Ymodem_SendChar(CAN);
            Ymodem_FlushInput();
            return YMODEM_ERROR;
        }
        
        /* 解析固件包头 */
        FirmwareVerifyStatus verify_status = Ymodem_ParseFirmwareHeader(ctx->packet_data, &file_info->firmware_header);
        
        if (verify_status == FIRMWARE_VERIFY_OK) {
            file_info->has_firmware_header = 1;
            LOGI(TAG, "Firmware header parsed successfully");
            LOGI(TAG, "Firmware size: %u bytes", file_info->firmware_header.firmware_size);
            LOGI(TAG, "Start address: 0x%08X", file_info->firmware_header.start_address);
            
            /* 跳过包头，写入固件数据 */
            uint8_t *firmware_data = ctx->packet_data + FIRMWARE_HEADER_SIZE;
            uint16_t firmware_data_len = data_length - FIRMWARE_HEADER_SIZE;
            
            if (firmware_data_len > 0) {
                if (IAP_Flash_WriteData(ctx->write_addr, firmware_data, firmware_data_len) == IAP_FLASH_OK) {
                    ctx->firmware_data_received += firmware_data_len;
                    ctx->write_addr += firmware_data_len;
                    LOGD(TAG, "Flash write successful: %d bytes written", firmware_data_len);
                } else {
                    LOGE(TAG, "Flash write failed at 0x%08X", ctx->write_addr);
                    return YMODEM_ERROR;
                }
            }
        } else {
            LOGE(TAG, "Firmware header parsing failed: status=%d", verify_status);
            return YMODEM_ERROR;
        }
    } else {
        /* 后续数据包，直接写入Flash */
        LOGD(TAG, "Writing data packet: addr=0x%08X, len=%d", ctx->write_addr, data_length);
        
        if (IAP_Flash_WriteData(ctx->write_addr, ctx->packet_data, data_length) == IAP_FLASH_OK) {
            ctx->firmware_data_received += data_length;
            ctx->write_addr += data_length;
            LOGD(TAG, "Flash write successful: %d bytes written", data_length);
        } else {
            LOGE(TAG, "Flash write failed at 0x%08X", ctx->write_addr);
            return YMODEM_ERROR;
        }
    }
    
    return YMODEM_OK;
}

/**
 * @brief 验证并保存固件
 */
static YmodemStatus Ymodem_VerifyAndSaveFirmware(YmodemFileInfo *file_info, uint32_t firmware_data_received)
{
    LOGI(TAG, "File reception completed, starting verification...");
    LOGI(TAG, "Total firmware data received: %u bytes", firmware_data_received);
    
    /* 验证固件MD5 */
    LOGI(TAG, "Verifying firmware MD5...");
    FirmwareVerifyStatus verify_status = Ymodem_VerifyFirmware(
        APPLICATION_START_ADDR, 
        file_info->firmware_header.firmware_size, 
        file_info->firmware_header.firmware_md5
    );
    
    if (verify_status != FIRMWARE_VERIFY_OK) {
        LOGE(TAG, "Firmware verification failed: status=%d", verify_status);
        LOGI(TAG, "Erasing firmware due to verification failure...");
        Ymodem_EraseFirmware(APPLICATION_START_ADDR, firmware_data_received);
        return YMODEM_ERROR;
    }
    
    LOGI(TAG, "Firmware verification successful!");
    
    /* 保存固件包头 */
    LOGI(TAG, "Saving firmware header...");
    if (Ymodem_SaveFirmwareHeader(&file_info->firmware_header) != YMODEM_OK) {
        LOGE(TAG, "Firmware header save failed (non-critical)");
    } else {
        LOGI(TAG, "Firmware header saved successfully");
    }
    
    LOGI(TAG, "Firmware reception and verification completed, waiting for EOT...");
    return YMODEM_OK;
}

/* =============================================================================
 * 私有函数实现 - 状态管理
 * =============================================================================
 */

/**
 * @brief 初始化Ymodem上下文
 */
static void Ymodem_InitContext(YmodemContext *ctx)
{
    ctx->state = YMODEM_STATE_IDLE;
    ctx->expected_packet = 1;
    ctx->retry_count = 0;
    ctx->write_addr = APPLICATION_START_ADDR;
    ctx->firmware_data_received = 0;
    ctx->packet_number = 0;
    ctx->packet_size = 0;
}

/**
 * @brief 重置Ymodem上下文（用于重启传输）
 */
static void Ymodem_ResetContext(YmodemContext *ctx)
{
    ctx->state = YMODEM_STATE_RECEIVING;
    ctx->expected_packet = 1;
    ctx->retry_count = 0;
    ctx->write_addr = APPLICATION_START_ADDR;
    ctx->firmware_data_received = 0;
}

/**
 * @brief 处理超时
 */
static YmodemStatus Ymodem_HandleTimeout(YmodemContext *ctx, YmodemFileInfo *file_info)
{
    ctx->retry_count++;
    LOGE(TAG, "Timeout occurred (retry %u/%u)", ctx->retry_count, YMODEM_MAX_RETRIES);
    
    if (ctx->retry_count >= YMODEM_MAX_RETRIES) {
        LOGE(TAG, "Max retries reached, cancelling and restarting");
        Ymodem_FlushInput();
        Ymodem_SendCancel();
        
        /* 重启传输 */
        Ymodem_ResetContext(ctx);
        file_info->received_size = 0;
        file_info->packet_count = 0;
        file_info->has_firmware_header = 0;
        
        LOGI(TAG, "Restarting Ymodem file reception...");
        Ymodem_FlushInput();
        Ymodem_SendChar('C');
        return YMODEM_OK;
    } else {
        /* 超时重试：发送 NAK 请求发送方重传当前包 */
        LOGD(TAG, "Timeout, sending NAK to request retransmission of packet %d", ctx->expected_packet);
        Ymodem_FlushInput();
        Ymodem_SendChar(NAK);
        return YMODEM_OK;
    }
}

/**
 * @brief 处理错误
 */
static YmodemStatus Ymodem_HandleError(YmodemContext *ctx, YmodemFileInfo *file_info)
{
    ctx->retry_count++;
    LOGE(TAG, "Reception error (retry %u/%u)", ctx->retry_count, YMODEM_MAX_RETRIES);
    
    if (ctx->retry_count >= YMODEM_MAX_RETRIES) {
        LOGE(TAG, "Max retries reached, cancelling and restarting");
        Ymodem_FlushInput();
        Ymodem_SendCancel();
        
        /* 重启传输 */
        Ymodem_ResetContext(ctx);
        file_info->received_size = 0;
        file_info->packet_count = 0;
        file_info->has_firmware_header = 0;
        
        LOGI(TAG, "Restarting Ymodem file reception...");
        Ymodem_FlushInput();
        Ymodem_SendChar('C');
        return YMODEM_OK;
    } else {
        LOGD(TAG, "Sending NAK to retry");
        Ymodem_FlushInput();
        Ymodem_SendChar(NAK);
        return YMODEM_OK;
    }
}

/* =============================================================================
 * 外部函数实现（来自原代码，保持兼容性）
 * =============================================================================
 */

/**
 * @brief 保存固件包头到指定扇区
 * @param header 固件包头指针
 * @return YMODEM_OK=成功，YMODEM_ERROR=失败
 */
YmodemStatus Ymodem_SaveFirmwareHeader(const FirmwareHeader *header)
{
    if (IAP_Flash_WriteData(FIRMWARE_HEADER_ADDR, (uint8_t*)header, FIRMWARE_HEADER_SIZE) != IAP_FLASH_OK) {
        LOGE(TAG, "Failed to write firmware header to flash");
        return YMODEM_ERROR;
    }
    return YMODEM_OK;
}

/**
 * @brief 解析固件包头
 */
FirmwareVerifyStatus Ymodem_ParseFirmwareHeader(uint8_t *data, FirmwareHeader *header)
{
    /* 验证魔术数字 */
    uint32_t magic = *(uint32_t*)(data + 0);
    if (magic != FIRMWARE_MAGIC_NUMBER) {
        return FIRMWARE_VERIFY_NOT_PKG_FORMAT;
    }
    
    /* 解析包头数据 */
    header->magic_number = magic;
    header->version = *(uint32_t*)(data + 4);
    header->firmware_size = *(uint32_t*)(data + 8);
    
    /* 复制MD5字段 */
    memcpy(header->firmware_md5, data + 12, 16);
    
    header->start_address = *(uint32_t*)(data + 28);
    header->build_time = *(uint32_t*)(data + 32);
    
    /* 复制字符串字段 */
    memcpy(header->version_string, data + 36, 16);
    memcpy(header->target_chip, data + 52, 8);
    memcpy(header->reserved, data + 60, 4);
    
    /* 验证版本 */
    if (header->version != FIRMWARE_VERSION) {
        return FIRMWARE_VERIFY_VERSION_ERROR;
    }
    
    /* 验证起始地址 */
    if (header->start_address != APPLICATION_START_ADDR) {
        return FIRMWARE_VERIFY_ADDRESS_ERROR;
    }
    
    /* 验证固件大小 */
    if (header->firmware_size == 0 || header->firmware_size > APPLICATION_MAX_SIZE) {
        return FIRMWARE_VERIFY_SIZE_ERROR;
    }
    
    return FIRMWARE_VERIFY_OK;
}

/**
 * @brief 验证固件数据
 */
FirmwareVerifyStatus Ymodem_VerifyFirmware(uint32_t start_addr, uint32_t size, uint8_t *expected_md5)
{
    uint8_t calculated_md5[16];
    uint32_t i;
    
    /* 计算固件MD5 */
    MD5_CTX context;
    MD5_Init(&context);
    
    /* 分块计算MD5以节省内存 */
    const uint32_t block_size = 256;
    uint8_t block_buffer[256];
    
    for (i = 0; i < size; i += block_size) {
        uint32_t current_block_size = (i + block_size > size) ? (size - i) : block_size;
        uint32_t j;
        
        /* 从Flash读取数据到缓冲区 */
        for (j = 0; j < current_block_size; j++) {
            block_buffer[j] = *(uint8_t*)(start_addr + i + j);
        }
        
        MD5_Update(&context, block_buffer, current_block_size);
    }
    
    MD5_Final(calculated_md5, &context);
    
    /* 比较MD5 */
    for (i = 0; i < 16; i++) {
        if (calculated_md5[i] != expected_md5[i]) {
            return FIRMWARE_VERIFY_FIRMWARE_MD5_ERROR;
        }
    }
    
    return FIRMWARE_VERIFY_OK;
}

/**
 * @brief 擦除固件数据
 */
YmodemStatus Ymodem_EraseFirmware(uint32_t start_addr, uint32_t size)
{
    uint32_t end_addr = start_addr + size;
    uint32_t current_addr;
    
    /* 按页擦除固件区域 */
    for (current_addr = start_addr; current_addr < end_addr; current_addr += FLASH_PAGE_SIZE) {
        if (IAP_Flash_ErasePage(current_addr) != IAP_FLASH_OK) {
            return YMODEM_ERROR;
        }
    }
    
    return YMODEM_OK;
}
