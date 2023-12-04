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
#include "nor_ops.h"
#include "math.h"

#define config_NOR_OPS_PRINTF_ENABLE 0

#define NOR_OPS_PRINTF_ENABLE config_NOR_OPS_PRINTF_ENABLE

/*
 * The instances to support the device drivers are global such that they
 * are initialized to zero each time the program runs. They could be local
 * but should at least be static so they are zeroed.
 */

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define INFO(_JedecId, _ExtId, _BlockSz, _N_Blocks,                                                 \
			 _SupModeFlag,_SpclFlag,    				                                            \
             _CR_Dummy_0,_CR_Dummy_1,_CR_Dummy_2,_CR_Dummy_3,_CR_Dummy_4,_CR_Dummy_5,_CR_Dummy_6,	\
             _SPICmdList0,_SPICmdList1,_SPICmdList2,_SPICmdList3,									\
			 _QPICmdList0,_QPICmdList1,_QPICmdList2,_QPICmdList3,									\
			 _OPICmdList0,_OPICmdList1,_OPICmdList2,_OPICmdList3,									\
			 _tW,_tDP,_tBP,_tPP,_tSE,_tBE32,_tBE,_tCE,_tWREAW)					     				\
	.Id = {                                                                                     	\
			((_JedecId) >> 16) & 0xff,		                                                        \
			((_JedecId) >>  8) & 0xff,						                                        \
			(_JedecId) & 0xff,								                                        \
			((_ExtId) >> 8) & 0xff, 						                                        \
			(_ExtId) & 0xff, 								                                        \
		  },                                                                                        \
	.IdLen = (!(_JedecId) ? 0 : (3 + ((_ExtId) ? 2 : 0))), 	                                        \
	.BlockSz = (_BlockSz),								                                            \
	.N_Blocks = (_N_Blocks),								                                        \
	.PageSz = PAGE_SZ, 										                                        \
	.SupModeFlag = (_SupModeFlag),                                                                  \
	.SpclFlag = (_SpclFlag),								                                        \
	.RdDummy = {                                                                                    \
				{((_CR_Dummy_0)>>8) & 0xff,((_CR_Dummy_0)>>0) & 0xff},                              \
                {((_CR_Dummy_1)>>8) & 0xff,((_CR_Dummy_1)>>0) & 0xff},                              \
                {((_CR_Dummy_2)>>8) & 0xff,((_CR_Dummy_2)>>0) & 0xff},                              \
                {((_CR_Dummy_3)>>8) & 0xff,((_CR_Dummy_3)>>0) & 0xff},                              \
                {((_CR_Dummy_4)>>8) & 0xff,((_CR_Dummy_4)>>0) & 0xff},                              \
                {((_CR_Dummy_5)>>8) & 0xff,((_CR_Dummy_5)>>0) & 0xff},                              \
                {((_CR_Dummy_6)>>8) & 0xff,((_CR_Dummy_6)>>0) & 0xff}                               \
               },    				                                                                \
     .SPICmdList = {_SPICmdList0,_SPICmdList1,_SPICmdList2,_SPICmdList3},  							\
	 .QPICmdList = {_QPICmdList0,_QPICmdList1,_QPICmdList2,_QPICmdList3},  							\
	 .OPICmdList = {_OPICmdList0,_OPICmdList1,_OPICmdList2,_OPICmdList3},							\
	 .tW     =_tW,											                                        \
	 .tDP    =_tDP,											                                        \
	 .tBP    =_tBP,   									                                            \
	 .tPP    =_tPP,   									                                            \
	 .tSE    =_tSE, 									                                            \
	 .tBE32  =_tBE32,										                                        \
	 .tBE    =_tBE,    										                                        \
	 .tCE    =_tCE,											                                        \
	 .tWREAW =_tWREAW,

typedef struct {
    char *name;
    u8 Id[SPI_NOR_FLASH_MAX_ID_LEN];
    u8 IdLen;
    u32 BlockSz;
    u16 N_Blocks;
    u16 PageSz;
    u16 AddrLen;
    u32 SupModeFlag;
    u32 SpclFlag;
    RdDummy RdDummy[7];
    u32 SPICmdList[4];
    u32 QPICmdList[4];
    u32 OPICmdList[4];
    u32 tW;
    u32 tDP;
    u32 tBP;
    u32 tPP;
    u32 tSE;
    u32 tBE32;
    u32 tBE;
    u32 tCE;
    u32 tWREAW;
} MxFlashInfo;

