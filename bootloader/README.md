# IAP Bootloader v1.0

## 项目简介

IAP (In-Application Programming) Bootloader 是一个基于Ymodem协议的嵌入式系统在线升级引导程序。该项目支持STM32F4系列和GD32F10x系列微控制器，通过串口接收固件文件并完成应用程序的在线升级。

## 项目特性

- **支持多种芯片**: STM32F4系列、GD32F10x系列
- **Ymodem协议**: 使用标准Ymodem协议进行固件传输，支持128字节和1024字节数据包
- **固件校验**: 支持MD5校验和固件包头验证
- **应用程序验证**: 自动检查应用程序有效性
- **调试日志**: 可配置的串口日志输出系统
- **灵活配置**: 通过配置文件支持不同硬件平台

## 快速开始

### 最小化配置步骤

1. **选择目标芯片**: 在`iap_config.h`中设置`USE_STM32 1`或`USE_GD32 1`
2. **配置Flash大小**: 根据芯片型号设置`FLASH_TOTAL_SIZE`
3. **实现必要的API**: 
   - `IAP_UART_SendByte()` - 串口发送
   - `IAP_UART_ReceiveByte()` - 串口接收
   - `IAP_Flash_Erase()` - Flash擦除
   - `IAP_Flash_Write()` - Flash写入
4. **编译并烧录**: 将Bootloader烧录到芯片
5. **测试升级**: 使用Ymodem工具发送固件包

### 固件包制作工具

推荐使用以下工具制作符合格式的固件包：
- 自定义Python脚本生成固件头
- 使用`cat`命令合并: `cat header.bin app.bin > firmware.pkg`
- 计算MD5值并写入固件头

## FLASH分配图

基于STM32F407ZG (512KB Flash) 的内存分配示例：

```
Flash起始地址: 0x08000000                    Flash结束地址: 0x08080000
├─────────────────────────────────────────────────────────────────────┤
│                        Flash Memory Layout                         │
├─────────────────────────────────────────────────────────────────────┤
│  Bootloader (32KB)         │  Vector Gap  │    Application Area    │
│  0x08000000 - 0x08007FFF   │   (512B)     │   0x08008200 - End     │
├─────────────────────────────────────────────────────────────────────┤
│                            │  0x08008000  │                        │
│      IAP Bootloader        │      -       │    User Application    │
│                            │  0x080081FF  │                        │
│                            │              │                        │
│  - 系统启动检查            │  固件包头    │  - 用户应用程序        │
│  - 应用程序验证            │  存储区域    │  - 中断向量表重定位    │
│  - Ymodem固件接收          │  (64字节)    │  - 应用程序逻辑        │
│  - Flash编程操作           │              │                        │
├─────────────────────────────────────────────────────────────────────┤

地址计算：
- BOOTLOADER_START_ADDR   = 0x08000000
- BOOTLOADER_SIZE         = 0x8000     (32KB)
- VECTOR_TABLE_ALIGNMENT  = 0x200      (512B)
- FIRMWARE_HEADER_ADDR    = 0x08008000 (包头存储地址)
- APPLICATION_START_ADDR  = 0x08008200 (应用程序起始地址)
- FLASH_TOTAL_SIZE        = 0x80000    (512KB)
- APPLICATION_MAX_SIZE    = ~479KB     (可用应用程序空间)
```

**重要说明：**
- Bootloader占用32KB，为IAP功能预留足够空间
- 512字节对齐区间用于存储固件包头信息  
- 应用程序起始地址必须512字节对齐，满足STM32中断向量表重定位要求
- 用户应用程序需要在链接脚本中设置正确的起始地址



## 项目结构

