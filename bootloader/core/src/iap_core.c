#include "iap_main.h"
#include <stdio.h>
#include <string.h>

#if IAP_LOG_PRINT_ENABLE
static const char *TAG = "IAP_Core";
#endif

/* ============================ 类型定义 ============================ */

/**
 * @Brief应用程序验证信息结构
 */
typedef struct {
    uint32_t stack_pointer;     /* 堆栈指针 */
    uint32_t reset_handler;     /* 重置处理程序功能 */
    uint8_t stack_valid;        /* 堆栈指针有效性 */
    uint8_t reset_valid;        /* 重置处理程序有效性 */
    uint8_t thumb_mode;         /* 拇指模式检查 */
} app_validation_info_t;

/* ============================ 私有函数声明 ============================ */

static IAP_Result iap_core_validate_application_vectors(app_validation_info_t *info);
static IAP_Result iap_core_prepare_jump_environment(void);
static IAP_Result iap_core_validate_firmware_header_address(void);
static void iap_core_display_firmware_information(const FirmwareHeader *header);
static void iap_core_format_md5_string(const uint8_t *md5, char *output, size_t output_size);

/* ============================ 全局变量 ============================ */

static FirmwareHeader g_jump_firmware_header;
/* ============================ 公共函数实现 ============================ */

/**
 * @brief 检查应用程序是否有效
 * @return 1-应用程序有效, 0-应用程序无效
 */
uint8_t IAP_Core_CheckApplication(void)
{
#if ENABLE_APP_VALIDATION
    app_validation_info_t validation_info = {0};
    IAP_Result result;
    
    LOGD(TAG, "Starting application validation...");
    
    /* 验证应用程序向量表 */
    result = iap_core_validate_application_vectors(&validation_info);
    if (result != IAP_RESULT_OK)
    {
        LOGD(TAG, "Application validation failed: SP=0x%08X, ResetHandler=0x%08X", 
             validation_info.stack_pointer, validation_info.reset_handler);
        return 0;
    }
    
    LOGD(TAG, "Application validation successful");
    return 1;
#else
    LOGD(TAG, "Application validation disabled, skipping check");
    return 1;
#endif
}

/**
 * @brief 跳转到应用程序
 * @return 无返回值（如果成功则不会返回）
 */
void IAP_Core_JumpToApplication(void)
{
    IAP_Result result;
    uint32_t app_stack_ptr;
    uint32_t app_reset_handler;
    void (*app_reset_func)(void);
    
    LOGD(TAG, "Preparing to jump to application...");
    
    /* 验证应用程序 */
    if (!IAP_Core_CheckApplication())
    {
        LOGD(TAG, "Application validation failed, canceling jump");
        return;
    }
    
    /* 尝试读取并显示固件信息 */
    if (IAP_Core_ReadFirmwareHeader(&g_jump_firmware_header) == 1)
    {
        LOG_PRINTF("=== Application Firmware Information ===\r\n");
        iap_core_display_firmware_information(&g_jump_firmware_header);
        LOG_PRINTF("==========================================\r\n");
        IAP_Core_Delay_ms(1000); /* 延时1秒让用户看到信息 */
    }
    else
    {
        LOGD(TAG, "No firmware header found, jumping directly...");
    }
    
    /* 获取应用程序向量（在 DeInit 前读取，保证日志能输出） */
    app_stack_ptr = *(volatile uint32_t*)APPLICATION_START_ADDR;
    app_reset_handler = *(volatile uint32_t*)(APPLICATION_START_ADDR + 4);

    LOGD(TAG, "Jump info: SP=0x%08X, ResetHandler=0x%08X",
         app_stack_ptr, app_reset_handler);
    LOGD(TAG, "Jumping to application...");

    /* 准备跳转环境（反初始化外设，UART 最后反初始化） */
    result = iap_core_prepare_jump_environment();
    if (result != IAP_RESULT_OK)
    {
        LOGD(TAG, "Failed to prepare jump environment");
        return;
    }

    /* 设置栈指针 */
    __set_MSP(app_stack_ptr);

    /* 设置中断向量表偏移 */
#if MCU_TYPE == USE_GD32
    SCB->VTOR = APPLICATION_START_ADDR;
#elif MCU_TYPE == USE_STM32
    SCB->VTOR = APPLICATION_START_ADDR;
#endif
    __DSB();
    __ISB();

    /* 执行跳转 */
    app_reset_func = (void (*)(void))app_reset_handler;
    app_reset_func();

    /* 如果执行到这里，说明跳转失败 */
    LOGD(TAG, "Jump failed!");
}

