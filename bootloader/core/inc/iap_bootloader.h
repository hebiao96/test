#ifndef __IAP_BOOTLOADER_H
#define __IAP_BOOTLOADER_H

#include "iap_types.h"

/* IAP Bootloader功能 */

/* 初始化所有IAP模块 */
void IAP_Bootloader_Init(void);

/* 去初始化所有IAP模块 */
void IAP_Bootloader_DeInit(void);

/* Ymodem固件升级 */
uint8_t IAP_Bootloader_YmodemUpdate(void);

/* Bootloader主循环 */
void IAP_Bootloader_MainLoop(void);

#endif /* __IAP_BOOTLOADER_H */
