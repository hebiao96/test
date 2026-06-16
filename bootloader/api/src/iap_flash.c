#include "iap_main.h"

#if IAP_LOG_PRINT_ENABLE
static const char *TAG = "IAP_Bootloader";
#endif

/**
 * @brief Flash初始化
 * @note 用户需要根据具体硬件平台实现此函数
 * 
 */
void IAP_Flash_Init(void)
{
   
#if MCU_TYPE == USE_STM32
    #if STM32F4xx
     /* STM32F4系列需要启用预取缓冲区和缓存以提高性能 */
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
    __HAL_FLASH_DATA_CACHE_ENABLE();
    #elif STM32F10x
    /* STM32F1系列通常不需要特殊配置，HAL库会处理等待状态 */
     /* 但可以根据需要调整等待状态以适应更高的时钟频率 */
     __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
    #endif

#elif MCU_TYPE == USE_GD32
    /* GD32 Flash初始化，通常不需要特殊操作 */
    /* FMC等待状态已在系统初始化时配置 */
#endif
}

/**
 * @brief Flash去初始化
 * @note 在跳转到应用程序前调用，确保Flash处于安全状态
 * 
 */
void IAP_Flash_DeInit(void)
{
#if MCU_TYPE == USE_STM32
    /* 锁定Flash */
    HAL_FLASH_Lock();
    
#elif MCU_TYPE == USE_GD32
    /* 锁定Flash */
    fmc_lock();
#endif
}

/**
 * @brief 解锁Flash
 * @return IAP_FLASH_OK=成功，其他=失败
 * 
 */
IAP_Flash_Status IAP_Flash_Unlock(void)
{
    /* 用户需要在此实现Flash解锁代码 */
#if MCU_TYPE == USE_STM32
    if (HAL_FLASH_Unlock() == HAL_OK) {
        return IAP_FLASH_OK;
    }
    return IAP_FLASH_ERROR;
#elif MCU_TYPE == USE_GD32
    /* 解锁FMC */
    fmc_unlock();
    return IAP_FLASH_OK;
#endif   
    return IAP_FLASH_ERROR; /* 占位返回值 */
}

/**
 * @brief 锁定Flash
 * 
 */
void IAP_Flash_Lock(void)
{
    /* 用户需要在此实现Flash锁定代码 */
#if MCU_TYPE == USE_STM32
    HAL_FLASH_Lock();
#elif MCU_TYPE == USE_GD32
    /* 锁定FMC */
    fmc_lock();
#endif
}

#if STM32F4xx
/**
 * @brief 根据地址获取扇区号（STM32F4专用）
 * @param addr Flash地址
 * @return 扇区号
 */
static uint32_t GetSectorFromAddr(uint32_t addr)
{
#if MCU_TYPE == USE_STM32
    uint32_t sector = 0;
    
    if(addr < 0x08004000)        /* 扇区0: 16KB */
    {
        sector = 0;
    }
    else if(addr < 0x08008000)   /* 扇区1: 16KB */
    {
        sector = 1;
    }
    else if(addr < 0x0800C000)   /* 扇区2: 16KB */
    {
        sector = 2;
    }
    else if(addr < 0x08010000)   /* 扇区3: 16KB */
    {
        sector = 3;
    }
    else if(addr < 0x08020000)   /* 扇区4: 64KB */
    {
        sector = 4;
    }
    else                         /* 扇区5+: 128KB */
    {
        sector = 5 + (addr - 0x08020000) / 0x20000;
    }
    
    return sector;
#else
    return 0;
#endif
}
#endif

#if STM32F4xx
/**
 * @brief 检查扇区是否已被擦除
 * @param sector_addr 扇区起始地址
 * @return 1=已擦除，0=未擦除
 */
static uint8_t IsSectorErased(uint32_t sector_addr)
{
#if MCU_TYPE == USE_STM32
    uint32_t sector_size;
    uint32_t i;
    
    /* 根据扇区地址确定扇区大小 */
    if(sector_addr < 0x08004000)        /* 扇区0: 16KB */
        sector_size = 0x4000;
    else if(sector_addr < 0x08008000)   /* 扇区1: 16KB */
        sector_size = 0x4000;
    else if(sector_addr < 0x0800C000)   /* 扇区2: 16KB */
        sector_size = 0x4000;
    else if(sector_addr < 0x08010000)   /* 扇区3: 16KB */
        sector_size = 0x4000;
    else if(sector_addr < 0x08020000)   /* 扇区4: 64KB */
        sector_size = 0x10000;
    else                                /* 扇区5+: 128KB */
        sector_size = 0x20000;
    
    /* 检查扇区内所有字节是否为0xFF */
    for(i = 0; i < sector_size; i += 4)  /* 按字检查提高效率 */
    {
        if(*(volatile uint32_t*)(sector_addr + i) != 0xFFFFFFFF)
        {
            return 0;  /* 发现非0xFF值，扇区未被擦除 */
        }
    }
    
    return 1;  /* 扇区已被擦除 */
#else
    return 0;  /* 非STM32平台，默认需要擦除 */
#endif
}
#endif

