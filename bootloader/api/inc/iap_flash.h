#ifndef __IAP_FLASH_H  
#define __IAP_FLASH_H

#include <stdint.h>

/* Flash操作结果类型定义 */
#ifndef IAP_FLASH_STATUS_DEFINED
#define IAP_FLASH_STATUS_DEFINED
typedef enum {
    IAP_FLASH_OK = 0,
    IAP_FLASH_ERROR,
    IAP_FLASH_TIMEOUT,
    IAP_FLASH_WRITE_PROTECTION
} IAP_Flash_Status;
#endif

/**
 * @file iap_flash.h
 * @brief IAP Flash操作接口定义
 * 
 * 本文件定义了IAP所需的Flash操作接口函数，这些函数需要用户根据具体硬件平台实现。
 * 用户需要在自己的工程中实现以下函数。
 * 
 * 注意：IAP_Flash_Status 类型定义在 iap_types.h 中
 */

/**
 * @brief Flash初始化
 * @note 用户需要实现此函数，完成Flash控制器初始化
 *       通常Flash初始化在系统启动时完成，此函数可以为空
 */
void IAP_Flash_Init(void);

/**
 * @brief Flash去初始化
 * @note 在跳转到应用程序前调用，完成Flash控制器去初始化
 *       确保Flash处于锁定状态
 */
void IAP_Flash_DeInit(void);

/**
 * @brief 解锁Flash
 * @return IAP_FLASH_OK=成功，其他=失败
 * @note 用户需要实现此函数，解锁Flash以允许写入/擦除操作
 *       在进行Flash写入或擦除操作前必须先解锁
 */
IAP_Flash_Status IAP_Flash_Unlock(void);

/**
 * @brief 锁定Flash
 * @note 用户需要实现此函数，锁定Flash以保护其不被意外修改
 *       在完成Flash写入或擦除操作后应该锁定Flash
 */
void IAP_Flash_Lock(void);

/**
 * @brief 擦除页(扇区)
 * @param page_addr 页起始地址（必须是页对齐的地址）
 * @return IAP_FLASH_OK=成功，其他=失败
 * @note 用户需要实现此函数，擦除指定的Flash页
 *       注意：地址必须是页对齐的（例如1KB页大小，地址必须是1024的倍数）
 */
IAP_Flash_Status IAP_Flash_ErasePage(uint32_t page_addr);

/**
 * @brief 擦除多个页(扇区)
 * @param start_addr 起始地址
 * @param end_addr 结束地址
 * @return IAP_FLASH_OK=成功，其他=失败
 * @note 用户需要实现此函数，擦除指定范围内的所有Flash页
 *       函数应该自动处理页对齐和范围检查
 */
IAP_Flash_Status IAP_Flash_ErasePages(uint32_t start_addr, uint32_t end_addr);

/**
 * @brief 写入数据
 * @param addr 写入地址
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return IAP_FLASH_OK=成功，其他=失败
 * @note 用户需要实现此函数，向Flash写入数据
 *       注意：写入前目标区域必须已被擦除（全FF）
 *       不同MCU可能有不同的写入单位（字节、半字、字）
 */
IAP_Flash_Status IAP_Flash_WriteData(uint32_t addr, uint8_t *data, uint16_t length);

/**
 * @brief 读取数据
 * @param addr 读取地址
 * @param data 数据缓冲区
 * @param length 数据长度
 * @note 用户需要实现此函数，从Flash读取数据
 *       Flash读取通常可以直接通过指针访问，但也可以使用专门的读取函数
 */
void IAP_Flash_ReadData(uint32_t addr, uint8_t *data, uint16_t length);

/**
 * @brief 获取页地址
 * @param addr 任意地址
 * @return 该地址所在页的起始地址
 * @note 用户需要实现此函数，将任意地址转换为页对齐的地址
 *       例如：对于1KB页大小，地址0x8000800应该返回0x8000800
 *       地址0x8000900应该返回0x8000800
 */
uint32_t IAP_Flash_GetPageAddr(uint32_t addr);

/**
 * @brief 检查地址是否有效
 * @param addr 要检查的地址
 * @return 1=有效，0=无效
 * @note 用户需要实现此函数，检查地址是否在允许的Flash范围内
 *       通常检查地址是否在应用程序区域内
 */
uint8_t IAP_Flash_IsValidAddr(uint32_t addr);

#endif /* __IAP_FLASH_H */
