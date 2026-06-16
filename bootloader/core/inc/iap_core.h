#ifndef __IAP_CORE_H
#define __IAP_CORE_H

#include "iap_types.h"
#include "iap_config.h"

/* IAP核心功能 */


/* 检查应用程序是否有效 */
uint8_t IAP_Core_CheckApplication(void);

/* 跳转到应用程序 */
void IAP_Core_JumpToApplication(void);

/* 固件信息相关 */
uint8_t IAP_Core_ReadFirmwareHeader(FirmwareHeader *header);
void IAP_Core_PrintFirmwareInfo(const FirmwareHeader *header);

/* 应用程序升级标志相关 */
uint8_t IAP_Core_CheckUpgradeFlag(void);
void IAP_Core_ClearUpgradeFlag(void);

/* 时间相关 */
uint32_t IAP_Core_GetTick(void);
void IAP_Core_Delay_ms(uint32_t ms);


#endif /* __IAP_CORE_H */