static MxFlashInfo SpiFlashParamsTable[] =
        { { "mx25um51245g", INFO(
                0xC2803A, 0, /* ID */
                64 * 1024, 1024, /*block size / block count */
                MODE_SPI|MODE_SOPI|MODE_DOPI, /*support mode*/
                SUPPORT_WRSR_CR|RDPASS_ADDR|SUPPORT_CR_ODS, /*special flag*/

                /** bit15-8 (DC Value); bit7-0 (Dummy Cycle) ; 	FF means this read command is not supported **/
                0x0008, /**Fast Read:             1S-1S-1S   //  Fast DTR Read(FASTDTRD): 1S-1D-1D **/
                0xFFFF, /**Dual Read     (DREAD): 1S-1S-2S                                         **/
                0xFFFF, /**Dual IO Read  (2READ): 1S-2S-2S   //  Dual IO Read  (2DTRD):   1S-2D-2D **/
                0xFFFF, /**Quad Read     (QREAD): 1S-1S-4S 								         **/
                0xFFFF, /**Quad IO Read  (4READ): 1S-4S-4S   //  Dual IO Read  (2DTRD):   1S-4D-4D **/
                0xFFFF, /**QPI mode Read:         4S-4S-4S                                         **/
                0x0014, /**SOPI mode Read(8READ): 8S-8S-8S   //  DOPI mode Read(8DTRD):   8D-8D-8D **/

                0x017AF040,0x02038008,0x00790028,0x80270047, /*SPI mode command list*/
                0,0,0,0, /*QPI mode command list*/
                0x037AF040,0x01808000,0x00310008,0x802700C7, /*OPI mode command list*/
                40000,10,30,1500,120000,0,650000,300000000,1 /*timing value*/
        ) }, { "mx25uw51245g", INFO(
                0xC2813A, 0, /* ID */
                64 * 1024, 1024, /*block size / block count */
                MODE_SPI|MODE_SOPI|MODE_DOPI, /*support mode*/
                SUPPORT_WRSR_CR|RDPASS_ADDR|SUPPORT_CR_ODS, /*special flag*/

                /** bit15-8 (DC Value); bit7-0 (Dummy Cycle) ; 	FF means this read command is not supported **/
                0x0008, /**Fast Read:             1S-1S-1S   //  Fast DTR Read(FASTDTRD): 1S-1D-1D **/
                0xFFFF, /**Dual Read     (DREAD): 1S-1S-2S                                         **/
                0xFFFF, /**Dual IO Read  (2READ): 1S-2S-2S   //  Dual IO Read  (2DTRD):   1S-2D-2D **/
                0xFFFF, /**Quad Read     (QREAD): 1S-1S-4S 								         **/
                0xFFFF, /**Quad IO Read  (4READ): 1S-4S-4S   //  Dual IO Read  (2DTRD):   1S-4D-4D **/
                0xFFFF, /**QPI mode Read:         4S-4S-4S                                         **/
                0x0706, /**SOPI mode Read(8READ): 8S-8S-8S   //  DOPI mode Read(8DTRD):   8D-8D-8D **/

                0x017AF040,0x02038008,0x00790028,0x80270047, /*SPI mode command list*/
                0,0,0,0, /*QPI mode command list*/
                0x037AF040,0x01808000,0x00310008,0x802700C7, /*OPI mode command list*/
                40000,10,30,1500,120000,0,650000,300000000,1 /*timing value*/
        ) }, { "mx25lw51245g", INFO(
                0xC2863A, 0, /* ID */
                64 * 1024, 1024, /*block size / block count */
                MODE_SPI|MODE_SOPI|MODE_DOPI, /*support mode*/
                SUPPORT_WRSR_CR|RDPASS_ADDR|SUPPORT_CR_ODS, /*special flag*/

                /** bit15-8 (DC Value); bit7-0 (Dummy Cycle) ; 	FF means this read command is not supported **/
                0x0008, /**Fast Read:             1S-1S-1S   //  Fast DTR Read(FASTDTRD): 1S-1D-1D **/
                0xFFFF, /**Dual Read     (DREAD): 1S-1S-2S                                         **/
                0xFFFF, /**Dual IO Read  (2READ): 1S-2S-2S   //  Dual IO Read  (2DTRD):   1S-2D-2D **/
                0xFFFF, /**Quad Read     (QREAD): 1S-1S-4S 								         **/
                0xFFFF, /**Quad IO Read  (4READ): 1S-4S-4S   //  Dual IO Read  (2DTRD):   1S-4D-4D **/
                0xFFFF, /**QPI mode Read:         4S-4S-4S                                         **/
                0x0706, /**SOPI mode Read(8READ): 8S-8S-8S   //  DOPI mode Read(8DTRD):   8D-8D-8D **/

                0x017AF040,0x02038008,0x00790028,0x80270047, /*SPI mode command list*/
                0,0,0,0, /*QPI mode command list*/
                0x037AF040,0x01808000,0x00310008,0x802700C7, /*OPI mode command list*/
                40000,10,30,1500,120000,0,650000,300000000,1 /*timing value*/
        ) }, { "mx25lm51245g", INFO(
                0xC2853A, 0, /* ID */
                64 * 1024, 1024, /*block size / block count */
                MODE_SPI|MODE_SOPI|MODE_DOPI, /*support mode*/
                SUPPORT_WRSR_CR|RDPASS_ADDR|SUPPORT_CR_ODS, /*special flag*/

                /** bit15-8 (DC Value); bit7-0 (Dummy Cycle) ; 	FF means this read command is not supported **/
                0x0008, /**Fast Read:             1S-1S-1S   //  Fast DTR Read(FASTDTRD): 1S-1D-1D **/
                0xFFFF, /**Dual Read     (DREAD): 1S-1S-2S                                         **/
                0xFFFF, /**Dual IO Read  (2READ): 1S-2S-2S   //  Dual IO Read  (2DTRD):   1S-2D-2D **/
                0xFFFF, /**Quad Read     (QREAD): 1S-1S-4S 								         **/
                0xFFFF, /**Quad IO Read  (4READ): 1S-4S-4S   //  Dual IO Read  (2DTRD):   1S-4D-4D **/
                0xFFFF, /**QPI mode Read:         4S-4S-4S                                         **/
                0x0706, /**SOPI mode Read(8READ): 8S-8S-8S   //  DOPI mode Read(8DTRD):   8D-8D-8D **/

                0x017AF040,0x02038008,0x00790028,0x80270047, /*SPI mode command list*/
                0,0,0,0, /*QPI mode command list*/
                0x037AF040,0x01808000,0x00310008,0x802700C7, /*OPI mode command list*/
                40000,10,30,1500,120000,0,650000,300000000,1 /*timing value*/
        ) },
                { "mx25u51245g",
                        INFO(
                                0xC2253A, 0x3A, 64 * 1024, 1024,
                                MODE_SPI|MODE_FS_READ|MODE_DUAL_READ|MODE_DUAL_IO_READ|MODE_QUAD_READ|MODE_QUAD_IO_READ|MODE_QUAD_IO_DT_READ|MODE_QPI,
                                SUPPORT_WRSR_CR|RDPASS_ADDR,
                                0x0008,0x0008,0x0208,0x0008,0x0208,0x0208,0xFFFF,
                                0x017CF043,0x026F893B,0x005F0039,0x802504E7, 0x0004F021,0x00088902,0x007F0028,0x800700C7, 0,0,0,0,
                                40000,10,60,750,400000,1000000,2000000,300000000,1
                        )

                },
                { "mx25l12855e",
                        INFO(
                                0xC22618, 0x88, 64 * 1024, 256,
                                MODE_SPI|MODE_FS_READ|MODE_DUAL_READ|MODE_DUAL_IO_READ|MODE_QUAD_READ|MODE_QUAD_IO_READ|MODE_FSDT_READ|MODE_DUAL_IO_DT_READ|MODE_QUAD_IO_DT_READ,
                                0,
                                0x0008,0x0008,0x0208,0x0008,0x0208,0x0208,0xFFFF,
                                0x0001705F,0x020081FB,0x006A0025,0x00300047, 0,0,0,0, 0,0,0,0,
                                100000,10,300,5000,300000,2000000,2000000,80000000,1
                        )

                },
                { "mx66l1g45g",
                        INFO(
                                0xC2201B, 0x1A, 64 * 1024, 2048,
                                MODE_SPI|MODE_FS_READ|MODE_DUAL_READ|MODE_DUAL_IO_READ|MODE_QUAD_READ|MODE_QUAD_IO_READ|MODE_FSDT_READ|MODE_DUAL_IO_DT_READ|MODE_QUAD_IO_DT_READ|MODE_QPI,
                                SUPPORT_WRSR_CR,
                                0x0008,0x0008,0x0208,0x0008,0x0208,0x0208,0xFFFF,
                                0x01FCF043,0x026F8FFB,0x007F0039,0x802504E7, 0x0004F021,0x00088902,0x007F0028,0x802704C7, 0,0,0,0,
                                140000,10,60,3000,400000,1000000,2000000,600000000,1
                        )

                },
                { "mx25l51245g",
                        INFO(
                                0xC2201A, 0x19, 64 * 1024, 1024,
                                MODE_SPI|MODE_FS_READ|MODE_DUAL_READ|MODE_DUAL_IO_READ|MODE_QUAD_READ|MODE_QUAD_IO_READ|MODE_FSDT_READ|MODE_DUAL_IO_DT_READ|MODE_QUAD_IO_DT_READ|MODE_QPI,
                                SUPPORT_WRSR_CR,
                                0x0008,0x0008,0x0208,0x0008,0x0208,0x0208,0xFFFF,
                                0x01FCF043,0x026F8FFB,0x007F0039,0x802504E7, 0x0004F021,0x00088902,0x007F0028,0x802704C7, 0,0,0,0,
                                140000,10,60,3000,400000,1000000,2000000,600000000,1
                        )

                },
                { "mx25u25635f",
                        INFO(
                                0xC2201A, 0x19, 64 * 1024, 512,
                                MODE_SPI|MODE_FS_READ|MODE_DUAL_READ|MODE_DUAL_IO_READ|MODE_QUAD_READ|MODE_QUAD_IO_READ|MODE_FSDT_READ|MODE_DUAL_IO_DT_READ|MODE_QUAD_IO_DT_READ|MODE_QPI,
                                SUPPORT_WRSR_CR,
                                0x0008,0x0008,0x0208,0x0008,0x0208,0x0208,0xFFFF,
                                0x01FCF043,0x026F8FFB,0x007F0039,0x802504E7, 0x0004F021,0x00088902,0x007F0028,0x802704C7, 0,0,0,0,
                                140000,10,60,3000,400000,1000000,2000000,600000000,1
                        )

                },
                { "mx25l25645G",
                        INFO(
                                0xC22019, 0x18, 64 * 1024, 512,
                                MODE_SPI|MODE_FS_READ|MODE_DUAL_READ|MODE_DUAL_IO_READ|MODE_QUAD_READ|MODE_QUAD_IO_READ|MODE_FSDT_READ|MODE_DUAL_IO_DT_READ|MODE_QUAD_IO_DT_READ|MODE_QPI,
                                SUPPORT_WRSR_CR,
                                0x0008,0x0008,0x0108,0x0008,0x0208,0x0208,0xFFFF,
                                0x01FCF043,0x026F8FFB,0x007F0039,0x802504E7, 0x0004F021,0x00088902,0x007F0028,0x802704C7, 0,0,0,0,
                                140000,10,60,3000,400000,1000000,2000000,600000000,1
                        )

                },
                { "mx25l12845G",
                        INFO(
                                0xC22018, 0x17, 64 * 1024, 256,
                                MODE_SPI|MODE_FS_READ|MODE_DUAL_READ|MODE_DUAL_IO_READ|MODE_QUAD_READ|MODE_QUAD_IO_READ|MODE_FSDT_READ|MODE_DUAL_IO_DT_READ|MODE_QUAD_IO_DT_READ|MODE_QPI,
                                SUPPORT_WRSR_CR,
                                0x0008,0x0008,0x0208,0x0008,0x0208,0x0208,0xFFFF,
                                0x01FCF043,0x026F8FFB,0x007F0039,0x802504E7, 0x0004F021,0x00088902,0x007F0028,0x802704C7, 0,0,0,0,
                                140000,10,60,3000,400000,1000000,2000000,600000000,1
                        )

                },
                { "mx25r6435f",
                        INFO(
                                0xC22817, 0x17, /* ID */
                                64 * 1024, 128, /*block size / block count */
                                MODE_SPI|MODE_FS_READ|MODE_DUAL_READ|MODE_DUAL_IO_READ|MODE_QUAD_READ|MODE_QUAD_IO_READ|MODE_QUAD_IO_WRITE, /*support mode*/
                                SUPPORT_WRSR_CR2|SUPPORT_CR_1BIT|ADDR_3BYTE_ONLY, /*special flag*/

                                /** bit15-8 (DC Value); bit7-0 (Dummy Cycle) ; 	FF means this read command is not supported **/
                                0xF008, /**Fast Read:             1S-1S-1S   //  Fast DTR Read(FASTDTRD): 1S-1D-1D **/
                                0xF008, /**Dual Read     (DREAD): 1S-1S-2S                                         **/
                                0xF008, /**Dual IO Read  (2READ): 1S-2S-2S   //  Dual IO Read  (2DTRD):   1S-2D-2D **/
                                0xF008, /**Quad Read     (QREAD): 1S-1S-4S 								         **/
                                0xF00A, /**Quad IO Read  (4READ): 1S-4S-4S   //  Dual IO Read  (2DTRD):   1S-4D-4D **/
                                0xFFFF, /**QPI mode Read:         4S-4S-4S                                         **/
                                0xFFFF, /**SOPI mode Read(8READ): 8S-8S-8S   //  DOPI mode Read(8DTRD):   8D-8D-8D **/

                                /*SPI mode command list*/
                                MX_RDID|MX_RES|MX_REMS|MX_WRSR|MX_RDCR|MX_WRSCUR|MX_RDSCUR,
                                MX_READ|MX_FASTREAD|MX_2READ|MX_DREAD|MX_4READ|MX_QREAD|MX_RDSFDP,
                                MX_PP|MX_4PP|MX_SE|MX_BE|MX_BE32K|MX_CE,
                                MX_DP|MX_SBL|MX_SUS_RES|MX_RST|MX_NOP|MX_SO,
                                /*QPI mode command list*/
                                0,0,0,0,
                                /*OPI mode command list*/
                                0,0,0,0,
                                /*timing value(us):tW,tDP,tBP,tPP,tSE,tBE32,tBE,tCE,tWREAW*/
                                30000,10,100,10000,240000,3000000,3500000,240000000,1
                        )

                },
                { "mx25r3235f",
                        INFO(
                                0xC22816, 0x16, /* ID */
                                64 * 1024, 64, /*block size / block count */
                                MODE_SPI|MODE_FS_READ|MODE_DUAL_READ|MODE_DUAL_IO_READ|MODE_QUAD_READ|MODE_QUAD_IO_READ|MODE_QUAD_IO_WRITE, /*support mode*/
                                SUPPORT_WRSR_CR2|SUPPORT_CR_1BIT|ADDR_3BYTE_ONLY, /*special flag*/

                                /** bit15-8 (DC Value); bit7-0 (Dummy Cycle) ; 	FF means this read command is not supported **/
                                0xF008, /**Fast Read:             1S-1S-1S   //  Fast DTR Read(FASTDTRD): 1S-1D-1D **/
                                0xF008, /**Dual Read     (DREAD): 1S-1S-2S                                         **/
                                0xF008, /**Dual IO Read  (2READ): 1S-2S-2S   //  Dual IO Read  (2DTRD):   1S-2D-2D **/
                                0xF008, /**Quad Read     (QREAD): 1S-1S-4S 								         **/
                                0xF00A, /**Quad IO Read  (4READ): 1S-4S-4S   //  Dual IO Read  (2DTRD):   1S-4D-4D **/
                                0xFFFF, /**QPI mode Read:         4S-4S-4S                                         **/
                                0xFFFF, /**SOPI mode Read(8READ): 8S-8S-8S   //  DOPI mode Read(8DTRD):   8D-8D-8D **/

                                /*SPI mode command list*/
                                MX_RDID|MX_RES|MX_REMS|MX_WRSR|MX_WRSCUR|MX_RDSCUR|MX_RDCR,
                                MX_READ|MX_FASTREAD|MX_2READ|MX_DREAD|MX_4READ|MX_QREAD|MX_RDSFDP,
                                MX_PP|MX_4PP|MX_SE|MX_BE|MX_BE32K|MX_CE,
                                MX_DP|MX_SBL|MX_SUS_RES|MX_RST|MX_NOP|MX_SO,
                                /*QPI mode command list*/
                                0,0,0,0,
                                /*OPI mode command list*/
                                0,0,0,0,
                                /*timing value(us):tW,tDP,tBP,tPP,tSE,tBE32,tBE,tCE,tWREAW*/
                                30000,10,100,10000,240000,3000000,3500000,240000000,1 //us uint
                        )

                },

        };

