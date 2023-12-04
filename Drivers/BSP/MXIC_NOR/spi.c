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

#include "spi.h"
#include "main.h"

#ifndef SPI_XFER_PERF
#define EXTRA_SZ    30
#define RDWR_BUF_SZ 256UL
static u8 ReadBuffer[EXTRA_SZ + RDWR_BUF_SZ], WriteBuffer[EXTRA_SZ + RDWR_BUF_SZ / 16];
#endif
extern SemaphoreHandle_t xBusMutex;
/*
 * Function:      MxAddr2Cmd
 * Arguments:      Spi,     pointer to an MxSpi structure of transfer.
 *                Addr,    the address to put into the send data buffer.
 *                CmdBuf,  the data will be send to controller.
 * Return Value:  None.
 * Description:   This function put the value of address into the send data buffer. This address is stored after the command code.
 */
static void MxAddr2Cmd(MxSpi *Spi, u32 Addr, u8 *CmdBuf) {
    int n;

    for (n = Spi->LenCmd; n <= Spi->LenCmd + Spi->LenAddr - 1; n++)
        CmdBuf[n] = Addr >> (Spi->LenAddr * 8 - (n - Spi->LenCmd + 1) * 8);
}
#ifdef SPI_XFER_PERF
int SpiFlashWrite(MxSpi *Spi, u32 Addr, u32 ByteCount, u8 *WrBuf, u8 WrCmd)
{
    int Status;
    u8 InstrBuf[6] = {0};
    int n;

    Spi->LenCmd = (Spi->CurMode & MODE_OPI) ? 2 : 1;

    for (n = 0; n < Spi->LenCmd; n++)
        InstrBuf[n] = (!n) ? WrCmd : ~WrCmd;
    MxAddr2Cmd(Spi, Addr, InstrBuf);

    Spi->IsRd = FALSE;

    Spi->TransFlag = XFER_START;
    Status = MxPolledTransfer(Spi, InstrBuf, NULL, Spi->LenCmd + Spi->LenAddr + Spi->LenDummy);
    if (Status != MXST_SUCCESS)
        return Status;

    Spi->TransFlag = XFER_END;
    return MxPolledTransfer(Spi, WrBuf, NULL, ByteCount);
}

int MxSpiFlashRead(MxSpi *Spi, u32 Addr, u32 ByteCount, u8 *RdBuf, u8 RdCmd)
{
    int Status;
    u8 InstrBuf[30];
    int n;

    memset(InstrBuf, 0xFF, 30);
    Spi->LenCmd = (Spi->CurMode & MODE_OPI) ? 2 : 1;

    for (n = 0; n < Spi->LenCmd; n++)
        InstrBuf[n] = (!n) ? RdCmd : ~RdCmd;
    MxAddr2Cmd(Spi, Addr, InstrBuf);

    Spi->IsRd = TRUE;

    Spi->TransFlag = XFER_START;
    Status = MxPolledTransfer(Spi, InstrBuf, NULL, Spi->LenCmd + Spi->LenAddr + Spi->LenDummy);
    if (Status != MXST_SUCCESS)
        return Status;

    Spi->TransFlag = XFER_END;
    return MxPolledTransfer(Spi, NULL, RdBuf, ByteCount);
}
#else

/*
 * Function:      SpiFlashWrite
 * Arguments:      Spi,       pointer to an MxSpi structure of transfer
 *                   Addr,      address to be written to
 *                   ByteCount, number of byte to write
 *                   WrBuf,     Pointer to a data buffer where the write data will be stored
 *                   WrCmd,     write command code to be written to the flash
 * Return Value:  MXST_SUCCESS
 *                MXST_FAILURE
 * Description:   This function prepares the data to be written and put them into data buffer,
 *                then call MxPolledTransfer function to start a write data transfer.
 */
