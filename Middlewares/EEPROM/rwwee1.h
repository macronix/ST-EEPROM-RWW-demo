/*
 * Copyright (c) 2022-2023 Macronix International Co. LTD. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RWWEE2_H_
#define RWWEE2_H_

#include "stdbool.h"
#include "stdio.h"

/* FreeRTOS includes */
#include "cmsis_os.h"
#include "stm32l4xx_hal.h"

/* STM32 includes */
//#include "stm32l4r9i_discovery_ospi_nor.h"
#include "mx25lm51245g.h"

#define __FREERTOS__

#ifdef __FREERTOS__
/* Use FreeRTOS */
typedef TimeOut_t osTimeOut;
#define osTaskDelay(xTicksToDelay) vTaskDelay(xTicksToDelay)
#define osTaskSetTimeOutState(pxTimeOut) vTaskSetTimeOutState(pxTimeOut)
#define osTaskCheckForTimeOut(pxTimeOut, pxTicksToWait) xTaskCheckForTimeOut(pxTimeOut, pxTicksToWait)
#else
/* Use Other RTOS */
typedef osTimeOut;
#define osTaskDelay(xTicksToDelay)
#define osTaskSetTimeOutState(pxTimeOut)
#define osTaskCheckForTimeOut(pxTimeOut, pxTicksToWait)
#error "please define RTOS APIs!"
#endif

/* Flash specifications */
#define MX_FLASH_CHUNK_SIZE             MX25LM51245G_CHUNK_SIZE
#define MX_FLASH_PAGE_SIZE              MX25LM51245G_PAGE_SIZE
#define MX_FLASH_SECTOR_SIZE            MX25LM51245G_SECTOR_SIZE
#define MX_FLASH_BLOCK_SIZE             MX25LM51245G_BLOCK_SIZE
#define MX_FLASH_TOTAL_SIZE             MX25LM51245G_FLASH_SIZE

#ifdef MX_FLASH_SUPPORT_RWW
/* Typical PP/SE/BE busy ticks */
#define MX_FLASH_PAGE_WRITE_TICKS       (1 / portTICK_PERIOD_MS)    /* include SW delay */
#define MX_FLASH_SECTOR_ERASE_TICKS     (24 / portTICK_PERIOD_MS)
#define MX_FLASH_BLOCK_ERASE_TICKS      (220 / portTICK_PERIOD_MS)

/* Flash bank information */
#define MX_FLASH_BANK_SIZE              MX25LM51245G_BANK_SIZE
#define MX_FLASH_BANK_SIZE_MASK         MX25LM51245G_BANK_SIZE_MASK
#define MX_FLASH_BANKS                  (MX_FLASH_TOTAL_SIZE / MX_FLASH_BANK_SIZE)
#endif

/* EEPROM physical layout */
#define MX_EEPROM_SECTORS_PER_CLUSTER   (32)
#define MX_EEPROM_CLUSTERS_PER_BANK     (112)
#define MX_EEPROM_BANKS                 (4)
#define MX_EEPROM_CLUSTER_SIZE          (MX_FLASH_SECTOR_SIZE * MX_EEPROM_SECTORS_PER_CLUSTER)
#define MX_EEPROM_BANK_SIZE             (MX_EEPROM_CLUSTER_SIZE * MX_EEPROM_CLUSTERS_PER_BANK)

#if defined(MX_FLASH_BANKS) && (MX_EEPROM_BANKS > MX_FLASH_BANKS)
#error "too many banks!"
#endif

#if (MX_EEPROM_SECTORS_PER_CLUSTER > 256)
#error "too many sectors per cluster!"
#endif