```
IAP/bootloader/
├── iap_main.h              # 主头文件，包含所有模块
├── api/                    # 硬件抽象层API
│   ├── inc/               # API头文件
│   │   ├── iap_flash.h    # Flash操作接口
│   │   ├── iap_uart.h     # 串口操作接口
│   │   └── iap_timer.h    # 定时器接口
│   └── src/               # API实现文件
│       ├── iap_flash.c    # Flash操作实现
│       ├── iap_uart.c     # 串口操作实现
│       └── iap_timer.c    # 定时器实现
└── core/                  # 核心功能模块
    ├── inc/               # 核心头文件
    │   ├── iap_types.h    # 类型定义
    │   ├── iap_config.h   # 配置文件
    │   ├── iap_core.h     # 核心功能接口
    │   ├── iap_bootloader.h # Bootloader主逻辑
    │   ├── iap_log.h      # 日志系统
    │   ├── iap_md5.h      # MD5核心算法
    │   └── ymodem.h       # Ymodem协议实现
    └── src/               # 核心实现文件
        ├── iap_core.c     # 核心功能实现
        ├── iap_bootloader.c # Bootloader主逻辑
        ├── iap_md5.c      # MD5算法实现
        └── ymodem.c       # Ymodem协议实现
```

## 模块功能说明

### 核心模块 (core/)

- **iap_types.h**: 定义了IAP系统中使用的所有数据类型和枚举
- **iap_config.h**: 系统配置文件，包含芯片选择、地址配置、串口配置等
- **iap_core.h/c**: 核心功能实现，包括应用程序检查、跳转、固件头读取等
- **iap_bootloader.h/c**: Bootloader主逻辑，包含初始化和主循环
- **ymodem.h/c**: Ymodem协议的完整实现
- **iap_md5.h/c**: MD5算法核心实现，用于固件校验
- **iap_log.h**: 可配置的日志系统，支持颜色输出和不同日志等级

### API模块 (api/)

API模块提供硬件抽象层接口，用户需要根据具体硬件平台实现这些接口：

- **iap_flash.h/c**: Flash操作接口（擦除、写入、读取）
- **iap_uart.h/c**: 串口操作接口（初始化、发送、接收）
- **iap_timer.h/c**: 定时器接口（延时、获取时间戳）

**注意**: MD5校验功能在core模块中实现，不在API层。

## 配置说明

### 芯片选择
```c
/* 在 iap_config.h 中选择目标芯片 */
#define USE_GD32    0
#define USE_STM32   1
```

### 内存配置
```c
#define BOOTLOADER_START_ADDR   0x08000000  /* Bootloader起始地址 */
#define BOOTLOADER_SIZE         0x8000      /* Bootloader大小 32KB */
#define APPLICATION_START_ADDR  (BOOTLOADER_START_ADDR + BOOTLOADER_SIZE + VECTOR_TABLE_ALIGNMENT)
#define FLASH_TOTAL_SIZE        0x80000     /* Flash总大小 512KB */
```

### 串口配置
```c
#define IAP_UART                USART1
#define IAP_UART_BAUDRATE       115200
#define IAP_UART_TX_PIN         GPIO_PIN_9
#define IAP_UART_RX_PIN         GPIO_PIN_10
#define IAP_UART_GPIO_PORT      GPIOA
#define IAP_UART_IRQHandler     USART1_IRQHandler
```

### 定时器配置
```c
#define IAP_TIMER htim1 
#define IAP_TIMER_IRQHandler TIM1_UP_TIM10_IRQHandler
```

## 使用方法

### 1. 集成到项目

将整个 `bootloader` 文件夹复制到您的项目中，并在您的主文件中包含：

```c
#include "iap_main.h"
```

### 2. 实现硬件相关接口

您需要在 `api/src/` 目录下的文件中实现具体的硬件操作：

- 实现Flash操作函数
- 实现串口通信函数
- 实现定时器函数

### 3. 配置系统参数

修改 `core/inc/iap_config.h` 中的配置参数以适应您的硬件平台。

### 4. 主程序调用