int MxSpiFlashWrite(MxSpi *Spi, u32 Addr, u32 ByteCount, u8 *WrBuf, u8 WrCmd) {
    int n, status;
    u32 LenInst;
    /*
     * Setup the write command with the specified address and data for the flash
     */
#ifdef BLOCK3_SPECIAL_HARDWARE_MODE
    if((Spi->HardwareMode == IOMode) || (Spi->HardwareMode == SdmaMode))
#endif
    {
#ifdef RWW_DRIVER_SUPPORT
    xSemaphoreTake(xBusMutex, portMAX_DELAY);
#endif
        Spi->IsRd = FALSE;
        Spi->LenCmd = (Spi->CurMode & MODE_OPI) ? 2 : 1;
        Spi->TransFlag = XFER_START | XFER_END;
        LenInst = Spi->LenCmd + Spi->LenAddr;

        for (n = 0; n < Spi->LenCmd; n++)
            WriteBuffer[n] = (!n) ? WrCmd : ~WrCmd;
        MxAddr2Cmd(Spi, Addr, WriteBuffer);

        memcpy(WriteBuffer + LenInst, WrBuf, ByteCount);

        status = MxPolledTransfer(Spi, WriteBuffer, NULL, ByteCount + LenInst);
#ifdef RWW_DRIVER_SUPPORT
    xSemaphoreGive(xBusMutex);
#endif
        return status;
    }
#ifdef BLOCK3_SPECIAL_HARDWARE_MODE
    else
    {
        /*
         * LnrMode or LnrDmaMode
         */
        return MxLnrModeWrite(Spi, WrBuf, Addr, ByteCount, WrCmd);
    }
#endif
}

/*
 * Function:      SpiFlashRead
 * Arguments:      Spi,       pointer to an MxSpi structure of transfer.
 *                   Addr:      address to be read.
 *                   ByteCount, number of byte to read.
 *                   RdBuf:     pointer to a data buffer where the read data will be stored.
 *                   RdCmd:     read command code to be written to the flash.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function calls MxPolledTransfer function to start a read data transfer,
 *                then put the read data into data buffer.
 */
int MxSpiFlashRead(MxSpi *Spi, u32 Addr, u32 ByteCount, u8 *RdBuf, u8 RdCmd) {
    int Status;
    int n;
    u32 RdSz, LenInst;
    /*
     * Setup the read command with the specified address, data and dummy for the flash
     */
#ifdef RWW_DRIVER_SUPPORT
    xSemaphoreTake(xBusMutex, portMAX_DELAY);
#endif
    Spi->IsRd = TRUE;
    Spi->LenCmd = (Spi->CurMode & MODE_OPI) ? 2 : 1;
    Spi->TransFlag = XFER_START | XFER_END;

    /* Set up the number of dummy cycles, it's dependent on address bus.
     * e.g. (S: single data rate, D: double data rate)
     * 1S-1S-1S, 8 dummy cycles, DUMMY_CNT = 1
     * 1S-4S-4S, 8 dummy cycles, DUMMY_CNT = 4
     * 8D-8D-8D, 8 dummy cycles, DUMMY_CNT = 16
     */
    LenInst = Spi->LenCmd + Spi->LenAddr + Spi->LenDummy;

    for (n = 0; n < Spi->LenCmd; n++)
        WriteBuffer[n] = (!n) ? RdCmd : ~RdCmd;

#ifdef BLOCK3_SPECIAL_HARDWARE_MODE
    if((Spi->HardwareMode == IOMode) || (Spi->HardwareMode == SdmaMode))
#endif
    {
        for (; ByteCount; RdBuf += RdSz, Addr += RdSz, ByteCount -= RdSz) {
            RdSz = ByteCount > RDWR_BUF_SZ ? RDWR_BUF_SZ : ByteCount;

            MxAddr2Cmd(Spi, Addr, WriteBuffer);

            for (n = Spi->LenCmd + Spi->LenAddr; n < LenInst; n++)
                WriteBuffer[n] = 0xFF;

            for (n = 0; n < (RdSz + LenInst); n++)
                ReadBuffer[n] = 0;

            Status = MxPolledTransfer(Spi, WriteBuffer, ReadBuffer, RdSz + LenInst);

            if (Status != MXST_SUCCESS)
                return Status;

            if (Spi->HardwareMode == IOMode)
                memcpy(RdBuf, ReadBuffer + LenInst, RdSz);
            else
                memcpy(RdBuf, ReadBuffer, RdSz);
        }
    }
#ifdef BLOCK3_SPECIAL_HARDWARE_MODE
    else
    {
        /*
         * LnrMode or LnrDmaMode
         */
        Status = MxLnrModeRead(Spi, RdBuf, Addr, ByteCount, RdCmd);
        if (Status != MXST_SUCCESS)
            return Status;
    }
#endif

#ifdef RWW_DRIVER_SUPPORT
    xSemaphoreGive(xBusMutex);
#endif
    return MXST_SUCCESS;
}

#endif