/* EEPROM parameters */
#define MX_EEPROM_HEADER_SIZE           (4)
#define MX_EEPROM_ENTRY_SIZE            (MX_FLASH_SECTOR_SIZE) //2048 or 4096
#define MX_EEPROM_PAGE_SIZE             (MX_EEPROM_ENTRY_SIZE - MX_EEPROM_HEADER_SIZE)
#define MX_EEPROM_ENTRIES_PER_SECTOR    (MX_FLASH_SECTOR_SIZE / MX_EEPROM_ENTRY_SIZE)
#define MX_EEPROM_SYSTEM_SECTOR         (MX_EEPROM_SECTORS_PER_CLUSTER - 1)
#define MX_EEPROM_SYSTEM_SECTOR_OFFSET  (MX_EEPROM_SYSTEM_SECTOR * MX_FLASH_SECTOR_SIZE)
#define MX_EEPROM_SYSTEM_ENTRY_SIZE     (16)
#define MX_EEPROM_SYSTEM_ENTRIES        (MX_FLASH_SECTOR_SIZE / MX_EEPROM_SYSTEM_ENTRY_SIZE)
#define MX_EEPROM_DATA_SECTORS          (MX_EEPROM_SYSTEM_SECTOR)
#define MX_EEPROM_FREE_SECTORS          (1)
#define MX_EEPROM_ENTRIES_PER_CLUSTER   (MX_EEPROM_ENTRIES_PER_SECTOR * MX_EEPROM_DATA_SECTORS)
#define MX_EEPROM_LPAS_PER_CLUSTER      (MX_EEPROM_DATA_SECTORS - MX_EEPROM_FREE_SECTORS)
#define MX_EEPROM_BLOCK_SIZE            (MX_EEPROM_PAGE_SIZE * MX_EEPROM_LPAS_PER_CLUSTER)
#define MX_EEPROM_BLOCKS                (MX_EEPROM_CLUSTERS_PER_BANK)
#define MX_EEPROM_SIZE                  (MX_EEPROM_BLOCK_SIZE * MX_EEPROM_BLOCKS)
#define MX_EEPROMS                      (MX_EEPROM_BANKS)
#define MX_EEPROM_TOTAL_SIZE            (MX_EEPROM_SIZE * MX_EEPROMS)

#if (MX_EEPROM_ENTRY_SIZE < MX_FLASH_CHUNK_SIZE)
#error "too small data entry size!"
#endif

#if (MX_EEPROM_ENTRIES_PER_SECTOR > 256)
#error "too many entries per sector!"
#endif

#if (MX_EEPROM_SYSTEM_ENTRY_SIZE < MX_FLASH_CHUNK_SIZE)
#error "too small system entry size!"
#endif

#define MX_EEPROM_HASH_CROSSBANK        0    /* Page hash */
#define MX_EEPROM_HASH_HYBRID           1    /* Block hash */
#define MX_EEPROM_HASH_SEQUENTIAL       2    /* Bank hash */

/* Address hash algorithm */
#define MX_EEPROM_HASH_AlGORITHM        MX_EEPROM_HASH_CROSSBANK

/* Wear leveling interval */
#define MX_EEPROM_WL_INTERVAL           10000

#ifdef MX_EEPROM_BACKGROUND_THREAD
#define MX_EEPROM_BG_THREAD_PRIORITY    osPriorityLow               /* Background thread priority */
#define MX_EEPROM_BG_THREAD_STACK_SIZE  256                         /* Background thread stack size */
#define MX_EEPROM_BG_THREAD_DELAY       10000                       /* Background thread delay (ms) */
#define MX_EEPROM_BG_THREAD_TIMEOUT     10                          /* Background thread timeout (ms) */
#endif

/* HW CRC protection */
#define MX_EEPROM_CRC_HW

#ifdef MX_EEPROM_PC_PROTECTION

#ifndef MX_EEPROM_CRC_HW
#define MX_EEPROM_CRC_HW
#endif

#if !defined(MX_FLASH_SUPPORT_RWW) && !defined(MX_EEPROM_ECC_CHECK)
#define MX_EEPROM_ECC_CHECK
#endif

#ifndef MX_EEPROM_READ_ROLLBACK
#define MX_EEPROM_READ_ROLLBACK
#endif

#endif

#if defined(MX_FLASH_SUPPORT_RWW) && defined(MX_EEPROM_ECC_CHECK)
#error "cannot read ECC status in RWW mode!"
#endif

#ifdef MX_EEPROM_CRC_HW
#define CRC16_POLY                      (0x1021)
#define CRC16_DATA_LENGTH               (MX_EEPROM_PAGE_SIZE / 4)
#endif

#define MX_EEPROM_WRITE_RETRIES         2    /* number of write retries */

#define MX_EEPROM_READ_RETRIES          2    /* number of read retries */

#if defined(MX_EEPROM_READ_ROLLBACK) && (MX_EEPROM_READ_RETRIES == 0)
#error "please set the number of read retries!"
#endif

#if defined(MX_GENERIC_RWW) && !defined(MX_FLASH_SUPPORT_RWW)
#error "please enable RWW feature!"
#endif

/* RWWEE ID: "MX" */
#define MFTL_ID            0x4D58

/* Error codes */
#define MX_OK              0    /* No error */
#define MX_EINVAL          1    /* Invalid argument */
#define MX_EFAULT          2    /* Bad address */
#define MX_ENOSPC          3    /* No space left on device */
#define MX_ENODEV          4    /* No such device */
#define MX_ENOMEM          5    /* Out of memory */
#define MX_EIO             6    /* I/O error */
#define MX_ENXIO           7    /* No such device or address */
#define MX_ENOFS           8    /* No valid file system */
#define MX_EECC            9    /* ECC failure */
#define MX_EPERM           10   /* Operation not permitted */
#define MX_EOS             11   /* OS error */

