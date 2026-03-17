/****************************************************************************
 * boards/xtensa/esp32s3/esp32s3-boss1-cx/include/board_memorymap.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __BOARDS_XTENSA_ESP32S3_ESP32S3_BOSS1_CX_INCLUDE_BOARD_MEMORYMAP_H
#define __BOARDS_XTENSA_ESP32S3_ESP32S3_BOSS1_CX_INCLUDE_BOARD_MEMORYMAP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/*============================================================================
 * IRAM (Internal RAM) - Code running from internal SRAM
 * Defined in: .iram0.vectors, .iram0.text, .iram0.text_end sections
 * Memory Region: iram0_0_seg (0x40378000 - 0x40400000)
 *============================================================================*/

/* IRAM 起始地址常量 (定义在链接脚本头部) */
#define DIRAM_I_START      0x40378000  /* IRAM 起始物理地址 */

/* .iram0.vectors 段 - 中断向量表 */
#define IRAM_START        (uintptr_t)_iram_start  /* IRAM 代码起始地址 */
#define INIT_START        (uintptr_t)_init_start  /* 初始化代码起始地址 */
#define INIT_END          (uintptr_t)_init_end    /* 初始化代码结束地址 */

/* .iram0.text 段 - IRAM 代码 */
#define IRAM_TEXT         (uintptr_t)_iram_text   /* IRAM 代码段结束标记 */

/* .iram0.text_end 段 - IRAM 结束标记 (含 16B padding + 256B 对齐) */
#define IRAM_END          (uintptr_t)_iram_end   /* IRAM 结束地址 (PMS 对齐后) */

/*============================================================================
 * DRAM (Internal RAM) - Data in internal SRAM
 * Defined in: .dram0.data, .dram0.bss sections
 * Memory Region: dram0_0_seg (0x3FC88000 - 0x3FC88000 + SRAM_DRAM0_SIZE)
 *============================================================================*/

/* .dram0.data 段 - 已初始化数据 (从 Flash 加载) */
#define DATA_START        (uintptr_t)_data_start  /* 数据段起始地址 */
#define DATA_END          (uintptr_t)_data_end    /* 数据段结束地址 */
#define SDATA             (uintptr_t)_sdata       /* 数据段起始地址 (同 DATA_START) */
#define EDATA             (uintptr_t)_edata       /* 数据段结束地址 (同 DATA_END) */

/* .dram0.bss 段 - 未初始化数据 (运行时清零) */
#define BSS_START         (uintptr_t)_bss_start  /* BSS 段起始地址 */
#define BSS_END           (uintptr_t)_bss_end     /* BSS 段结束地址 */
#define SBSS              (uintptr_t)_sbss        /* BSS 段起始地址 (同 BSS_START) */
#define EBSS              (uintptr_t)_ebss        /* BSS 段结束地址 (同 BSS_END) */

/* 堆起始地址 (位于 .dram0.data 段结束后) */
#define SHEAP             (uintptr_t)_sheap       /* 堆起始地址 */

/*============================================================================
 * Flash IROM - Code mapped from external Flash
 * Defined in: .flash.text section
 * Memory Region: irom0_0_seg (虚拟地址映射, 实际存储在外部 Flash)
 *============================================================================*/

/* .flash.text 段 - Flash 代码段 (64KB 对齐) */
#define FLASH_TEXT_START  (uintptr_t)_stext      /* Flash 代码起始地址 */
#define FLASH_TEXT_END    (uintptr_t)_etext      /* Flash 代码结束地址 */

/* 指令预取保留区 (CPU 预取 16 字节) */
#define INSTR_RESERVED_START  (uintptr_t)_instruction_reserved_start  /* 指令保留区起始 */
#define INSTR_RESERVED_END    (uintptr_t)_instruction_reserved_end    /* 指令保留区结束 */

/* 镜像信息 (用于引导加载) */
#define IMAGE_IROM_VMA   (uintptr_t)_image_irom_vma   /* IROM 虚拟地址 (VMA) */
#define IMAGE_IROM_LMA   (uintptr_t)_image_irom_lma   /* IROM 加载地址 (LMA) */
#define IMAGE_IROM_SIZE  (uintptr_t)_image_irom_size  /* IROM 大小 */

/*============================================================================
 * Flash DROM - Read-only data mapped from external Flash
 * Defined in: .flash.rodata, .flash.rodata_dummy sections
 * Memory Region: drom0_0_seg (虚拟地址映射, 实际存储在外部 Flash)
 *============================================================================*/

