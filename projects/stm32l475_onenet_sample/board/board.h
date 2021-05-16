/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-5      SummerGift   first version
 * 2019-04-24     yangjie      Use the end of ZI as HEAP_BEGIN
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <rtthread.h>
#include <stm32l4xx.h>
#include "drv_common.h"
#include "drv_gpio.h"
#include "drv_usart.h"


#ifdef __cplusplus
extern "C" {
#endif

#define STM32_FLASH_START_ADRESS       ((uint32_t)0x08010000)
#define STM32_FLASH_SIZE               (448 * 1024)
#define STM32_FLASH_END_ADDRESS        ((uint32_t)(STM32_FLASH_START_ADRESS + STM32_FLASH_SIZE))

#define STM32_SRAM1_SIZE               (96)
#define STM32_SRAM1_START              (0x20000000)
#define STM32_SRAM1_END                (STM32_SRAM1_START + STM32_SRAM1_SIZE * 1024)

#define STM32_SRAM2_SIZE               (32)
#define STM32_SRAM2_BEGIN              (0x10000000u)
#define STM32_SRAM2_END                (0x10000000 + STM32_SRAM2_SIZE * 1024)
#define STM32_SRAM2_HEAP_SIZE          ((uint32_t)STM32_SRAM2_END - (uint32_t)STM32_SRAM2_BEGIN)

#if defined(__CC_ARM) || defined(__CLANG_ARM)
extern int Image$$RW_IRAM1$$ZI$$Limit;
#define HEAP_BEGIN      ((void *)&Image$$RW_IRAM1$$ZI$$Limit)
#elif __ICCARM__
#pragma section="CSTACK"
#define HEAP_BEGIN      (__segment_end("CSTACK"))
#else
extern int __bss_end;
#define HEAP_BEGIN      ((void *)&__bss_end)
#endif

#define HEAP_END                       STM32_SRAM1_END

void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif

#endif