/**
 * @brief 读取固件包头信息
 * @param header 固件包头结构指针
 * @return 1=成功，0=失败
 */
uint8_t IAP_Core_ReadFirmwareHeader(FirmwareHeader *header)
{
    IAP_Result result;
    const uint32_t *flash_addr;
    uint32_t *header_data;
    
    /* 参数验证 */
    if (header == NULL)
    {
        LOGD(TAG, "Firmware header pointer is NULL");
        return 0;
    }
    
    /* 验证固件头地址 */
    result = iap_core_validate_firmware_header_address();
    if (result != IAP_RESULT_OK)
    {
        LOGD(TAG, "Firmware header address validation failed");
        return 0;
    }
    
    /* 清零结构体 */
    memset(header, 0, sizeof(FirmwareHeader));
    
    /* 读取固件头数据 */
    flash_addr = (const uint32_t*)FIRMWARE_HEADER_ADDR;
    header_data = (uint32_t*)header;
    
    LOGD(TAG, "Reading firmware header from address 0x%08X", FIRMWARE_HEADER_ADDR);
    
    /* 按32位字读取，提高效率 */
    for (uint32_t i = 0; i < (FIRMWARE_HEADER_SIZE / sizeof(uint32_t)); i++)
    {
        header_data[i] = flash_addr[i];
    }
    
    /* 验证魔术数字 */
    if (header->magic_number != FIRMWARE_MAGIC_NUMBER)
    {
        LOGD(TAG, "Magic number mismatch: expected=0x%08X, actual=0x%08X", 
             FIRMWARE_MAGIC_NUMBER, header->magic_number);
        return 0;
    }
    
    LOGD(TAG, "Firmware header read successfully, version: %s", header->version_string);
    return 1;
}

/**
 * @brief 打印固件信息
 * @param header 固件包头结构指针
 */
void IAP_Core_PrintFirmwareInfo(const FirmwareHeader *header)
{
    if (header == NULL)
    {
        LOG_PRINTF("Error: Firmware header is NULL\r\n");
        return;
    }
    
    iap_core_display_firmware_information(header);
}

/**
 * @brief 获取系统时钟节拍
 * @return 系统时钟节拍值（毫秒）
 */
uint32_t IAP_Core_GetTick(void)
{
#if MCU_TYPE == USE_STM32
    return HAL_GetTick();
#elif MCU_TYPE == USE_GD32
    return get_Tick();
#else
    /* 如果平台不支持，返回0 */
    LOGD(TAG, "Warning: Current platform does not support system tick");
    return 0;
#endif
}

/**
 * @brief 系统延时函数
 * @param ms 延时时间（毫秒）
 */
void IAP_Core_Delay_ms(uint32_t ms)
{
    if (ms == 0)
    {
        return; /* 0延时直接返回 */
    }
    
#if MCU_TYPE == USE_STM32
    HAL_Delay(ms);
#elif MCU_TYPE == USE_GD32
    delay_1ms(ms);
#else
    /* 如果平台不支持，使用简单的循环延时 */
    LOGD(TAG, "Warning: Using simple loop delay, accuracy may be poor");
    for (volatile uint32_t i = 0; i < (ms * 1000); i++)
    {
        __NOP(); /* 空操作 */
    }
#endif
}

/**
 * @brief 检查应用程序升级标志
 * @return 1: 需要升级, 0: 不需要升级
 */
uint8_t IAP_Core_CheckUpgradeFlag(void)
{
    volatile const uint32_t *flag_addr = (volatile const uint32_t*)IAP_UPGRADE_FLAG_ADDR;
    uint32_t flag_value;
    
    /* 读取升级标志 */
    flag_value = *flag_addr;
    
    LOGD(TAG, "Upgrade flag check: addr=0x%08X, value=0x%08X", 
         IAP_UPGRADE_FLAG_ADDR, flag_value);
    
    /* 检查标志值 */
    if (flag_value == IAP_UPGRADE_FLAG_VALUE)
    {
        LOGD(TAG, "Upgrade flag detected, entering upgrade mode");
        return 1;
    }
    
    LOGD(TAG, "No upgrade flag detected");
    return 0;
}

/**
 * @brief 清除应用程序升级标志
 */
void IAP_Core_ClearUpgradeFlag(void)
{
    volatile uint32_t *flag_addr = (volatile uint32_t*)IAP_UPGRADE_FLAG_ADDR;
    
    LOGD(TAG, "Clearing upgrade flag: addr=0x%08X", IAP_UPGRADE_FLAG_ADDR);
    
    /* 清除标志值 */
    *flag_addr = IAP_UPGRADE_FLAG_CLEAR;
    
    LOGD(TAG, "Upgrade flag cleared");
}