/*
 * Function:      MxSoftwareInit
 * Arguments:	  Mxic,          pointer to an mxchip structure of nor flash device.
 *                EffectiveAddr, base address.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for initializing the variables.
 */
int MxSoftwareInit(MxChip *Mxic) {
#ifdef PLATFORM_XILINX
    int Status;
#endif

    static MxSpi Spi;
    Mxic->Priv = &Spi;

    /*
     * If the device is busy, disallow the initialize and return a status
     * indicating it is already started. This allows the user to stop the
     * device and re-initialize, but prevents a user from inadvertently
     * initializing. This assumes the busy flag is cleared at startup.
     */
    if (Spi.IsBusy == TRUE) {
        return MXST_DEVICE_IS_STARTED;
    }

    /*
     * Set some default values.
     */
    Spi.IsBusy = FALSE;
#ifdef CONTR_25F0A
    Spi.BaseAddress = BASEADDRESS;
#endif
    Spi.SendBufferPtr = NULL;
    Spi.RecvBufferPtr = NULL;
    Spi.RequestedBytes = 0;
    Spi.RemainingBytes = 0;
    Spi.IsReady = MX_COMPONENT_IS_READY;
    Spi.CurMode = MODE_SPI;
    Spi.CurAddrMode = SELECT_4B;
    Spi.FlashProtocol = PROT_1_1_1;
    Spi.PreambleEn = FALSE;
    Spi.DataPass = FALSE;
    Spi.SopiDqs = FALSE;
    Spi.HardwareMode = IOMode;

#ifdef BLOCK3_SPECIAL_HARDWARE_MODE
#ifdef PLATFORM_XILINX
    XDmaPs DmaInstance;
    XDmaPs_Config *DmaCfg;

    /* Initialize the Xilinx DMA Driver */
    DmaCfg = XDmaPs_LookupConfig(DMA_DEVICE_ID);
    if (DmaCfg == NULL) {
        return MXST_FAILURE;
    }

    Status = XDmaPs_CfgInitialize(&DmaInstance, DmaCfg, DmaCfg->BaseAddress);
    if (Status != MXST_SUCCESS) {
        return MXST_FAILURE;
    }

    u8 *DmaInstance;

    Spi.DmaOpt.Channel = 0;
    Spi.DmaOpt.DmaInst = &DmaInstance;
#endif
#endif

#ifdef USING_MX25Rxx_DEVICE
    Spi.MX25RPowerMode = ULTRA_LOW_POWER_MODE;
#endif

    return MXST_SUCCESS;
}

/*
 * Function:      MxIdMatch
 * Arguments:	  Mxic:     pointer to an mxchip structure of nor flash device.
 *                Id:       data buffer to store the ID value.
 * Return Value:  MXST_SUCCESS.
 *                MXST_ID_NOT_MATCH.
 * Description:   This function is for checking if the ID value is matched to the ID in flash list.
 *                If they are matched, flash information will be assigned to Mxic structure.
 */
static int MxIdMatch(MxChip *Mxic, u8 *Id) {
    MxFlashInfo *FlashInfo;
    int n, m;

    for (n = 0; n < ARRAY_SIZE(SpiFlashParamsTable); n++) {
        FlashInfo = &SpiFlashParamsTable[n];
        if (!memcmp(Id, FlashInfo->Id, 3)) {
            for (m = 0; m < SPI_NOR_FLASH_MAX_ID_LEN; m = m + 1)
                Mxic->Id[m] = FlashInfo->Id[m];
            Mxic->ChipSupMode = FlashInfo->SupModeFlag;
            Mxic->ChipSpclFlag = FlashInfo->SpclFlag;
            Mxic->PageSz = FlashInfo->PageSz;
            Mxic->BlockSz = FlashInfo->BlockSz;
            Mxic->N_Blocks = FlashInfo->N_Blocks;
            Mxic->ChipSz = Mxic->BlockSz * Mxic->N_Blocks;
            Mxic->RdDummy = FlashInfo->RdDummy;
            Mxic->SPICmdList = FlashInfo->SPICmdList;
            Mxic->QPICmdList = FlashInfo->QPICmdList;
            Mxic->OPICmdList = FlashInfo->OPICmdList;
            Mxic->tW = FlashInfo->tW;
            Mxic->tDP = FlashInfo->tDP;
            Mxic->tBP = FlashInfo->tBP;
            Mxic->tPP = FlashInfo->tPP;
            Mxic->tSE = FlashInfo->tSE;
            Mxic->tBE32 = FlashInfo->tBE32;
            Mxic->tBE = FlashInfo->tBE;
            Mxic->tCE = FlashInfo->tCE;
            Mxic->WriteBuffStart = FALSE;
            for (m = 0; m < PASSWORD_LEN; m = m + 1) {
                Mxic->Pwd[m] = PASSWORD_INIT_VALUE;
            }
            return MXST_SUCCESS;
        }
    }
    return MXST_ID_NOT_MATCH;
}

/*
 * Function:      MxScanMode
 * Arguments:	  Mxic:      pointer to an mxchip structure of nor flash device.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for getting the current mode of flash by reading and matching ID value in different mode.
 */
int MxScanMode(MxChip *Mxic) {
    int Status;
    MxSpi *Spi = Mxic->Priv;
    u8 Id[SPI_NOR_FLASH_MAX_ID_LEN];

#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tStart matching the device ID\r\n");
#endif
#if 1
    /*
     * Standard SPI
     */
#if 1
    Spi->CurMode = MODE_DOPI;
    Spi->FlashProtocol = PROT_8D_8D_8D;
    Status = MxWRCR2(Mxic, CR2_OPI_EN_ADDR, 0);
    if (Status != MXST_SUCCESS)
        return Status;
#endif
    Spi->CurMode = MODE_SPI;
    Spi->FlashProtocol = PROT_1_1_1;
    Spi->CurMode = MODE_SPI;

    Status = MxRDID(Mxic, 3, Id);
    if (Status != MXST_SUCCESS)
        return Status;

#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tSPI, ID: %02X%02X%02X\r\n", Id[0], Id[1], Id[2]);
#endif
    Status = MxIdMatch(Mxic, Id);
    if (Status == MXST_SUCCESS)
        return Status;
#endif
    /*
     * OCTA DOPI
     */
    Spi->CurMode = MODE_DOPI;
    Status = MxRDID(Mxic, 3, Id);
    if (Status != MXST_SUCCESS)
        return Status;
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tDOPI, ID: %02X%02X%02X\r\n", Id[0], Id[1], Id[2]);
#endif
    Status = MxIdMatch(Mxic, Id);
    if (Status == MXST_SUCCESS)
        return Status;

    /*
     * OCTA SOPI
     */
    Spi->CurMode = MODE_SOPI;
    Status = MxRDID(Mxic, 3, Id);
    if (Status != MXST_SUCCESS)
        return Status;
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tSOPI, ID: %02X%02X%02X\r\n", Id[0], Id[1], Id[2]);
#endif
    Status = MxIdMatch(Mxic, Id);
    if (Status == MXST_SUCCESS)
        return Status;

    Spi->CurMode = MODE_QPI;
    Status = MxQPIID(Mxic, 3, Id);
    if (Status != MXST_SUCCESS)
        return Status;
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\t QPI, ID: %02X%02X%02X\r\n", Id[0], Id[1], Id[2]);
#endif
    Status = MxIdMatch(Mxic, Id);
    if (Status == MXST_SUCCESS)
        return Status;

#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t@warning: device ID match failed\r\n");
#endif
    return MXST_ID_NOT_MATCH;
}

/*
 * Function:      MxRdDmyWRCR
 * Arguments:	   Mxic:      pointer to an mxchip structure of nor flash device.
 * Return Value:  MXST_SUCCESS
 *                MXST_FAILURE
 * Description:   This function is for setting dummy cycle in different read mode by writing configuration register.
 */
int MxRdDmyWRCR(MxChip *Mxic) {
    u8 Cr[2];
    u8 Sr[3];
    u8 Status, RdProt;
    u8 IsCrBit7, IsCrBit6;
    MxSpi *Spi = Mxic->Priv;

    if (Mxic->ChipSupMode & MODE_DOPI) {
        Status = MxRDCR2(Mxic, CR2_DC_ADDR, Cr);
        if (Status != MXST_SUCCESS)
            return Status;
    } else {
        Status = MxRDCR(Mxic, Cr);
        if (Status != MXST_SUCCESS)
            return Status;
    }

    switch (Spi->CurMode) {
    case MODE_FS_READ:
    case MODE_FSDT_READ:
        RdProt = 0;
        break;
    case MODE_DUAL_READ:
        RdProt = 1;
        break;
    case MODE_DUAL_IO_READ:
    case MODE_DUAL_IO_DT_READ:
        RdProt = 2;
        break;
    case MODE_QUAD_READ:
        RdProt = 3;
        break;
    case MODE_QUAD_IO_READ:
    case MODE_QUAD_IO_DT_READ:
        RdProt = 4;
        break;
    case MODE_QPI:
        RdProt = 5;
        break;
    case MODE_SOPI:
    case MODE_DOPI:
        RdProt = 6;
        break;
    default:
        break;
    }

    if (Mxic->RdDummy[RdProt].CRValue != 0xFF) {
        if (Mxic->ChipSupMode & MODE_DOPI) {
            Cr[0] &= 0xF8;
            Cr[0] |= (Mxic->RdDummy[RdProt].CRValue & 0x07);
            Status = MxWRCR2(Mxic, CR2_DC_ADDR, Cr);
            if (Status != MXST_SUCCESS)
                return Status;
        } else {
            Status = MxRDSR(Mxic, Sr);
            if (Status != MXST_SUCCESS)
                return Status;
            IsCrBit7 = ((Mxic->RdDummy[RdProt].CRValue & 0x0F) == 0x0F);
            IsCrBit6 = ((Mxic->RdDummy[RdProt].CRValue & 0xF0) == 0xF0);
            if (IsCrBit7) {
                Cr[0] &= 0x7F;
                Cr[0] |= (Mxic->RdDummy[RdProt].CRValue & 0x10) << 3;
            } else if (IsCrBit6) {
                Cr[0] &= 0xBF;
                Cr[0] |= (Mxic->RdDummy[RdProt].CRValue & 0x01) << 6;
            } else {
                Cr[0] &= 0x3F;
                Cr[0] |= (Mxic->RdDummy[RdProt].CRValue & 0x03) << 6;
            }
            Sr[1] = Cr[0];
            Sr[2] = Cr[1];
            Status = MxWRSR(Mxic, Sr);
            if (Status != MXST_SUCCESS)
                return Status;
        }
        return Mxic->RdDummy[RdProt].Dummy;
    } else if (Mxic->RdDummy[RdProt].Dummy != 0xFF) {
        return Mxic->RdDummy[RdProt].Dummy;
    }
    return MXST_SUCCESS;
}

