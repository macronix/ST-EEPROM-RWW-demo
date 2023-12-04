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

#include "app.h"
#include "main.h"

#define config_APP_PRINTF_ENABLE 0
#define APP_PRINTF_ENABLE config_APP_PRINTF_ENABLE

extern uint32_t delay_timing;
/*
 * Function:      MxInit
 * Arguments:	  Mxic,  pointer to an mxchip structure of nor flash device.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for initializing the device and controller.
 */
int MxInit(MxChip *Mxic) {
    int Status;

#if APP_PRINTF_ENABLE
    Mx_printf("\r\n  ------MxInit Start!------\r\n");
#endif

    memset(Mxic, 0, sizeof(MxChip));

    Status = MxSoftwareInit(Mxic);
    if (Status != MXST_SUCCESS)
        return Status;

    Status = MxGetHcVer(Mxic->Priv);
    if (Status != MXST_SUCCESS)
        return Status;

    Status = MxHardwareInit(Mxic->Priv);
    if (Status != MXST_SUCCESS)
        return Status;

    Status = MxScanMode(Mxic);
    if (Status != MXST_SUCCESS)
        return Status;

    Status = MxChipReset(Mxic);
    if (Status != MXST_SUCCESS)
        return Status;

    Status = MxSoftwareInit(Mxic);
    if (Status != MXST_SUCCESS)
        return Status;

    Status = MxChangeMode(Mxic, MODE_DOPI, SELECT_4B);
    if (Status != MXST_SUCCESS)
        return Status;

    /* ------------------------------------------------------*/

#if APP_PRINTF_ENABLE
    Mx_printf("  ------MxInit End!------\r\n");
#endif
    return MXST_SUCCESS;
}

#ifdef BLOCK0_BASIC
/*
 * Function:      MxSetMode
 * Arguments:	  Mxic,        pointer to an mxchip structure of nor flash device.
 *                SetMode,     variable in which to store the operation mode to set.
 *                SetAddrMode, variable in which to store the address mode to set.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for setting the operation mode and address mode.
 */
int MxSetMode(MxChip *Mxic, u32 SetMode, u32 SetAddrMode) {
    int Status;
    MxSpi *Spi = Mxic->Priv;

    u8 Temp_HardwareMode = Spi->HardwareMode;
    Spi->HardwareMode = IOMode;

    Status = MxChangeMode(Mxic, SetMode, SetAddrMode);
    if (Status != MXST_SUCCESS) {
        Spi->HardwareMode = Temp_HardwareMode;
        return Status;
    }

    Spi->HardwareMode = Temp_HardwareMode;

    return MXST_SUCCESS;
}

/*
 * Function:      MxSetMode
 * Arguments:	  Mxic,        pointer to an mxchip structure of nor flash device.
 *                SetMode,     variable in which to store the MX25Rxx power mode to set.(High Performance Mode or Ultra Lower Power Mode)
 *                             1) ULTRA_LOW_POWER_MODE:  High Performance Mode
 *                             2) HIGH_PERFORMANCE_MODE: Ultra Lower Power Mode
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for setting the operation mode and address mode.
 */
int MxSetMX25RPowerMode(MxChip *Mxic, u8 SetMode) {
    int Status;
    u8 Sr;
    u8 Cr[2];
    u8 Sr_Cr[3];

    MxSpi *Spi = Mxic->Priv;

    u8 Temp_HardwareMode = Spi->HardwareMode;
    Spi->HardwareMode = IOMode;

    Status = MxRDSR(Mxic, &Sr);
    if (Status != MXST_SUCCESS) {
        Spi->HardwareMode = Temp_HardwareMode;
        return Status;
    }

    Status = MxRDCR(Mxic, &Cr[0]);
    if (Status != MXST_SUCCESS) {
        Spi->HardwareMode = Temp_HardwareMode;
        return Status;
    }

    Sr_Cr[0] = Sr;
    Sr_Cr[1] = Cr[0];
    Sr_Cr[2] = SetMode;

    Status = MxWRSR(Mxic, Sr_Cr);
    if (Status != MXST_SUCCESS) {
        Spi->HardwareMode = Temp_HardwareMode;
        return Status;
    }

#ifdef USING_MX25Rxx_DEVICE
    if(SetMode)
        Spi->MX25RPowerMode = HIGH_PERFORMANCE_MODE;
    else
        Spi->MX25RPowerMode = ULTRA_LOW_POWER_MODE;
#endif

    Spi->HardwareMode = Temp_HardwareMode;

    return MXST_SUCCESS;
}

/*
 * Function:      MxEnterIOMode
 * Arguments:      Mxic,        pointer to an mxchip structure of nor flash device.
 *
 * Return Value:  none
 * Description:   This function is for entering io mode.
 */
void MxEnterIOMode(MxChip *Mxic) {
#if APP_PRINTF_ENABLE
    Mx_printf("\r\n\t ----Enter IO mode---- \r\n");
#endif
    MxSpi *pSpi = Mxic->Priv;
    pSpi->HardwareMode = IOMode;
}

/*
 * Function:      MxEnterLinearMode(or called mapping )
 * Arguments:	  Mxic,        pointer to an mxchip structure of nor flash device.
 *
 * Return Value:  none
 * Description:   This function is for entering Linear mode.
 */