/**
 * @brief 检查页是否已被擦除 (GD32专用或STM32F10x)
 * @param page_addr 页起始地址
 * @return 1=已擦除，0=未擦除
 */
static uint8_t IsPageErased(uint32_t page_addr)
{
#if MCU_TYPE == USE_GD32
    uint32_t i;
    
    /* 检查页内所有字节是否为0xFF */
    for(i = 0; i < FLASH_PAGE_SIZE; i += 4)  /* 按字检查提高效率 */
    {
        if(*(volatile uint32_t*)(page_addr + i) != 0xFFFFFFFF)
        {
            return 0;  /* 发现非0xFF值，页未被擦除 */
        }
    }
    
    return 1;  /* 页已被擦除 */
#elif MCU_TYPE == USE_STM32
#if STM32F10x
    uint32_t i;
    
    /* 检查页内所有字节是否为0xFF */
    for(i = 0; i < FLASH_PAGE_SIZE; i += 4)  /* 按字检查提高效率 */
    {
        if(*(volatile uint32_t*)(page_addr + i) != 0xFFFFFFFF)
        {
            return 0;  /* 发现非0xFF值，页未被擦除 */
        }
    }
    
    return 1;  /* 页已被擦除 */
#endif
    
#else
    return 0;  /* 非GD32、stm32f10x平台，默认需要擦除 */
#endif
}

/**
 * @brief 擦除页
 * @param page_addr 页起始地址（必须是页对齐的地址）
 * @return IAP_FLASH_OK=成功，其他=失败
 * 
 */
IAP_Flash_Status IAP_Flash_ErasePage(uint32_t page_addr)
{
    /* 用户需要在此实现Flash页擦除代码 */
#if MCU_TYPE == USE_STM32
#if STM32F4xx
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError;
    HAL_StatusTypeDef hal_status;
    uint32_t sector_start_addr;
    
    /* 检查地址有效性 */
    if(!IAP_Flash_IsValidAddr(page_addr))
    {
        return IAP_FLASH_ERROR;
    }
    
    /* 获取扇区起始地址 */
    sector_start_addr = IAP_Flash_GetPageAddr(page_addr);
    
    /* 检查扇区是否已被擦除，如果已擦除则跳过 */
    if(IsSectorErased(sector_start_addr))
    {
        return IAP_FLASH_OK;  /* 扇区已擦除，无需重复擦除 */
    }
    
    /* 解锁Flash */
    hal_status = HAL_FLASH_Unlock();
    if(hal_status != HAL_OK)
    {
        return IAP_FLASH_ERROR;
    }
    
    /* 配置擦除结构体 */
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Sector = GetSectorFromAddr(page_addr);
    EraseInitStruct.NbSectors = 1;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3; /* 2.7V-3.6V */
    
    /* 执行擦除 */
    hal_status = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
    
    /* 锁定Flash */
    HAL_FLASH_Lock();
    
    if(hal_status == HAL_OK)
    {
        return IAP_FLASH_OK;
    }
    
    return IAP_FLASH_ERROR;

 #endif

 #if STM32F10x
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError;
    HAL_StatusTypeDef status;

    /* 检查地址有效性 */
    if(!IAP_Flash_IsValidAddr(page_addr))
    {
        return IAP_FLASH_ERROR;
    }

    /* 获取页起始地址 */
    uint32_t page_start_addr = IAP_Flash_GetPageAddr(page_addr);

    /* 检查页是否已被擦除，如果已擦除则跳过 */
    if(IsPageErased(page_start_addr))
    {
        return IAP_FLASH_OK;  /* 页已擦除，无需重复擦除 */
    }

    /* 解锁Flash */
    HAL_FLASH_Unlock();

    /* 配置擦除结构体 */
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = page_start_addr;
    EraseInitStruct.NbPages     = 1;

    /* 执行页擦除 */
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

    /* 锁定Flash */
    HAL_FLASH_Lock();

    if(status == HAL_OK)
    {
        return IAP_FLASH_OK;
    }

    return IAP_FLASH_ERROR;
#endif

#elif MCU_TYPE == USE_GD32
    fmc_state_enum fmc_state = FMC_READY;
    
    /* 检查地址有效性 */
    if(!IAP_Flash_IsValidAddr(page_addr))
    {
        return IAP_FLASH_ERROR;
    }
    
    /* 获取页起始地址 */
    uint32_t page_start_addr = IAP_Flash_GetPageAddr(page_addr);
    
    /* 检查页是否已被擦除，如果已擦除则跳过 */
    if(IsPageErased(page_start_addr))
    {
        return IAP_FLASH_OK;  /* 页已擦除，无需重复擦除 */
    }
    
    /* 解锁FMC */
    fmc_unlock();
    
    /* 清除所有挂起的标志 */
    fmc_flag_clear(FMC_FLAG_BANK0_END);
    fmc_flag_clear(FMC_FLAG_BANK0_WPERR);
    fmc_flag_clear(FMC_FLAG_BANK0_PGERR);
    
    /* 执行页擦除 */
    fmc_state = fmc_page_erase(page_start_addr);
    
    /* 锁定FMC */
    fmc_lock();
    
    if(fmc_state == FMC_READY)
    {
        return IAP_FLASH_OK;
    }
    
    return IAP_FLASH_ERROR;
#endif
    
    return IAP_FLASH_ERROR; /* 占位返回值 */
}