/*
 * Function:      MxChangeMode
 * Arguments:	   Mxic,        pointer to an mxchip structure of nor flash device.
 *                SetMode,     variable in which to store the operation mode to set.
 *                SetAddrMode, variable in which to store the address mode to set.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for changing the operation mode and address mode.
 */
int MxChangeMode(MxChip *Mxic, u32 SetMode, u32 SetAddrMode) {
    int Status;
    MxSpi *Spi = Mxic->Priv;
    u32 SupMode = Mxic->ChipSupMode;
    static u8 FirstFlag = 0;
    u8 CR2Value[2] = { 0 };

    if (SetAddrMode == ADDR_MODE_AUTO_SEL) {
        /*
         * if bigger than 128Mb,select 4 byte address
         */
        if (Mxic->ChipSz > 0xFFFFFF) {
            Spi->CurAddrMode = SELECT_4B;
            if ((Mxic->SPICmdList[MX_MS_RST_SECU_SUSP] & MX_4B)
                && (!(Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD))) {
                Status = MxEN4B(Mxic);
                if (Status == MXST_SUCCESS)
                    return Status;
            }
        } else
            Spi->CurAddrMode = SELECT_3B;
    } else {
        if ((Mxic->ChipSz < 0xFFFFFF) && (SetAddrMode == SELECT_4B)) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("Flash is only supported for 3 byte address mode\r\n");
#endif
            Spi->CurAddrMode = SELECT_3B;
        } else if ((Mxic->ChipSz > 0xFFFFFF) && (!(Mxic->SPICmdList[MX_MS_RST_SECU_SUSP] & MX_4B))
            && (!(Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD)) && (SetAddrMode == SELECT_3B)) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("Flash is only supported for 4 byte address mode\r\n");
#endif

            Spi->CurAddrMode = SELECT_4B;
        }

        else if ((Mxic->SPICmdList[MX_MS_RST_SECU_SUSP] & MX_4B)
            && (!(Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD))) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("Flash is supported for 3 byte and 4 byte address mode,Change by EN4B/EX4B\r\n");
#endif
            if (SetAddrMode == SELECT_4B) {
                Status = MxEN4B(Mxic);
                if (Status == MXST_SUCCESS)
                    return Status;
            } else {
                Status = MxEX4B(Mxic);
                if (Status == MXST_SUCCESS)
                    return Status;
            }
            Spi->CurAddrMode = SetAddrMode;
        } else
            Spi->CurAddrMode = SetAddrMode;
    }

    if ((Spi->CurMode == SetMode) && (FirstFlag)) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tFlash does not need change mode\r\n");
#endif
        return MXST_SUCCESS;
    }

    /*********************�z�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�{************************
     **********************�x         OCTA R/W/E           �x************************
     **********************�|�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�}***********************/
    /*
     * Change to DOPI mode
     */

    if (SetMode & MODE_DOPI) {
        if (!(SupMode & MODE_DOPI)) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tFlash is not supported for DOPI mode\r\n");

    Mx_printf("\t\tChange to [Standard SPI] mode\r\n");
#endif
            if (Spi->CurMode & MODE_QPI) {
                Status = MxRSTQPI(Mxic);
                if (Status != MXST_SUCCESS)
                    return Status;
            }
            Spi->CurMode = MODE_SPI;
            Spi->FlashProtocol = PROT_1_1_1;

            Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ? MxREAD :
                ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxREAD4B : MxREAD);
            Mxic->AppGrp._Write = (Spi->CurAddrMode == SELECT_3B) ? MxPP :
                ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxPP4B : MxPP);
            Mxic->AppGrp._Erase = (Spi->CurAddrMode == SELECT_3B) ?
                MxBE : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxBE4B : MxBE);
            Mxic->AppGrp._Erase = (Spi->CurAddrMode == SELECT_3B) ? MxSE :
                ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxSE4B : MxSE);
        } else {
            if (Spi->CurMode != MODE_SPI) {
                CR2Value[0] = CR2_SPI_EN;
                Status = MxWRCR2(Mxic, CR2_OPI_EN_ADDR, CR2Value);
                if (Status != MXST_SUCCESS)
                    return Status;
                Spi->CurMode = MODE_SPI;
                Spi->FlashProtocol = PROT_1_1_1;
            }
            CR2Value[0] = CR2_DOPI_EN;
            Status = MxWRCR2(Mxic, CR2_OPI_EN_ADDR, CR2Value);
            if (Status != MXST_SUCCESS)
                return Status;

#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tChange Mode To [DOPI]\r\n");
#endif

            Spi->CurMode = MODE_DOPI;
            Spi->FlashProtocol = PROT_8D_8D_8D;
            Mxic->AppGrp._Read = Mx8DTRD;
            Mxic->AppGrp._Write = Mx8DTRPP;
            Mxic->AppGrp._Erase = MxSE4B; //Mx8DTRBE;
        }
    }

    /*
     * Change to SOPI mode
     */
    else if (SetMode & MODE_SOPI) {
        if (!(SupMode & MODE_SOPI)) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tFlash is not supported for SOPI mode\r\n");

    Mx_printf("\t\tChange to [Standard SPI] mode\r\n");
#endif
            if (Spi->CurMode & MODE_QPI) {
                Status = MxRSTQPI(Mxic);
                if (Status != MXST_SUCCESS)
                    return Status;
            }
            Spi->CurMode = MODE_SPI;
            Spi->FlashProtocol = PROT_1_1_1;
            Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ?
                MxREAD : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxREAD4B : MxREAD);
            Mxic->AppGrp._Write = (Spi->CurAddrMode == SELECT_3B) ?
                MxPP : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxPP4B : MxPP);
            Mxic->AppGrp._Erase = (Spi->CurAddrMode == SELECT_3B) ?
                MxBE : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxBE4B : MxBE);
        } else {
            if (Spi->CurMode != MODE_SPI) {
                CR2Value[0] = CR2_SPI_EN;
                Status = MxWRCR2(Mxic, CR2_OPI_EN_ADDR, CR2Value);
                if (Status != MXST_SUCCESS)
                    return Status;
                Spi->CurMode = MODE_SPI;
                Spi->FlashProtocol = PROT_1_1_1;
            }
            if (Spi->SopiDqs)
                CR2Value[0] = CR2_SOPI_DQS_EN;
            else
                CR2Value[0] = 0;
            Status = MxWRCR2(Mxic, CR2_DQS_EN_ADDR, CR2Value);
            if (Status != MXST_SUCCESS)
                return Status;
            CR2Value[0] = CR2_SOPI_EN;
            Status = MxWRCR2(Mxic, CR2_OPI_EN_ADDR, CR2Value);
            if (Status != MXST_SUCCESS)
                return Status;

#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tChange Mode To [SOPI]\r\n");
#endif

            Spi->CurMode = MODE_SOPI;
            Spi->FlashProtocol = PROT_8_8_8;
            Mxic->AppGrp._Read = Mx8READ;
            Mxic->AppGrp._Write = Mx8PP;
            Mxic->AppGrp._Erase = Mx8BE;
        }
    }
    /*********************�z�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�{************************
     **********************�x          QPI R/W/E           �x************************
     **********************�|�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�}***********************/
    /*
     * Change to QPI mode
     */
    else if (SetMode & MODE_QPI) {
        if (!(SupMode & MODE_QPI)) {

#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tFlash is not supported for QPI mode\r\n");
    Mx_printf("\t\tChange to [Standard SPI] mode\r\n");
#endif

            if (Spi->CurMode & (MODE_OPI)) {
                CR2Value[0] = CR2_SPI_EN;
                Status = MxWRCR2(Mxic, CR2_OPI_EN_ADDR, CR2Value);
                if (Status != MXST_SUCCESS)
                    return Status;
            }
            Spi->CurMode = MODE_SPI;
            Spi->FlashProtocol = PROT_1_1_1;
            Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ?
                MxREAD : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxREAD4B : MxREAD);
            Mxic->AppGrp._Write = (Spi->CurAddrMode == SELECT_3B) ?
                MxPP : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxPP4B : MxPP);
            Mxic->AppGrp._Erase = (Spi->CurAddrMode == SELECT_3B) ?
                MxBE : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxBE4B : MxBE);

        } else {
            Status = MxEQPI(Mxic);
            if (Status != MXST_SUCCESS)
                return Status;
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tChange Mode To [QPI]\r\n");
#endif
            Spi->CurMode = MODE_QPI;
            Spi->FlashProtocol = PROT_4_4_4;
            Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ?
                Mx4READ : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? Mx4READ4B : Mx4READ);
            Mxic->AppGrp._Write = (Spi->CurAddrMode == SELECT_3B) ?
                MxPP : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxPP4B : MxPP);
            Mxic->AppGrp._Erase = (Spi->CurAddrMode == SELECT_3B) ? MxBE :
                ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxBE4B : MxBE);
        }
    }
    /*********************�z�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�{************************
     **********************�x          SPI R/W/E           �x************************
     **********************�|�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�}***********************/
    /*
     * Change to SPI mode
     */
    else {

        if (Spi->CurMode & (MODE_OPI)) {
            CR2Value[0] = CR2_SPI_EN;
            Status = MxWRCR2(Mxic, CR2_OPI_EN_ADDR, CR2Value);
            if (Status != MXST_SUCCESS)
                return Status;
        } else if (Spi->CurMode & MODE_QPI) {
            Status = MxRSTQPI(Mxic);
            if (Status != MXST_SUCCESS)
                return Status;
        }

        /**********************�x          SPI Read            �x************************/
        /*
         * Change to SPI Quad IO Read mode
         */
        if (SetMode & MODE_QUAD_IO_READ) {
            if (!(SupMode & MODE_QUAD_IO_READ)) {

#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tFlash is not supported for [Quad IO Read(144)] mode\r\n");
    Mx_printf("\t\tChange to [SPI Normal Read] mode\r\n");
#endif

                Spi->CurMode = MODE_SPI;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ? MxREAD :
                    ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxREAD4B : MxREAD);
            } else {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tChange to [Quad IO Read(144)] mode\r\n");
#endif
                Spi->CurMode = MODE_QUAD_IO_READ;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ?
                    Mx4READ : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? Mx4READ4B : Mx4READ);
            }
        } else if (SetMode & MODE_QUAD_READ) {
            /*
             * Change to SPI Quad Read mode
             */
            if (!(SupMode & MODE_QUAD_READ)) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tFlash is not supported for [Quad Read(114)] mode\r\n");
    Mx_printf("\t\tChange to [SPI Normal Read] mode\r\n");
#endif
                Spi->CurMode = MODE_SPI;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ? MxREAD :
                    ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxREAD4B : MxREAD);
            } else {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tChange to [Quad Read(114)] mode\r\n");
#endif
                Spi->CurMode = MODE_QUAD_READ;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ?
                    MxQREAD : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxQREAD4B : MxQREAD);
            }
        } else if (SetMode & MODE_DUAL_IO_READ) {
            /*
             * Change to SPI Dual IO Read mode
             */
            if (!(SupMode & MODE_DUAL_IO_READ)) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tFlash is not supported for [Dual IO Read(122)] mode\r\n");
    Mx_printf("\t\tChange to [SPI Normal Read] mode\r\n");
#endif
                Spi->CurMode = MODE_SPI;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ? MxREAD :
                    ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxREAD4B : MxREAD);
            } else {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tChange to [Dual IO Read(122)] mode\r\n");
#endif
                Spi->CurMode = MODE_DUAL_IO_READ;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ? Mx2READ :
                    ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? Mx2READ4B : Mx2READ);
            }
        } else if (SetMode & MODE_DUAL_READ) {
            /*
             * Change to SPI Dual Read mode
             */
            if (!(SupMode & MODE_DUAL_READ)) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tFlash is not supported for [Dual Read(112)] mode\r\n");
    Mx_printf("\t\tChange to [SPI Normal Read] mode\r\n");
#endif
                Spi->CurMode = MODE_SPI;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ?
                    MxREAD : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxREAD4B : MxREAD);
            } else {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tChange to [Dual Read(112)] mode\r\n");
#endif
                Spi->CurMode = MODE_DUAL_READ;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ? MxDREAD :
                    ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxDREAD4B : MxDREAD);
            }
        } else if (SetMode & MODE_FS_READ) {
            /*
             * Change to SPI Fast Read mode
             */
            if (!(SupMode & MODE_FS_READ)) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tFlash is not supported for [Fast Read(111)] mode\r\n");
    Mx_printf("\t\tChange to [SPI Normal Read] mode\r\n");
#endif
                Spi->CurMode = MODE_SPI;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ?
                    MxREAD : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxREAD4B : MxREAD);
            } else {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tChange to [Fast Read(111)] mode\r\n");
#endif
                Spi->CurMode = MODE_FS_READ;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ?
                    MxFASTREAD : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxFASTREAD4B : MxFASTREAD);
            }
        } else if (SetMode & MODE_FSDT_READ) {
            /*
             * Change to SPI FastDT Read mode
             */
            if (!(SupMode & MODE_FSDT_READ)) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tFlash is not supported for [FastDT Read(111)] mode\r\n");
    Mx_printf("\t\tChange to [SPI Normal Read] mode\r\n");
#endif
                Spi->CurMode = MODE_SPI;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ?
                    MxREAD :((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxREAD4B : MxREAD);
            } else {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tChange to [FastDT Read(1-1D-1D)] mode\r\n");
#endif
                Spi->CurMode = MODE_FSDT_READ;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ?
                    MxFASTDTRD : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxFASTDTRD4B : MxFASTDTRD);
            }
        } else if (SetMode & MODE_DUAL_IO_DT_READ) {
            /*
             * Change to SPI Dual IO DT Read mode
             */
            if (!(SupMode & MODE_DUAL_IO_DT_READ)) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tFlash is not supported for [Dual IO DT Read(122)] mode\r\n");
    Mx_printf("\t\tChange to [SPI Normal Read] mode\r\n");
#endif
                Spi->CurMode = MODE_SPI;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ?
                    MxREAD : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxREAD4B : MxREAD);
            } else {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tChange to [Dual IO DT Read(1-2D-2D)] mode\r\n");
#endif
                Spi->CurMode = MODE_DUAL_IO_DT_READ;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ?
                    Mx2DTRD : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? Mx2DTRD4B : Mx2DTRD);
            }
        } else if (SetMode & MODE_QUAD_IO_DT_READ) {
            /*
             * Change to SPI QUAD IO DT Read mode
             */
            if (!(SupMode & MODE_QUAD_IO_DT_READ)) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tFlash is not supported for [QUAD IO DT Read(144)] mode\r\n");
    Mx_printf("\t\tChange to [SPI Normal Read] mode\r\n");
#endif
                Spi->CurMode = MODE_SPI;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ?
                    MxREAD : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxREAD4B : MxREAD);
            } else {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tChange to [QUAD IO DT Read(1-4D-4D)] mode\r\n");
#endif
                Spi->CurMode = MODE_QUAD_IO_DT_READ;
                Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ?
                    Mx4DTRD : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? Mx4DTRD4B : Mx4DTRD);
            }
        } else {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tChange Mode To [Standard SPI Read]\r\n");
#endif
            Spi->CurMode = MODE_SPI;
            Mxic->AppGrp._Read = (Spi->CurAddrMode == SELECT_3B) ?
                MxREAD : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxREAD4B : MxREAD);
        }
        Spi->FlashProtocol = PROT_1_1_1;
        /*********************�z�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�{************************
         **********************�x          SPI Write           �x************************
         **********************�|�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�}***********************/
        /*
         * Change to SPI Quad IO Write mode
         */
        if (SetMode & MODE_QUAD_IO_WRITE) {
            if (!(SupMode & MODE_QUAD_IO_WRITE)) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tFlash is not supported for [Quad IO Write(144)] mode\r\n");
    Mx_printf("\t\tChange to [SPI Normal Write] mode\r\n");
#endif
                Mxic->AppGrp._Write = (Spi->CurAddrMode == SELECT_3B) ?
                    MxPP : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxPP4B : MxPP);
            } else {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tChange to [Quad IO Write(144)] mode\r\n");
#endif
                Spi->CurMode |= MODE_QUAD_IO_WRITE;
                Spi->FlashProtocol = PROT_1_4_4;
                Mxic->AppGrp._Write = (Spi->CurAddrMode == SELECT_3B) ?
                    Mx4PP : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? Mx4PP4B : Mx4PP);
            }

        } else {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tChange Mode To [Standard SPI Write]\r\n");
#endif
            Mxic->AppGrp._Write = (Spi->CurAddrMode == SELECT_3B) ?
                MxPP: ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxPP4B : MxPP);
        }

        Mxic->AppGrp._Erase = (Spi->CurAddrMode == SELECT_3B) ?
            MxSE : ((Mxic->SPICmdList[MX_RD_CMDS] & MX_4B_RD) ? MxSE4B : MxSE);
    }
    /* setup read dummy */
    Spi->LenDummy = MxRdDmyWRCR(Mxic);

    if ((Spi->CurMode != MODE_QPI) & (Spi->CurMode != MODE_SOPI) & (Spi->CurMode != MODE_DOPI))
        Spi->FlashProtocol = PROT_1_1_1;

    if (!FirstFlag)
        FirstFlag = 1;

    return MXST_SUCCESS;
}

