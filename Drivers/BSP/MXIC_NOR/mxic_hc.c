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

#include "mxic_hc.h"

#ifdef CONTR_25F0A
    #include "xil_cache.h"
    #include "xil_cache_l.h"
#endif

#ifdef CONTR_25F0A
    u8 SdmaBuf[32];
#endif

#ifdef PLATFORM_ST
OSPI_HandleTypeDef OSPIHandle;
volatile u8 CmdCplt = 0;
volatile u8 TxCplt = 0;
volatile u8 RxCplt = 0;
#endif

#ifdef CONTR_25F0A
/*
 * Function:      MxRd32
 * Arguments:      BaseAddr,  the address of register in host controller.
 * Return Value:  The value of the request register.
 * Description:   This function is used for getting the value of the request register.
 */
u32 inline MxRd32(u32 *BaseAddr)
{
    return *(volatile u32 *) BaseAddr;
}

/*
 * Function:      MxWr32
 * Arguments:      BaseAddr,  the address of register in host controller.
 *           Val,       the value of the request register to set.
 * Return Value:  None.
 * Description:   This function is used for setting the value of the request register.
 */
void inline MxWr32(u32 *BaseAddr, u32 Val)
{
    *(u32 *)BaseAddr = Val;
}
#endif

/*
 * Function:      MxHardwareInit
 * Arguments:      Spi,       pointer to an MxSpi structure of transfer.
 * Return Value:  MXST_SUCCESS.
 * Description:   This function is used for initializing the controller by set the registers of controller to proper value.
 */
int MxHardwareInit(MxSpi *Spi) {
#ifdef CONTR_25F0A
        MxHcRegs *Regs = (MxHcRegs *)Spi->BaseAddress;
    MxWr32(&Regs->DataStrob, 0x00000000);

    MxWr32(&Regs->IdlyCode4, INPUT_DELAY_INIT_VALUE);
    MxWr32(&Regs->IdlyCode5, INPUT_DELAY_INIT_VALUE);
    MxWr32(&Regs->IdlyCode0, INPUT_DELAY_INIT_VALUE);
    MxWr32(&Regs->IdlyCode1, INPUT_DELAY_INIT_VALUE);

    MxWr32(&Regs->IdlyCode2, INPUT_DELAY_INIT_VALUE);
    MxWr32(&Regs->IdlyCode3, INPUT_DELAY_INIT_VALUE);
    MxWr32(&Regs->IdlyCode6, INPUT_DELAY_INIT_VALUE);
    MxWr32(&Regs->IdlyCode7, INPUT_DELAY_INIT_VALUE);

    MxWr32(&Regs->IntrStsEn, INTRSTS_ALL_EN_MASK);

    MxWr32(&Regs->HcConfig,
        HC_IFC_SING_SPI |
        HC_LT_SPI_NOR |
        HC_SLV_LOWER |
        HC_MAN_CS_EN);

    MxWr32(&Regs->HcEn, HC_DISABLE);
    #elif PLATFORM_ST

    OSPIHandle.Instance = OCTOSPI2;

    if (HAL_OSPI_DeInit(&OSPIHandle) != HAL_OK) {
        return MXST_FAILURE;
    }

    OSPIHandle.Init.ClockPrescaler = 2;
    OSPIHandle.Init.FifoThreshold = 4;
    OSPIHandle.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
    OSPIHandle.Init.DeviceSize = POSITION_VAL(EXTERNAL_FLASH_SIZE) - 1;
    OSPIHandle.Init.ChipSelectHighTime = 2;
    OSPIHandle.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
    OSPIHandle.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
    OSPIHandle.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
    OSPIHandle.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
    OSPIHandle.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_ENABLE;
    OSPIHandle.Init.ChipSelectBoundary = 0;
    OSPIHandle.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
    if (HAL_OSPI_Init(&OSPIHandle) != HAL_OK) {
        return MXST_FAILURE;
    }

    /* ----4) Initialization MXIC SPI NOR Timer---- */
    if (MXICSPINORTimer_Init() != HAL_OK) {
        return MXST_FAILURE;
    }
#endif

    return MXST_SUCCESS;
}

/*
 * Function:      MxPolledTransfer
 * Arguments:      Spi,       pointer to an MxSpi structure of transfer.
 *               WrBuf,     pointer to a data buffer where the write data will be stored.
 *               RdBuf,     pointer to a data buffer where the read data will be stored.
 *               ByteCount, the byte count of the data will be transferred.
 * Return Value:  MXST_SUCCESS.
 * Description:   This function is used for transferring specified data on the bus in polled mode.
 */