/* ============================ 私有函数实现 ============================ */

/**
 * @brief 验证应用程序向量表
 * @param info 验证信息结构指针
 * @return IAP_Result 验证结果
 */
static IAP_Result iap_core_validate_application_vectors(app_validation_info_t *info)
{
    if (info == NULL)
    {
        return IAP_RESULT_ERROR;
    }
    
    /* 读取向量表信息 */
    info->stack_pointer = *(volatile uint32_t*)APPLICATION_START_ADDR;
    info->reset_handler = *(volatile uint32_t*)(APPLICATION_START_ADDR + 4);
    
    /* 检查栈指针是否在RAM范围内 */
    info->stack_valid = ((info->stack_pointer & RAM_START_MASK) == RAM_START_ADDR);
    if (!info->stack_valid)
    {
        LOGD(TAG, "Stack pointer validation failed: 0x%08X", info->stack_pointer);
        return IAP_RESULT_INVALID_APP;
    }
    
    /* 检查复位向量是否在Flash范围内 */
    info->reset_valid = ((info->reset_handler & FLASH_START_MASK) == FLASH_START_ADDR);
    if (!info->reset_valid)
    {
        LOGD(TAG, "Reset handler address validation failed: 0x%08X", info->reset_handler);
        return IAP_RESULT_INVALID_APP;
    }
    
    /* 检查复位向量是否为奇数（Thumb模式） */
    info->thumb_mode = ((info->reset_handler & THUMB_MODE_MASK) != 0);
    if (!info->thumb_mode)
    {
        LOGD(TAG, "Thumb mode check failed: 0x%08X", info->reset_handler);
        return IAP_RESULT_INVALID_APP;
    }
    
    return IAP_RESULT_OK;
}

/**
 * @brief 准备跳转环境
 * @return IAP_Result 准备结果
 */
static IAP_Result iap_core_prepare_jump_environment(void)
{
    LOGD(TAG, "Deinitializing all peripherals...");
    LOGD(TAG, "Disabling all interrupts...");

    /* 去初始化所有外设 */
    IAP_Bootloader_DeInit();

    /* 关闭所有中断 */
    __disable_irq();

    return IAP_RESULT_OK;
}

/**
 * @brief 验证固件头地址有效性
 * @return IAP_Result 验证结果
 */
static IAP_Result iap_core_validate_firmware_header_address(void)
{
    /* 检查固件头地址是否在有效范围内 */
    if (FIRMWARE_HEADER_ADDR < BOOTLOADER_START_ADDR || 
        FIRMWARE_HEADER_ADDR >= APPLICATION_START_ADDR)
    {
        LOGD(TAG, "Firmware header address out of range: 0x%08X (range: 0x%08X - 0x%08X)", 
             FIRMWARE_HEADER_ADDR, BOOTLOADER_START_ADDR, APPLICATION_START_ADDR);
        return IAP_RESULT_ERROR;
    }
    
    return IAP_RESULT_OK;
}

/**
 * @brief 显示固件信息（内部实现）
 * @param header 固件包头结构指针
 */
static void iap_core_display_firmware_information(const FirmwareHeader *header)
{
    char md5_string[33] = {0}; /* 32字符 + null终止符 */
    
    /* 打印基本信息 */
    LOG_PRINTF("Firmware Version: %s\r\n", header->version_string);
    LOG_PRINTF("Target Chip: %s\r\n", header->target_chip);
    LOG_PRINTF("Firmware Size: %u bytes\r\n", header->firmware_size);
    LOG_PRINTF("Start Address: 0x%08X\r\n", header->start_address);
    LOG_PRINTF("Build Time: %u (Unix timestamp)\r\n", header->build_time);
    
    /* 格式化并打印MD5 */
    iap_core_format_md5_string(header->firmware_md5, md5_string, sizeof(md5_string));
    LOG_PRINTF("MD5 Hash: %s\r\n", md5_string);
}

/**
 * @brief 格式化MD5字符串
 * @param md5 MD5字节数组
 * @param output 输出字符串缓冲区
 * @param output_size 输出缓冲区大小
 */
static void iap_core_format_md5_string(const uint8_t *md5, char *output, size_t output_size)
{
    if (md5 == NULL || output == NULL || output_size < 33)
    {
        return;
    }
    
    /* 格式化MD5为十六进制字符串 */
    for (int i = 0; i < 16; i++)
    {
        snprintf(output + (i * 2), 3, "%02X", md5[i]);
    }
    
    output[32] = '\0'; /* 确保字符串终止 */
}