#ifdef BLOCK2_SERCURITY_OTP
/*
 * Function:      MxQryLockMode
 * Arguments:	    Mxic,  pointer to an mxchip structure of nor flash device.
 * Return Value:  MXST_FAILURE.
 * 				BP_MODE.
 *                ASP_SOLID_MODE.
 *                ASP_PWD_MODE.
 *                SBP_MODE
 * Description:   This function is for getting protection mode.
 */
int MxQryLockMode(MxChip *Mxic) {
    int Status;
    u8 SecurReg;
    u8 LockReg[2];

    if (Mxic->SPICmdList[MX_ID_REG_CMDS] & MX_RDSCUR) {
        Status = MxRDSCUR(Mxic, &SecurReg);
        if (Status != MXST_SUCCESS)
            return Status;
        if (!(SecurReg & WPSEL_MASK)) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tCurrent mode is BP Mode\r\n");
#endif
            return BP_MODE;
        }
    } else {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tCurrent mode is BP Mode\r\n");
#endif
        return BP_MODE;
    }

    if (Mxic->SPICmdList[MX_MS_RST_SECU_SUSP] & MX_SBLK) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tCurrent mode is Single Block Protection Mode\r\n");
#endif
        return SBP_MODE;
    }

    Status = MxRDLR(Mxic, LockReg);
    if (Status != MXST_SUCCESS)
        return Status;
    if (!(LockReg[0] & LR_PWDP_MASK)) {
#if NOR_OPS_PRINTF_ENABLE
    Mx_printf("\t\tCurrent mode is ASP password Mode\r\n");
#endif
        return ASP_PWD_MODE;
    }

#if NOR_OPS_PRINTF_ENABLE
  	Mx_printf("\t\tCurrent mode is ASP solid Mode\r\n");
#endif
    return ASP_SOLID_MODE;
}

/*
 * Function:      MxSetBoundary
 * Arguments:	    Mxic,     pointer to an mxchip structure of nor flash device.
 *                ofs_s,    start address of area to lock or unlock.
 *                ofs_d,    end address of area to lock or unlock.
 *                islocked, to lock or to unlock flash.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for cutting offset to boundary point.
 */
int MxSetBoundary(MxChip *Mxic, u32 *ofs_s, u32 *ofs_e, u8 islocked) {
    u32 ofs, bdy_size, mod, wp64k_first, wp64k_last;

    if (*ofs_s >= Mxic->ChipSz)
        return MXST_FAILURE;

    if (*ofs_e >= Mxic->ChipSz)
        *ofs_e = Mxic->ChipSz - 1;

    /* check ofs */
    ofs = *ofs_s;
    wp64k_first = MX_WP64K_FIRST;
    wp64k_last = MX_WP64K_LAST(Mxic->ChipSz);

    bdy_size =
            (*ofs_s < wp64k_first || *ofs_s >= wp64k_last) ? MX_4KB : MX_64KB;
    mod = ofs % bdy_size;
    if (mod)
        *ofs_s += bdy_size - mod;

    ofs = *ofs_e;
    bdy_size =
            (*ofs_e <= wp64k_first || *ofs_e > wp64k_last) ? MX_4KB : MX_64KB;
    mod = ofs % bdy_size;
    if (mod && mod != (bdy_size - 1))
        *ofs_e -= mod + 1;
    else if (!mod)
        (*ofs_e)--;

    if ((*ofs_s > *ofs_e && !islocked) || *ofs_s >= Mxic->ChipSz
            || (*ofs_e) >= Mxic->ChipSz)
        return MXST_FAILURE;

    return MXST_SUCCESS;
}

