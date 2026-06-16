#ifndef __IAP_TYPES_H
#define __IAP_TYPES_H

#include <stdint.h>

/**
 * @file iap_types.h
 * @brief IAP 公共类型定义
 * 
 * 本文件定义了IAP系统中使用的公共类型定义，避免循环依赖。
 */

/* Flash操作结果 */
#ifndef IAP_FLASH_STATUS_DEFINED
#define IAP_FLASH_STATUS_DEFINED
typedef enum {
    IAP_FLASH_OK = 0,
    IAP_FLASH_ERROR,
    IAP_FLASH_TIMEOUT,
    IAP_FLASH_WRITE_PROTECTION
} IAP_Flash_Status;
#endif

/* IAP核心功能结果 */
#ifndef IAP_RESULT_DEFINED
#define IAP_RESULT_DEFINED
typedef enum {
    IAP_RESULT_OK = 0,
    IAP_RESULT_ERROR,
    IAP_RESULT_INVALID_APP,
    IAP_RESULT_FLASH_ERROR,
    IAP_RESULT_UPDATE_FAILED
} IAP_Result;
#endif

/* IAP Bootloader功能结果 */
#ifndef IAP_BOOT_RESULT_DEFINED
#define IAP_BOOT_RESULT_DEFINED
typedef enum {
    IAP_BOOT_RESULT_JUMP_APP = 0,      /* 跳转到应用程序 */
    IAP_BOOT_RESULT_UPDATE_MODE,       /* 进入升级模式 */
    IAP_BOOT_RESULT_UPDATE_SUCCESS,    /* 升级成功 */
    IAP_BOOT_RESULT_UPDATE_FAILED      /* 升级失败 */
} IAP_BootResult;
#endif

/* Ymodem状态 */
#ifndef YMODEM_STATUS_DEFINED
#define YMODEM_STATUS_DEFINED
typedef enum {
    YMODEM_OK = 0,
    YMODEM_ERROR,
    YMODEM_TIMEOUT,       
    YMODEM_CANCEL,
    YMODEM_ABORT,
    YMODEM_COMPLETE,      /* 传输完成状态 */
    YMODEM_EOT            /* 接收到EOT信号 */
} YmodemStatus;
#endif

/* Ymodem接收状态 */
#ifndef YMODEM_STATE_DEFINED
#define YMODEM_STATE_DEFINED
typedef enum {
    YMODEM_STATE_IDLE = 0,
    YMODEM_STATE_RECEIVING,
    YMODEM_STATE_COMPLETE,
    YMODEM_STATE_ERROR
} YmodemState;
#endif

/* 固件校验状态 */
#ifndef FIRMWARE_VERIFY_STATUS_DEFINED
#define FIRMWARE_VERIFY_STATUS_DEFINED
typedef enum {
    FIRMWARE_VERIFY_OK = 0,
    FIRMWARE_VERIFY_MAGIC_ERROR,
    FIRMWARE_VERIFY_VERSION_ERROR,
    FIRMWARE_VERIFY_HEADER_ERROR,
    FIRMWARE_VERIFY_FIRMWARE_MD5_ERROR,
    FIRMWARE_VERIFY_SIZE_ERROR,
    FIRMWARE_VERIFY_ADDRESS_ERROR,
    FIRMWARE_VERIFY_NOT_PKG_FORMAT
} FirmwareVerifyStatus;
#endif

/* 固件包头结构定义 */
#ifndef FIRMWARE_HEADER_DEFINED
#define FIRMWARE_HEADER_DEFINED
typedef struct {
    uint32_t magic_number;      /* 偏移0: 魔术数字 */
    uint32_t version;           /* 偏移4: 版本号 */
    uint32_t firmware_size;     /* 偏移8: 固件大小 */
    uint8_t firmware_md5[16];   /* 偏移12: 固件MD5校验值 */
    uint32_t start_address;     /* 偏移28: 起始地址 */
    uint32_t build_time;        /* 偏移32: 构建时间 */
    char version_string[16];    /* 偏移36: 版本字符串 */
    char target_chip[8];        /* 偏移52: 目标芯片 */
    uint8_t reserved[4];        /* 偏移60: 保留字段 */
} __attribute__((packed)) FirmwareHeader;
#endif

/* 文件信息结构 */
#ifndef YMODEM_FILE_INFO_DEFINED
#define YMODEM_FILE_INFO_DEFINED
typedef struct {
    char filename[256];
    uint32_t filesize;
    uint32_t received_size;
    uint32_t packet_count;
    uint8_t *data_buffer;
    uint32_t crc_check;
    FirmwareHeader firmware_header;  /* 固件包头信息 */
    uint8_t has_firmware_header;     /* 是否包含固件包头 */
} YmodemFileInfo;
#endif

/* Ymodem接收上下文结构体 */
#ifndef YMODEM_CONTEXT_DEFINED
#define YMODEM_CONTEXT_DEFINED
typedef struct {
    YmodemState state;              /* 当前状态 */
    uint8_t expected_packet;        /* 期望的包序号 */
    uint32_t retry_count;           /* 重试计数 */
    uint32_t write_addr;            /* Flash写入地址 */
    uint32_t firmware_data_received; /* 已接收的固件数据量 */
    uint8_t packet_data[1024];      /* 数据包缓冲区 */
    uint8_t packet_number;          /* 当前包序号 */
    uint16_t packet_size;           /* 当前包大小 */
} YmodemContext;
#endif

#endif /* __IAP_TYPES_H */