void MxEnterLinearMode(MxChip *Mxic) {
#if APP_PRINTF_ENABLE
    Mx_printf("\r\n\t ----Enter Linear mode---- \r\n");
#endif
    MxSpi *pSpi = Mxic->Priv;
    pSpi->HardwareMode = LnrMode;
}

/*
 * Function:      MxEnter LnrDmaMode
 * Arguments:	  Mxic,        pointer to an mxchip structure of nor flash device.
 *
 * Return Value:  none
 * Description:   This function is for entering linear DMA mode.
 *                STMPlatform none
 *                25F0A call LnrDMA mode
 */
void MxEnterLnrDmaMode(MxChip *Mxic) {
#if APP_PRINTF_ENABLE
    Mx_printf("\r\n\t ----Enter Linear DMA mode---- \r\n");
#endif
    MxSpi *pSpi = Mxic->Priv;
    pSpi->HardwareMode = LnrDmaMode;
}

/*
 * Function:      MxEnterSDmaMode
 * Arguments:	  Mxic,        pointer to an mxchip structure of nor flash device.
 *
 * Return Value:  none
 * Description:   This function is for entering SDMA mode.
 *                STMPlatform call DMA mode
 *                25F0A call SDMA mode
 */
void MxEnterSDmaMode(MxChip *Mxic) {
#if APP_PRINTF_ENABLE
    Mx_printf("\r\n\t ----Enter SDMA mode---- \r\n");
#endif
    MxSpi *pSpi = Mxic->Priv;
    pSpi->HardwareMode = SdmaMode;
}

int MxAddrSpanBank(u32 addr, u32 length) {
    u8 b0 = (addr & BANK_MASK) >> BANK_BITS;
    u8 b1 = ((addr + length) & BANK_MASK) >> BANK_BITS;
    return (b0 != b1);
}

/*
 * Function:      MxRead
 * Arguments:	  Mxic:      pointer to an mxchip structure of nor flash device.
 *                Addr:      device address to read.
 *                ByteCount: number of bytes to read.
 *                Buf:       pointer to a data buffer where the read data will be stored.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function issues the Read commands to SPI Flash and reads data from the array.
 *                Data size is specified by ByteCount.
 *                It is called by different Read commands functions like MxREAD, Mx8READ and etc.
 */
#ifndef RWW_DRIVER_SUPPORT
int MxRead(MxChip *Mxic, u32 Addr, u32 ByteCount, u8 *Buf)
{
    return Mxic->AppGrp._Read(Mxic, Addr, ByteCount, Buf);
}

int MxWrite(MxChip *Mxic, u32 Addr, u32 ByteCount, u8 *Buf)
{
    return Mxic->AppGrp._Write(Mxic, Addr, ByteCount, Buf);
}

int MxErase(MxChip *Mxic, u32 Addr, u32 EraseSizeCount)
{
    return Mxic->AppGrp._Erase(Mxic, Addr, EraseSizeCount);
}
#else
int MxRead(MxChip *Mxic, u32 Addr, u32 ByteCount, u8 *Buf) {
    int status;
    int busy_stat;
    u32 len;
    u32 cnt;
    if (MxAddrSpanBank(Addr, ByteCount)) {
        len = BANK_LEN - Addr % BANK_LEN;

        while ((busy_bank & 0x7F) & (1 << BANKS(Addr))) {
            taskYIELD();
        }
        xSemaphoreTake(xReadMutex, portMAX_DELAY);
        status = Mxic->AppGrp._Read(Mxic, Addr, len, Buf);
        xSemaphoreGive(xReadMutex);
        while (busy_bank & 0x80) {
            taskYIELD();
        }

        for (cnt = len; cnt < ByteCount; cnt += len) {

            len = ByteCount - cnt;
            if (len > BANK_LEN)
                len = BANK_LEN;

            while ((busy_bank & 0x7F) & (1 << BANKS(Addr))) {
                taskYIELD();
            }
            xSemaphoreTake(xReadMutex, portMAX_DELAY);
            status = Mxic->AppGrp._Read(Mxic, Addr + cnt, len, Buf + cnt);
            xSemaphoreGive(xReadMutex);
            while (busy_bank & 0x80) {
                taskYIELD();
            }
        }
    } else {
        while ((busy_bank & 0x7F) & (1 << BANKS(Addr))) {
            taskYIELD();
        }
        xSemaphoreTake(xReadMutex, portMAX_DELAY);
        status = Mxic->AppGrp._Read(Mxic, Addr, ByteCount, Buf);
        xSemaphoreGive(xReadMutex);
        if (busy_bank & 0x80)
            taskYIELD();
    }

    return status;
}

/*
 * Function:      MxWrite
 * Arguments:	  Mxic:      pointer to an mxchip structure of nor flash device.
 *                Addr:      device address to program.
 *                ByteCount: number of bytes to program.
 *                Buf:       pointer to a data buffer where the program data will be stored.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 *                MXST_TIMEOUT.
 * Description:   This function programs location to the specified data.
 *                It is called by different Read commands functions like MxPP, MxPP4B and etc.
 */
extern SemaphoreHandle_t xBinarySemaphore;