/*
 * Function:      MxGetLockedRange
 * Arguments:	   Mxic,  pointer to an mxchip structure of nor flash device.
 *                Sr,    the value of status register.
 *                TB,    the value of TB bit in configuration register.
 *                Addr,  the start address of locked area.
 *                Len,   bytes number of locked area.
 * Return Value:  NONE.
 * Description:   This function is for getting the range of locked area by the value of status register and configuration register.
 */
static void MxGetLockedRange(MxChip *Mxic, u8 Sr, u8 TB, u32 *Addr, u64 *Len) {
    u8 Mask = SR_BP3 | SR_BP2 | SR_BP1 | SR_BP0;
    int Shift = ffs(Mask) - 1;

    if (!(Sr & Mask)) {
        /* No protection */
        *Addr = 0;
        *Len = 0;
    } else {
        *Len = pow(2, ((Sr & Mask) >> Shift) - 1) * 64 * 1024;
        if (!TB)
            *Addr = Mxic->ChipSz - *Len;
        else
            *Addr = 0;
    }
}

/*
 * Function:      MxIsLockedSr
 * Arguments:	   Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  the start address of checking area.
 *                Len,   bytes number of checking area.
 *                Sr,    the value of status register.
 * Return Value:  MXST_FAILURE.
 * 			   FLASH_IS_LOCKED.
 * 			   FLASH_IS_UNLOCKED.
 * Description:   This function is for checking if the area is locked by the value of status register and configuration register.
 */
static int MxIsLockedSr(MxChip *Mxic, u32 Addr, u64 Len, u8 Sr) {
    u32 LockOffs;
    u64 LockLen;
    u8 Cr, Status;

    if ((Mxic->ChipSpclFlag & SUPPORT_WRSR_CR)
            && (Mxic->SPICmdList[MX_ID_REG_CMDS] & MX_RDCR)) {
        Status = MxRDCR(Mxic, &Cr);
        if (Status != MXST_SUCCESS)
            return Status;
    }

    MxGetLockedRange(Mxic, Sr, Cr & CR_TB_MASK, &LockOffs, &LockLen);

    if ((Addr + Len <= LockOffs + LockLen) && (Addr >= LockOffs))
        return FLASH_IS_LOCKED;
    else
        return FLASH_IS_UNLOCKED;
}

/*
 * Function:      MxLock
 * Arguments:	   Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of locked area.
 *                Len,   number of bytes to lock.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for writing protection a specified block (or sector) of flash memory in BP protection mode
 *                by writing status register.
 */
int MxLock(MxChip *Mxic, u32 Addr, u64 Len) {
    u8 Status;
    u8 SrOld, SrNew, Cr;
    u8 Sr[2];
    u32 ofs_s, ofs_e;
    u8 Mask = SR_BP3 | SR_BP2 | SR_BP1 | SR_BP0;
    u8 Shift = ffs(Mask) - 1, Val;

    ofs_s = Addr;
    ofs_e = Addr + Len - 1;

    Status = MxSetBoundary(Mxic, &ofs_s, &ofs_e, 0);
    Addr = ofs_s;
    Len = ofs_e - Addr + 1;

    Status = MxRDSR(Mxic, &SrOld);
    if (Status != MXST_SUCCESS)
        return Status;

    if ((Mxic->ChipSpclFlag & SUPPORT_WRSR_CR)
            && (Mxic->SPICmdList[MX_ID_REG_CMDS] & MX_RDCR)) {
        Status = MxRDCR(Mxic, &Cr);
        if (Status != MXST_SUCCESS)
            return Status;
    }

    /*
     * T/B = 0 : lock from the last block(bottom)
     */
    if (!(Cr & CR_TB_MASK)) {
        /* SPI NOR always locks to the end */
        if (Addr + Len != Mxic->ChipSz) {
            /* Does combined region extend to end? */
            if (!MxIsLockedSr(Mxic, Addr + Len, Mxic->ChipSz - Addr - Len, SrOld)) {
                Mx_printf("combined region does not extend to end\r\n");
                return MXST_FAILURE;
            }
            Len = Mxic->ChipSz - Addr;
        }
    }
    /*
     * T/B = 1 : lock from the first block(top)
     */
    else {
        /* SPI NOR always locks from the top */
        if (Addr != 0) {
            /* Does combined region from the top? */
            if (!MxIsLockedSr(Mxic, 0, Addr, SrOld)) {
                Mx_printf("combined region does not from the top\r\n");
                return MXST_FAILURE;
            }
            Len = Addr + Len;
        }
    }

    Val = ((int) log2(Len / 64 / 1024) + 1) << Shift;

    if (Val & ~Mask)
        return MXST_FAILURE;
    /* Don't "lock" with no region! */
    if (!(Val & Mask))
        return MXST_FAILURE;

    SrNew = (SrOld & ~Mask) | Val;

    /* Only modify protection if it will not unlock other areas */
    if ((SrNew & Mask) <= (SrOld & Mask))
        return MXST_FAILURE;

    Sr[0] = SrNew;
    Sr[1] = Cr;
    Status = MxWRSR(Mxic, &Sr[0]);
    if (Status != MXST_SUCCESS)
        return Status;
    return Status;
}

/*
 * Function:      MxUnlock
 * Arguments:	    Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of unlocked area.
 *                Len,   number of bytes to unlock.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for canceling the block (or sector)  write protection state in BP protection mode
 *                by writing status register.
 */
int MxUnlock(MxChip *Mxic, u32 Addr, u64 Len) {
    u8 Status;
    u8 SrOld, SrNew, Cr;
    u8 Sr[2];
    u32 ofs_s, ofs_e;
    u8 Mask = SR_BP3 | SR_BP2 | SR_BP1 | SR_BP0;
    u8 Shift = ffs(Mask) - 1, Val;

    ofs_s = Addr;
    ofs_e = Addr + Len - 1;

    Status = MxSetBoundary(Mxic, &ofs_s, &ofs_e, 0);
    Addr = ofs_s;
    Len = ofs_e - Addr + 1;

    Status = MxRDSR(Mxic, &SrOld);
    if (Status != MXST_SUCCESS)
        return Status;

    if ((Mxic->ChipSpclFlag & SUPPORT_WRSR_CR)
            && (Mxic->SPICmdList[MX_ID_REG_CMDS] & MX_RDCR)) {
        Status = MxRDCR(Mxic, &Cr);
        if (Status != MXST_SUCCESS)
            return Status;
    }

    /* Cannot unlock; would unlock larger region than requested */
    if ((!(Cr & CR_TB_MASK)) && MxIsLockedSr(Mxic, Addr - Mxic->BlockSz, Mxic->BlockSz, SrOld)) {
        Mx_printf("@ERR: Cannot unlock; would unlock larger region than requested\r\n");
        return MXST_FAILURE;
    } else if ((Cr & CR_TB_MASK) && MxIsLockedSr(Mxic, Addr + Len, Mxic->BlockSz, SrOld)) {
        Mx_printf("@ERR: Cannot unlock; would unlock larger region than requested\r\n");
        return MXST_FAILURE;
    }

    Len = Mxic->ChipSz - (Addr + Len);

    if (Len > 0)
        Val = ((int) log2(Len / 64 / 1024) + 1) << Shift;
    else
        Val = 0;

    SrNew = (SrOld & ~Mask) | Val;

    /* Only modify protection if it will not lock other areas */
    if ((SrNew & Mask) >= (SrOld & Mask))
        return MXST_FAILURE;

    Sr[0] = SrNew;
    Sr[1] = Cr;
    return MxWRSR(Mxic, &Sr[0]);
}

/*
 * Function:      MxIsLocked
 * Arguments:	   Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of checking area.
 *                Len,   number of bytes to check.
 * Return Value:  MXST_FAILURE
 * 			   FLASH_IS_UNLOCKED.
 *                FLASH_IS_LOCKED.
 * Description:   This function is for checking if the block (or sector) is locked in BP protection mode.
 */
int MxIsLocked(MxChip *Mxic, u32 Addr, u64 Len) {
    u8 Sr;
    u8 Status;

    Status = MxRDSR(Mxic, &Sr);
    if (Status != MXST_SUCCESS)
        return Status;
    return MxIsLockedSr(Mxic, Addr, Len, Sr);
}

/*
 * Function:      MxIsSpbBitLocked
 * Arguments:	    Mxic:  pointer to an mxchip structure of nor flash device.
 * Return Value:  MXST_FAILURE.
 *                MXST_DEVICE_SPB_IS_LOCKED : SPB is locked down(SPB lock down bit is 0).
 *                MXST_DEVICE_SPB_ISNOT_LOCKED : SPB is not locked down(SPB lock down bit is 1).
 * Description:   This function is for checking if SPB bit is locked down by reading SPBLK bit.
 */
int MxIsSpbBitLocked(MxChip *Mxic) {
    int Status;
    u8 SpbLKB;

    if (Mxic->SPICmdList[MX_ID_REG_CMDS] & MX_SPBLK) {
        Status = MxRDSPBLK(Mxic, &SpbLKB);
        if (Status != MXST_SUCCESS)
            return Status;
        Mx_printf("\t\tSPB Lock Register : %02X\r\n", SpbLKB);
        if (SpbLKB & SPBLR_SPBLB_MASK) {
            Mx_printf("\t\tSPB is not locked down,SPB can be changed\r\n");
            return MXST_DEVICE_SPB_ISNOT_LOCKED;
        } else {
            Mx_printf("\t\tSPB is locked down,SPB cannot be changed\r\n");
            return MXST_DEVICE_SPB_IS_LOCKED;
        }
    } else {
        Status = MxRDLR(Mxic, &SpbLKB);
        if (Status != MXST_SUCCESS)
            return Status;
        Mx_printf("\t\tLock Register : %02X\r\n", SpbLKB);
        if (SpbLKB & LR_SPBLB_MASK) {
            Mx_printf("\t\tSPB is not locked down,SPB can be changed\r\n");
            return MXST_DEVICE_SPB_ISNOT_LOCKED;
        } else {
            Mx_printf("\t\tSPB is locked down,SPB cannot be changed\r\n");
            return MXST_DEVICE_SPB_IS_LOCKED;
        }
    }
}

/*
 * Function:      MxSpbBitLockDown
 * Arguments:	    Mxic:  pointer to an mxchip structure of nor flash device.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for is setting SPB lock.
 */