/**
 * @brief 擦除多个页
 * @param start_addr 起始地址
 * @param end_addr 结束地址
 * @return IAP_FLASH_OK=成功，其他=失败
 * 
 */
IAP_Flash_Status IAP_Flash_ErasePages(uint32_t start_addr, uint32_t end_addr)
{
IAP_Flash_Status status = IAP_FLASH_OK;
    
#if MCU_TYPE == USE_STM32
    uint32_t current_addr;
    
    
    /* 检查地址范围 */
    if(start_addr < FIRMWARE_HEADER_ADDR || end_addr > FLASH_END_ADDR)
    {
        return IAP_FLASH_ERROR;
    }
    
    /* 检查地址对齐 */
    if(start_addr >= end_addr)
    {
        return IAP_FLASH_ERROR;
    }
    
    /* 解锁Flash */
    if(IAP_Flash_Unlock() != IAP_FLASH_OK)
    {
        return IAP_FLASH_ERROR;
    }
#if STM32F4xx
    /* 擦除所有扇区 */
    current_addr = IAP_Flash_GetPageAddr(start_addr);
    while(current_addr < end_addr)
    {
        /* 获取下一个扇区的起始地址，先计算再擦除 */
        uint32_t next_addr;
        
        if(current_addr < 0x08004000)        /* 扇区0: 16KB */
        {
            next_addr = current_addr + 0x4000;
        }
        else if(current_addr < 0x08008000)   /* 扇区1: 16KB */
        {
            next_addr = current_addr + 0x4000;
        }
        else if(current_addr < 0x0800C000)   /* 扇区2: 16KB */
        {
            next_addr = current_addr + 0x4000;
        }
        else if(current_addr < 0x08010000)   /* 扇区3: 16KB */
        {
            next_addr = current_addr + 0x4000;
        }
        else if(current_addr < 0x08020000)   /* 扇区4: 64KB */
        {
            next_addr = current_addr + 0x10000;
        }
        else                                 /* 扇区5+: 128KB */
        {
            next_addr = current_addr + 0x20000;
        }
        
        /* 检查扇区是否已被擦除，如果已擦除则跳过 */
        if(!IsSectorErased(current_addr))
        {
            /* 擦除当前扇区（不需要再次解锁） */
            FLASH_EraseInitTypeDef EraseInitStruct;
            uint32_t SectorError;
            
            EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
            EraseInitStruct.Sector = GetSectorFromAddr(current_addr);
            EraseInitStruct.NbSectors = 1;
            EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3; /* 2.7V-3.6V */
            
            if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
            {
                status = IAP_FLASH_ERROR;
                break;
            }
        }
        
        current_addr = next_addr;
    }
#endif

#if STM32F10x
    /* 擦除所有页 */
    current_addr = IAP_Flash_GetPageAddr(start_addr);
    while(current_addr < end_addr)
    {
        /* 检查页是否已被擦除，如果已擦除则跳过 */
        if(!IsPageErased(current_addr))
        {
            /* 擦除当前页（不需要再次解锁） */
            FLASH_EraseInitTypeDef EraseInitStruct;
            uint32_t PageError;
            
            EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
            EraseInitStruct.PageAddress = current_addr;
            EraseInitStruct.NbPages     = 1;
            
            if(HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK)
            {
                status = IAP_FLASH_ERROR;
                break;
            }
        }
        
        current_addr += FLASH_PAGE_SIZE;  /* 移动到下一页 */
    }
#endif
    
    /* 锁定Flash */
    IAP_Flash_Lock();
    
    return status;
#elif MCU_TYPE == USE_GD32
    uint32_t current_addr;
    fmc_state_enum fmc_state = FMC_READY;
    
    /* 检查地址范围 */
    if(start_addr < FIRMWARE_HEADER_ADDR || end_addr > FLASH_END_ADDR)
    {
        return IAP_FLASH_ERROR;
    }
    
    /* 检查地址对齐 */
    if(start_addr >= end_addr)
    {
        return IAP_FLASH_ERROR;
    }
    
    /* 解锁FMC */
    fmc_unlock();
    
    /* 清除所有挂起的标志 */
    fmc_flag_clear(FMC_FLAG_BANK0_END);
    fmc_flag_clear(FMC_FLAG_BANK0_WPERR);
    fmc_flag_clear(FMC_FLAG_BANK0_PGERR);
    
    /* 擦除所有页面 */
    current_addr = IAP_Flash_GetPageAddr(start_addr);
    while(current_addr < end_addr && fmc_state == FMC_READY)
    {
        /* 检查页是否已被擦除，如果已擦除则跳过 */
        if(!IsPageErased(current_addr))
        {
            /* 执行页擦除 */
            fmc_state = fmc_page_erase(current_addr);
            if(fmc_state != FMC_READY)
            {
                status = IAP_FLASH_ERROR;
                break;
            }
        }
        
        /* 移动到下一页 */
        current_addr += FLASH_PAGE_SIZE;
    }
    
    /* 锁定FMC */
    fmc_lock();
    
    return status;
#else
    return status;
#endif
}

