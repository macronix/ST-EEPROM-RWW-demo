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

#ifndef MX_DEFINE_H
#define MX_DEFINE_H

#define DEBUG 1
#define PLATFORM_ST 1

#ifdef PLATFORM_XILINX

#include "xil_printf.h"
#include "xil_io.h"
#include "xuartps_hw.h"
#include "xtime_l.h"
#include "sleep.h"
#include "xdmaps.h"

#define BASEADDRESS_UART 0xE0001000
#define DMA_DEVICE_ID    XPAR_XDMAPS_1_DEVICE_ID
#define GetChar()        XUartPs_RecvByte(BASEADDRESS_UART)
#define Mx_printf        xil_printf
#define MxTime           XTime
#define MxTime_GetTime   XTime_GetTime
#define HAL_Delay_Us     usleep

#ifdef CONTR_25F0A
#define BASEADDRESS        0x43C30000
#define BASEADDRESS_CLK    0x43C20000
#define AXI_SLAVE_BASEADDR 0xA0000000
#define FPGA3_CLK
#define FPGA0_CLK_ADDR     0xF8000170
#define FPGA3_CLK_ADDR     0xF80001A0
#define ARM_PLL            0x20
#define DDR_PLL            0x30
#define PLL_MASK           0x30

#elif CONTR_2504
/*
 * User Add
 */
#endif

#elif PLATFORM_ST
#include <stddef.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "stm32l4xx_hal.h"
#include "usart.h"
#include "mxic_spi_nor_timer.h"

#define Mx_printf           printf
#define HAL_Delay_Us        MXIC_SPI_NOR_HAL_Delay_Us //HAL_Delay
#define MxGetTime(TimeVal)  TimeVal = READ_REG(MXIC_SPI_NOR_TIM->CNT)
#define MxSetTime(TimeVal)  WRITE_REG(MXIC_SPI_NOR_TIM->CNT, TimeVal)
#define GetChar()           __serial_io_getchar()
#define QSPI_BASEADDR       0x90000000  //---for linear read
#define EXTERNAL_FLASH_SIZE 0x8FFFFFFF
#define RWW_DRIVER_SUPPORT

#ifdef USING_MX25Rxx_DEVICE
#define MX25R_ULTRA_LOW_POWER_MODE_FREQUENCY  8*1000000  //8MHz
#define MX25R_HIGH_PERFORMANCE_MODE_FREQUENCY 20*1000000 //10MHz

#define MX25R_POWER_TEST
#endif

#define MXIC_XIP_ENTER_CODE         0x5A
#define MXIC_XIP_EXIT_CODE          0xFF
#endif

typedef unsigned char u8; /**< unsigned 8-bit */
typedef char int8; /**< signed 8-bit */
typedef unsigned short u16; /**< unsigned 16-bit */
typedef short int16; /**< signed 16-bit */
typedef unsigned long u32; /**< unsigned 32-bit */
typedef unsigned long long u64; /**< unsigned 32-bit */
typedef long int32; /**< signed 32-bit */
typedef float Xfloat32; /**< 32-bit floating point */
typedef double Xfloat64; /**< 64-bit double precision FP */
typedef unsigned long Xboolean; /**< boolean (XTRUE or XFALSE) */

enum DeviceStatus {
    MXST_SUCCESS = 0L,
    MXST_FAILURE = 1L,
    MXST_TIMEOUT = 2L,
    MXST_DEVICE_IS_STARTED = 3L,
    MXST_DEVICE_IS_STOPPED = 4L,
    MXST_ID_NOT_MATCH = 5L,
    MXST_DEVICE_BUSY = 6L, /* device is busy */
    MXST_DEVICE_READY = 7L, /* device is ready */
    MXST_DEVICE_SPBLK_PROTECTED = 8L,
    MXST_DEVICE_SPB_IS_LOCKED = 9L,
    MXST_DEVICE_SPB_ISNOT_LOCKED = 10L,
};

typedef struct {
    u8 CRValue;
    u8 Dummy;
} RdDummy;

#ifndef TRUE
#define TRUE  1U
#endif
#ifndef FALSE
#define FALSE  0U
#endif

#define MX_COMPONENT_IS_READY     0x11111111U  /**< component has been initialized */
#define MX_COMPONENT_IS_STARTED   0x22222222U  /**< component has been started */

#define TEST_SZ_PERFORMANCE 256
#define TEST_SZ 16UL
#define PAGE_SZ 256UL
#define SECTOR_SZ 512UL
#define SECTOR4KB_SZ 0x1000UL
#define BLOCK32KB_SZ 0x8000UL
#define BLOCK64KB_SZ 0x10000UL

#define SPI_NOR_FLASH_MAX_ID_LEN 5
#define PASSWORD_LEN 8
#define PASSWORD_INIT_VALUE 0xFF

#define FREQ_INIT   20000000
#define FREQ_DEFAULT 266000000
#define FREQ_SPI_MAX 150000000

#define BLOCK0_BASIC

#define BLOCK1_SPECIAL_FUNCTION

#define BLOCK2_SERCURITY_OTP

#endif