int MxWrite(MxChip *Mxic, u32 Addr, u32 ByteCount, u8 *Buf) {
    int status;
    int busy_stat;
    u32 cnt, total_num, len;

    busy_bank |= 0x80;

    for (cnt = 0; cnt < ByteCount; cnt += len) {
        len = ByteCount - cnt;
        if (len > Mxic->PageSz) {
            len = Mxic->PageSz;
        }
        while ((MxGetStatus(Mxic) & 0x01) || delay_timing) {
            taskYIELD();
        }
        xSemaphoreTake(xWriteMutex, portMAX_DELAY);
        status = Mxic->AppGrp._Write(Mxic, Addr + cnt, len, Buf + cnt);
        busy_bank |= 1 << BANKS(Addr + cnt);
        xSemaphoreGive(xWriteMutex);
        {
            vTaskPrioritySet(NULL, uxTaskPriorityGet(NULL) + 1);
            xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);
            vTaskPrioritySet(NULL, uxTaskPriorityGet(NULL) - 1);
        }
    }

    busy_bank &= 0x7F;

    return status;
}

/*
 * Function:      MxBufferRead
 * Arguments:	  Mxic:      pointer to an mxchip structure of nor flash device.
 *                Addr:      device address to read.
 *                ByteCount: number of bytes to read.
 *                Buf:       pointer to a data buffer where the read data will be stored.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for read the page buffer.
 */
int MxBufferRead(MxChip *Mxic, u32 Addr, u32 ByteCount, u8 *Buf) {
    int status;

    while (uxSemaphoreGetCount(xBufferMutex)) {
        taskYIELD();
    }

    xSemaphoreTake(xReadMutex, portMAX_DELAY);
    status = MxRDBUF(Mxic, Addr, ByteCount, Buf);
    xSemaphoreGive(xReadMutex);

    return status;
}

/*
 * Function:      MxBufferWrite
 * Arguments:	  Mxic:      pointer to an mxchip structure of nor flash device.
 *                Addr:      device address to write.
 *                ByteCount: number of bytes to write.
 *                Buf:       pointer to a data buffer where the write data will be stored.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for write the page buffer to implement RWW function.
 */
int MxBufferWrite(MxChip *Mxic, u32 Addr, u32 ByteCount, u8 *Buf) {
    int Status;

    busy_bank |= 0x80;

    while ((MxGetStatus(Mxic) & 0x01) || delay_timing) {
        taskYIELD();
    }

    if (uxSemaphoreGetCount(xBufferMutex)
            || (xSemaphoreGetMutexHolder(xBufferMutex)
                    != xTaskGetCurrentTaskHandle())) {
        xSemaphoreTake(xBufferMutex, portMAX_DELAY);
    }

    xSemaphoreTake(xWriteMutex, portMAX_DELAY);
    busy_bank |= 1 << BANKS(Addr);
    if (Mxic->WriteBuffStart == FALSE) {
        Mxic->WriteBuffStart = TRUE;
        Status = MxWRBI(Mxic, Addr, ByteCount, Buf);
    } else if (ByteCount > 0) {
        Status = MxWRCT(Mxic, Addr, ByteCount, Buf);
    }

    if (ByteCount == 0) {
        Status = MxWRCF(Mxic);
        STM32_Timer_Config(2);
        Mxic->WriteBuffStart = FALSE;
        xSemaphoreGive(xBufferMutex);
    }

    xSemaphoreGive(xWriteMutex);
    if (MxGetStatus(Mxic) & 0x01) {
        vTaskPrioritySet(NULL, uxTaskPriorityGet(NULL) + 1);
        xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);
        vTaskPrioritySet(NULL, uxTaskPriorityGet(NULL) - 1);
    }

    busy_bank &= 0x7F;
    return MXST_SUCCESS;
}

/*
 * Function:      MxErase
 * Arguments:	  Mxic:           pointer to an mxchip structure of nor flash device.
 *                Addr:           device address to erase.
 *                EraseSizeCount: number of block or sector to erase.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 *                MXST_TIMEOUT.
 * Description:   This function erases the data in the specified Block or Sector.
 *                 Function issues all required commands and polls for completion.
 *                 It is called by different Read commands functions like MxSE, MxSE4B and etc.
 */

int MxErase(MxChip *Mxic, u32 Addr, u32 EraseSizeCount) {
    int status;

    busy_bank |= 0x80;
    for (int i = 0; i < EraseSizeCount; i++) {
        while (/*!uxSemaphoreGetCount(xReadMutex) ||*/(MxGetStatus(Mxic) & 0x01) || delay_timing) {
            taskYIELD();
        }

        xSemaphoreTake(xWriteMutex, portMAX_DELAY);

        status = Mxic->AppGrp._Erase(Mxic, Addr + i * SECTOR4KB_SZ, 1);
        busy_bank |= 1 << BANKS(Addr+i*SECTOR4KB_SZ);
        xSemaphoreGive(xWriteMutex);
        if (MxGetStatus(Mxic) & 0x01) {
            vTaskPrioritySet(NULL, uxTaskPriorityGet(NULL) + 1);
            xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);
            vTaskPrioritySet(NULL, uxTaskPriorityGet(NULL) - 1);
        }
    }

    busy_bank &= 0x7F;

    return status;
}
#endif

