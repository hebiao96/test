/**
 * @file iap_bootloader.c
 * @brief IAP Bootloader主控制模块 - 重构版本
 * @version 2.0
 * @date 2025-08-06
 * 
 * 本文件实现了IAP Bootloader的主要控制逻辑，包括：
 * - 模块初始化和去初始化
 * - 应用程序检查和跳转
 * - Ymodem固件升级流程
 * - 升级标志处理
 * - 状态机驱动的主循环
 */

#include "iap_main.h"
#include <stdio.h>
#include <string.h>

#if IAP_LOG_PRINT_ENABLE
static const char *TAG = "IAP_Bootloader";
#endif

/* =============================================================================
 * 私有常量定义
 * =============================================================================
 */

/* Bootloader状态定义 */
typedef enum {
    BOOTLOADER_STATE_INIT,          /* 初始化状态 */
    BOOTLOADER_STATE_CHECK_UPGRADE, /* 检查升级标志 */
    BOOTLOADER_STATE_CHECK_APP,     /* 检查应用程序 */
    BOOTLOADER_STATE_UPGRADE,       /* 执行升级 */
    BOOTLOADER_STATE_JUMP_APP,      /* 跳转到应用程序 */
    BOOTLOADER_STATE_ERROR          /* 错误状态 */
} BootloaderState;

/* 升级结果定义 */
typedef enum {
    UPGRADE_RESULT_SUCCESS,         /* 升级成功 */
    UPGRADE_RESULT_FAILED,          /* 升级失败 */
    UPGRADE_RESULT_CANCELLED        /* 升级取消 */
} UpgradeResult;

/* 重试计数器 */
#define MAX_UPGRADE_RETRIES         3
#define UPGRADE_RETRY_DELAY_MS      2000
#define APP_JUMP_DELAY_MS          100
#define POST_UPGRADE_DELAY_MS      1000

/* =============================================================================
 * 私有函数声明
 * =============================================================================
 */

/* 状态处理函数 */
static BootloaderState IAP_Bootloader_HandleInit(void);
static BootloaderState IAP_Bootloader_HandleCheckUpgrade(void);
static BootloaderState IAP_Bootloader_HandleCheckApp(void);
static BootloaderState IAP_Bootloader_HandleUpgrade(uint32_t *retry_count);
static BootloaderState IAP_Bootloader_HandleJumpApp(void);

/* 辅助函数 */
static void IAP_Bootloader_PrintBanner(void);
static void IAP_Bootloader_PrintSystemInfo(void);
static UpgradeResult IAP_Bootloader_PerformUpgrade(void);
static void IAP_Bootloader_HandleUpgradeResult(UpgradeResult result);

/* =============================================================================
 * 公共接口实现
 * =============================================================================
 */

/**
 * @brief 初始化所有IAP模块
 */
void IAP_Bootloader_Init(void)
{
    LOGI(TAG, "Initializing IAP Bootloader modules...");
    
    /* 按照依赖关系顺序初始化各个功能模块 */
    IAP_Flash_Init();
     IAP_UART_Init(); // 已在IAP_CAN_Init中初始化
    //IAP_CAN_Init();   //stm32f103采用串口
    IAP_TIMER_Init();
    Ymodem_Init();
    
    LOGI(TAG, "IAP Bootloader initialization completed");
}

/**
 * @brief 去初始化所有IAP模块
 */
void IAP_Bootloader_DeInit(void)
{
    LOGI(TAG, "De-initializing IAP Bootloader modules...");

    /* 按照初始化的逆序进行去初始化，UART 最后反初始化以保证日志能完整输出 */
    IAP_TIMER_DeInit();
    IAP_Flash_DeInit();

    LOGI(TAG, "IAP Bootloader de-initialization completed");

    IAP_UART_DeInit();  /* UART 必须最后反初始化 */
}

/**
 * @brief Ymodem固件升级
 * @return 1=成功，0=失败
 */
uint8_t IAP_Bootloader_YmodemUpdate(void)
{
    UpgradeResult result = IAP_Bootloader_PerformUpgrade();
    IAP_Bootloader_HandleUpgradeResult(result);
    
    return (result == UPGRADE_RESULT_SUCCESS) ? 1 : 0;
}

/**
 * @brief Bootloader主循环 - 状态机驱动
 */