int MxPolledTransfer(MxSpi *Spi, u8 *WrBuf, u8 *RdBuf, u32 ByteCount) {
#ifdef CONTR_25F0A

    MxHcRegs *Regs = (MxHcRegs*) Spi->BaseAddress;
    u8 TempByteCount;
    u32 RegHcEn, RegSs0Ctrl, RegHcConfig, DmaMasterConfig = 0;
    u32 *TempTxd, WrData, RdData;
    u8 i;
    u32 SysAddr = 0x00;

    /*
     * The RecvBufPtr argument can be NULL.
     */

    Xil_AssertNonvoid(Spi != NULL);
    Xil_AssertNonvoid(Spi->IsReady == XIL_COMPONENT_IS_READY);
    /*
     * Check whether there is another transfer in progress. Not thread-safe.
     */
    if (Spi->IsBusy) {
        return MXST_DEVICE_BUSY;
    }
    /*
     * Set the busy flag, which will be cleared when the transfer is
     * entirely done.
     */
    Spi->IsBusy = TRUE;

    /*
     * Set up buffer pointers.
     */
    Spi->SendBufferPtr = WrBuf;
    Spi->RecvBufferPtr = RdBuf;

    Spi->RequestedBytes = ByteCount;
    Spi->RemainingBytes = ByteCount;

    if (Spi->TransFlag & XFER_START) {
        /*
         * Set up bus IO for CMD, ADDR and DATA.
         * Neither write pattern 112 nor 114 is not supported.
         */
        switch (Spi->FlashProtocol) {
            case PROT_1_1_1:
                RegSs0Ctrl = SS0_CMD_BUS_IO_1 | SS0_ADDR_BUS_IO_1
                    | SS0_DATA_BUS_IO_1;
                break;
            case PROT_1_1D_1D:
                RegSs0Ctrl = SS0_CMD_BUS_IO_1 | SS0_ADDR_BUS_IO_1
                    | SS0_DATA_BUS_IO_1 | SS0_ADDR_BUS_DDR_EN
                    | SS0_DATA_BUS_DDR_EN;
                break;
            case PROT_1_1_2:
                RegSs0Ctrl = SS0_CMD_BUS_IO_1 | SS0_ADDR_BUS_IO_1
                    | SS0_DATA_BUS_IO_2;
                break;
            case PROT_1_2_2:
                RegSs0Ctrl = SS0_CMD_BUS_IO_1 | SS0_ADDR_BUS_IO_2
                    | SS0_DATA_BUS_IO_2;
                break;
            case PROT_1_2D_2D:
                RegSs0Ctrl = SS0_CMD_BUS_IO_1 | SS0_ADDR_BUS_IO_2
                    | SS0_DATA_BUS_IO_2 | SS0_ADDR_BUS_DDR_EN
                    | SS0_DATA_BUS_DDR_EN;
                break;
            case PROT_1_1_4:
                RegSs0Ctrl = SS0_CMD_BUS_IO_1 | SS0_ADDR_BUS_IO_1
                    | SS0_DATA_BUS_IO_4;
                break;
            case PROT_1_4_4:
                RegSs0Ctrl = SS0_CMD_BUS_IO_1 | SS0_ADDR_BUS_IO_4
                    | SS0_DATA_BUS_IO_4;
                break;
            case PROT_1_4D_4D:
                RegSs0Ctrl = SS0_CMD_BUS_IO_1 | SS0_ADDR_BUS_IO_4
                    | SS0_DATA_BUS_IO_4 | SS0_ADDR_BUS_DDR_EN
                    | SS0_DATA_BUS_DDR_EN;
                break;
            case PROT_4_4_4:
                RegSs0Ctrl = SS0_CMD_BUS_IO_4 | SS0_ADDR_BUS_IO_4
                    | SS0_DATA_BUS_IO_4;
                break;
            case PROT_4_4D_4D:
                RegSs0Ctrl = SS0_CMD_BUS_IO_4 | SS0_ADDR_BUS_IO_4
                    | SS0_DATA_BUS_IO_4 | SS0_ADDR_BUS_DDR_EN
                    | SS0_DATA_BUS_DDR_EN;
                break;
            case PROT_8_8_8:
                if (Spi->SopiDqs)
                    RegSs0Ctrl = SS0_CMD_BUS_IO_8 | SS0_ADDR_BUS_IO_8
                        | SS0_DATA_BUS_IO_8 | SS0_DUAL_CMD_EN | SS0_DQS_EN;
                else
                    RegSs0Ctrl = SS0_CMD_BUS_IO_8 | SS0_ADDR_BUS_IO_8
                        | SS0_DATA_BUS_IO_8 | SS0_DUAL_CMD_EN;
                break;
            case PROT_8D_8D_8D:
                RegSs0Ctrl = SS0_CMD_BUS_IO_8 | SS0_ADDR_BUS_IO_8
                    | SS0_DATA_BUS_IO_8 | SS0_CMD_BUS_DDR_EN
                    | SS0_ADDR_BUS_DDR_EN | SS0_DATA_BUS_DDR_EN
                    | SS0_DUAL_CMD_EN | SS0_DQS_EN;
                break;
            default:
                RegSs0Ctrl = 0;
                break;
        }

        if (Spi->LenDummy) {
            RegSs0Ctrl |= Spi->LenDummy << SS0_DUMMY_CNT_OFS;
        }
        /* Set up the number of address cycles. */
        RegSs0Ctrl |= Spi->LenAddr << SS0_ADDR_CNT_OFS;

        if (Spi->IsRd) {
            RegSs0Ctrl |= SS0_DD_READ;
        }

        /* Write Reg.SS0Ctrl */
        MxWr32(&Regs->Ss0Ctrl, RegSs0Ctrl);

        /* Write Reg.HcConfig */
        RegHcConfig = HC_IFC_SING_SPI | HC_LT_SPI_NOR | HC_SLV_LOWER
            | HC_MAN_CS_EN;
        /* Assert Data Pass. */
        if (Spi->DataPass) {
            RegHcConfig |= HC_DATA_PASS;
        } else
            RegHcConfig &= (~HC_DATA_PASS);
        MxWr32(&Regs->HcConfig, RegHcConfig);

        /* Enable the host controller. */
        RegHcEn = MxRd32(&Regs->HcEn);
        MxWr32(&Regs->HcEn, RegHcEn | HC_EN);

        /* Assert CS Start. */
        RegHcConfig |= HC_MAN_CS;

        MxWr32(&Regs->HcConfig, RegHcConfig);

#ifdef BLOCK3_SPECIAL_HARDWARE_MODE
        /* Assert SDMA. */
        if(Spi->HardwareMode == SdmaMode)
        {
            SysAddr = SdmaBuf;
            MxWr32(&Regs->SdmaAddr, SysAddr);
            Mx_printf("  SysAddr: %02X\r\n", SysAddr);
            MxWr32(&Regs->DmaMCnt, ByteCount - (Spi->LenCmd + Spi->LenAddr + Spi->LenDummy));
            Mx_printf("  SysAddr: %02X\r\n", MxRd32(&Regs->DmaMCnt));
            if (Spi->IsRd)
            {
                DmaMasterConfig |= DMAM_DD_READ;
            }
            DmaMasterConfig |= DMAM_EN;
            MxWr32(&Regs->DmaMConfig, DmaMasterConfig);
            Mx_printf("  SysAddr: %02X\r\n", MxRd32(&Regs->DmaMConfig));
        }
#endif
    }

    if(   (Spi->FlashProtocol == PROT_1_1D_1D) || (Spi->FlashProtocol == PROT_1_1_2) || (Spi->FlashProtocol == PROT_1_2_2)
       || (Spi->FlashProtocol == PROT_1_2D_2D) || (Spi->FlashProtocol == PROT_1_1_4) || (Spi->FlashProtocol == PROT_1_4_4)
       || (Spi->FlashProtocol == PROT_1_4D_4D) || (Spi->FlashProtocol == PROT_4_4D_4D)
       || (Spi->HardwareMode == SdmaMode) )
    {
        for(i=0; i<Spi->LenCmd + Spi->LenAddr + Spi->LenDummy; i++)
        {
            WrData = Spi->SendBufferPtr ? *(u32 *)Spi->SendBufferPtr : 0xFFFFFFFF;
            while (!(MxRd32(&Regs->IntrSts) & INTRSTS_TX_N_FULL));
            MxWr32(&Regs->Txd1, WrData);
            while (!(MxRd32(&Regs->IntrSts) & INTRSTS_RX_N_EMPT));
            RdData = MxRd32(&Regs->Rxd);
            Spi->SendBufferPtr += 1;
            Spi->RecvBufferPtr += 1;
        }
        ByteCount -= Spi->LenCmd + Spi->LenAddr + Spi->LenDummy;
    }

    if(Spi->HardwareMode == IOMode)
    {
        while (ByteCount) {
        TempByteCount = ByteCount > 4 ? 4 : ByteCount;
        switch (TempByteCount) {
            case 4:    TempTxd = &Regs->Txd0;    break;
            case 1:    TempTxd = &Regs->Txd1;    break;
            case 2:    TempTxd = &Regs->Txd2;    break;
            case 3:    TempTxd = &Regs->Txd3;    break;
        }
        WrData = Spi->SendBufferPtr ? *(u32 *)Spi->SendBufferPtr : 0xFFFFFFFF;
        while (!(MxRd32(&Regs->IntrSts) & INTRSTS_TX_N_FULL));
        MxWr32(TempTxd, WrData);
        u32 i = 0;
        do
        {
            i ++;
            if(i > 3000)
            {
                Mx_printf("\t@warning:wait for DQS time out\r\n", WrData);
                /* De-assert CS. */
                MxWr32(&Regs->HcConfig, RegHcConfig & ~HC_MAN_CS);
                /* Disable the host controller. */
                MxWr32(&Regs->HcEn, HC_DISABLE);
                Spi->IsBusy = FALSE;
                return MXST_TIMEOUT;
            }
        }
        while (!(MxRd32(&Regs->IntrSts) & INTRSTS_RX_N_EMPT));
        RdData = MxRd32(&Regs->Rxd);
        if (Spi->RecvBufferPtr) {
            *(u32 *)Spi->RecvBufferPtr = RdData >> (8 * (4 - TempByteCount));
            Spi->RecvBufferPtr += TempByteCount;
        }
        Spi->SendBufferPtr += TempByteCount;
        ByteCount -= TempByteCount;
        }
    }

#ifdef BLOCK3_SPECIAL_HARDWARE_MODE
    else if (Spi->HardwareMode == SdmaMode) {
        if (!Spi->IsRd) {
            memcpy((void*) SysAddr, (Spi->SendBufferPtr), ByteCount);
            Xil_DCacheFlushRange(SysAddr, ByteCount);
        } else
            Xil_DCacheInvalidateRange(SysAddr, ByteCount);

        /* Start DMA Master Transfer */
        MxWr32(&Regs->DmaMConfig, DmaMasterConfig | DMAM_START);
        Mx_printf("  SysAddr2: %02X\r\n", DmaMasterConfig);
        Mx_printf("  SysAddr3: %02X\r\n", DmaMasterConfig | DMAM_START);
        Mx_printf("  SysAddr4: %02X\r\n", MxRd32(&Regs->DmaMConfig));
        HAL_Delay_Us(100);
        Mx_printf("  SysAddr: %02X\r\n", MxRd32(&Regs->IntrSts));
        while (!(MxRd32(&Regs->IntrSts) & INTRSTS_DMA_FINISH));
        Mx_printf("  SysAddr: %02X\r\n", MxRd32(&Regs->IntrSts));

        MxWr32(&Regs->IntrSts, MxRd32(&Regs->IntrSts) | INTRSTS_DMA_FINISH | INTRSTS_SDMA_INT);
        DmaMasterConfig = 0;
        MxWr32(&Regs->DmaMConfig, DmaMasterConfig);
        if (Spi->IsRd) {
            Xil_DCacheInvalidateRange(SysAddr, ByteCount);
            memcpy((void*) RdBuf, (const void*) (SysAddr), ByteCount);
        }
    }
#endif

    if (Spi->TransFlag & XFER_END) {
        /* De-assert CS. */
        MxWr32(&Regs->HcConfig, RegHcConfig & ~HC_MAN_CS);

        /* Disable the host controller. */
        MxWr32(&Regs->HcEn, HC_DISABLE);
    }
    Spi->IsBusy = FALSE;

#elif PLATFORM_ST

    OSPI_RegularCmdTypeDef s_command;
    uint8_t n, ExtraSz = Spi->LenCmd + Spi->LenAddr + Spi->LenDummy;
    uint32_t Addr = 0;

    uint32_t b_t = ByteCount;
    uint32_t e_t = ExtraSz;

    if (Spi->IsBusy) {
        printf("device busy\r\n");
        return MXST_DEVICE_BUSY;
    }

    Spi->IsBusy = TRUE;

    for (n = 0; n <= Spi->LenAddr - 1; n++)
        Addr |= (uint32_t) WrBuf[Spi->LenCmd + n] << ((Spi->LenAddr - 1 - n) * 8);

    s_command.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    s_command.FlashId = HAL_OSPI_FLASH_ID_1;
    s_command.Instruction = WrBuf[0];
    s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    s_command.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    s_command.Address = Addr;
    s_command.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
    s_command.NbData = ByteCount - ExtraSz;
    s_command.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
    s_command.DummyCycles = Spi->LenDummy;
    s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    s_command.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;
    s_command.DQSMode = HAL_OSPI_DQS_DISABLE;

    switch (Spi->FlashProtocol) {
        case PROT_1_1_1:
            s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
            s_command.DataMode = HAL_OSPI_DATA_1_LINE;
            break;

        case PROT_1_1D_1D:
            s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
            s_command.DataMode = HAL_OSPI_DATA_1_LINE;
            s_command.DataDtrMode = HAL_OSPI_DATA_DTR_ENABLE;
            break;

        case PROT_1_1_2:
            s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
            s_command.DataMode = HAL_OSPI_DATA_2_LINES;
            break;

        case PROT_1_1D_2D:
            s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
            s_command.DataMode = HAL_OSPI_DATA_2_LINES;
            s_command.DataDtrMode = HAL_OSPI_DATA_DTR_ENABLE;
            break;

        case PROT_1_2_2:
            s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode = HAL_OSPI_ADDRESS_2_LINES;
            s_command.DataMode = HAL_OSPI_DATA_2_LINES;
            break;

        case PROT_1_2D_2D:
            s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode = HAL_OSPI_ADDRESS_2_LINES;
            s_command.DataMode = HAL_OSPI_DATA_2_LINES;
            s_command.DataDtrMode = HAL_OSPI_DATA_DTR_ENABLE;
            break;

        case PROT_1_1_4:
            s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
            s_command.DataMode = HAL_OSPI_DATA_4_LINES;
            break;

        case PROT_1_1D_4D:
            s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
            s_command.DataMode = HAL_OSPI_DATA_4_LINES;
            s_command.DataDtrMode = HAL_OSPI_DATA_DTR_ENABLE;
            break;

        case PROT_1_4_4:
            s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode = HAL_OSPI_ADDRESS_4_LINES;
            s_command.DataMode = HAL_OSPI_DATA_4_LINES;

            if (
#ifdef USING_MX25Rxx_DEVICE
    (Spi->MX25RPowerMode == HIGH_PERFORMANCE_MODE) &&
#endif
            (Spi->IsRd)) {

#if MXIC_HC_PRINTF_ENABLE
    Mx_printf("Performance Enhance Mode Entered \r\n");
#endif

                s_command.DummyCycles = Spi->LenDummy - 2;

                s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
                s_command.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
                s_command.AlternateBytes = MXIC_XIP_ENTER_CODE;

                s_command.SIOOMode = HAL_OSPI_SIOO_INST_ONLY_FIRST_CMD;
            }
            break;

        case PROT_1_4D_4D:
            s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode = HAL_OSPI_ADDRESS_4_LINES;
            s_command.DataMode = HAL_OSPI_DATA_4_LINES;
            s_command.DataDtrMode = HAL_OSPI_DATA_DTR_ENABLE;

            if (
#ifdef USING_MX25Rxx_DEVICE
    (Spi->MX25RPowerMode == HIGH_PERFORMANCE_MODE) &&
#endif
            (Spi->IsRd)) {
#if MXIC_HC_PRINTF_ENABLE
                Mx_printf("Performance Enhance Mode Entered \r\n");
#endif

                s_command.DummyCycles = Spi->LenDummy - 2;

                s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
                s_command.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
                s_command.AlternateBytes = MXIC_XIP_ENTER_CODE;

                s_command.SIOOMode = HAL_OSPI_SIOO_INST_ONLY_FIRST_CMD;
            }
            break;

        case PROT_4_4_4:
            s_command.InstructionMode = HAL_OSPI_INSTRUCTION_4_LINES;
            s_command.AddressMode = HAL_OSPI_ADDRESS_4_LINES;
            s_command.DataMode = HAL_OSPI_DATA_4_LINES;

            if (
#ifdef USING_MX25Rxx_DEVICE
    (Spi->MX25RPowerMode == HIGH_PERFORMANCE_MODE) &&
#endif
            (Spi->IsRd)) {
#if MXIC_HC_PRINTF_ENABLE
    Mx_printf("Performance Enhance Mode Entered \r\n");
#endif

                s_command.DummyCycles = Spi->LenDummy - 2;

                s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
                s_command.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
                s_command.AlternateBytes = MXIC_XIP_ENTER_CODE;

                s_command.SIOOMode = HAL_OSPI_SIOO_INST_ONLY_FIRST_CMD;
            }
            break;

        case PROT_4_4D_4D:
            s_command.InstructionMode = HAL_OSPI_INSTRUCTION_4_LINES;
            s_command.AddressMode = HAL_OSPI_ADDRESS_4_LINES;
            s_command.DataMode = HAL_OSPI_DATA_4_LINES;
            s_command.DataDtrMode = HAL_OSPI_DATA_DTR_ENABLE;

            if (Spi->IsRd) {
                s_command.DummyCycles = Spi->LenDummy - 2;

                s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
                s_command.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
                s_command.AlternateBytes = MXIC_XIP_ENTER_CODE;

                s_command.SIOOMode = HAL_OSPI_SIOO_INST_ONLY_FIRST_CMD;
            }
            break;

        case PROT_8_8_8:
            s_command.Instruction = (WrBuf[0] << 8) | WrBuf[1];
            s_command.InstructionMode = HAL_OSPI_INSTRUCTION_8_LINES;
            s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
            s_command.InstructionSize = HAL_OSPI_INSTRUCTION_16_BITS;
            s_command.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
            s_command.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
            s_command.DataMode = HAL_OSPI_DATA_8_LINES;
            s_command.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
            s_command.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;
            s_command.DQSMode = HAL_OSPI_DQS_DISABLE;
            break;

        case PROT_8D_8D_8D:
            s_command.Instruction = (WrBuf[0] << 8) | WrBuf[1];
            s_command.InstructionMode = HAL_OSPI_INSTRUCTION_8_LINES;
            s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_ENABLE;
            s_command.InstructionSize = HAL_OSPI_INSTRUCTION_16_BITS;
            s_command.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
            s_command.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_ENABLE;
            s_command.DataMode = HAL_OSPI_DATA_8_LINES;
            s_command.DataDtrMode = HAL_OSPI_DATA_DTR_ENABLE;
            s_command.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;
            s_command.DQSMode = HAL_OSPI_DQS_ENABLE;
            break;
    }

    /*correct  s_command.DataMode*//*very important*/
    if ((ByteCount - ExtraSz) == 0)
        s_command.DataMode = HAL_OSPI_DATA_NONE;

    switch (Spi->LenAddr) {
        case 0:
            s_command.AddressMode = HAL_OSPI_ADDRESS_NONE;
            break;
        case 1:
            s_command.AddressSize = HAL_OSPI_ADDRESS_8_BITS;
            break;
        case 2:
            s_command.AddressSize = HAL_OSPI_ADDRESS_16_BITS;
            break;
        case 3:
            s_command.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
            break;
        case 4:
            s_command.AddressSize = HAL_OSPI_ADDRESS_32_BITS;
            break;
    }

    if (Spi->IsRd)/*read*/
    {

        /* Configure the command */
        if (HAL_OSPI_Command(&OSPIHandle, &s_command,
                HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
            printf("QSPI com error\r\n");
            return MXST_FAILURE; //return QSPI_ERROR;
        }

        /* Reception of the data */
        if (Spi->HardwareMode == IOMode)/*IO mode*/
        {
            if (HAL_OSPI_Receive(&OSPIHandle, RdBuf + ExtraSz,
                    HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
                printf("receive error\r\n");

                Mx_printf(" s_command.Instruction     :      %lX\r\n",
                        s_command.Instruction);
                Mx_printf(" s_command.InstructionMode :      %lX\r\n",
                        s_command.InstructionMode);
                Mx_printf(" s_command.InstructionSize :      %lX\r\n",
                        s_command.InstructionSize);
                Mx_printf(" s_command.InstructionDtrMode:    %lX\r\n",
                        s_command.InstructionDtrMode);
                Mx_printf(" s_command.AddressMode     :      %lX\r\n",
                        s_command.AddressMode);
                Mx_printf(" s_command.AddressSize     :      %lX\r\n",
                        s_command.AddressSize);
                Mx_printf(" s_command.Address         :      %lX\r\n",
                        s_command.Address);
                Mx_printf(" s_command.AddressDtrMode  :      %lX\r\n",
                        s_command.AddressDtrMode);

                Mx_printf(" s_command.DataMode        :      %lX\r\n",
                        s_command.DataMode);
                Mx_printf(" s_command.DataDtrMode     :      %lX\r\n",
                        s_command.DataDtrMode);
                Mx_printf(" s_command.NbData          :      %lX\r\n",
                        s_command.NbData);
                Mx_printf(" s_command.DummyCycles     :      %lX\r\n",
                        s_command.DummyCycles);
                Mx_printf(" s_command.DQSMode           :      %lX\r\n",
                        s_command.DQSMode);
                Mx_printf(" s_command.SIOOMode        :      %lX\r\n",
                        s_command.SIOOMode);

                return MXST_FAILURE; //return QSPI_ERROR;
            }
        } else if (Spi->HardwareMode == SdmaMode)/*SDMA mode*/
        {
            if (HAL_OSPI_Receive_DMA(&OSPIHandle, RdBuf) != HAL_OK)
                return MXST_FAILURE;

            RxCplt = 0;
        } else/*mode error*/
        {
#if MXIC_HC_PRINTF_ENABLE
    Mx_printf("QSPI read mode error\r\n");
#endif
            return MXST_FAILURE;
        }
    } else/*write*/
    {
        /***************for debug use***************************/

        if (Spi->HardwareMode == IOMode)/*IO mode*/
        {

            if (HAL_OSPI_Command(&OSPIHandle, &s_command,
                HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
                printf("write command failed\r\n");
                return MXST_FAILURE;
            }

        } else if (Spi->HardwareMode == SdmaMode)/*SDMA mode*/
        {
            if ((ByteCount - ExtraSz) == 0)/*no data phase type*/
            {
                if (HAL_OSPI_Command_IT(&OSPIHandle, &s_command) != HAL_OK)
                    return MXST_FAILURE;

                while (CmdCplt == 0) {
                }
                CmdCplt = 0;

            } else /*exist data phase type*/
            {
                if (HAL_OSPI_Command(&OSPIHandle, &s_command,
                    HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
                    return MXST_FAILURE;
            }
        }

        /* Transmission of the data */
        if (ByteCount - ExtraSz) {
            if (Spi->HardwareMode == IOMode)/*IO mode*/
            {
                if (HAL_OSPI_Transmit(&OSPIHandle, WrBuf + ExtraSz,
                    HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
                    printf("write data failed\r\n");
                    printf("bytecount: %d, extrasz: %d\r\n", ByteCount, ExtraSz);
                    printf("b_t: %d, e_t: %d\r\n", b_t, e_t);
                    Mx_printf(" s_command.Instruction : %lX\r\n", s_command.Instruction);
                    Mx_printf(" s_command.InstructionMode: %lX\r\n", s_command.InstructionMode);
                    Mx_printf(" s_command.InstructionSize: %lX\r\n", s_command.InstructionSize);
                    Mx_printf(" s_command.InstructionDtrMode: %lX\r\n", s_command.InstructionDtrMode);
                    Mx_printf(" s_command.AddressMode: %lX\r\n", s_command.AddressMode);
                    Mx_printf(" s_command.AddressSize: %lX\r\n", s_command.AddressSize);
                    Mx_printf(" s_command.Address: %lX\r\n", s_command.Address);
                    Mx_printf(" s_command.AddressDtrMode: %lX\r\n", s_command.AddressDtrMode);

                    Mx_printf(" s_command.DataMode: %lX\r\n", s_command.DataMode);
                    Mx_printf(" s_command.DataDtrMode: %lX\r\n", s_command.DataDtrMode);
                    Mx_printf(" s_command.NbData: %lX\r\n", s_command.NbData);
                    Mx_printf(" s_command.DummyCycles: %lX\r\n", s_command.DummyCycles);
                    Mx_printf(" s_command.DQSMode: %lX\r\n", s_command.DQSMode);
                    Mx_printf(" s_command.SIOOMode: %lX\r\n", s_command.SIOOMode);
                    return MXST_FAILURE;
                }
            } else if (Spi->HardwareMode == SdmaMode)/*SDMA mode*/
            {
                if (HAL_OSPI_Transmit_DMA(&OSPIHandle, WrBuf + ExtraSz) != HAL_OK)
                    return MXST_FAILURE;

                while (TxCplt == 0) {
                }
                TxCplt = 0;

            } else/*mode error*/
            {
#if MXIC_HC_PRINTF_ENABLE
    Mx_printf("QSPI write mode error\r\n");
#endif
                return MXST_FAILURE;
            }
        }
    }

    Spi->IsBusy = FALSE;
#endif

    return MXST_SUCCESS;
}

#ifdef BLOCK3_SPECIAL_HARDWARE_MODE
int MxLnrModeRead(MxSpi *Spi, u8 *RdBuf,u32 Address, u32 ByteCount, u8 ReadCmd)
{
#ifdef CONTR_25F0A
    int Status;
    MxHcRegs *Regs = (MxHcRegs *)Spi->BaseAddress;
    u32 LrdConfig, LrdCtrl ;
    Xil_AssertNonvoid(Spi != NULL);
    Xil_AssertNonvoid(Spi->IsReady == XIL_COMPONENT_IS_READY);
    #ifdef AXI_SLAVE_BASEADDR
    /* set the value of Reg.LrdConfig */
    switch (Spi->FlashProtocol)
    {
        case PROT_1_1_1:
        LrdConfig = LRD_CMD_BUS_IO_1 | LRD_ADDR_BUS_IO_1 | LRD_DATA_BUS_IO_1;
        break;
        case PROT_1_1D_1D:
        LrdConfig = LRD_CMD_BUS_IO_1 | LRD_ADDR_BUS_IO_1 | LRD_DATA_BUS_IO_1 | LRD_ADDR_BUS_DDR_EN | LRD_DATA_BUS_DDR_EN;
        break;
        case PROT_1_1_2:
        LrdConfig = LRD_CMD_BUS_IO_1 | LRD_ADDR_BUS_IO_1 | LRD_DATA_BUS_IO_2;
        break;
        case PROT_1_2_2:
        LrdConfig = LRD_CMD_BUS_IO_1 | LRD_ADDR_BUS_IO_2 | LRD_DATA_BUS_IO_2;
        break;
        case PROT_1_2D_2D:
        LrdConfig = LRD_CMD_BUS_IO_1 | LRD_ADDR_BUS_IO_2 | LRD_DATA_BUS_IO_2 | LRD_ADDR_BUS_DDR_EN | LRD_DATA_BUS_DDR_EN;
        break;
        case PROT_1_1_4:
        LrdConfig = LRD_CMD_BUS_IO_1 | LRD_ADDR_BUS_IO_1 | LRD_DATA_BUS_IO_4;
        break;
        case PROT_1_4_4:
        LrdConfig = LRD_CMD_BUS_IO_1 | LRD_ADDR_BUS_IO_4 | LRD_DATA_BUS_IO_4;
        break;
        case PROT_1_4D_4D:
        LrdConfig = LRD_CMD_BUS_IO_1 | LRD_ADDR_BUS_IO_4 | LRD_DATA_BUS_IO_4 | LRD_ADDR_BUS_DDR_EN | LRD_DATA_BUS_DDR_EN;
        break;
        case PROT_4_4_4:
        LrdConfig = LRD_CMD_BUS_IO_4 | LRD_ADDR_BUS_IO_4 | LRD_DATA_BUS_IO_4;
        break;
        case PROT_4_4D_4D:
        LrdConfig = LRD_CMD_BUS_IO_4 | LRD_ADDR_BUS_IO_4 | LRD_DATA_BUS_IO_4 | LRD_ADDR_BUS_DDR_EN | LRD_DATA_BUS_DDR_EN;
        break;
        case PROT_8_8_8:
        if (Spi->SopiDqs)
            LrdConfig = LRD_CMD_BUS_IO_8 | LRD_ADDR_BUS_IO_8 | LRD_DATA_BUS_IO_8 | LRD_DUAL_CMD_EN | LRD_DQS_EN;
        else
            LrdConfig = LRD_CMD_BUS_IO_8 | LRD_ADDR_BUS_IO_8 | LRD_DATA_BUS_IO_8 | LRD_DUAL_CMD_EN;
        break;
        case PROT_8D_8D_8D:
        LrdConfig = LRD_CMD_BUS_IO_8 | LRD_ADDR_BUS_IO_8 | LRD_DATA_BUS_IO_8 | LRD_CMD_BUS_DDR_EN |LRD_ADDR_BUS_DDR_EN | LRD_DATA_BUS_DDR_EN | LRD_DUAL_CMD_EN | LRD_DQS_EN;
        break;
        default:
        LrdConfig = 0;
        break;
    }

    if (Spi->LenDummy) {
        LrdConfig |= Spi->LenDummy << LRD_DUMMY_CNT_OFS;
    }
    /* Set up the number of address cycles. */
    LrdConfig |= Spi->LenAddr << LRD_ADDR_CNT_OFS;

    if (Spi->IsRd) {
        LrdConfig |= LRD_DD_READ;
    }

    /* Write Reg.LrdConfig */
    MxWr32(&Regs->LrdConfig, LrdConfig);

    /* Write Reg.LrdCtrl */
    LrdCtrl = LRD_CTRL_SLV_ACT_LOW | (~ReadCmd)<<LRD_CTRL_CMD_CODE_2ND_OFS | ReadCmd<<LRD_CTRL_CMD_CODE_1ST_OFS;
    MxWr32(&Regs->LrdCtrl, LrdCtrl);

    /* Write Reg.LrdAddr */
    MxWr32(&Regs->LrdAddr, 0);

    /* Write Reg.LrdRng */
    MxWr32(&Regs->LrdRng, 0);

    /* Write Reg.LrdAXIAddr */
    MxWr32(&Regs->AxiSlaveAddr, AXI_SLAVE_BASEADDR);

    /* Enable Linear Addressing Mode */
    LrdCtrl |= LRD_CTRL_EN;
    MxWr32(&Regs->LrdCtrl, LrdCtrl);

    if(Spi->HardwareMode == LnrMode)
        memcpy((void*)RdBuf,(const void*)(AXI_SLAVE_BASEADDR + Address),ByteCount);
    else
    {
        Spi->DmaOpt.DmaCmd.ChanCtrl.SrcBurstSize = 4;
        Spi->DmaOpt.DmaCmd.ChanCtrl.SrcBurstLen = 4;
        Spi->DmaOpt.DmaCmd.ChanCtrl.SrcInc = 1;
        Spi->DmaOpt.DmaCmd.ChanCtrl.DstBurstSize = 4;
        Spi->DmaOpt.DmaCmd.ChanCtrl.DstBurstLen = 4;
        Spi->DmaOpt.DmaCmd.ChanCtrl.DstInc = 1;
        Spi->DmaOpt.DmaCmd.BD.SrcAddr = AXI_SLAVE_BASEADDR + Address;
        Spi->DmaOpt.DmaCmd.BD.DstAddr = (u32)RdBuf;
        Spi->DmaOpt.DmaCmd.BD.Length = ByteCount;

        Status = XDmaPs_Start(Spi->DmaOpt.DmaInst, Spi->DmaOpt.Channel, &Spi->DmaOpt.DmaCmd, 0);
        if (Status!= MXST_SUCCESS) {
            return MXST_FAILURE;
        }
        /* wait until DMA transfer is done */
        while ((XDmaPs_ReadReg(Spi->DmaOpt.DmaInst->Config.BaseAddress,XDMAPS_CS0_OFFSET)& XDMAPS_DS_DMA_STATUS)
            != XDMAPS_DS_DMA_STATUS_STOPPED);

        Mx_printf("  XDMAPS_CC_0_OFFSET: %02X\r\n", XDmaPs_ReadReg(Spi->DmaOpt.DmaInst->Config.BaseAddress,XDMAPS_CC_0_OFFSET));
        Mx_printf("  XDMAPS_ES_OFFSET: %02X\r\n", XDmaPs_ReadReg(Spi->DmaOpt.DmaInst->Config.BaseAddress,XDMAPS_ES_OFFSET));
        Mx_printf("  XDMAPS_INTSTATUS_OFFSET: %02X\r\n", XDmaPs_ReadReg(Spi->DmaOpt.DmaInst->Config.BaseAddress,XDMAPS_INTSTATUS_OFFSET));
        Mx_printf("  XDMAPS_INTCLR_OFFSET: %02X\r\n", XDmaPs_ReadReg(Spi->DmaOpt.DmaInst->Config.BaseAddress,XDMAPS_INTCLR_OFFSET));
            XDmaPs_WriteReg(Spi->DmaOpt.DmaInst->Config.BaseAddress,XDMAPS_INTCLR_OFFSET,0xFF);
        Mx_printf("  XDMAPS_INTSTATUS_OFFSET: %02X\r\n", XDmaPs_ReadReg(Spi->DmaOpt.DmaInst->Config.BaseAddress,XDMAPS_INTSTATUS_OFFSET));
        /* make the DMA channel be idle */
        Spi->DmaOpt.DmaInst->Chans[Spi->DmaOpt.Channel].DmaCmdToHw = NULL;
    }
    /*
     * Disable Linear Addressing Mode / Disable the controller
     */
    LrdCtrl = !LRD_CTRL_EN;
    MxWr32(&Regs->LrdCtrl, LrdCtrl);
    /* wait for IntrSts.INTRSTS_LRD_DIS_RDY is ready */
    while (!(MxRd32(&Regs->IntrSts) & INTRSTS_LRD_DIS_RDY));
        MxWr32(&Regs->IntrSts, MxRd32(&Regs->IntrSts) |(INTRSTS_LRD_DIS_RDY));
    return MXST_SUCCESS;

    #else
    return MXST_FAILURE;
#endif
#else
    QSPI_CommandTypeDef s_command;
    QSPI_MemoryMappedTypeDef sMemMappedCfg;

    if (Spi->IsBusy)
        return MXST_DEVICE_BUSY;

    Spi->IsBusy = TRUE;

    s_command.Instruction       = ReadCmd;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;

    s_command.DummyCycles       = Spi->LenDummy ;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    switch (Spi->FlashProtocol) {
        case PROT_1_1_1:
            s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
            s_command.DataMode          = QSPI_DATA_1_LINE;
            s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
            break;

        case PROT_1_1D_1D:
            s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
            s_command.DataMode          = QSPI_DATA_1_LINE;
            s_command.DdrMode           = QSPI_DDR_MODE_ENABLE;
            break;

        case PROT_1_1_2:
            s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
            s_command.DataMode          = QSPI_DATA_2_LINES;
            s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
            break;

        case PROT_1_1D_2D:
            s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
            s_command.DataMode          = QSPI_DATA_2_LINES;
            s_command.DdrMode           = QSPI_DDR_MODE_ENABLE;
            break;

        case PROT_1_2_2:
            s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode       = QSPI_ADDRESS_2_LINES;
            s_command.DataMode          = QSPI_DATA_2_LINES;
            s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
            break;

        case PROT_1_2D_2D:
            s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode       = QSPI_ADDRESS_2_LINES;
            s_command.DataMode          = QSPI_DATA_2_LINES;
            s_command.DdrMode           = QSPI_DDR_MODE_ENABLE;
            break;

        case PROT_1_1_4:
            s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
            s_command.DataMode          = QSPI_DATA_4_LINES;
            s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
            break;

        case PROT_1_1D_4D:
            s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
            s_command.DataMode          = QSPI_DATA_4_LINES;
            s_command.DdrMode           = QSPI_DDR_MODE_ENABLE;
            break;

        case PROT_1_4_4:
            s_command.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
            s_command.AddressMode        = QSPI_ADDRESS_4_LINES;
            s_command.DataMode           = QSPI_DATA_4_LINES;
            s_command.DdrMode            = QSPI_DDR_MODE_DISABLE;

        if(
        #ifdef USING_MX25Rxx_DEVICE
          (Spi->MX25RPowerMode == HIGH_PERFORMANCE_MODE) &&
#endif
        (Spi->IsRd)
          )
        {
#if MXIC_HC_PRINTF_ENABLE
        Mx_printf("Performance Enhance Mode Entered \r\n");
#endif

        s_command.DummyCycles        = Spi->LenDummy - 2 ;

        s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_4_LINES;
        s_command.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
        s_command.AlternateBytes     = MXIC_XIP_ENTER_CODE;

        s_command.SIOOMode           = QSPI_SIOO_INST_ONLY_FIRST_CMD;
        }
          break;

      case PROT_1_4D_4D:
          s_command.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
          s_command.AddressMode        = QSPI_ADDRESS_4_LINES;
          s_command.DataMode           = QSPI_DATA_4_LINES;
          s_command.DdrMode          = QSPI_DDR_MODE_ENABLE;

        if(
#ifdef USING_MX25Rxx_DEVICE
        (Spi->MX25RPowerMode == HIGH_PERFORMANCE_MODE) &&
#endif
        (Spi->IsRd)
          )
        {
#if MXIC_HC_PRINTF_ENABLE
        Mx_printf("Performance Enhance Mode Entered \r\n");
#endif

        s_command.DummyCycles        = Spi->LenDummy - 2 ;

        s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_4_LINES;
        s_command.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
        s_command.AlternateBytes     = MXIC_XIP_ENTER_CODE;

        s_command.SIOOMode           = QSPI_SIOO_INST_ONLY_FIRST_CMD;
        }
          break;

      case PROT_4_4_4:
          s_command.InstructionMode    = QSPI_INSTRUCTION_4_LINES;
          s_command.AddressMode        = QSPI_ADDRESS_4_LINES;
          s_command.DataMode           = QSPI_DATA_4_LINES;
          s_command.DdrMode          = QSPI_DDR_MODE_DISABLE;

        if(
#ifdef USING_MX25Rxx_DEVICE
        (Spi->MX25RPowerMode == HIGH_PERFORMANCE_MODE) &&
#endif
        (Spi->IsRd)
          )
        {
#if MXIC_HC_PRINTF_ENABLE
        Mx_printf("Performance Enhance Mode Entered \r\n");
#endif

        s_command.DummyCycles        = Spi->LenDummy - 2 ;

        s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_4_LINES;
        s_command.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
        s_command.AlternateBytes     = MXIC_XIP_ENTER_CODE;

        s_command.SIOOMode           = QSPI_SIOO_INST_ONLY_FIRST_CMD;
        }
          break;

      case PROT_4_4D_4D:
          s_command.InstructionMode    = QSPI_INSTRUCTION_4_LINES;
          s_command.AddressMode        = QSPI_ADDRESS_4_LINES;
          s_command.DataMode           = QSPI_DATA_4_LINES;
          s_command.DdrMode          = QSPI_DDR_MODE_ENABLE;

        if(
#ifdef USING_MX25Rxx_DEVICE
        (Spi->MX25RPowerMode == HIGH_PERFORMANCE_MODE) &&
#endif
        (Spi->IsRd)
          )
        {
#if MXIC_HC_PRINTF_ENABLE
        Mx_printf("Performance Enhance Mode Entered \r\n");
#endif

        s_command.DummyCycles        = Spi->LenDummy - 2 ;

        s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_4_LINES;
        s_command.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
        s_command.AlternateBytes     = MXIC_XIP_ENTER_CODE;

        s_command.SIOOMode           = QSPI_SIOO_INST_ONLY_FIRST_CMD;
        }
          break;
      }

    /*correct  s_command.DataMode*//*very important*/
    if( ByteCount == 0)
    s_command.DataMode  = QSPI_DATA_NONE;

    switch (Spi->LenAddr) {
      case 0:
          s_command.AddressMode        = QSPI_ADDRESS_NONE;
          break;
      case 1:
          s_command.AddressSize       = QSPI_ADDRESS_8_BITS;
          break;
      case 2:
          s_command.AddressSize       = QSPI_ADDRESS_16_BITS;
          break;
      case 3:
          s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
          break;
      case 4:
          s_command.AddressSize       = QSPI_ADDRESS_32_BITS;
          break;
    }

/***************for debug use***************************/
#if MXIC_HC_PRINTF_ENABLE
      Mx_printf(" Instruction     :      %X\r\n",  ReadCmd);
      Mx_printf(" InstructionMode :      %lX\r\n", s_command.InstructionMode);
      Mx_printf(" AddressMode     :      %lX\r\n", s_command.AddressMode);
      Mx_printf(" AddressSize     :      %lX\r\n", s_command.AddressSize);
      Mx_printf(" Address         :      %lX\r\n", Address);
      Mx_printf(" DataMode        :      %lX\r\n", s_command.DataMode);
      Mx_printf(" NbData          :      %lX\r\n", ByteCount);
      printf(" DummyCycles     :      %lX\r\n", s_command.DummyCycles);
#endif
/***************for debug use***************************/

    /* Configure the memory mapped mode */
    sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

    if (HAL_QSPI_MemoryMapped(&QSPIHandle, &s_command, &sMemMappedCfg) != HAL_OK)
    {
        return MXST_FAILURE;
    }

    if(Spi->IsRd)
    {
        memcpy((void*)RdBuf, (const void*)(QSPI_BASEADDR + Address), ByteCount);
    }
    else
    {
#if MXIC_HC_PRINTF_ENABLE
        Mx_printf(" >>>> STM platform only support read in mapping mode, don't support write in mapping mode\r\n");
#endif
    }

    if (HAL_QSPI_Abort(&QSPIHandle) != HAL_OK)
    {
        return MXST_FAILURE;
    }

    Spi->IsBusy = FALSE;

    return MXST_SUCCESS;

#endif
}

int MxLnrModeWrite(MxSpi *Spi, u8 *WrBuf,u32 Address, u32 ByteCount, u8 WriteCmd)
{
#ifdef CONTR_25F0A
    int Status;
    MxHcRegs *Regs = (MxHcRegs *)Spi->BaseAddress;
    u32 LwrConfig, LwrCtrl;
    Xil_AssertNonvoid(Spi != NULL);
    Xil_AssertNonvoid(Spi->IsReady == XIL_COMPONENT_IS_READY);
    #ifdef AXI_SLAVE_BASEADDR
    /* set the value of Reg.LwrConfig */
    switch (Spi->FlashProtocol)
    {
        case PROT_1_1_1:
        LwrConfig = LWR_CMD_BUS_IO_1 | LWR_ADDR_BUS_IO_1 | LWR_DATA_BUS_IO_1;
        break;
        case PROT_1_1D_1D:
        LwrConfig = LWR_CMD_BUS_IO_1 | LWR_ADDR_BUS_IO_1 | LWR_DATA_BUS_IO_1 | LWR_ADDR_BUS_DDR_EN | LWR_DATA_BUS_DDR_EN;
        break;
        case PROT_1_1_2:
        LwrConfig = LWR_CMD_BUS_IO_1 | LWR_ADDR_BUS_IO_1 | LWR_DATA_BUS_IO_2;
        break;
        case PROT_1_2_2:
        LwrConfig = LWR_CMD_BUS_IO_1 | LWR_ADDR_BUS_IO_2 | LWR_DATA_BUS_IO_2;
        break;
        case PROT_1_2D_2D:
        LwrConfig = LWR_CMD_BUS_IO_1 | LWR_ADDR_BUS_IO_2 | LWR_DATA_BUS_IO_2 | LWR_ADDR_BUS_DDR_EN | LWR_DATA_BUS_DDR_EN;
        break;
        case PROT_1_1_4:
        LwrConfig = LWR_CMD_BUS_IO_1 | LWR_ADDR_BUS_IO_1 | LWR_DATA_BUS_IO_4;
        break;
        case PROT_1_4_4:
        LwrConfig = LWR_CMD_BUS_IO_1 | LWR_ADDR_BUS_IO_4 | LWR_DATA_BUS_IO_4;
        break;
        case PROT_1_4D_4D:
        LwrConfig = LWR_CMD_BUS_IO_1 | LWR_ADDR_BUS_IO_4 | LWR_DATA_BUS_IO_4 | LWR_ADDR_BUS_DDR_EN | LWR_DATA_BUS_DDR_EN;
        break;
        case PROT_4_4_4:
        LwrConfig = LWR_CMD_BUS_IO_4 | LWR_ADDR_BUS_IO_4 | LWR_DATA_BUS_IO_4;
        break;
        case PROT_4_4D_4D:
        LwrConfig = LWR_CMD_BUS_IO_4 | LWR_ADDR_BUS_IO_4 | LWR_DATA_BUS_IO_4 | LWR_ADDR_BUS_DDR_EN | LWR_DATA_BUS_DDR_EN;
        break;
        case PROT_8_8_8:
        if (Spi->SopiDqs)
            LwrConfig = LWR_CMD_BUS_IO_8 | LWR_ADDR_BUS_IO_8 | LWR_DATA_BUS_IO_8 | LWR_DUAL_CMD_EN | LWR_DQS_EN;
        else
            LwrConfig = LWR_CMD_BUS_IO_8 | LWR_ADDR_BUS_IO_8 | LWR_DATA_BUS_IO_8 | LWR_DUAL_CMD_EN;
        break;
        case PROT_8D_8D_8D:
        LwrConfig = LWR_CMD_BUS_IO_8 | LWR_ADDR_BUS_IO_8 | LWR_DATA_BUS_IO_8 | LWR_CMD_BUS_DDR_EN |LWR_ADDR_BUS_DDR_EN | LWR_DATA_BUS_DDR_EN | LWR_DUAL_CMD_EN | LWR_DQS_EN;
        break;
        default:
        LwrConfig = 0;
        break;
    }

    if (Spi->LenDummy) {
        LwrConfig |= Spi->LenDummy << LWR_DUMMY_CNT_OFS;
    }
    /* Set up the number of address cycles. */
    LwrConfig |= Spi->LenAddr << LWR_ADDR_CNT_OFS;

    /* Write Reg.LwrConfig */
    MxWr32(&Regs->LwrConfig, LwrConfig);

    /* Write Reg.LwrCtrl */
    LwrCtrl = LWR_CTRL_SLV_ACT_LOW | (~WriteCmd)<<LWR_CTRL_CMD_CODE_2ND_OFS | WriteCmd<<LWR_CTRL_CMD_CODE_1ST_OFS;
    MxWr32(&Regs->LwrCtrl, LwrCtrl);

    /* Write Reg.LwrAddr */
    MxWr32(&Regs->LwrAddr, 0);

    /* Write Reg.LwrRng */
    MxWr32(&Regs->LwrRng, 0);

    /* Write Reg.LwrAXIAddr */
    MxWr32(&Regs->AxiSlaveAddr, AXI_SLAVE_BASEADDR);

    /* Enable Linear Addressing Mode */
    LwrCtrl |= LWR_CTRL_EN;
    MxWr32(&Regs->LwrCtrl, LwrCtrl);

    if(Spi->HardwareMode == LnrMode)
        memcpy(( void*)(AXI_SLAVE_BASEADDR + Address),(void*)WrBuf,ByteCount);
    else
    {
        Spi->DmaOpt.DmaCmd.ChanCtrl.SrcBurstSize = 2;
        Spi->DmaOpt.DmaCmd.ChanCtrl.SrcBurstLen = 7;
        Spi->DmaOpt.DmaCmd.ChanCtrl.SrcInc = 1;
        Spi->DmaOpt.DmaCmd.ChanCtrl.DstBurstSize = 2;
        Spi->DmaOpt.DmaCmd.ChanCtrl.DstBurstLen = 7;
        Spi->DmaOpt.DmaCmd.ChanCtrl.DstInc = 1;
        Spi->DmaOpt.DmaCmd.BD.DstAddr = AXI_SLAVE_BASEADDR + Address;
        Spi->DmaOpt.DmaCmd.BD.SrcAddr = (u32)WrBuf;
        Spi->DmaOpt.DmaCmd.BD.Length = ByteCount;

        Status = XDmaPs_Start(Spi->DmaOpt.DmaInst, Spi->DmaOpt.Channel, &Spi->DmaOpt.DmaCmd, 0);
        if (Status!= MXST_SUCCESS) {
            return MXST_FAILURE;
        }

        while ((XDmaPs_ReadReg(Spi->DmaOpt.DmaInst->Config.BaseAddress,XDMAPS_CS0_OFFSET)& XDMAPS_DS_DMA_STATUS)
           != XDMAPS_DS_DMA_STATUS_STOPPED);

        Mx_printf("  XDMAPS_CC_0_OFFSET: %02X\r\n", XDmaPs_ReadReg(Spi->DmaOpt.DmaInst->Config.BaseAddress,XDMAPS_CC_0_OFFSET));
        Mx_printf("  XDMAPS_ES_OFFSET: %02X\r\n", XDmaPs_ReadReg(Spi->DmaOpt.DmaInst->Config.BaseAddress,XDMAPS_ES_OFFSET));
        Mx_printf("  XDMAPS_INTSTATUS_OFFSET: %02X\r\n", XDmaPs_ReadReg(Spi->DmaOpt.DmaInst->Config.BaseAddress,XDMAPS_INTSTATUS_OFFSET));
        Mx_printf("  XDMAPS_INTCLR_OFFSET: %02X\r\n", XDmaPs_ReadReg(Spi->DmaOpt.DmaInst->Config.BaseAddress,XDMAPS_INTCLR_OFFSET));
        XDmaPs_WriteReg(Spi->DmaOpt.DmaInst->Config.BaseAddress,XDMAPS_INTCLR_OFFSET,0xFF);


        Spi->DmaOpt.DmaInst->Chans[Spi->DmaOpt.Channel].DmaCmdToHw = NULL;
    }

    /*
     * Disable Linear Addressing Mode / Disable the controller
     */
    LwrCtrl = !LRD_CTRL_EN;
    MxWr32(&Regs->LwrCtrl, LwrCtrl);

    /* wait for IntrSts.INTRSTS_LWR_DIS_RDY is ready */
    while (!(MxRd32(&Regs->IntrSts) & INTRSTS_LWR_DIS_RDY));
    MxWr32(&Regs->IntrSts, MxRd32(&Regs->IntrSts) |(INTRSTS_LWR_DIS_RDY));

    return MXST_SUCCESS;

    #else
    return MXST_FAILURE;
#endif
#else
    return MXST_SUCCESS;
#endif
}
#endif

/*
 * Function:     MxGetHcVer
 * Arguments:     Spi,   pointer to an MxSpi structure of transfer.
 * Return Value: None.
 * Description:  This function is used for getting the version of hardware controller.
 */
int MxGetHcVer(MxSpi *Spi) {
#ifdef CONTR_25F0A
    MxHcRegs *Regs = (MxHcRegs *)Spi->BaseAddress;
    Mx_printf("\tRegHcVer: %02X\r\n",  MxRd32(&Regs->HcVer));
    return MXST_SUCCESS;
    #else
    return MXST_SUCCESS;
#endif
}

#if PLATFORM_ST
/*Private function for STM Platform*/

/**
 * @brief QSPI MSP Initialization
 *        This function configures the hardware resources used in this example:
 *           - Peripheral's clock enable
 *           - Peripheral's GPIO Configuration
 *           - DMA configuration for requests by peripheral
 *           - NVIC configuration for DMA and QSPI interrupts
 * @param hqspi: QSPI handle pointer
 * @retval None
 */

/*
 *
 */
void STM_Set_CSPin_OSPI(void) {
    GPIO_InitTypeDef gpio_init_structure;

    gpio_init_structure.Pin = OSPI_CS_PIN;
    gpio_init_structure.Mode = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_structure.Alternate = GPIO_AF10_OCTOSPIM_P1;
    HAL_GPIO_Init(OSPI_CS_GPIO_PORT, &gpio_init_structure);
}

void STM_Set_CSPin_GPIO(void) {
    GPIO_InitTypeDef gpio_init_structure;

    gpio_init_structure.Pin = OSPI_CS_PIN;
    gpio_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_structure.Alternate = 0;
    HAL_GPIO_Init(OSPI_CS_GPIO_PORT, &gpio_init_structure);

    /*Set to High*/
    HAL_GPIO_WritePin(OSPI_CS_GPIO_PORT, OSPI_CS_PIN, GPIO_PIN_SET);
}

void STM_Set_CSPin_High(void) {
    GPIO_InitTypeDef gpio_init_structure;

    gpio_init_structure.Pin = OSPI_CS_PIN;
    gpio_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_structure.Alternate = 0;
    HAL_GPIO_Init(OSPI_CS_GPIO_PORT, &gpio_init_structure);

    /*Set to High*/
    HAL_GPIO_WritePin(OSPI_CS_GPIO_PORT, OSPI_CS_PIN, GPIO_PIN_SET);
}

void STM_Set_CSPin_Low(void) {
    GPIO_InitTypeDef gpio_init_structure;

    gpio_init_structure.Pin = OSPI_CS_PIN;
    gpio_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_structure.Alternate = 0;
    HAL_GPIO_Init(OSPI_CS_GPIO_PORT, &gpio_init_structure);

    /*Set to Low*/
    HAL_GPIO_WritePin(OSPI_CS_GPIO_PORT, OSPI_CS_PIN, GPIO_PIN_RESET);
}

/**
 * @brief  Command completed callbacks.
 * @param  hqspi: QSPI handle
 * @retval None
 */
void HAL_OSPI_CmdCpltCallback(OSPI_HandleTypeDef *hqspi) {
#if MXIC_HC_PRINTF_ENABLE
    Mx_printf("HAL_QSPI_RxCpltCallback\r\n");
#endif
    CmdCplt++;
}

/**
 * @brief  Rx Transfer completed callbacks.
 * @param  hqspi: QSPI handle
 * @retval None
 */
void HAL_OSPI_RxCpltCallback(OSPI_HandleTypeDef *hqspi) {
#if MXIC_HC_PRINTF_ENABLE
    Mx_printf("HAL_QSPI_RxCpltCallback\r\n");
#endif
    RxCplt++;
}

/**
 * @brief  Tx Transfer completed callbacks.
 * @param  hqspi: QSPI handle
 * @retval None
 */
void HAL_OSPI_TxCpltCallback(OSPI_HandleTypeDef *hqspi) {
#if MXIC_HC_PRINTF_ENABLE
     Mx_printf("HAL_QSPI_TxCpltCallback\r\n");
#endif
    TxCplt++;
}

/**
 * @brief  Transfer Error callback.
 * @param  hqspi: QSPI handle
 * @retval None
 */
void HAL_OSPI_ErrorCallback(OSPI_HandleTypeDef *hqspi) {
    while (1) {
#if MXIC_HC_PRINTF_ENABLE
        Mx_printf("QSPI Error occured\r\n");
#endif
    }
}

#endif