#endif

#ifdef BLOCK1_SPECIAL_FUNCTION
/*
 * Function:      MxSuspend.
 * Arguments:	  Mxic,  pointer to an mxchip structure of nor flash device.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function suspends Sector-Erase, Block-Erase or Page-Program operations and conduct other operations.
 */
int MxSuspend(MxChip *Mxic) {
    return MxPGMERS_SUSPEND(Mxic);
}

/*
 * Function:      MxResume
 * Arguments:	  Mxic,  pointer to an mxchip structure of nor flash device.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function resumes Sector-Erase, Block-Erase or Page-Program operations.
 */
int MxResume(MxChip *Mxic) {
    return MxPGMERS_RESUME(Mxic);
}

/*
 * Function:      MxDeepPowerDown
 * Arguments:	  Mxic:  pointer to an mxchip structure of nor flash device.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for setting the device on the minimizing the power consumption.
 */
int MxDeepPowerDown(MxChip *Mxic) {
    int Status;
    MxSpi *Spi = Mxic->Priv;

    u8 Temp_HardwareMode = Spi->HardwareMode;
    Spi->HardwareMode = IOMode;

    Status = MxDP(Mxic);
    if (Status != MXST_SUCCESS) {
        Spi->HardwareMode = Temp_HardwareMode;
        return Status;
    }

    Spi->HardwareMode = Temp_HardwareMode;

    return MXST_SUCCESS;
}

/*
 * Function:      MxRealseDeepPowerDown
 * Arguments:	  Mxic:  pointer to an mxchip structure of nor flash device.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for putting the device in the Stand-by Power mode.
 */
int MxRealseDeepPowerDown(MxChip *Mxic) {
    int Status;
    MxSpi *Spi = Mxic->Priv;

    u8 Temp_HardwareMode = Spi->HardwareMode;
    Spi->HardwareMode = IOMode;

    Status = MxRDP(Mxic);
    if (Status != MXST_SUCCESS) {
        Spi->HardwareMode = Temp_HardwareMode;
        return Status;
    }

    Spi->HardwareMode = Temp_HardwareMode;

    return MXST_SUCCESS;
}

#endif

#ifdef BLOCK2_SERCURITY_OTP
/*
 * Function:      MxEnterOTP
 * Arguments:	  Mxic:  pointer to an mxchip structure of nor flash device.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for entering the secured OTP mode.
 */
int MxEnterOTP(MxChip *Mxic) {
    int Status;
    MxSpi *Spi = Mxic->Priv;

    u8 Temp_HardwareMode = Spi->HardwareMode;
    Spi->HardwareMode = IOMode;

    Status = MxENSO(Mxic);
    if (Status != MXST_SUCCESS) {
        Spi->HardwareMode = Temp_HardwareMode;
        return Status;
    }

    Spi->HardwareMode = Temp_HardwareMode;

    return MXST_SUCCESS;
}

/*
 * Function:      MxExitOTP
 * Arguments:	  Mxic:  pointer to an mxchip structure of nor flash device.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for exiting the secured OTP mode.
 */
int MxExitOTP(MxChip *Mxic) {
    int Status;
    MxSpi *Spi = Mxic->Priv;

    u8 Temp_HardwareMode = Spi->HardwareMode;
    Spi->HardwareMode = IOMode;

    Status = MxEXSO(Mxic);
    if (Status != MXST_SUCCESS) {
        Spi->HardwareMode = Temp_HardwareMode;
        return Status;
    }

    Spi->HardwareMode = Temp_HardwareMode;

    return MXST_SUCCESS;
}

/*
 * Function:      MxLockFlash
 * Arguments:	  Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of locked area.
 *                Len,   number of bytes to lock.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for writing protection a specified block (or sector) of flash memory.
 */
int MxLockFlash(MxChip *Mxic, u32 Addr, u64 Len) {
    int Status;
    MxSpi *Spi = Mxic->Priv;

    u8 Temp_HardwareMode = Spi->HardwareMode;
    Spi->HardwareMode = IOMode;

    Status = Mxic->AppGrp._Lock(Mxic, Addr, Len);
    if (Status != MXST_SUCCESS) {
        Spi->HardwareMode = Temp_HardwareMode;
        return Status;
    }

    Spi->HardwareMode = Temp_HardwareMode;

    return MXST_SUCCESS;
}

/*
 * Function:      MxUnlockFlash
 * Arguments:	  Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of unlocked area.
 *                Len,   number of bytes to unlock.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function will cancel the block (or sector)  write protection state.
 */
int MxUnlockFlash(MxChip *Mxic, u32 Addr, u64 Len) {
    int Status;
    MxSpi *Spi = Mxic->Priv;

    u8 Temp_HardwareMode = Spi->HardwareMode;
    Spi->HardwareMode = IOMode;

    Status = Mxic->AppGrp._Unlock(Mxic, Addr, Len);
    if (Status != MXST_SUCCESS) {
        Spi->HardwareMode = Temp_HardwareMode;
        return Status;
    }

    Spi->HardwareMode = Temp_HardwareMode;

    return MXST_SUCCESS;
}