void IAP_Bootloader_MainLoop(void)
{
    BootloaderState state = BOOTLOADER_STATE_INIT;
    uint32_t upgrade_retry_count = 0;
    
    LOGI(TAG, "Starting IAP Bootloader main loop...");
    
    /* 打印启动横幅 */
    IAP_Bootloader_PrintBanner();
    
    /* 主状态机循环 */
    while (1) {
        LOGD(TAG, "=== Bootloader State: %d ===", state);
        
        switch (state) {
            case BOOTLOADER_STATE_INIT:
                state = IAP_Bootloader_HandleInit();
                break;
                
            case BOOTLOADER_STATE_CHECK_UPGRADE:
                state = IAP_Bootloader_HandleCheckUpgrade();
                break;
                
            case BOOTLOADER_STATE_CHECK_APP:
                state = IAP_Bootloader_HandleCheckApp();
                break;
                
            case BOOTLOADER_STATE_UPGRADE:
                state = IAP_Bootloader_HandleUpgrade(&upgrade_retry_count);
                break;
                
            case BOOTLOADER_STATE_JUMP_APP:
                state = IAP_Bootloader_HandleJumpApp();
                break;
                
            case BOOTLOADER_STATE_ERROR:
            default:
                LOGE(TAG, "Bootloader entered error state, restarting...");
                IAP_Core_Delay_ms(UPGRADE_RETRY_DELAY_MS);
                state = BOOTLOADER_STATE_INIT;
                upgrade_retry_count = 0;
                break;
        }
    }
}

/* =============================================================================
 * 私有函数实现 - 状态处理
 * =============================================================================
 */

/**
 * @brief 处理初始化状态
 */
static BootloaderState IAP_Bootloader_HandleInit(void)
{
    LOGI(TAG, "Bootloader initialization completed");
    IAP_Bootloader_PrintSystemInfo();
    
    /* 转到检查升级标志状态 */
    return BOOTLOADER_STATE_CHECK_UPGRADE;
}

/**
 * @brief 处理检查升级标志状态
 */
static BootloaderState IAP_Bootloader_HandleCheckUpgrade(void)
{
    if (IAP_Core_CheckUpgradeFlag()) {
        LOGI(TAG, "Application upgrade flag detected!");
        
        /* 清除升级标志 */
        IAP_Core_ClearUpgradeFlag();
        LOGI(TAG, "Upgrade flag cleared, starting upgrade process...");
        
        /* 转到升级状态 */
        return BOOTLOADER_STATE_UPGRADE;
    } else {
        LOGD(TAG, "No upgrade flag detected, checking application...");
        
        /* 转到检查应用程序状态 */
        return BOOTLOADER_STATE_CHECK_APP;
    }
}

/**
 * @brief 处理检查应用程序状态
 */
static BootloaderState IAP_Bootloader_HandleCheckApp(void)
{
    if (IAP_Core_CheckApplication()) {
        LOGI(TAG, "Valid application found");
        
        /* 转到跳转应用程序状态 */
        return BOOTLOADER_STATE_JUMP_APP;
    } else {
        LOGI(TAG, "No valid application found, entering upgrade mode...");
        
        /* 转到升级状态 */
        return BOOTLOADER_STATE_UPGRADE;
    }
}

/**
 * @brief 处理升级状态
 */
static BootloaderState IAP_Bootloader_HandleUpgrade(uint32_t *retry_count)
{
    UpgradeResult result;
    
    LOGI(TAG, "Starting Ymodem upgrade (attempt %u/%u)...", 
         *retry_count + 1, MAX_UPGRADE_RETRIES);
    
    /* 执行升级 */
    result = IAP_Bootloader_PerformUpgrade();
    IAP_Bootloader_HandleUpgradeResult(result);
    
    if (result == UPGRADE_RESULT_SUCCESS) {
        LOGI(TAG, "Upgrade completed successfully!");
        
        /* 延时后检查应用程序 */
        IAP_Core_Delay_ms(POST_UPGRADE_DELAY_MS);
        *retry_count = 0;
        return BOOTLOADER_STATE_CHECK_APP;
        
    } else if (result == UPGRADE_RESULT_CANCELLED) {
        LOGI(TAG, "Upgrade cancelled by user");
        
        /* 重置重试计数，转到检查应用程序 */
        *retry_count = 0;
        return BOOTLOADER_STATE_CHECK_APP;
        
    } else {
        /* 升级失败 */
        (*retry_count)++;
        
        if (*retry_count >= MAX_UPGRADE_RETRIES) {
            LOGE(TAG, "Max upgrade retries reached (%u), checking for existing app...", 
                 MAX_UPGRADE_RETRIES);
            *retry_count = 0;
            return BOOTLOADER_STATE_CHECK_APP;
        } else {
            LOGE(TAG, "Upgrade failed, retrying in %u ms... (attempt %u/%u)", 
                 UPGRADE_RETRY_DELAY_MS, *retry_count + 1, MAX_UPGRADE_RETRIES);
            IAP_Core_Delay_ms(UPGRADE_RETRY_DELAY_MS);
            return BOOTLOADER_STATE_UPGRADE;
        }
    }
}