/* .flash.rodata 段 - 只读数据段 (64KB 对齐) */
#define RODATA_START      (uintptr_t)_srodata    /* 只读数据起始地址 */
#define RODATA_END        (uintptr_t)_erodata    /* 只读数据结束地址 */

/* 只读数据保留区 */
#define RODATA_RESERVED_START  (uintptr_t)_rodata_reserved_start  /* 只读数据保留区起始 */
#define RODATA_RESERVED_END    (uintptr_t)_rodata_reserved_end    /* 只读数据保留区结束 */
#define RODATA_RESERVED_ALIGN  (uintptr_t)_rodata_reserved_align   /* 只读数据对齐要求 (64KB) */

/* .flash.rodata_dummy 段 - 虚拟只读段 (用于填充) */
#define FLASH_RODATA_DUMMY_START  (uintptr_t)_flash_rodata_dummy_start  /* 虚拟只读段起始 */

/* 镜像信息 (用于引导加载) */
#define IMAGE_DROM_VMA   (uintptr_t)_image_drom_vma   /* DROM 虚拟地址 (VMA) */
#define IMAGE_DROM_LMA   (uintptr_t)_image_drom_lma   /* DROM 加载地址 (LMA) */
#define IMAGE_DROM_SIZE  (uintptr_t)_image_drom_size  /* DROM 大小 */

/* 4 字节字面量池 */
#define LIT4_START       (uintptr_t)_lit4_start   /* 4字节字面量起始 */
#define LIT4_END         (uintptr_t)_lit4_end     /* 4字节字面量结束 */

/* 初始化函数 */
#define SINIT             (uintptr_t)_sinit        /* 初始化函数起始 */
#define EINIT             (uintptr_t)_einit        /* 初始化函数结束 */

/*============================================================================
 * C++ Initialization - C++ 构造器和析构器
 * Defined in: .flash.rodata section
 *============================================================================*/

/* C++ 全局对象构造器数组 */
#define INIT_ARRAY_START  (uintptr_t)__init_array_start  /* 构造器数组起始 */
#define INIT_ARRAY_END    (uintptr_t)__init_array_end    /* 构造器数组结束 */

/*============================================================================
 * TLS (Thread-Local Storage) - 线程本地存储
 * Defined in: .flash.rodata section (within .flash.rodata)
 *============================================================================*/

/* TLS 区域 */
#define THREAD_LOCAL_START    (uintptr_t)_thread_local_start  /* TLS 起始地址 */
#define THREAD_LOCAL_END      (uintptr_t)_thread_local_end    /* TLS 结束地址 */

/* TLS 数据 (已初始化) */
#define TDATA_START           (uintptr_t)_stdata   /* TLS 数据起始 */
#define TDATA_END             (uintptr_t)_etdata   /* TLS 数据结束 */

/* TLS BSS (未初始化) */
#define TBSS_START            (uintptr_t)_stbss    /* TLS BSS 起始 */
#define TBSS_END              (uintptr_t)_etbss    /* TLS BSS 结束 */

/*============================================================================
 * Exception Handling - Xtensa 异常处理表
 * Defined in: .flash.rodata section
 *============================================================================*/

/* 异常向量表 */
#define XT_EXCEPTION_TABLE        (uintptr_t)__XT_EXCEPTION_TABLE_  /* 异常表起始 */

/* 异常描述表 */
#define XT_EXCEPTION_DESCS       (uintptr_t)__XT_EXCEPTION_DESCS_  /* 异常描述起始 */
#define XT_EXCEPTION_DESCS_END    (uintptr_t)__XT_EXCEPTION_DESCS_END__  /* 异常描述结束 */

/*============================================================================
 * SOC Reserved Memory - ESP32-S3 保留内存区域
 * Defined in: .flash.rodata section
 *============================================================================*/

/* SOC 保留内存区域 (由 ESP-IDF 定义) */
#define SOC_RESERVED_MEMORY_REGION_START  (uintptr_t)soc_reserved_memory_region_start  /* SOC 保留区起始 */
#define SOC_RESERVED_MEMORY_REGION_END    (uintptr_t)soc_reserved_memory_region_end    /* SOC 保留区结束 */

/*============================================================================
 * System Initialization Functions - 系统初始化函数数组
 * Defined in: .flash.rodata section
 *============================================================================*/