/*
 * Function:      MxIsFlashLocked
 * Arguments:	  Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of checking area.
 *                Len,   number of bytes to check.
 * Return Value:  MXST_FAILURE.
 * 				  FLASH_IS_UNLOCKED.
 *                FLASH_IS_LOCKED.
 * Description:   This function is for checking if the block (or sector) is locked.
 */
int MxIsFlashLocked(MxChip *Mxic, u32 Addr, u64 Len) {
    int Status;
    MxSpi *Spi = Mxic->Priv;

    u8 Temp_HardwareMode = Spi->HardwareMode;
    Spi->HardwareMode = IOMode;

    Status = Mxic->AppGrp._IsLocked(Mxic, Addr, Len);
    if (Status != MXST_SUCCESS) {
        Spi->HardwareMode = Temp_HardwareMode;
        return Status;
    }

    Spi->HardwareMode = Temp_HardwareMode;

    return MXST_SUCCESS;
}

/*
 * Function:      MxDPBLockFlash
 * Arguments:	  Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of locked area in dynamic protection mode.
 *                Len,   number of bytes to lock in dynamic protection mode.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for writing protection a specified block (or sector) of flash memory in dynamic protection mode.
 */
int MxDPBLockFlash(MxChip *Mxic, u32 Addr, u64 Len) {
    int Status;
    MxSpi *Spi = Mxic->Priv;

    u8 Temp_HardwareMode = Spi->HardwareMode;
    Spi->HardwareMode = IOMode;

    Status = MxDpbLock(Mxic, Addr, Len);
    if (Status != MXST_SUCCESS) {
        Spi->HardwareMode = Temp_HardwareMode;
        return Status;
    }

    Spi->HardwareMode = Temp_HardwareMode;

    return MXST_SUCCESS;
}

/*
 * Function:      MxDPBUnlockFlash
 * Arguments:	  Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of unlocked area in dynamic protection mode.
 *                Len,   number of bytes to unlock in dynamic protection mode.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function will cancel the block (or sector)  write protection state in dynamic protection mode.
 */
int MxDPBUnlockFlash(MxChip *Mxic, u32 Addr, u64 Len) {
    int Status;
    MxSpi *Spi = Mxic->Priv;

    u8 Temp_HardwareMode = Spi->HardwareMode;
    Spi->HardwareMode = IOMode;

    Status = MxDpbUnlock(Mxic, Addr, Len);
    if (Status != MXST_SUCCESS) {
        Spi->HardwareMode = Temp_HardwareMode;
        return Status;
    }

    Spi->HardwareMode = Temp_HardwareMode;

    return MXST_SUCCESS;
}

/*
 * Function:      MxIsFlashDPBLocked
 * Arguments:	  Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of checking area in dynamic protection mode.
 *                Len,   number of bytes to check in dynamic protection mode.
 * Return Value:  MXST_FAILURE.
 * 				  FLASH_IS_UNLOCKED.
 *                FLASH_IS_LOCKED.
 * Description:   This function is for checking if the block (or sector) is locked in dynamic protection mode.
 */
int MxIsFlashDPBLocked(MxChip *Mxic, u32 Addr, u64 Len) {
    int Status;
    MxSpi *Spi = Mxic->Priv;

    u8 Temp_HardwareMode = Spi->HardwareMode;
    Spi->HardwareMode = IOMode;

    Status = MxDpbIsLocked(Mxic, Addr, Len);
    if (Status != MXST_SUCCESS) {
        Spi->HardwareMode = Temp_HardwareMode;
        return Status;
    }

    Spi->HardwareMode = Temp_HardwareMode;

    return MXST_SUCCESS;
}

/*
 * Function:      MxGetLockMode
 * Arguments:	  Mxic,  pointer to an mxchip structure of nor flash device.
 * Return Value:  MXST_FAILURE.
 *                BP_MODE.
 *                ASP_SOLID_MODE.
 *                ASP_PWD_MODE.
 *                SBP_MODE.
 * Description:   This function is for getting protection mode.
 */
int MxGetCurLockMode(MxChip *Mxic) {
    int Status;
    MxSpi *Spi = Mxic->Priv;

    u8 Temp_HardwareMode = Spi->HardwareMode;
    Spi->HardwareMode = IOMode;

    Status = MxQryLockMode(Mxic);
    if (Status != MXST_SUCCESS) {
        Spi->HardwareMode = Temp_HardwareMode;
        return Status;
    }

    Spi->HardwareMode = Temp_HardwareMode;

    return MXST_SUCCESS;
}

/*
 * Function:      MxSetLockMode
 * Arguments:	  Mxic,     pointer to an mxchip structure of nor flash device.
 * 				  LockMode, variable in which to store the lock mode to set.
 * Return Value:  MXST_SUCCESS.
 *				  MXST_FAILURE.
 *
 * Description:   This function is for getting protection mode.
 */