/**
 * @brief 写入数据
 * @param addr 写入地址
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return IAP_FLASH_OK=成功，其他=失败
 */
IAP_Flash_Status IAP_Flash_WriteData(uint32_t addr, uint8_t *data, uint16_t length)
{
    /* 用户需要在此实现Flash数据写入代码 */
#if MCU_TYPE == USE_STM32
    uint16_t i;   
    /* 检查参数有效性 */
    if(data == NULL || length == 0)
    {
        return IAP_FLASH_ERROR;
    }
    
    /* 检查地址范围 */
    if(!IAP_Flash_IsValidAddr(addr) || !IAP_Flash_IsValidAddr(addr + length - 1))
    {
        return IAP_FLASH_ERROR;
    }
    
    if(IAP_Flash_Unlock() != IAP_FLASH_OK)
    {
        return IAP_FLASH_ERROR;
    }

#if STM32F4xx
    /* STM32F4按字节写入 */
    for(i = 0; i < length; i++)
    {
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr + i, data[i]) != HAL_OK)
        {
            IAP_Flash_Lock();
            return IAP_FLASH_ERROR;
        }
    }
#elif STM32F10x
    /* STM32F1按半字写入（F103仅支持16位编程） */
    for(i = 0; i < length; i += 2)
    {
        uint16_t halfword_data;
        if(i + 1 < length)
        {
            halfword_data = data[i] | (data[i + 1] << 8);
        }
        else
        {
            halfword_data = data[i] | 0xFF00; /* 最后一个字节，高8位填0xFF（保持擦除态） */
        }

        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, halfword_data) != HAL_OK)
        {
            IAP_Flash_Lock();
            return IAP_FLASH_ERROR;
        }
    }
#else
    #error "Unsupported STM32 variant"
#endif
    IAP_Flash_Lock();
    
    return IAP_FLASH_OK;