```c
int main(void)
{
    /* 系统初始化 */
    SystemInit();
    
    /* 初始化IAP Bootloader */
    IAP_Bootloader_Init();
    
    /* 运行Bootloader主循环 */
    IAP_Bootloader_MainLoop();
    
    /* 永远不会到达这里 */
    while(1);  

### 5. 升级标志实现

应用程序可以通过设置RAM中的特定标志来请求进入IAP升级模式：

```c
/* 在应用程序中设置升级标志 */
void RequestIAPUpgrade(void)
{
    /* 设置升级标志到RAM特定地址 */
    *((uint32_t*)IAP_UPGRADE_FLAG_ADDR) = IAP_UPGRADE_FLAG_VALUE;
    
    /* 重启系统 */
    NVIC_SystemReset();
}
```

**标志位配置：**
- `IAP_UPGRADE_FLAG_ADDR`: 0x20000000 (RAM起始地址)
- `IAP_UPGRADE_FLAG_VALUE`: 0x5AA5C33C (升级标志值)  
- `IAP_UPGRADE_FLAG_CLEAR`: 0x00000000 (清除标志值)

**工作原理：**
1. 应用程序调用`RequestIAPUpgrade()`设置升级标志并重启
2. Bootloader启动时检查RAM中的升级标志
3. 如果检测到升级标志，进入Ymodem升级模式
4. 升级开始前清除标志，防止重复进入升级模式

**不初始化标志：**

按照下图设置，防止启动时清除了升级标志位

![RAM配置](/imgs/RAM配置.png)

## 工作流程

1. **系统启动**: 检查是否有升级标志
2. **应用程序检查**: 验证现有应用程序是否有效
3. **升级模式**: 如果需要升级或应用程序无效，进入Ymodem升级模式
4. **固件接收**: 通过串口接收Ymodem协议传输的固件文件
5. **固件验证**: 验证固件包头和MD5校验值
6. **Flash编程**: 将固件写入Flash存储器
7. **跳转应用**: 验证成功后跳转到新的应用程序

## 固件包格式

固件包包含64字节的包头结构：

```c
typedef struct {
    uint32_t magic_number;      /* 魔术数字: 0x12345678 */
    uint32_t version;           /* 版本号 */
    uint32_t firmware_size;     /* 固件大小 */
    uint8_t firmware_md5[16];   /* 固件MD5校验值 */
    uint32_t start_address;     /* 起始地址 */
    uint32_t build_time;        /* 构建时间 */
    char version_string[16];    /* 版本字符串 */
    char target_chip[8];        /* 目标芯片 */
    uint8_t reserved[4];        /* 保留字段 */
} FirmwareHeader;
```

## 日志系统

支持可配置的日志输出，包含以下等级：
- LOG_ERROR: 错误信息
- LOG_WARN: 警告信息  
- LOG_INFO: 一般信息
- LOG_DEBUG: 调试信息
- LOG_VERBOSE: 详细信息

## 版本信息

- **当前版本**: v1.0  
- **协议支持**: Ymodem Protocol Only
- **支持芯片**: STM32F4系列, GD32F10x系列
- **编译环境**: ARM GCC, Keil MDK, STM32CubeIDE
- **固件包格式**: 自定义固件头 + 二进制文件 + MD5校验

## 注意事项

1. **地址配置**: 确保Bootloader和应用程序的地址配置正确
2. **中断向量表**: 应用程序需要重新定位中断向量表
3. **时钟配置**: 确保系统时钟配置正确
4. **串口配置**: 确保串口配置与上位机一致
5. **Flash大小**: 根据实际芯片型号配置Flash大小

## 技术支持

如果您在使用过程中遇到问题，请检查：
1. 硬件连接是否正确
2. 配置参数是否正确
3. API接口是否正确实现
4. 固件包格式是否正确

## 更新历史

- **v1.0** (2025-01-01):
  - 支持STM32F4和GD32F10x系列芯片
  - 实现完整的Ymodem协议支持
  - 添加固件包头验证和MD5校验
  - 支持应用程序有效性检查和自动跳转
  - 可配置的日志系统和调试功能
  - 移除LED依赖，简化硬件需求
  - 完善的错误处理和重试机制

---

*本项目为嵌入式系统IAP升级解决方案，适用于产品固件在线升级需求。*