/* ESP 系统初始化函数数组 (按优先级排序) */
#define ESP_SYSTEM_INIT_FN_ARRAY_START  (uintptr_t)_esp_system_init_fn_array_start  /* 系统初始化函数数组起始 */
#define ESP_SYSTEM_INIT_FN_ARRAY_END    (uintptr_t)_esp_system_init_fn_array_end    /* 系统初始化函数数组结束 */

/*============================================================================
 * External RAM (PSRAM) - 外部 PSRAM 内存
 * Defined in: .ext_ram.bss section (CONFIG_XTENSA_EXTMEM_BSS only)
 * Memory Region: extern_ram_seg
 *============================================================================*/

/* 外部 RAM BSS 段 (WiFi/网络等大型缓存) */
#define EXT_RAM_BSS_START   (uintptr_t)_ext_ram_bss_start  /* 外部 RAM BSS 起始 */
#define EXT_RAM_BSS_END     (uintptr_t)_ext_ram_bss_end    /* 外部 RAM BSS 结束 */

/*============================================================================
 * RTC Memory - 实时时钟内存
 * Defined in: .rtc.text, .rtc.force_fast, .rtc.heap, .rtc_reserved sections
 * Memory Region: rtc_iram_seg, rtc_slow_seg, rtc_reserved_seg
 *============================================================================*/

/* RTC 代码段 (运行在 RTC 快速内存) */
#define RTC_FAST_START     (uintptr_t)_rtc_fast_start    /* RTC 快速代码起始 */
#define RTC_TEXT_START     (uintptr_t)_rtc_text_start    /* RTC 代码起始 */
#define RTC_TEXT_END       (uintptr_t)_rtc_text_end       /* RTC 代码结束 */

/* RTC 强制快速内存 (RTC_FAST_ATTR) */
#define RTC_FORCE_FAST_START   (uintptr_t)_rtc_force_fast_start  /* RTC 强制快速起始 */
#define RTC_FORCE_FAST_END     (uintptr_t)_rtc_force_fast_end    /* RTC 强制快速结束 */

/* RTC 堆 (特殊用途堆) */
#define RTCHEAP            (uintptr_t)_srtcheap       /* RTC 堆起始地址 */

/* RTC 保留内存 (固定地址，深度睡眠保持) */
#define RTC_RESERVED_START    (uintptr_t)_rtc_reserved_start  /* RTC 保留区起始 */
#define RTC_RESERVED_END      (uintptr_t)_rtc_reserved_end    /* RTC 保留区结束 */

/****************************************************************************
 * Public Data
 ****************************************************************************/

/*============================================================================
 * IRAM Symbols - Defined in: .iram0.vectors, .iram0.text, .iram0.text_end
 *============================================================================*/

extern uint8_t _iram_start[];      /* IRAM 代码起始 */
extern uint8_t _iram_end[];        /* IRAM 结束 (含 PMS 对齐) */
extern uint8_t _init_start[];      /* 初始化代码起始 */
extern uint8_t _init_end[];        /* 初始化代码结束 */
extern uint8_t _iram_text[];       /* IRAM 代码段结束标记 */

/*============================================================================
 * DRAM Symbols - Defined in: .dram0.data, .dram0.bss
 *============================================================================*/

extern uint8_t _data_start[];      /* 数据段起始 */
extern uint8_t _data_end[];        /* 数据段结束 */
extern uint8_t _sdata[];           /* 数据段起始 (同 _data_start) */
extern uint8_t _edata[];           /* 数据段结束 (同 _data_end) */
extern uint8_t _bss_start[];       /* BSS 段起始 */
extern uint8_t _bss_end[];         /* BSS 段结束 */
extern uint8_t _sbss[];            /* BSS 段起始 (同 _bss_start) */
extern uint8_t _ebss[];            /* BSS 段结束 (同 _bss_end) */
extern uint8_t _sheap[];           /* 堆起始地址 */

/*============================================================================
 * Flash IROM Symbols - Defined in: .flash.text
 *============================================================================*/

extern uint8_t _stext[];                   /* Flash 代码起始 */
extern uint8_t _etext[];                   /* Flash 代码结束 */
extern uint8_t _instruction_reserved_start[];  /* 指令保留区起始 */
extern uint8_t _instruction_reserved_end[];    /* 指令保留区结束 */
extern uint8_t _image_irom_vma[];          /* IROM 虚拟地址 */
extern uint8_t _image_irom_lma[];          /* IROM 加载地址 */
extern uint8_t _image_irom_size[];          /* IROM 大小 */