/* Maximum unsigned value */
#define DATA_NONE8         0xff
#define DATA_NONE16        0xffff
#define DATA_NONE32        0xffffffffUL

/* fred: For testing */
extern osMutexId UartLock;
#define pr_time(fmt, ...) ({                                \
    osMutexWait(UartLock, osWaitForever);                   \
    printf("[%lu] " fmt, osKernelSysTick(), ##__VA_ARGS__); \
    osMutexRelease(UartLock); })

#ifdef MX_DEBUG
#define mx_log(...)
#define mx_info(args...) do {printf(args);} while(0)
#define mx_err(args...) do {printf(args);} while(0)
#else
#define mx_log(...)
#define mx_info(...)
#define mx_err(...)
#endif

/* Macros */
#undef min_t
#define min_t(type, x, y) ({ \
    type __min1 = (x);       \
    type __min2 = (y);       \
    __min1 < __min2 ? __min1: __min2; })

/* RWWEE operations */
typedef enum {
    OPS_READ = 0x5244,
    OPS_WRITE = 0x7772,
    OPS_ERASE_BEGIN = 0x4553,
    OPS_ERASE_END = 0x6565,
    OPS_NONE = 0x4E4E,
} rwwee_ops;

#pragma pack(1)    /* byte alignment */

/* System Entry */
struct system_entry {
    uint16_t id;
    uint16_t ops;
    uint16_t arg;
    uint16_t cksum;
};

/* Data entry header */
struct eeprom_header {
    uint8_t LPA;
    uint8_t LPA_inv;
    uint16_t crc;
    uint8_t pad[MX_EEPROM_HEADER_SIZE - 4];
};

/* Data Entry */
struct eeprom_entry {
    struct eeprom_header header;
    uint8_t data[MX_EEPROM_PAGE_SIZE];
};

#pragma pack()    /* default alignment */

/* Bank information */
struct bank_info {
    uint32_t bank; /* current bank */
    uint32_t bank_offset; /* bank address */

    uint32_t block; /* current block */
    uint32_t block_offset; /* block address */
    struct eeprom_entry cache; /* entry cache */
    bool cache_dirty; /* cache status */

    /* address mapping */
    uint8_t l2ps[MX_EEPROM_LPAS_PER_CLUSTER];
    uint8_t l2pe[MX_EEPROM_LPAS_PER_CLUSTER];
    uint8_t p2l[MX_EEPROM_DATA_SECTORS]; /* TODO: bitmap */

    uint32_t dirty_block; /* obsoleted sector to be erased */
    uint32_t dirty_sector; /* obsoleted sector to be erased */

    /* system entry address */
    uint32_t sys_entry[MX_EEPROM_BLOCKS];

    osMutexId lock; /* bank mutex lock */

#ifdef MX_DEBUG
   /* sector erase count statistics */
   uint32_t eraseCnt[MX_EEPROM_BLOCKS][MX_EEPROM_SECTORS_PER_CLUSTER];
#endif
};

/* RWW information */
struct rww_info {
    bool initialized; /* EEPROM status */

#ifdef MX_FLASH_SUPPORT_RWW
    uint32_t busyBank;          /* current busy bank */
    TickType_t waitTicks;       /* ticks to wait */
    osTimeOut timeout;          /* busy start time */
    osMutexId busyLock;         /* RWW lock */
#endif

    osMutexId deviceLock; /* device lock */
};

/* EEPROM information */
struct eeprom_info {
    bool initialized; /* EEPROM status */

#ifdef MX_EEPROM_BACKGROUND_THREAD
    bool bgStart;                     /* Background thread control */
    osThreadId bgThreadID;            /* Background thread ID */
#endif

    struct bank_info bi[MX_EEPROMS]; /* bank info */

    osMutexId crcLock; /* HW CRC mutex lock */

    uint32_t rwCnt; /* User R/W statistics */
};

/* EEPROM parameter */
struct eeprom_param {
    uint32_t eeprom_page_size;
    uint32_t eeprom_bank_size;
    uint32_t eeprom_banks;
    uint32_t eeprom_total_size;
    uint32_t eeprom_hash_algorithm;
};

struct eeprom_api {
    int (*mx_eeprom_format)(void);
    int (*mx_eeprom_init)(void);
    void (*mx_eeprom_deinit)(void);
    int (*mx_eeprom_read)(uint32_t addr, uint32_t len, uint8_t *buf);
    int (*mx_eeprom_write)(uint32_t addr, uint32_t len, uint8_t *buf);
    int (*mx_eeprom_sync_write)(uint32_t addr, uint32_t len, uint8_t *buf);
    uint32_t offset;
    uint32_t size;
};

#endif /* RWWEE2_H_ */