int MxSpbBitLockDown(MxChip *Mxic) {
    int Status;
    u8 LockReg[2];

    if (Mxic->SPICmdList[MX_ID_REG_CMDS] & MX_SPBLK) {
        Status = MxSPBLK(Mxic);
        if (Status != MXST_SUCCESS)
            return Status;
    } else {
        Status = MxRDLR(Mxic, LockReg);
        if (Status != MXST_SUCCESS)
            return Status;
        LockReg[0] &= LR_SPBLB_LOCK_EN;
        Status = MxWRLR(Mxic, LockReg);
        if (Status != MXST_SUCCESS)
            return Status;
    }

    return Status;
}
/*
 * Function:      MxAspIsLockedCheck
 * Arguments:	    Mxic,     pointer to an mxchip structure of nor flash device.
 *                ofs_s,    start address of checking area.
 *                ofs_d,    end address of checking area.
 *                ASP_MODE, advanced sector protection mode.
 * Return Value:  MXST_FAILURE.
 * 				FLASH_IS_UNLOCKED.
 *                FLASH_IS_LOCKED.
 *                FLASH_IS_PARTIALLY_LOCKED
 * Description:   This function is for checking if the block (or sector) is locked in advanced sector protection mode.
 */
static int MxAspIsLockedCheck(MxChip *Mxic, u32 ofs_s, u32 ofs_e, enum MX_ASP_MODE ASP_MODE) {
    u32 bdy_size, ofs_s_org = ofs_s, wp64k_first = MX_WP64K_FIRST, wp64k_last = MX_WP64K_LAST(Mxic->ChipSz);
    u8 val_old, val;
    int Status;

    for (val = 0; ofs_s <= ofs_e; ofs_s += bdy_size) {
        bdy_size = (ofs_s < wp64k_first || ofs_s >= wp64k_last) ? MX_4KB : MX_64KB;

        switch (ASP_MODE) {
        case ASP_SPB:
            Status = MxRDSPB(Mxic, ofs_s, &val);
            break;
        case ASP_DPB:
            Status = MxRDDPB(Mxic, ofs_s, &val);
            break;
        case ASP_SB:
            Status = MxRDBLOCK(Mxic, ofs_s, &val);
            break;
        default:
            return MXST_FAILURE;
        }
        if (Status != MXST_SUCCESS)
            return Status;

        if (ofs_s == ofs_s_org)
            val_old = val;

        if (val_old != val)
            return FLASH_IS_PARTIALLY_LOCKED;
    }
    return !val ? FLASH_IS_UNLOCKED : FLASH_IS_LOCKED;
}

/*
 * Function:      MxAspModeSet
 * Arguments:	  Mxic,     pointer to an mxchip structure of nor flash device.
 *                ofs_s,    start address of area to lock or unlock.
 *                ofs_d,    end address of area to lock or unlock.
 *                ASP_MODE, advanced sector protection mode.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for lock or unlock flash in advanced sector protection mode.
 *                It is only for DPB lock/unlock and SPB lock.
 */
static int MxAspModeSet(MxChip *Mxic, u32 ofs_s, u32 ofs_e,
        enum MX_ASP_MODE ASP_MODE) {
    u8 val_lock = 0xFF, val_unlock = 0x00;
    int Status;
    u32 bdy_size, wp64k_first = MX_WP64K_FIRST, wp64k_last = MX_WP64K_LAST(
            Mxic->ChipSz);

    for (; ofs_s <= ofs_e; ofs_s += bdy_size) {
        bdy_size = (ofs_s < wp64k_first || ofs_s >= wp64k_last) ? MX_4KB : MX_64KB;

        switch (ASP_MODE) {
        case ASP_DPB_LOCK:
            Status = MxWRDPB(Mxic, ofs_s, &val_lock);
            if (Status != MXST_SUCCESS)
                return Status;
            break;
        case ASP_DPB_UNLOCK:
            Status = MxWRDPB(Mxic, ofs_s, &val_unlock);
            if (Status != MXST_SUCCESS)
                return Status;
            break;
        case ASP_SPB_LOCK:
            Status = MxWRSPB(Mxic, ofs_s);
            if (Status != MXST_SUCCESS)
                return Status;
            break;
        case ASP_SBLK:
            Status = MxSBLK(Mxic, ofs_s);
            if (Status != MXST_SUCCESS)
                return Status;
            break;
        case ASP_SBULK:
            Status = MxSBULK(Mxic, ofs_s);
            if (Status != MXST_SUCCESS)
                return Status;
            break;
        default:
            return MXST_FAILURE;
        }
    }
    return Status;
}
/*
 * Function:      MxSpbLock
 * Arguments:	 Mxic,  pointer to an mxchip structure of nor flash device.
 *                ofs_s,  start address of area to lock.
 *                ofs_d,  end address of area to lock.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 *                MXST_DEVICE_SPB_IS_LOCKED
 * Description:   This function is for writing protection a specified block (or sector) of flash memory in solid protection mode.
 */
static int MxSpbLock(MxChip *Mxic, u32 ofs_s, u32 ofs_e) {
    if (MxIsSpbBitLocked(Mxic) == MXST_DEVICE_SPB_ISNOT_LOCKED) {
        return MxAspModeSet(Mxic, ofs_s, ofs_e, ASP_SPB_LOCK);
    } else {
        Mx_printf("\t\tSPB is locked down,Unlock SPB failed\r\n");
        return MXST_DEVICE_SPB_IS_LOCKED;
    }
}

/*
 * Function:      MxSpbUnlock
 * Arguments:	    Mxic,  pointer to an mxchip structure of nor flash device.
 *                ofs_s,  start address of area to unlock.
 *                ofs_d,  end address of area to unlock.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 *                MXST_DEVICE_SPB_IS_LOCKED.
 * Description:   This function will cancel the block (or sector)  write protection state in solid protection mode.
 */
static int MxSpbUnlock(MxChip *Mxic, u32 ofs_s, u32 ofs_e) {
    if (MxIsSpbBitLocked(Mxic) == MXST_DEVICE_SPB_ISNOT_LOCKED) {
        /* erase spb */
        return MxESSPB(Mxic);
    } else {
        Mx_printf("\t\tSPB is locked down,Unlock SPB failed\r\n");
        return MXST_DEVICE_SPB_IS_LOCKED;
    }
}

/*
 * Function:      MxSpbIsLocked
 * Arguments:	   Mxic,  pointer to an mxchip structure of nor flash device.
 *                ofs_s,  start address of checking area.
 *                ofs_d,  end address of checking area.
 * Return Value:  MXST_FAILURE.
 * 			   FLASH_IS_UNLOCKED.
 *                FLASH_IS_LOCKED.
 * Description:   This function is for checking if the block (or sector) is locked in solid protection mode.
 */
static int MxSpbIsLocked(MxChip *Mxic, u32 ofs_s, u32 ofs_e) {
    return MxAspIsLockedCheck(Mxic, ofs_s, ofs_e, ASP_SPB);
}

/*
 * Function:      MxAspLock
 * Arguments:	   Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of locked area in advanced sector protection mode.
 *                Len,   number of bytes to lock in advanced sector protection mode.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for writing protection a specified block (or sector) of flash memory in advanced sector protection mode.
 */
int MxAspLock(MxChip *Mxic, u32 Addr, u64 Len) {
    int Status, LockMode;
    u32 ofs_s, ofs_e;

    LockMode = MxQryLockMode(Mxic);
    if (LockMode == MXST_FAILURE)
        return LockMode;

    if (LockMode == BP_MODE)
        return MxLock(Mxic, Addr, Len);

    /* for SP and DP */
    ofs_s = Addr;
    ofs_e = Addr + Len - 1;
    Status = MxSetBoundary(Mxic, &ofs_s, &ofs_e, 0);
    if (Status != MXST_SUCCESS)
        return Status;
    if (LockMode == ASP_SOLID_MODE) {
        Mx_printf("\t\tLock flash in SPB mode by WRSPB command \r\n");
        return MxSpbLock(Mxic, ofs_s, ofs_e);
    }

    if (LockMode == ASP_PWD_MODE) {
        Mx_printf("ERROR: please use 'PASSWORD LOCK SPB MODE' to lock SPBs with password protection mode\r\n");
        return MXST_FAILURE;
    }

    return Status;
}

/*
 * Function:      MxAspUnlock
 * Arguments:	 Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of unlocked area in advanced sector protection mode.
 *                Len,   number of bytes to unlock in advanced sector protection mode.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for canceling the block (or sector)  write protection state in advanced sector protection mode.
 */
int MxAspUnlock(MxChip *Mxic, u32 Addr, u64 Len) {
    int Status, LockMode;
    u32 ofs_s, ofs_e;

    LockMode = MxQryLockMode(Mxic);
    if (LockMode == MXST_FAILURE)
        return LockMode;

    if (LockMode == BP_MODE)
        return MxUnlock(Mxic, Addr, Len);

    /* for SP and DP */
    ofs_s = Addr;
    ofs_e = Addr + Len - 1;
    Status = MxSetBoundary(Mxic, &ofs_s, &ofs_e, 0);
    if (Status)
        return Status;
    if (LockMode == ASP_SOLID_MODE) {
        Mx_printf("\t\tUnlock flash in SPB mode by ESSPB command \r\n");
        return MxSpbUnlock(Mxic, ofs_s, ofs_e);
    }

    if (LockMode == ASP_PWD_MODE) {
        Mx_printf("ERROR: please use 'PASSWORD UNLOCK SPB MODE' to unlock SPBs with password protection mode\n");
        return MXST_FAILURE;
    }
    return Status;
}

/*
 * Function:      MxAspIsLocked
 * Arguments:	 Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of check area in advanced sector protection mode.
 *                Len,   number of bytes to check area in advanced sector protection mode.
 * Return Value:  MXST_FAILURE.
 * 				 FLASH_IS_UNLOCKED.
 *                FLASH_IS_LOCKED.
 * Description:   This function is for checking if the block (or sector) is locked in advanced protection mode.
 */
int MxAspIsLocked(MxChip *Mxic, u32 Addr, u64 Len) {
    int Status, LockMode;
    u32 ofs_s, ofs_e, ofs_s_org, ofs_e_org, wp64k_first, wp64k_last;

    LockMode = MxQryLockMode(Mxic);
    if (LockMode == MXST_FAILURE)
        return LockMode;

    if (LockMode == BP_MODE)
        return MxIsLocked(Mxic, Addr, Len);

    ofs_s_org = ofs_s = Addr;
    ofs_e_org = ofs_e = Addr + Len - 1;
    wp64k_first = MX_WP64K_FIRST;
    wp64k_last = MX_WP64K_LAST(Mxic->ChipSz);
    Status = MxSetBoundary(Mxic, &ofs_s, &ofs_e, 1);
    if (Status)
        return Status;

    if (ofs_s_org != ofs_s && ofs_s) ofs_s -= (ofs_s <= wp64k_first || ofs_s > wp64k_last) ? MX_4KB : MX_64KB;

    if (ofs_e_org != ofs_e && ofs_e) ofs_e += (ofs_e < wp64k_first || ofs_e >= wp64k_last) ? MX_4KB : MX_64KB;

    return MxSpbIsLocked(Mxic, ofs_s, ofs_e);
}