#elif MCU_TYPE == USE_GD32
    uint16_t i;
    fmc_state_enum fmc_state = FMC_READY;
    
    /* 检查参数有效性 */
    if(data == NULL || length == 0)
    {
        return IAP_FLASH_ERROR;
    }
    
    /* 检查地址范围 */
    if(!IAP_Flash_IsValidAddr(addr) || !IAP_Flash_IsValidAddr(addr + length - 1))
    {
        return IAP_FLASH_ERROR;
    }
    
    /* 解锁FMC */
    fmc_unlock();
    
    /* 清除所有挂起的标志 */
    fmc_flag_clear(FMC_FLAG_BANK0_END);
    fmc_flag_clear(FMC_FLAG_BANK0_WPERR);
    fmc_flag_clear(FMC_FLAG_BANK0_PGERR);
    
    /* GD32按字或半字写入（根据地址对齐情况） */
    i = 0;
    while(i < length && fmc_state == FMC_READY)
    {
        if((addr + i) % 4 == 0 && (length - i) >= 4)
        {
            /* 4字节对齐，按字写入 */
            uint32_t word_data = *((uint32_t*)(data + i));
            fmc_state = fmc_word_program(addr + i, word_data);
            i += 4;
        }
        else if((addr + i) % 2 == 0 && (length - i) >= 2)
        {
            /* 2字节对齐，按半字写入 */
            uint16_t halfword_data = *((uint16_t*)(data + i));
            fmc_state = fmc_halfword_program(addr + i, halfword_data);
            i += 2;
        }
        else
        {
            /* 单字节写入（通过半字写入实现） */
            uint16_t halfword_data;
            if(i + 1 < length)
            {
                halfword_data = data[i] | (data[i + 1] << 8);
                fmc_state = fmc_halfword_program(addr + i, halfword_data);
                i += 2;
            }
            else
            {
                halfword_data = data[i] | 0xFF00;
                fmc_state = fmc_halfword_program(addr + i, halfword_data);
                i += 1;
            }
        }
    }
    
    /* 锁定FMC */
    fmc_lock();
    
    if(fmc_state == FMC_READY)
    {
        return IAP_FLASH_OK;
    }
    
    return IAP_FLASH_ERROR;
#endif
    
    return IAP_FLASH_ERROR; /* 占位返回值 */
}

/**
 * @brief 读取数据
 * @param addr 读取地址
 * @param data 数据缓冲区
 * @param length 数据长度
 * 
 */
void IAP_Flash_ReadData(uint32_t addr, uint8_t *data, uint16_t length)
{
    /* Flash读取通常可以直接通过指针访问 */
    uint16_t i;
    
    for(i = 0; i < length; i++)
    {
        data[i] = *(volatile uint8_t*)(addr + i);
    }
    
    /* 如果需要特殊的读取函数，请在此修改 */
}

/**
 * @brief 获取页地址
 * @param addr 任意地址
 * @return 该地址所在页的起始地址
 * 
 */
uint32_t IAP_Flash_GetPageAddr(uint32_t addr)
{
    /* 将地址对齐到页边界 */
#if MCU_TYPE == USE_STM32
#if STM32F4xx
    /* STM32F407ZG扇区布局：
     * 扇区0: 0x08000000-0x08003FFF (16KB)
     * 扇区1: 0x08004000-0x08007FFF (16KB)  
     * 扇区2: 0x08008000-0x0800BFFF (16KB)
     * 扇区3: 0x0800C000-0x0800FFFF (16KB)
     * 扇区4: 0x08010000-0x0801FFFF (64KB)
     * 扇区5: 0x08020000-0x0803FFFF (128KB)
     * 扇区6: 0x08040000-0x0805FFFF (128KB)
     * 扇区7: 0x08060000-0x0807FFFF (128KB)
     * ...
     */
    if(addr < 0x08004000)        /* 扇区0: 16KB */
    {
        return 0x08000000;
    }
    else if(addr < 0x08008000)   /* 扇区1: 16KB */
    {
        return 0x08004000;
    }
    else if(addr < 0x0800C000)   /* 扇区2: 16KB */
    {
        return 0x08008000;
    }
    else if(addr < 0x08010000)   /* 扇区3: 16KB */
    {
        return 0x0800C000;
    }
    else if(addr < 0x08020000)   /* 扇区4: 64KB */
    {
        return 0x08010000;
    }
    else                         /* 扇区5+: 128KB */
    {
        /* 计算128KB对齐的扇区起始地址 */
        return ((addr - 0x08020000) & ~0x1FFFF) + 0x08020000;
    }
#endif

#if STM32F10x
    /* STM32F103VET6每页2KB，地址必须2KB对齐 */
    return (addr & ~(FLASH_PAGE_SIZE - 1));
#endif

#elif MCU_TYPE == USE_GD32
    /* GD32页对齐：每页2KB */
    return (addr & ~(FLASH_PAGE_SIZE - 1));
#endif
    
    return (addr & ~(FLASH_PAGE_SIZE - 1));
}

/**
 * @brief 检查地址是否有效
 * @param addr 要检查的地址
 * @return 1=有效，0=无效
 */
uint8_t IAP_Flash_IsValidAddr(uint32_t addr)
{
    /* 检查地址是否在应用程序区域内 */
    return (addr >= FIRMWARE_HEADER_ADDR && addr < FLASH_END_ADDR);
}