int MxSetLockMode(MxChip *Mxic, int LockMode) {
    int Status;
    u8 LrOld[2], LrNew[2];
    AppGrp *App = &Mxic->AppGrp;
    u8 CurLockMode = MxQryLockMode(Mxic);

    switch (LockMode) {
    case BP_MODE:
        if (CurLockMode == BP_MODE) {
            App->_Lock = MxLock;
            App->_Unlock = MxUnlock;
            App->_IsLocked = MxIsLocked;
        } else {
            Mx_printf("\t@warning:WPSEL bit is 1,cannot set to BP mode!\r\n");
            return CurLockMode;
        }
        break;
    case ASP_SOLID_MODE:
        if (Mxic->SPICmdList[MX_ID_REG_CMDS] & MX_SPB) {
            if (CurLockMode != ASP_PWD_MODE) {
                App->_Lock = MxAspLock;
                App->_Unlock = MxAspUnlock;
                App->_IsLocked = MxAspIsLocked;
                /*
                 *if CurLockMode is ASP_SOLID_MODE,no need set ASP_SOLID_MODE again
                 */
                if (CurLockMode != ASP_SOLID_MODE) {
                    Mx_printf("\tChange to ASP_SOLID_MODE!\r\n");
                    Status = MxAspWpselLock(Mxic);
                    if (Status != MXST_SUCCESS)
                        return Status;
                }
            } else {
                Mx_printf(
                        "\t@warning:PWD protect mode is set,cannot set to SP protect mode!\r\n");
                return CurLockMode;
            }
        } else {
            Mx_printf("\t@warning:Solid Protect mode is not supported!\r\n");
            return CurLockMode;
        }
        break;
    case ASP_PWD_MODE:
        if (Mxic->SPICmdList[MX_ID_REG_CMDS] & MX_PASS) {
            Status = MxRDLR(Mxic, &LrOld[0]);
            if (Status != MXST_SUCCESS)
                return Status;
            /*
             * LR bit 1(SPMLB) should be "1"(if 0, password protection mode is disable permanently(MX66L1G45G))
             * or bit 1 is reserved ,default is "1"(MX25UM51245G)
             */
            if (LrOld[0] & LR_SP_MASK)

            {
                App->_Lock = MxPwdLockSpb;
                App->_Unlock = MxPwdUnlockSpb;
                App->_IsLocked = MxAspIsLocked;

                /*
                 * if CurLockMode is ASP_PWD_MODE,no need set ASP_PWD_MODE again
                 */
                if (CurLockMode != ASP_PWD_MODE) {
                    Mx_printf("\tChange to ASP_PWD_MODE!\r\n");
                    Status = MxAspWpselLock(Mxic);
                    if (Status != MXST_SUCCESS)
                        return Status;

                    Status = MxWRPASS(Mxic, Mxic->Pwd);
                    if (Status != MXST_SUCCESS)
                        return Status;
                    Status = MxRDLR(Mxic, LrOld);
                    if (Status != MXST_SUCCESS)
                        return Status;
                    LrNew[0] = LrOld[0] & LR_PWD_EN;
                    Status = MxWRLR(Mxic, LrNew);
                    if (Status != MXST_SUCCESS)
                        return Status;
                }

            } else {
                Mx_printf(
                        "\t@warning:SP protect mode is set,cannot set to PWD protect mode!\r\n");
                App->_Lock = MxAspLock;
                App->_Unlock = MxAspUnlock;
                App->_IsLocked = MxAspIsLocked;
                return CurLockMode;
            }
        } else {
            Mx_printf("\t@warning:Password Protect mode is not supported!\r\n");
            return CurLockMode;
        }
        break;
    case SBP_MODE:
        if (Mxic->SPICmdList[MX_MS_RST_SECU_SUSP] & MX_SBLK) {
            Status = MxAspWpselLock(Mxic);
            if (Status != MXST_SUCCESS)
                return Status;
            App->_Lock = MxSingleBlockLock;
            App->_Unlock = MxSingleBlockUnlock;
            App->_IsLocked = MxSingleBlockIsLocked;
        } else {
            Mx_printf(
                    "\t@warning:Single Block Protect mode is not supported!\r\n");
            return CurLockMode;
        }
        break;
    default:
        break;
    }
    return MXST_SUCCESS;
}

#endif

#ifdef BLOCK0_BASIC
#ifdef CONTR_25F0A
/*
 * Function:     MxCalibration_PhaseDly
 * Arguments:	 Mxic,     pointer to an mxchip structure of nor flash device.
 * Return Value: None.
 * Description:  This function is for setting the value of phase when changing the frequency.
 */