/*
 * Function:      MxAspWpselLock
 * Arguments:	  Mxic:  pointer to an mxchip structure of nor flash device.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for changing into ASP mode by setting the bit 7 in security register (WPSEL).
 */
int MxAspWpselLock(MxChip *Mxic) {
    return MxWPSEL(Mxic);
}

/*
 * Function:      MxPwdAuth
 * Arguments:	  Mxic,     pointer to an mxchip structure of nor flash device.
 * 				  Password, data buffer to store the 64 bit password.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for inputing password to lock or unlock SPB protection.
 */
int MxPwdAuth(MxChip *Mxic, u8 *Password) {
    return MxPASSULK(Mxic, Password);
}

/*
 * Function:      MxPwdLockUnlockSpb
 * Arguments:	  Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of locked or unlocked area in password protection mode.
 *                Len,   number of bytes to lock or unlock in password protection mode.
 *                Pwd,   data buffer to store the 64 bit password.
 *                IsLockSpb, flag to determine to perform lock or unlock SPB
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function will write protection a specified block (or sector) or cancel the block (or sector)
 *                write protection state in password protection mode.
 */
static int MxPwdLockUnlockSpb(MxChip *Mxic, u32 Addr, u64 Len, u8 *Pwd, u8 IsLockSpb) {
    int Status, LockMode;
    u32 ofs_s, ofs_e;

    LockMode = MxQryLockMode(Mxic);
    if (LockMode == MXST_FAILURE)
        return LockMode;
    if ((LockMode == BP_MODE) || (LockMode == ASP_SOLID_MODE)) {
        Mx_printf("ERROR: please use 'SET PASSWORD MODE' to enter password protection mode\n");
        return MXST_FAILURE;
    }

    Status = MxPwdAuth(Mxic, Pwd);
    if (Status != MXST_SUCCESS)
        return Status;

    /* lock SPBs */
    ofs_s = Addr;
    ofs_e = Addr + Len - 1;
    Status = MxSetBoundary(Mxic, &ofs_s, &ofs_e, 0);
    if (Status != MXST_SUCCESS)
        return Status;

    return IsLockSpb ? MxSpbLock(Mxic, ofs_s, ofs_e) : MxSpbUnlock(Mxic, ofs_s, ofs_e);
}

/*
 * Function:      MxAspLock
 * Arguments:	  Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of locked area in password protection mode.
 *                Len,   number of bytes to lock in password protection mode.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for writing protection a specified block (or sector) of flash memory in password protection mode.
 */
int MxPwdLockSpb(MxChip *Mxic, u32 Addr, u64 Len) {
    return MxPwdLockUnlockSpb(Mxic, Addr, Len, Mxic->Pwd, 1);
}

/*
 * Function:      MxPwdUnlockSpb
 * Arguments:	   Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of unlocked area in password protection mode.
 *                Len,   number of bytes to unlock in password protection mode.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function will cancel the block (or sector) write protection state in password protection mode.
 */
int MxPwdUnlockSpb(MxChip *Mxic, u32 Addr, u64 Len) {
    return MxPwdLockUnlockSpb(Mxic, Addr, Len, Mxic->Pwd, 0);
}

/*
 * Function:      MxDpbLock
 * Arguments:	   Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of locked area in dynamic protection mode.
 *                Len,   number of bytes to lock in dynamic protection mode.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for writing protection a specified block (or sector) of flash memory in dynamic protection mode.
 */
int MxDpbLock(MxChip *Mxic, u32 Addr, u64 Len) {
    int Status;
    u32 ofs_s, ofs_e;

    ofs_s = Addr;
    ofs_e = Addr + Len - 1;
    Status = MxSetBoundary(Mxic, &ofs_s, &ofs_e, 0);
    if (Status != MXST_SUCCESS)
        return Status;
    return MxAspModeSet(Mxic, ofs_s, ofs_e, ASP_DPB_LOCK);
}

/*
 * Function:      MxDPBUnlockFlash
 * Arguments:	   Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of unlocked area in dynamic protection mode.
 *                Len,   number of bytes to unlock in dynamic protection mode.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for canceling the block (or sector) write protection state in dynamic protection mode.
 */
int MxDpbUnlock(MxChip *Mxic, u32 Addr, u64 Len) {
    int Status;
    u32 ofs_s, ofs_e;

    ofs_s = Addr;
    ofs_e = Addr + Len - 1;
    Status = MxSetBoundary(Mxic, &ofs_s, &ofs_e, 0);
    if (Status != MXST_SUCCESS)
        return Status;
    return MxAspModeSet(Mxic, ofs_s, ofs_e, ASP_DPB_UNLOCK);
}

/*
 * Function:      MxIsFlashDPBLocked
 * Arguments:	    Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of checking area in dynamic protection mode.
 *                Len,   number of bytes to check in dynamic protection mode.
 * Return Value:  MXST_FAILURE.
 * 				FLASH_IS_UNLOCKED.
 *                FLASH_IS_LOCKED.
 * Description:   This function is for checking if the block (or sector) is locked in dynamic protection mode.
 */
int MxDpbIsLocked(MxChip *Mxic, u32 Addr, u64 Len) {
    int Status;
    u32 ofs_s_org, ofs_e_org, ofs_s, ofs_e, wp64k_first, wp64k_last;

    ofs_s_org = ofs_s = Addr;
    ofs_e_org = ofs_e = Addr + Len - 1;
    Status = MxSetBoundary(Mxic, &ofs_s, &ofs_e, 1);
    if (Status != MXST_SUCCESS)
        return Status;
    wp64k_first = MX_WP64K_FIRST;
    wp64k_last = MX_WP64K_LAST(Mxic->ChipSz);

    if (ofs_s_org != ofs_s && ofs_s)
        ofs_s -= (ofs_s <= wp64k_first || ofs_s > wp64k_last) ? MX_4KB : MX_64KB;

    if (ofs_e_org != ofs_e && ofs_e)
        ofs_e += (ofs_e < wp64k_first || ofs_e >= wp64k_last) ? MX_4KB : MX_64KB;

    return MxAspIsLockedCheck(Mxic, ofs_s, ofs_e, ASP_DPB);
}

/*
 * Function:      MxSingleBlockLock
 * Arguments:	 Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of locked area.
 *                Len,   number of bytes to lock.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for writing protection a specified block (or sector) of flash memory in single block protection mode.
 */
int MxSingleBlockLock(MxChip *Mxic, u32 Addr, u64 Len) {

    int Status;
    u32 ofs_s, ofs_e;
    u8 Wpsel;

    Status = MxRDSCUR(Mxic, &Wpsel);
    if (Status != MXST_SUCCESS)
        return Status;

    if (!(Wpsel & WPSEL_MASK))
        return MxLock(Mxic, Addr, Len);

    ofs_s = Addr;
    ofs_e = Addr + Len - 1;
    Status = MxSetBoundary(Mxic, &ofs_s, &ofs_e, 0);
    if (Status)
        return Status;
    return MxAspModeSet(Mxic, ofs_s, ofs_e, ASP_SBLK);
}

/*
 * Function:      MxSingleBlockUnlock
 * Arguments:	    Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of unlocked area.
 *                Len,   number of bytes to unlock.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for canceling the block (or sector)  write protection state in single block protection mode.
 */
int MxSingleBlockUnlock(MxChip *Mxic, u32 Addr, u64 Len) {
    int Status;
    u32 ofs_s, ofs_e;
    u8 Wpsel;

    Status = MxRDSCUR(Mxic, &Wpsel);
    if (Status != MXST_SUCCESS)
        return Status;

    if (!(Wpsel & WPSEL_MASK))
        return MxLock(Mxic, Addr, Len);

    ofs_s = Addr;
    ofs_e = Addr + Len - 1;
    Status = MxSetBoundary(Mxic, &ofs_s, &ofs_e, 0);
    if (Status)
        return Status;
    return MxAspModeSet(Mxic, ofs_s, ofs_e, ASP_SBULK);
}

/*
 * Function:      MxSingleBlockIsLocked
 * Arguments:	    Mxic,  pointer to an mxchip structure of nor flash device.
 *                Addr,  32 bit flash memory address of checking area.
 *                Len,   number of bytes to check.
 * Return Value:  MXST_FAILURE.
 * 				FLASH_IS_UNLOCKED.
 *                FLASH_IS_LOCKED.
 * Description:   This function is for checking if the block (or sector) is locked in single block protection mode.
 */
int MxSingleBlockIsLocked(MxChip *Mxic, u32 Addr, u64 Len) {
    int Status;
    u32 ofs_s_org, ofs_e_org, ofs_s, ofs_e, wp64k_first, wp64k_last;

    u8 Wpsel;

    Status = MxRDSCUR(Mxic, &Wpsel);
    if (Status != MXST_SUCCESS)
        return Status;

    if (!(Wpsel & WPSEL_MASK))
        return MxIsLocked(Mxic, Addr, Len);

    ofs_s_org = ofs_s = Addr;
    ofs_e_org = ofs_e = Addr + Len - 1;
    Status = MxSetBoundary(Mxic, &ofs_s, &ofs_e, 1);
    if (Status)
        return Status;

    wp64k_first = MX_WP64K_FIRST;
    wp64k_last = MX_WP64K_LAST(Mxic->ChipSz);

    if (ofs_s_org != ofs_s && ofs_s)
        ofs_s -= (ofs_s <= wp64k_first || ofs_s > wp64k_last) ? MX_4KB : MX_64KB;

    if (ofs_e_org != ofs_e && ofs_e)
        ofs_e += (ofs_e < wp64k_first || ofs_e >= wp64k_last) ? MX_4KB : MX_64KB;

    return MxAspIsLockedCheck(Mxic, ofs_s, ofs_e, ASP_SB);
}
#endif

/*
 * Function:      MxChipReset
 * Arguments:	    Mxic,  pointer to an mxchip structure of nor flash device.
 * Return Value:  MXST_SUCCESS.
 *                MXST_FAILURE.
 * Description:   This function is for reset the flash in software way.
 */
int MxChipReset(MxChip *Mxic) {
    u8 Status;
    Status = MxRSTEN(Mxic);
    if (Status != MXST_SUCCESS)
        return Status;
    Status = MxRST(Mxic);
    HAL_Delay_Us(500); //500us;
    return Status;
}
