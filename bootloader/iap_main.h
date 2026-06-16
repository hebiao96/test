#ifndef __IAP_MAIN_H
#define __IAP_MAIN_H

/**
 * @file iap_main.h
 * @brief IAP系统主头文件
 * 
 * 本文件包含IAP系统的所有头文件，用户只需包含此文件即可使用所有IAP功能。
 * 
 * 依赖关系：
 * - iap_types.h: 基础类型定义 (无依赖)
 * - iap_config.h: 配置和宏定义 (依赖 iap_types.h)
 * - API头文件: 接口定义 (依赖基础类型)
 * - CORE头文件: 核心功能 (依赖配置和类型)
 */

/* 基础类型定义 */
#include "core/inc/iap_types.h"

/* 配置文件 */
#include "core/inc/iap_config.h"

/* API接口 */
#include "api/inc/iap_uart.h"
#include "api/inc/iap_can.h"
#include "api/inc/iap_timer.h"
#include "api/inc/iap_flash.h"

/* 核心功能 */
#include "core/inc/iap_core.h"
#include "core/inc/iap_bootloader.h"
#include "core/inc/iap_md5.h"
#include "core/inc/ymodem.h"

/* 调试接口 */
#include "core/inc/iap_log.h"

#endif /* __IAP_MAIN_H */