/*============================================================================
 * Flash DROM Symbols - Defined in: .flash.rodata, .flash.rodata_dummy
 *============================================================================*/

extern uint8_t _srodata[];                 /* 只读数据起始 */
extern uint8_t _erodata[];                  /* 只读数据结束 */
extern uint8_t _rodata_reserved_start[];    /* 只读数据保留区起始 */
extern uint8_t _rodata_reserved_end[];      /* 只读数据保留区结束 */
extern uint8_t _rodata_reserved_align[];    /* 只读数据对齐要求 */
extern uint8_t _flash_rodata_dummy_start[]; /* 虚拟只读段起始 */
extern uint8_t _image_drom_vma[];          /* DROM 虚拟地址 */
extern uint8_t _image_drom_lma[];          /* DROM 加载地址 */
extern uint8_t _image_drom_size[];         /* DROM 大小 */
extern uint8_t _lit4_start[];               /* 4字节字面量起始 */
extern uint8_t _lit4_end[];                 /* 4字节字面量结束 */
extern uint8_t _sinit[];                    /* 初始化函数起始 */
extern uint8_t _einit[];                    /* 初始化函数结束 */

/*============================================================================
 * C++ Initialization Symbols - Defined in: .flash.rodata
 *============================================================================*/

extern uint8_t __init_array_start[];        /* C++ 构造器数组起始 */
extern uint8_t __init_array_end[];          /* C++ 构造器数组结束 */

/*============================================================================
 * TLS Symbols - Defined in: .flash.rodata (within .flash.rodata)
 *============================================================================*/

extern uint8_t _thread_local_start[];       /* TLS 起始 */
extern uint8_t _thread_local_end[];         /* TLS 结束 */
extern uint8_t _stdata[];                   /* TLS 数据起始 */
extern uint8_t _etdata[];                   /* TLS 数据结束 */
extern uint8_t _stbss[];                    /* TLS BSS 起始 */
extern uint8_t _etbss[];                    /* TLS BSS 结束 */

/*============================================================================
 * Exception Handling Symbols - Defined in: .flash.rodata
 *============================================================================*/

extern uint8_t __XT_EXCEPTION_TABLE_[];         /* 异常表 */
extern uint8_t __XT_EXCEPTION_DESCS_[];          /* 异常描述起始 */
extern uint8_t __XT_EXCEPTION_DESCS_END__[];     /* 异常描述结束 */

/*============================================================================
 * SOC Reserved Memory Symbols - Defined in: .flash.rodata
 *============================================================================*/

extern uint8_t soc_reserved_memory_region_start[];  /* SOC 保留区起始 */
extern uint8_t soc_reserved_memory_region_end[];    /* SOC 保留区结束 */

/*============================================================================
 * System Initialization Functions Symbols - Defined in: .flash.rodata
 *============================================================================*/

extern uint8_t _esp_system_init_fn_array_start[];  /* 系统初始化函数数组起始 */
extern uint8_t _esp_system_init_fn_array_end[];    /* 系统初始化函数数组结束 */

/*============================================================================
 * External RAM (PSRAM) Symbols - Defined in: .ext_ram.bss
 * (Only available when CONFIG_XTENSA_EXTMEM_BSS is enabled)
 *============================================================================*/

extern uint8_t _ext_ram_bss_start[];        /* 外部 RAM BSS 起始 */
extern uint8_t _ext_ram_bss_end[];          /* 外部 RAM BSS 结束 */

/*============================================================================
 * RTC Memory Symbols - Defined in: .rtc.text, .rtc.force_fast,
 *                           .rtc.heap, .rtc_reserved
 *============================================================================*/

extern uint8_t _rtc_fast_start[];           /* RTC 快速代码起始 */
extern uint8_t _rtc_text_start[];           /* RTC 代码起始 */
extern uint8_t _rtc_text_end[];             /* RTC 代码结束 */
extern uint8_t _rtc_force_fast_start[];     /* RTC 强制快速起始 */
extern uint8_t _rtc_force_fast_end[];       /* RTC 强制快速结束 */
extern uint8_t _srtcheap[];                 /* RTC 堆起始 */
extern uint8_t _rtc_reserved_start[];       /* RTC 保留区起始 */
extern uint8_t _rtc_reserved_end[];         /* RTC 保留区结束 */

#endif /* __BOARDS_XTENSA_ESP32S3_ESP32S3_BOSS1_CX_INCLUDE_BOARD_MEMORYMAP_H */