int MxCalibration_PhaseDly(MxChip *Mxic)
{
    u8 CR2[1] = {0};
    u8 Flag = 0;
    u8 PhaseCode, StepCode, StartCode, EndCode;
    int Status;
    Mx_printf("\tStart Phase Delay Calibration  \r\n", 0);
    StartCode = 0;
    EndCode = 0;
    StepCode = 10;
    for (PhaseCode = 0; PhaseCode < 180; PhaseCode += StepCode)
    {
        MxWr32((u32 *)(BASEADDRESS_CLK + 0x0218), (PhaseCode * 1000) );
        MxWr32((u32 *)(BASEADDRESS_CLK + 0x025C), 0x07 );
        MxWr32((u32 *)(BASEADDRESS_CLK + 0x025C), 0x02 );
        while( !(MxRd32((u32 *)(BASEADDRESS_CLK + 0x0004)) & 0x01) );

        Status = MxRDCR2(Mxic, CR2_OPI_EN_ADDR, CR2);
        if (Status != MXST_SUCCESS)
        {
            if (Flag)
            {
                EndCode = (PhaseCode - StepCode);
                break;
            }
            Mx_printf("\t\tFail [PhaseCode:%03d]\r\n", PhaseCode);
        }
        else
        {
            if (!Flag)
            {
                Flag = 1;
                StartCode = PhaseCode;
            }
            Mx_printf("\t\tPASS [PhaseCode:%03d] [CR2:%02X %02X]\r\n", PhaseCode, CR2[1], CR2[0]);
        }
        Mx_printf("\t  [PhaseCode:%03d]\r\n", PhaseCode);
    }
    Mx_printf("\t\tStartCode:%03d, EndCode:%03d\r\n", StartCode, EndCode);
    if (Flag && (EndCode >= StartCode)) {
        Mx_printf("\t\tNew PhaseCode:%03d\r\n", (StartCode + EndCode) / 2);
        MxWr32((u32 *)(BASEADDRESS_CLK + 0x0218), (StartCode + EndCode) * 1000 / 2 );
        MxWr32((u32 *)(BASEADDRESS_CLK + 0x025C), 0x07 );
        MxWr32((u32 *)(BASEADDRESS_CLK + 0x025C), 0x02 );
        while( !(MxRd32((u32 *)(BASEADDRESS_CLK + 0x0004)) & 0x01) );
    }
    else {
        Mx_printf("\t\tCalibration [output delay] is not successful \r\n");
        return MXST_FAILURE;
    }
    return MXST_SUCCESS;
}
/*
 * Function:     MxCalibration_InputDly
 * Arguments:     Mxic,     pointer to an mxchip structure of nor flash device.
 * Return Value: None.
 * Description:  This function is for setting the value of input delay.
 */
