#ifndef __LOG_H_
#define __LOG_H_

#include <stdio.h>
#include "core/inc/iap_config.h"
#ifdef __cplusplus
extern "C"
{
#endif

/* 日志等级定义 */
#define LOG_NONE 0
#define LOG_ERROR 1
#define LOG_WARN 2
#define LOG_INFO 3
#define LOG_DEBUG 4
#define LOG_VERBOSE 5

/* -----------------配置------------------ */
#if IAP_LOG_PRINT_ENABLE
#define LOG_PRINTF printf // 默认打印函数
#define CONFIG_LOG_LEVEL LOG_DEBUG // 默认日志等级
#else
#define LOG_PRINTF(format, ...)   // 默认打印函数
#define CONFIG_LOG_LEVEL LOG_NONE // 默认日志等级
#endif

#ifndef CONFIG_LOG_COLOR_ENABLE
#define CONFIG_LOG_COLOR_ENABLE 1 // 默认开启颜色
#endif

#if  MCU_TYPE == USE_STM32
#define LOG_GETTICK HAL_GetTick() // 默认获取时间标签函数
#elif MCU_TYPE == USE_GD32
#include "systick.h"
#define LOG_GETTICK get_Tick()              // 默认获取时间标签函数
#endif
/* -------------------------------------- */

/* 颜色输出控制 */
#if CONFIG_LOG_COLOR_ENABLE
#define CSI_BLACK "30"
#define CSI_RED "31"
#define CSI_GREEN "32"
#define CSI_YELLOW "33"
#define CSI_BLUE "34"
#define CSI_FUCHSIN "35"
#define CSI_CYAN "36"
#define CSI_WHITE "37"
#define CSI_BLACK_L "90"
#define CSI_RED_L "91"
#define CSI_GREEN_L "92"
#define CSI_YELLOW_L "93"
#define CSI_BLUE_L "94"
#define CSI_FUCHSIN_L "95"
#define CSI_CYAN_L "96"
#define CSI_WHITE_L "97"
#define CSI_DEFAULT "39"
#define LOG_COLOR(COLOR) "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR) "\033[1;" COLOR "m"
#define LOG_RESET_COLOR "\033[0m"
#define LOG_COLOR_E LOG_COLOR(CSI_RED)
#define LOG_COLOR_W LOG_COLOR(CSI_YELLOW)
#define LOG_COLOR_I LOG_COLOR(CSI_GREEN)
#define LOG_COLOR_D LOG_COLOR(CSI_BLUE)
#define LOG_COLOR_V LOG_COLOR(CSI_CYAN)
#else
#define LOG_COLOR_E
#define LOG_COLOR_W
#define LOG_COLOR_I
#define LOG_COLOR_D
#define LOG_COLOR_V
#define LOG_RESET_COLOR
#endif

/* 格式化打印 */
#define LOG_FORMAT(tag, letter, format)                                    \
    LOG_COLOR_##letter #letter " (%d) %s: " format LOG_RESET_COLOR "\r\n", \
        LOG_GETTICK, tag

/* 打印实现 */
#if (CONFIG_LOG_LEVEL >= LOG_ERROR)
#define LOGE(tag, format, ...) LOG_PRINTF(LOG_FORMAT(tag, E, format), ##__VA_ARGS__)
#else
#define LOGE(tag, format, ...)
#endif

#if (CONFIG_LOG_LEVEL >= LOG_WARN)
#define LOGW(tag, format, ...) LOG_PRINTF(LOG_FORMAT(tag, W, format), ##__VA_ARGS__)
#else
#define LOGW(tag, format, ...)
#endif

#if (CONFIG_LOG_LEVEL >= LOG_INFO)
#define LOGI(tag, format, ...) LOG_PRINTF(LOG_FORMAT(tag, I, format), ##__VA_ARGS__)
#else
#define LOGI(tag, format, ...)
#endif

#if (CONFIG_LOG_LEVEL >= LOG_DEBUG)
#define LOGD(tag, format, ...) LOG_PRINTF(LOG_FORMAT(tag, D, format), ##__VA_ARGS__)
#else
#define LOGD(tag, format, ...)
#endif

#if (CONFIG_LOG_LEVEL >= LOG_VERBOSE)
#define LOGV(tag, format, ...) LOG_PRINTF(LOG_FORMAT(tag, V, format), ##__VA_ARGS__)
#else
#define LOGV(tag, format, ...)
#endif

#ifdef __cplusplus
}
#endif
#endif /* __LOG_H_ */