/**
 * @brief 处理跳转应用程序状态
 */
static BootloaderState IAP_Bootloader_HandleJumpApp(void)
{
    LOGI(TAG, "Jumping to application...");
    IAP_Core_Delay_ms(APP_JUMP_DELAY_MS);
    
    /* 跳转到应用程序 - 此函数不应返回 */
    IAP_Core_JumpToApplication();
    
    /* 如果跳转失败，返回错误状态 */
    LOGE(TAG, "Failed to jump to application!");
    return BOOTLOADER_STATE_ERROR;
}

/* =============================================================================
 * 私有函数实现 - 辅助函数
 * =============================================================================
 */

/**
 * @brief 打印启动横幅
 */
static void IAP_Bootloader_PrintBanner(void)
{
    LOG_PRINTF("\r\n");
    LOG_PRINTF("*************************************************\r\n");
    LOG_PRINTF("*           %s            *\r\n", IAP_VERSION_STRING);
    LOG_PRINTF("*    Ready for firmware upgrade via Ymodem     *\r\n");
    LOG_PRINTF("*************************************************\r\n");
}

/**
 * @brief 打印系统信息
 */
static void IAP_Bootloader_PrintSystemInfo(void)
{
    LOG_PRINTF("Boot Address: 0x%08X\r\n", (unsigned int)BOOTLOADER_START_ADDR);
    LOG_PRINTF("App Address:  0x%08X\r\n", (unsigned int)APPLICATION_START_ADDR);
    LOG_PRINTF("App End:      0x%08X\r\n", (unsigned int)APPLICATION_END_ADDR);
    LOG_PRINTF("Flash Size:   %uKB\r\n", (unsigned int)(FLASH_TOTAL_SIZE / 1024));
    LOG_PRINTF("App Max Size: %uKB\r\n", (unsigned int)(APPLICATION_MAX_SIZE / 1024));
    LOG_PRINTF("-------------------------------------------------\r\n");
    
    LOGD(TAG, "System information displayed");
}

/**
 * @brief 执行Ymodem升级
 */
static UpgradeResult IAP_Bootloader_PerformUpgrade(void)
{
    YmodemFileInfo file_info;
    YmodemStatus status;
    
    /* 发送升级提示信息 */
    LOGI(TAG, "Ready for Ymodem transfer...");
    LOGD(TAG, "APP_START: 0x%08X, APP_END: 0x%08X", 
         (unsigned int)APPLICATION_START_ADDR, (unsigned int)APPLICATION_END_ADDR);
    
    /* 执行Ymodem接收 */
    status = Ymodem_ReceiveFile(&file_info);
    
    /* 根据状态返回结果 */
    switch (status) {
        case YMODEM_OK:
            LOGI(TAG, "Firmware update completed successfully!");
            LOGI(TAG, "Updated file: %s (%u bytes)", 
                 file_info.filename, file_info.filesize);
            return UPGRADE_RESULT_SUCCESS;
            
        case YMODEM_CANCEL:
            LOGI(TAG, "Firmware update cancelled by sender");
            return UPGRADE_RESULT_CANCELLED;
            
        default:
            LOGE(TAG, "Firmware update failed with status: %d", status);
            return UPGRADE_RESULT_FAILED;
    }
}

/**
 * @brief 处理升级结果
 */
static void IAP_Bootloader_HandleUpgradeResult(UpgradeResult result)
{
    switch (result) {
        case UPGRADE_RESULT_SUCCESS:
            LOGI(TAG, "=== UPGRADE SUCCESS ===");
            break;
            
        case UPGRADE_RESULT_CANCELLED:
            LOGI(TAG, "=== UPGRADE CANCELLED ===");
            break;
            
        case UPGRADE_RESULT_FAILED:
        default:
            LOGE(TAG, "=== UPGRADE FAILED ===");
            break;
    }
}