int MxCalibration_InputDly(MxChip *Mxic)
{
    MxSpi *Spi = Mxic->Priv;
    MxHcRegs *Regs = (MxHcRegs *)Spi->BaseAddress;
    u8 Flag = 0;
    u8 CrOri[2], CR_R[2], CR_W[2];
    u8 IdlyCode, StepCode, StartCode, EndCode;
    u8 SrOri[2];
    u8 SR_W[2];
    int Status, i, n;

    u8 Preamble_R[16] = {0};
    u8 Preamble_DOPI[16] = {
        0x00, 0x00, 0xFF, 0xFF,
        0xFF, 0x00, 0x08, 0x00,
        0x00, 0xF7, 0xFF, 0x00,
        0x08, 0xF7, 0x00, 0xF7
    };
    u8 Preamble_SOPI[16] = {
        0x00, 0x00, 0xFF, 0xFF,
        0x00, 0xFF, 0x00, 0x08,
        0xF7, 0x00, 0x00, 0xFF,
        0xF7, 0x08, 0xF7, 0x00
    };
    u8 Preamble_SPI[1] = {0x0D};

    u8 *Preamble;
    u8 PreambleCnt;
    if(Spi->CurMode == MODE_DOPI)
    {
        Preamble = Preamble_DOPI;
        PreambleCnt = 16;
    }
    else if(Spi->CurMode == MODE_SOPI)
    {
        Preamble = Preamble_SOPI;
        PreambleCnt = 16;
    }
    else
    {
        Preamble = Preamble_SPI;
        PreambleCnt = 1;
    }

    Mx_printf("\tStart Input Delay Calibration \r\n", 0);

    for (n=0; n<4; n+=1)
    {
        MxWr32(&Regs->DataStrob, n);
        Mx_printf("  Regs->DataStrob: %02X\r\n", n);

        Status = MxRDCR(Mxic, CrOri);
        if (Status != MXST_SUCCESS)
            return Status;
        Mx_printf("\t\tBF PreambleEn, RDCR[%02X]\r\n", CrOri[0]);

        if((Spi->CurMode == MODE_SPI)||(Spi->CurMode == MODE_QPI))
        {
            u8 SrOri[2];
            Status = MxRDSR(Mxic, SrOri);
            if (Status != MXST_SUCCESS)
                return Status;
            u8 SR_W[2];
            SR_W[0] = SrOri[0];
            SR_W[1] = CrOri[0] | 0x10;
            Status = MxWRSR(Mxic, SR_W);
            if (Status != MXST_SUCCESS)
                return Status;
            Mx_printf("\t\tPreambleEn, WRSR[%02X] [%02X]\r\n", SR_W[0],SR_W[1]);
        }
        else
        {
            CR_W[0] = CrOri[0] | 0x10;
            Status = MxWRCR(Mxic, CR_W);
            Mx_printf("\t\tPreambleEn, WRCR[%02X]\r\n", CR_W[0]);
        }
        CR_R[0] = 0;
        Status = MxRDCR(Mxic, CR_R);
        Mx_printf("\t\tAF PreambleEn, RDCR[%02X]\r\n", CR_R[0]);

        if (!(CR_R[0] & 0x10))
        {
            Mx_printf("@@@@warning:Preamble maybe not enable !!\r\n");
        }

        Spi->PreambleEn = 1;
        StartCode = 0;
        EndCode = 31;
        StepCode = 1;
        for (IdlyCode=0; IdlyCode<32; IdlyCode+=StepCode)
        {
            MxWr32(&Regs->IdlyCode4, (IdlyCode<<24 | IdlyCode<<16 | IdlyCode<<8 | IdlyCode) );
            MxWr32(&Regs->IdlyCode5, (IdlyCode<<24 | IdlyCode<<16 | IdlyCode<<8 | IdlyCode) );

            if(Spi->CurMode == MODE_SPI)
                Status = MxFASTREAD(Mxic, 0, PreambleCnt, Preamble_R);
            else
                Status = MxRead(Mxic, 0, PreambleCnt, Preamble_R);
            if (Status != MXST_SUCCESS)
                return Status;

            for (i=0; i<PreambleCnt; i+=1)
            {
                if (Preamble_R[i] != Preamble[i]) {
                    Mx_printf("\tFail [IdlyCode:%03d]\r\n", IdlyCode);

                    Mx_printf("\t\tPreambleCnt @%2d  \r\n", i);
                    Mx_printf("\t\tPreamble   ==>[%02X] \r\n", Preamble[i]);
                    Mx_printf("\t\tPreamble_R ==>[%02X] \r\n", Preamble_R[i]);
                    break;
                }
            }
            if (i<PreambleCnt)
            {
                if (Flag)
                {
                    EndCode = IdlyCode - 1;
                    break;
                }
            }
            else
            {
                if (!Flag)
                {
                    Flag = 1;
                    StartCode = IdlyCode;
                }
                Mx_printf("\t\tPASS [IdlyCode:%03d]\r\n", IdlyCode);
            }
        }
        if(!EndCode)
            EndCode = StartCode;
        Mx_printf("\t\tStartCode:%03d, EndCode:%03d\r\n", StartCode, EndCode);

        if (Flag && (EndCode>=StartCode))
        {
            Mx_printf("\t\tNew IdlyCode:%03d\r\n", ((StartCode+EndCode)/2) );
            MxWr32(&Regs->IdlyCode4, (((StartCode+EndCode)/2)<<24 | ((StartCode+EndCode)/2)<<16 | ((StartCode+EndCode)/2)<<8 | ((StartCode+EndCode)/2)) );
            MxWr32(&Regs->IdlyCode5, (((StartCode+EndCode)/2)<<24 | ((StartCode+EndCode)/2)<<16 | ((StartCode+EndCode)/2)<<8 | ((StartCode+EndCode)/2)) );
            break;
        }
        else
        {
            if(i >= 3)
            {
                Mx_printf("\t\tCalibration [input delay] is not successful \r\n");
                if((Spi->CurMode == MODE_SPI)||(Spi->CurMode == MODE_QPI))
                {
                    SR_W[0] = SrOri[0];
                    SR_W[1] = CrOri[0];
                    Status = MxWRSR(Mxic, SR_W);
                    if (Status != MXST_SUCCESS)
                        return Status;
                }
                else
                {
                    CR_W[0] = CrOri[0];
                    Status = MxWRCR(Mxic, CR_W);
                    if (Status != MXST_SUCCESS) {
                        Mx_printf("\t\t[ERR] fail for disable PreambleEn\r\n", IdlyCode);
                        return Status;
                    }
                }
                Spi->PreambleEn = 0;
                return MXST_FAILURE;
            }
        }
    }
    /*
     * Remove PreambleEn
     */
    if((Spi->CurMode == MODE_SPI)||(Spi->CurMode == MODE_QPI))
    {
        SR_W[0] = SrOri[0];
        SR_W[1] = CrOri[0];
        Status = MxWRSR(Mxic, SR_W);
        if (Status != MXST_SUCCESS)
            return Status;
    }
    else
    {
        CR_W[0] = CrOri[0];
        Status = MxWRCR(Mxic, CR_W);
        if (Status != MXST_SUCCESS) {
            Mx_printf("\t\t[ERR] fail for disable PreambleEn\r\n", IdlyCode);
            return Status;
        }
    }
    CR_R[0] = 0;
    Status = MxRDCR(Mxic, CR_R);
    if (Status != MXST_SUCCESS)
        return Status;
    Mx_printf("\t\tAF Remove PreambleEn, RDCR[%02X]\r\n", CR_R[0]);
    if ((CR_R[0] & 0x10)==0)
        Spi->PreambleEn = 0;
    else
        Mx_printf("\t\tRemove PreambleEn fail\r\n");

    return MXST_SUCCESS;
}

/*
 * Function:     MxCalibration
 * Arguments:     Mxic,     pointer to an mxchip structure of nor flash device.
 * Return Value: None.
 * Description:  This function is for setting the value of phase and input delay.
 */
int MxCalibration(MxChip *Mxic)
{
    int Status;
    MxSpi *Spi = Mxic->Priv;

    if(Spi->CurMode & MODE_DOPI)
    {
        Status = MxCalibration_PhaseDly(Mxic);
        if (Status != MXST_SUCCESS)
            return Status;
    }

    Status = MxCalibration_InputDly(Mxic);
    if (Status == MXST_SUCCESS)
        return Status;

    return MXST_SUCCESS;
}
#endif
#endif
