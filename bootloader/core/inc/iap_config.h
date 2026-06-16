#ifndef __IAP_CONFIG_H
#define __IAP_CONFIG_H

#include "stdint.h"
#include "iap_types.h"

/* 选择目标芯片类型 */
#define USE_GD32 1
#define USE_STM32 2
#define MCU_TYPE USE_STM32


/* 如果使用GD32 */
#if MCU_TYPE == USE_GD32
    #define GD32F10X    0
    #define GD32F30X    1
    #define GD32F4XX    0
    #if GD32F10X
        #include "gd32f10x.h"
    #elif GD32F30X
        #include "gd32f30x.h"
    #elif GD32F4XX
        #include "gd32f4xx.h"
    #endif
#endif

#if GD32F30X
    #define FLASH_PAGE_SIZE 0x800 /* 2KB */
    #define VECTOR_TABLE_ALIGNMENT 0x200 // 中断向量表对齐大小, 512字节，（82个事件中断+16个系统中断）*4后向上取2的整数倍
#endif

#if GD32F10X
    #define FLASH_PAGE_SIZE 0x400 /* 1KB */
    #define VECTOR_TABLE_ALIGNMENT 0x200 // 中断向量表对齐大小, 512字节，（82个事件中断+16个系统中断）*4后向上取2的整数倍
#endif

/* 如果使用STM32 */
#if MCU_TYPE == USE_STM32
    #define STM32F10x 1
    #define STM32F4xx 0
    #if STM32F10x
        #include "stm32f1xx.h"
    #elif STM32F4xx
        #include "stm32f4xx.h"
    #endif
#endif
#if STM32F4xx
    #define FLASH_PAGE_SIZE 0x1000       /* 4KB */
    #define VECTOR_TABLE_ALIGNMENT 0x400 // 中断向量表对齐大小, 1024字节，（82个事件中断+16个系统中断）*4后向上取2的整数倍
#endif

#if STM32F10x/*stm32f103vet6每页2kb*/
    #ifndef FLASH_PAGE_SIZE
    #define FLASH_PAGE_SIZE 0x800        /* 2KB,hal自带 */
    #endif
    #define VECTOR_TABLE_ALIGNMENT 0x200 // 中断向量表对齐大小, 512字节，（60个事件中断+16个系统中断）*4后向上取2的整数倍
#endif

/* FLASH配置 */
//#define FLASH_TOTAL_SIZE 0x40000//256KB
#define FLASH_TOTAL_SIZE 0x80000//512KB
#define FLASH_END_ADDR (BOOTLOADER_START_ADDR + FLASH_TOTAL_SIZE)

/* Bootloader配置 */
#define BOOTLOADER_START_ADDR 0x08000000 /* Bootloader起始地址 */
#define BOOTLOADER_SIZE 0x8000           /* Bootloader大小 32KB */

/* 固件包头存储配置 */
#define FIRMWARE_HEADER_SIZE 64                                                /* 固件包头大小 */
#define FIRMWARE_HEADER_ADDR (BOOTLOADER_START_ADDR + BOOTLOADER_SIZE) /* 包头存储地址：Bootloader后APP前 */

/* APP配置 */
#define APPLICATION_START_ADDR (FIRMWARE_HEADER_ADDR + VECTOR_TABLE_ALIGNMENT) /* APP起始地址 */
#define APPLICATION_MAX_SIZE (FLASH_END_ADDR - APPLICATION_START_ADDR)
#define APPLICATION_END_ADDR FLASH_END_ADDR

/* 应用程序升级标志配置 */
#define IAP_UPGRADE_FLAG_ADDR 0x20000000  /* RAM起始地址，不会被初始化 */
#define IAP_UPGRADE_FLAG_VALUE 0x5AA5C33C /* 升级标志值 */
#define IAP_UPGRADE_FLAG_CLEAR 0x00000000 /* 标志清除值 */

/* 串口打印配置 */
#define IAP_LOG_PRINT_ENABLE 1 /* 是否启用UART打印，1=启用，0=禁用 */

/* Ymodem串口配置 */
#if MCU_TYPE == USE_STM32
#define IAP_UART USART1
#define IAP_UART_BAUDRATE 115200
#define IAP_UART_TX_PIN GPIO_PIN_9
#define IAP_UART_RX_PIN GPIO_PIN_10
#define IAP_UART_GPIO_PORT GPIOA
#define IAP_UART_IRQHandler USART1_IRQHandler
#elif MCU_TYPE == USE_GD32
#define IAP_UART USART1
#define IAP_UART_BAUDRATE 115200
#define IAP_UART_TX_PIN GPIO_PIN_2
#define IAP_UART_RX_PIN GPIO_PIN_3
#define IAP_UART_GPIO_PORT GPIOA
#define IAP_UART_IRQHandler USART1_IRQHandler
#endif

/* Ymodem定时器配置 */
#if MCU_TYPE == USE_STM32
#define IAP_TIMER htim1 /* 定时器句柄 */
    #if STM32F4xx
        #define IAP_TIMER_IRQHandler TIM1_UP_TIM10_IRQHandler
        #elif STM32F10x
        #define IAP_TIMER_IRQHandler TIM1_UP_IRQHandler
    #endif
#elif MCU_TYPE == USE_GD32
#define IAP_TIMER TIMER1 /* 定时器 */
#define IAP_TIMER_IRQHandler TIMER1_IRQHandler
#endif

/* Ymodem配置 */
#define YMODEM_CHAR_TIMEOUT     2000   /* 增加到2秒 */
#define YMODEM_PACKET_TIMEOUT   5000   /* 包间超时5秒 */
#define YMODEM_MAX_RETRIES 10   /* 最大重试次数 */
#define YMODEM_PACKET_128 128   /* 128字节数据包 */
#define YMODEM_PACKET_1024 1024 /* 1024字节数据包 */

/* 版本信息 */
#define IAP_VERSION_MAJOR 1 /* 主版本号 */
#define IAP_VERSION_MINOR 0 /* 次版本号 */
#define IAP_VERSION_STRING "IAP Bootloader v1.0"

/* 安全检查配置 */
#define ENABLE_APP_VALIDATION 1     /* 启用应用程序有效性检查 */
#define RAM_START_MASK 0xFFFE0000   /* RAM起始地址掩码 */
#define RAM_START_ADDR 0x20000000   /* RAM起始地址 */
#define FLASH_START_MASK 0xFFF00000 /* Flash起始地址掩码 */
#define FLASH_START_ADDR 0x08000000 /* Flash起始地址 */
#define THUMB_MODE_MASK 0x01        /* Thumb模式掩码 */

/* 固件包头定义 */
#define FIRMWARE_MAGIC_NUMBER 0x12345678 /* 固件包魔术数字 */
#define FIRMWARE_HEADER_SIZE 64          /* 固件包头大小 */
#define FIRMWARE_VERSION 1               /* 固件包版本 */

#endif /* __IAP_CONFIG_H */
