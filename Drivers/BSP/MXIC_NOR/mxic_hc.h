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

#ifndef MXIC_HC_H_
#define MXIC_HC_H_

#include "mx_define.h"
/*
 * define TransFlag
 */
#define XFER_START    1
#define XFER_END    2

typedef struct {
    u32 BaseAddress;
    u32 IsReady; /**< Device is initialized and ready */
    u8 *InstrBufPtr;
    u8 LenCmd;
    u8 LenDummy;
    u8 LenAddr;
    u32 CurMode;
    u8 CurAddrMode;
    u8 IsRd;
    u32 CurrFreq;
    u8 WrCmd;
    u8 RdCmd;
    u8 *SendBufferPtr; /**< Buffer to send (state) */
    u8 *RecvBufferPtr; /**< Buffer to receive (state) */
    int RequestedBytes; /**< Number of bytes to transfer (state) */
    int RemainingBytes; /**< Number of bytes left to transfer(state) */
    u32 IsBusy; /**< A transfer is in progress (state) */
    u8 TransFlag;
    u8 FlashProtocol;
    u8 PreambleEn;
    u8 DataPass;
    u8 SopiDqs;
    u8 HardwareMode;
#ifdef CONTR_25F0A
    struct {
        u8 Channel;
        XDmaPs_Cmd DmaCmd;
        XDmaPs *DmaInst;
    } DmaOpt;
#endif

#ifdef USING_MX25Rxx_DEVICE
    u8 MX25RPowerMode;
#endif
} MxSpi;

enum FlashProtocol {
    PROT_1_1_1,
    PROT_1_1D_1D,
    PROT_1_1_2,
    PROT_1_1D_2D,
    PROT_1_2_2,
    PROT_1_2D_2D,
    PROT_1_1_4,
    PROT_1_1D_4D,
    PROT_1_4_4,
    PROT_1_4D_4D,
    PROT_4_4_4,
    PROT_4_4D_4D,
    PROT_8_8_8,
    PROT_8D_8D_8D,
};

enum CurrentMode {
    MODE_SPI = 0x00000001,
    MODE_SOPI = 0x00000002,
    MODE_DOPI = 0x00000004,
    MODE_OPI = (MODE_SOPI | MODE_DOPI),
    MODE_QPI = 0x00000008,
    MODE_FS_READ = 0x00000010,
    MODE_DUAL_READ = 0x00000020, //PROT_1_1_2
    MODE_DUAL_IO_READ = 0x00000040, //PROT_1_2_2
    MODE_QUAD_READ = 0x00000080, //PROT_1_1_4
    MODE_QUAD_IO_READ = 0x00000100, //PROT_1_4_4
    MODE_DUAL_WRITE = 0x00000200,
    MODE_DUAL_IO_WRITE = 0x00000400,
    MODE_QUAD_WRITE = 0x00000800,
    MODE_QUAD_IO_WRITE = 0x00001000,
    MODE_FSDT_READ = 0x00002000,
    MODE_DUAL_IO_DT_READ = 0x00004000,
    MODE_QUAD_IO_DT_READ = 0x00008000,
};

enum Mx25RPowerMode {
    ULTRA_LOW_POWER_MODE = 0x00, HIGH_PERFORMANCE_MODE = 0x02,
};

enum HardwareMode {
    IOMode, LnrMode, LnrDmaMode, SdmaMode,
};

#define INPUT_DELAY_INIT_VALUE  0x13131313
#define BIT(x) (1U<<(x))


#ifdef CONTR_25F0A
/*
* Host Controller Configuration Register (HC_CONFIG, 0x00)
*/
#define HC_IFC_OFS              27
#define HC_IFC_MASK             (0x1f << HC_IFC_OFS)
#define HC_IFC_SING_SPI         ((0x00) << HC_IFC_OFS)
#define HC_IFC_SING_4IO         ((0x01) << HC_IFC_OFS)
#define HC_IFC_SING_8IO         ((0x02) << HC_IFC_OFS)
#define HC_IFC_DUAL_STAC_4IO    ((0x11) << HC_IFC_OFS)
#define HC_IFC_DUAL_STAC_8IO    ((0x12) << HC_IFC_OFS)
#define HC_IFC_DUAL_PARA        ((0x18) << HC_IFC_OFS)
#define HC_IFC_DUAL_PARA_8IO    ((0x1A) << HC_IFC_OFS)
#define HC_IFC_DUAL_PARA_16IO   ((0x1C) << HC_IFC_OFS)

#define HC_UT_OFS                     25
#define HC_UT_MASK                    (3 << HC_UT_OFS)
#define HC_UT_SPI_NOR                 (0 << HC_UT_OFS)
#define HC_UT_SPI_NAND                (1 << HC_UT_OFS)
#define HC_UT_SPI_RAM                 (2 << HC_UT_OFS)
#define HC_UT_ONFI_NAND               (3 << HC_UT_OFS)

#define HC_LT_OFS                     23
#define HC_LT_MASK                    (3 << HC_LT_OFS)
#define HC_LT_SPI_NOR                 (0 << HC_LT_OFS)
#define HC_LT_SPI_NAND                (1 << HC_LT_OFS)
#define HC_LT_SPI_RAM                 (2 << HC_LT_OFS)
#define HC_LT_ONFI_NAND               (3 << HC_LT_OFS)

#define HC_SLV_ACT_OFS                21
#define HC_SLV_ACT_MASK               (3 << HC_SLV_ACT_OFS)
#define HC_SLV_LOWER                  (0 << HC_SLV_ACT_OFS)
#define HC_SLV_UPPER                  (1 << HC_SLV_ACT_OFS)

#define HC_PH_POL_OFS                 19
#define HC_PH_POL_MASK                (3 << HC_PH_POL_OFS)
#define HC_CLK_POL                    (1 << HC_PH_POL_OFS)
#define HC_CLK_PHA                    (2 << HC_PH_POL_OFS)
#define HC_MODE0                      0
#define HC_MODE1                      HC_CLK_PHA
#define HC_MODE2                      HC_CLK_POL
#define HC_MODE3                      (HC_CLK_POL | HC_CLK_PHA)

#define HC_ENDIAN_OFS                 18
#define HC_ENDIAN_MASK                (1 << HC_ENDIAN_OFS)
#define HC_ENDIAN_LITTLE              (0 << HC_ENDIAN_OFS)
#define HC_ENDIAN_BIG                 (1 << HC_ENDIAN_OFS)

#define HC_DATA_PASS                   BIT(17)
#define HC_IDLE_PIN                    BIT(16)
#define HC_MAN_STRT_EN                 BIT(3)
#define HC_MAN_STRT                    BIT(2)
#define HC_MAN_CS_EN                   BIT(1)
#define HC_MAN_CS                      BIT(0)

/*
* Host Controller Interrupt Status Register(INTR_STS, 0x04)
*/
#define INTRSTS_RYBYB_RDY              BIT(26)
#define INTRSTS_RDSR_RDY               BIT(25)
#define INTRSTS_LNR_SUSP               BIT(24)
#define INTRSTS_ECC_ERR                BIT(17)
#define INTRSTS_CRC_ERR                BIT(16)
#define INTRSTS_LWR_DIS_RDY            BIT(12)
#define INTRSTS_LRD_DIS_RDY            BIT(11)
#define INTRSTS_SDMA_INT               BIT(10)
#define INTRSTS_DMA_FINISH             BIT(9)
#define INTRSTS_RX_N_FULL              BIT(3)
#define INTRSTS_RX_N_EMPT              BIT(2)
#define INTRSTS_TX_N_FULL              BIT(1)
#define INTRSTS_TX_EMPT                BIT(0)
#define INTRSTS_ALL_MASK               0x03FFFF00

/*
* Host Controller Interrupt Status Register Enable (INTR_STS_EN, 0x08)
*/
#define INTRSTS_RYBYB_RDY_EN           BIT(26)
#define INTRSTS_RDSR_RDY_EN            BIT(25)
#define INTRSTS_LNR_SUSP_EN            BIT(24)
#define INTRSTS_ECC_ERR_EN             BIT(17)
#define INTRSTS_CRC_ERR_EN             BIT(16)
#define INTRSTS_LWR_DIS_RDY_EN         BIT(12)
#define INTRSTS_LRD_DIS_RDY_EN         BIT(11)
#define INTRSTS_SDMA_INT_EN            BIT(10)
#define INTRSTS_DMA_FINISH_EN          BIT(9)
#define INTRSTS_TRANS_CMPLT_EN         BIT(8)
#define INTRSTS_RX_N_FULL_EN           BIT(3)
#define INTRSTS_RX_N_EMPT_EN           BIT(2)
#define INTRSTS_TX_N_FULL_EN           BIT(1)
#define INTRSTS_TX_EMPT_EN             BIT(0)
#define INTRSTS_ALL_EN_MASK            0x07031E0F

/*
* Host Controller Interrupt Status Register Signal Enable (INTRPT_STS_SIG, 0x0c)
*/
#define INTRSTS_RYBYB_RDY_SIG        BIT(26)
#define INTRSTS_RDSR_RDY_SIG         BIT(25)
#define INTRSTS_LNR_SUSP_SIG         BIT(24)
#define INTRSTS_ECC_ERR_SIG          BIT(17)
#define INTRSTS_CRC_ERR_SIG          BIT(16)
#define INTRSTS_LWR_DIS_RDY_SIG      BIT(12)
#define INTRSTS_LRD_DIS_RDY_SIG      BIT(11)
#define INTRSTS_SDMA_INT_SIG         BIT(10)
#define INTRSTS_DMA_FINISH_SIG       BIT(9)
#define INTRSTS_TRANS_CMPLT_SIG      BIT(8)
#define INTRSTS_RX_N_FULL_SIG        BIT(3)
#define INTRSTS_RX_N_EMPT_SIG        BIT(2)
#define INTRSTS_TX_N_FULL_SIG        BIT(1)
#define INTRSTS_TX_EMPT_SIG          BIT(0)
#define INTRSTS_ALL_SIG_MASK         0x07030F0F
/*
* Host Controller Hardware Enable Register(HC_EN, 0x10)
*/
#define HC_EN                        BIT(0)
#define HC_DISABLE                        0

/*
* SS0 Control Register (SS0_CTRL, 0x30)
*/
#define SS0_DD_OFS                     23
#define SS0_DD_MASK                    (1 << SS0_DD_OFS)
#define SS0_DD_WRITE                   (0 << SS0_DD_OFS)
#define SS0_DD_READ                    (1 << SS0_DD_OFS)

#define SS0_DUMMY_CNT_OFS             17
#define SS0_DUMMY_CNT_MASK            (0x3F << SS0_DUMMY_CNT_OFS)

#define SS0_ADDR_CNT_OFS             14
#define SS0_ADDR_CNT_MASK            (7 << SS0_ADDR_CNT_OFS)

#define SS0_DUAL_CMD_EN                BIT(13)
#define SS0_OCTA_CRC_EN                BIT(12)
#define SS0_DQS_EN                     BIT(11)
#define SS0_ENHC_EN                    BIT(10)
#define SS0_PREAM_EN                   BIT(9)

#define SS0_DATA_BUS_OFS             6
#define SS0_DATA_BUS_MASK            (7 << SS0_DATA_BUS_OFS)
#define SS0_DATA_BUS_IO_1            (0 << SS0_DATA_BUS_OFS)
#define SS0_DATA_BUS_IO_2            (1 << SS0_DATA_BUS_OFS)
#define SS0_DATA_BUS_IO_4            (2 << SS0_DATA_BUS_OFS)
#define SS0_DATA_BUS_IO_8            (3 << SS0_DATA_BUS_OFS)
#define SS0_DATA_BUS_DDR_EN          (4 << SS0_DATA_BUS_OFS)

#define SS0_ADDR_BUS_OFS             3
#define SS0_ADDR_BUS_MASK            (7 << SS0_ADDR_BUS_OFS)
#define SS0_ADDR_BUS_IO_1            (0 << SS0_ADDR_BUS_OFS)
#define SS0_ADDR_BUS_IO_2            (1 << SS0_ADDR_BUS_OFS)
#define SS0_ADDR_BUS_IO_4            (2 << SS0_ADDR_BUS_OFS)
#define SS0_ADDR_BUS_IO_8            (3 << SS0_ADDR_BUS_OFS)
#define SS0_ADDR_BUS_DDR_EN            (4 << SS0_ADDR_BUS_OFS)

#define SS0_CMD_BUS_OFS             0
#define SS0_CMD_BUS_MASK            (7 << SS0_CMD_BUS_OFS)
#define SS0_CMD_BUS_IO_1            (0 << SS0_CMD_BUS_OFS)
#define SS0_CMD_BUS_IO_2            (1 << SS0_CMD_BUS_OFS)
#define SS0_CMD_BUS_IO_4            (2 << SS0_CMD_BUS_OFS)
#define SS0_CMD_BUS_IO_8            (3 << SS0_CMD_BUS_OFS)
#define SS0_CMD_BUS_DDR_EN          (4 << SS0_CMD_BUS_OFS)

/*
* SS1 Control Register (SS1_CTRL, 0x34)
*/
#define SS1_DD_OFS                     23
#define SS1_DD_MASK                    (1 << SS1_DD_OFS)
#define SS1_DD_WRITE                   (0 << SS1_DD_OFS)
#define SS1_DD_READ                    (1 << SS1_DD_OFS)

#define SS1_DUMMY_CNT_OFS              17
#define SS1_DUMMY_CNT_MASK             (0x3F << SS1_DUMMY_CNT_OFS)

#define SS1_ADDR_CNT_OFS             14
#define SS1_ADDR_CNT_MASK            (7 << SS1_ADDR_CNT_OFS)

#define SS1_DUAL_CMD_EN             BIT(13)
#define SS1_OCTA_CRC_EN             BIT(12)
#define SS1_DQS_EN                  BIT(11)
#define SS1_ENHC_EN                 BIT(10)
#define SS1_PREAM_EN                BIT(9)

#define SS1_DATA_BUS_OFS             6
#define SS1_DATA_BUS_MASK            (7 << SS1_DATA_BUS_OFS)
#define SS1_DATA_BUS_IO_1            (0 << SS1_DATA_BUS_OFS)
#define SS1_DATA_BUS_IO_2            (1 << SS1_DATA_BUS_OFS)
#define SS1_DATA_BUS_IO_4            (2 << SS1_DATA_BUS_OFS)
#define SS1_DATA_BUS_IO_8            (3 << SS1_DATA_BUS_OFS)
#define SS1_DATA_BUS_DDR_EN          (4 << SS1_DATA_BUS_OFS)

#define SS1_ADDR_BUS_OFS             3
#define SS1_ADDR_BUS_MASK            (7 << SS1_ADDR_BUS_OFS)
#define SS1_ADDR_BUS_IO_1            (0 << SS1_ADDR_BUS_OFS)
#define SS1_ADDR_BUS_IO_2            (1 << SS1_ADDR_BUS_OFS)
#define SS1_ADDR_BUS_IO_4            (2 << SS1_ADDR_BUS_OFS)
#define SS1_ADDR_BUS_IO_8            (3 << SS1_ADDR_BUS_OFS)
#define SS1_ADDR_BUS_DDR_EN          (4 << SS1_ADDR_BUS_OFS)

#define SS1_CMD_BUS_OFS             0
#define SS1_CMD_BUS_MASK            (7 << SS1_CMD_BUS_OFS)
#define SS1_CMD_BUS_IO_1            (0 << SS1_CMD_BUS_OFS)
#define SS1_CMD_BUS_IO_2            (1 << SS1_CMD_BUS_OFS)
#define SS1_CMD_BUS_IO_4            (2 << SS1_CMD_BUS_OFS)
#define SS1_CMD_BUS_IO_8            (3 << SS1_CMD_BUS_OFS)
#define SS1_CMD_BUS_DDR_EN          (4 << SS1_CMD_BUS_OFS)

/*
* OCTA Flash CRC Config Register (OCTA_CRC, 0x38)
*/
#define OCTA_CRC_CHUNK_1_IN_EN      BIT(19)
#define OCTA_CRC_CHUNK_1_OFS        17
#define OCTA_CRC_CHUNK_1_MASK       (3 << OCTA_CRC_CHUNK_1_OFS)
#define OCTA_CRC_CHUNK_1_16B        (0 << OCTA_CRC_CHUNK_1_OFS)
#define OCTA_CRC_CHUNK_1_32B        (1 << OCTA_CRC_CHUNK_1_OFS)
#define OCTA_CRC_CHUNK_1_64B        (2 << OCTA_CRC_CHUNK_1_OFS)
#define OCTA_CRC_CHUNK_1_128B       (3 << OCTA_CRC_CHUNK_1_OFS)

#define OCTA_CRC_CHUNK_1_OUT_EN     BIT(16)

#define OCTA_CRC_CHUNK_0_IN_EN      BIT(3)
#define OCTA_CRC_CHUNK_0_OFS        1
#define OCTA_CRC_CHUNK_0_MASK       (3 << OCTA_CRC_CHUNK_0_OFS)
#define OCTA_CRC_CHUNK_0_16B        (0 << OCTA_CRC_CHUNK_0_OFS)
#define OCTA_CRC_CHUNK_0_32B        (1 << OCTA_CRC_CHUNK_0_OFS)
#define OCTA_CRC_CHUNK_0_64B        (2 << OCTA_CRC_CHUNK_0_OFS)
#define OCTA_CRC_CHUNK_0_128B       (3 << OCTA_CRC_CHUNK_0_OFS)

#define OCTA_CRC_CHUNK_0_OUT_EN     BIT(0)

/*
* ONFI NAND SLAVE 0 Data Input Count Register (ONFI0_DIN_CNT, 0x3c)
*/
#define ONFI_0_DIN_CNT_MASK            0xFFFFFFFF

/*
* ONFI NAND SLAVE 1 Data Input Count Register (ONFI1_DIN_CNT, 0x40)
*/
#define ONFI_1_DIN_CNT_MASK            0xFFFFFFFF

/*
* LRD Config Register (LRD_CONFIG, 0x44)
*/
#define LRD_DD_OFS                  23
#define LRD_DD_MASK                 (1 << LRD_DD_OFS)
#define LRD_DD_WRITE                (0 << LRD_DD_OFS)
#define LRD_DD_READ                 (1 << LRD_DD_OFS)


#define LRD_DUMMY_CNT_OFS             17
#define LRD_DUMMY_CNT_MASK            (0x3F << LRD_DUMMY_CNT_OFS)

#define LRD_ADDR_CNT_OFS             14
#define LRD_ADDR_CNT_MASK            (7 << LRD_ADDR_CNT_OFS)

#define LRD_DUAL_CMD_EN               BIT(13)
#define LRD_OCTA_CRC_EN               BIT(12)
#define LRD_DQS_EN                    BIT(11)
#define LRD_ENHC_EN                   BIT(10)
#define LRD_PREAM_EN                  BIT(9)

#define LRD_DATA_BUS_OFS             6
#define LRD_DATA_BUS_MASK            (7 << LRD_DATA_BUS_OFS)
#define LRD_DATA_BUS_IO_1            (0 << LRD_DATA_BUS_OFS)
#define LRD_DATA_BUS_IO_2            (1 << LRD_DATA_BUS_OFS)
#define LRD_DATA_BUS_IO_4            (2 << LRD_DATA_BUS_OFS)
#define LRD_DATA_BUS_IO_8            (3 << LRD_DATA_BUS_OFS)
#define LRD_DATA_BUS_DDR_EN          (4 << LRD_DATA_BUS_OFS)

#define LRD_ADDR_BUS_OFS             3
#define LRD_ADDR_BUS_MASK            (7 << LRD_ADDR_BUS_OFS)
#define LRD_ADDR_BUS_IO_1            (0 << LRD_ADDR_BUS_OFS)
#define LRD_ADDR_BUS_IO_2            (1 << LRD_ADDR_BUS_OFS)
#define LRD_ADDR_BUS_IO_4            (2 << LRD_ADDR_BUS_OFS)
#define LRD_ADDR_BUS_IO_8            (3 << LRD_ADDR_BUS_OFS)
#define LRD_ADDR_BUS_DDR_EN          (4 << LRD_ADDR_BUS_OFS)

#define LRD_CMD_BUS_OFS             0
#define LRD_CMD_BUS_MASK            (7 << LRD_CMD_BUS_OFS)
#define LRD_CMD_BUS_IO_1            (0 << LRD_CMD_BUS_OFS)
#define LRD_CMD_BUS_IO_2            (1 << LRD_CMD_BUS_OFS)
#define LRD_CMD_BUS_IO_4            (2 << LRD_CMD_BUS_OFS)
#define LRD_CMD_BUS_IO_8            (3 << LRD_CMD_BUS_OFS)
#define LRD_CMD_BUS_DDR_EN          (4 << LRD_CMD_BUS_OFS)

/*
* LRD Mode Control Register (LRD_CTRL, 0x48)
*/
#define LRD_CTRL_EN                    BIT(31)

#define LRD_CTRL_SLV_ACT_OFS         21
#define LRD_CTRL_SLV_ACT_MASK        (3 << LRD_CTRL_SLV_ACT_OFS)
#define LRD_CTRL_SLV_ACT_LOW         (0 << LRD_CTRL_SLV_ACT_OFS)
#define LRD_CTRL_SLV_ACT_UP          (1 << LRD_CTRL_SLV_ACT_OFS)

#define LRD_CTRL_CMD_CODE_2ND_OFS     8
#define LRD_CTRL_CMD_CODE_2ND_MASK    (0xFF << LRD_CTRL_CMD_CODE_2ND_OFS)

#define LRD_CTRL_CMD_CODE_1ST_OFS     0
#define LRD_CTRL_CMD_CODE_1ST_MASK    (0xFF << LRD_CTRL_CMD_CODE_1ST_OFS)


/*
* LRD Mode Start Mapping Address Register (LRD_ADDR, 0x4c)
*/
#define LRD_ADDR_MASK                0xFFFFFFFF

/*
* LRD Mode Start Mapping Range Register (LRD_RNG, 0x50)
*/
#define LRD_RNG_MASK                0xFFFFFFFF

/*
* AXI SLAVE BUS Based Address Register (AXI_SLV_ADDR, 0x54)
*/
#define AXI_SLV_ADDR_MASK            0xFFFFFFFF

/*
* DMAC Read Configuration Register (DMAC_RD_CONFIG, 0x58)
*/
#define DMAC_RD_PERIPH_EN           BIT(31)
#define DMAC_RD_ALL_FLUSH_EN        BIT(30)
#define DMAC_RD_LAST_FLUSH_EN       BIT(29)

#define DMAC_RD_QE_OFS                16
#define DMAC_RD_QE_MASK               (0xF << DMAC_RD_QE_OFS)

#define DMAC_RD_BRST_LEN_OFS         12
#define DMAC_RD_BRST_LEN_MASK        (0xF << DMAC_RD_BRST_LEN_OFS)

#define DMAC_RD_BRST_SIZE_OFS        8
#define DMAC_RD_BRST_SIZE_MASK       (7 << DMAC_RD_BRST_SIZE_OFS)
#define DMAC_RD_BRST_SIZE_4B         (2 << DMAC_RD_BRST_SIZE_OFS)

#define DMAC_RD_DD_OFS                 1
#define DMAC_RD_DD_MASK                (1 << DMAC_RD_DD_OFS)
#define DMAC_RD_DD_WRITE               (0 << DMAC_RD_DD_OFS)
#define DMAC_RD_DD_READ                (1 << DMAC_RD_DD_OFS)

#define DMAC_RD_START                BIT(0)

/*
* DMAC_Transfer Byte Count Register (DMAC_CNT, 0x5c)
*/
#define DMAC_CNT_MASK                0xFFFFFFFF

/*
* SDMA System Address Register (SDMA_ADDR, 0x60)
*/
#define SDMA_ADDR_MASK                0xFFFFFFFF

/*
* DMA Master Configuration Register (DMAM_CONFIG, 0x64)
*/
#define DMAM_START                    BIT(31)
#define DMAM_CONT                    BIT(30)

#define DMAM_SDMA_GAP_OFS             2
#define DMAM_SDMA_GAP_MASK            (7 << DMAM_SDMA_GAP_OFS)
#define DMAM_SDMA_GAP_4KB             (0 << DMAM_SDMA_GAP_OFS)
#define DMAM_SDMA_GAP_8KB             (1 << DMAM_SDMA_GAP_OFS)
#define DMAM_SDMA_GAP_16KB            (2 << DMAM_SDMA_GAP_OFS)
#define DMAM_SDMA_GAP_32KB            (3 << DMAM_SDMA_GAP_OFS)
#define DMAM_SDMA_GAP_64KB            (4 << DMAM_SDMA_GAP_OFS)
#define DMAM_SDMA_GAP_128KB           (5 << DMAM_SDMA_GAP_OFS)
#define DMAM_SDMA_GAP_256KB           (6 << DMAM_SDMA_GAP_OFS)
#define DMAM_SDMA_GAP_512KB           (7 << DMAM_SDMA_GAP_OFS)

#define DMAM_DD_OFS                 1
#define DMAM_DD_MASK                (1 << DMAM_DD_OFS)
#define DMAM_DD_WRITE               (0 << DMAM_DD_OFS)
#define DMAM_DD_READ                (1 << DMAM_DD_OFS)

#define DMAM_EN                        BIT(0)

/*
* DMA Master Transfer Byte Byte Count Register (DMAM_CNT, 0x68)
*/
#define DMAM_CNT                    0xFFFFFFFF

/*
* Timer Threshold REgister of LNR Addressing Mode Register (LNR_TIMER_TH, 0x6c)
*/
#define LNR_TIMER_TH_MASK            0xFFFFFFFF

/*
* RWW Configuration Register (RWW_CONFIG, 0x70)
*/
#define RWW_DD_OFS                     23
#define RWW_DD_MASK                    (1 << RWW_DD_OFS)
#define RWW_DD_WRITE                   (0 << RWW_DD_OFS)
#define RWW_DD_READ                    (1 << RWW_DD_OFS)

#define RWW_DUMMY_CNT_OFS             17
#define RWW_DUMMY_CNT_MASK            (0x3F << RWW_DUMMY_CNT_OFS)

#define RWW_ADDR_CNT_OFS             14
#define RWW_ADDR_CNT_MASK            (07 << RWW_ADDR_CNT_OFS)

#define RWW_DUAL_CMD_EN                BIT(13)
#define RWW_OCTA_CRC_EN                BIT(12)
#define RWW_DQS_EN                     BIT(11)
#define RWW_ENHC_EN                    BIT(10)
#define RWW_PREAM_EN                   BIT(9)

#define RWW_DATA_BUS_OFS             6
#define RWW_DATA_BUS_MASK            (7 << RWW_DATA_BUS_OFS)
#define RWW_DATA_BUS_IO_1            (0 << RWW_DATA_BUS_OFS)
#define RWW_DATA_BUS_IO_2            (1 << RWW_DATA_BUS_OFS)
#define RWW_DATA_BUS_IO_4            (2 << RWW_DATA_BUS_OFS)
#define RWW_DATA_BUS_IO_8            (3 << RWW_DATA_BUS_OFS)
#define RWW_DATA_BUS_DDR_EN          (4 << RWW_DATA_BUS_OFS)

#define RWW_ADDR_BUS_OFS             3
#define RWW_ADDR_BUS_MASK            (7 << RWW_ADDR_BUS_OFS)
#define RWW_ADDR_BUS_IO_1            (0 << RWW_ADDR_BUS_OFS)
#define RWW_ADDR_BUS_IO_2            (1 << RWW_ADDR_BUS_OFS)
#define RWW_ADDR_BUS_IO_4            (2 << RWW_ADDR_BUS_OFS)
#define RWW_ADDR_BUS_IO_8            (3 << RWW_ADDR_BUS_OFS)
#define RWW_ADDR_BUS_DDR_EN          (4 << RWW_ADDR_BUS_OFS)

#define RWW_CMD_BUS_OFS             0
#define RWW_CMD_BUS_MASK            (7 << RWW_CMD_BUS_OFS)
#define RWW_CMD_BUS_IO_1            (0 << RWW_CMD_BUS_OFS)
#define RWW_CMD_BUS_IO_2            (1 << RWW_CMD_BUS_OFS)
#define RWW_CMD_BUS_IO_4            (2 << RWW_CMD_BUS_OFS)
#define RWW_CMD_BUS_IO_8            (3 << RWW_CMD_BUS_OFS)
#define RWW_CMD_BUS_DDR_EN          (4 << RWW_CMD_BUS_OFS)

/*
* RWW Control Register (RWW_CTRL, 0x74)
*/
#define RWW_CTRL_EN                    BIT(31)

#define RWW_CTRL_SLV_ACT_OFS         21
#define RWW_CTRL_SLV_ACT_MASK        (3 << RWW_CTRL_SLV_ACT_OFS)
#define RWW_CTRL_SLV_ACT_LOW         (0 << RWW_CTRL_SLV_ACT_OFS)
#define RWW_CTRL_SLV_ACT_UP          (1 << RWW_CTRL_SLV_ACT_OFS)

#define RWW_CTRL_CMD_CODE_2ND_OFS         8
#define RWW_CTRL_CMD_CODE_2ND_MASK        (0xFF << RWW_CTRL_CMD_CODE_2ND_OFS)

#define RWW_CTRL_CMD_CODE_1ST_OFS         0
#define RWW_CTRL_CMD_CODE_1ST_MASK        (0xFF << RWW_CTRL_CMD_CODE_1ST_OFS)

/*
* Randomizer Configuration Register 0 (RDM_CONFIG_0, 0x78)
*/
#define RDM_CONFIG_0_POLY_OFS         0
#define RDM_CONFIG_0_POLY_MASK        (0x1FFFF << RDM_CONFIG_0_POLY_OFS)

/*
* Randomizer Configuration Register 1 (RDM_CONFIG_1, 0x7c)
*/
#define RDM_CONFIG_1_EN                BIT(31)

#define RDM_CONFIG_1_INIT_SEED_OFS     0
#define RDM_CONFIG_1_INIT_SEED_MASK    (0xFFFF << RDM_CONFIG_1_INIT_SEED_OFS)

/*
* LWR Mode Config (LWR_CONFIG, 0x80)
*/
#define LWR_DD_OFS                     23
#define LWR_DD_MASK                    (1 << LWR_DD_OFS)
#define LWR_DD_WRITE                   (0 << LWR_DD_OFS)
#define LWR_DD_READ                    (1 << LWR_DD_OFS)


#define LWR_DUMMY_CNT_OFS             17
#define LWR_DUMMY_CNT_MASK            (0x3F << LWR_DUMMY_CNT_OFS)

#define LWR_ADDR_CNT_OFS             14
#define LWR_ADDR_CNT_MASK            (7 << LWR_ADDR_CNT_OFS)

#define LWR_DUAL_CMD_EN                BIT(13)
#define LWR_OCTA_CRC_EN                BIT(12)
#define LWR_DQS_EN                     BIT(11)
#define LWR_ENHC_EN                    BIT(10)
#define LWR_PREAM_EN                   BIT(9)

#define LWR_DATA_BUS_OFS             6
#define LWR_DATA_BUS_MASK            (7 << LWR_DATA_BUS_OFS)
#define LWR_DATA_BUS_IO_1            (0 << LWR_DATA_BUS_OFS)
#define LWR_DATA_BUS_IO_2            (1 << LWR_DATA_BUS_OFS)
#define LWR_DATA_BUS_IO_4            (2 << LWR_DATA_BUS_OFS)
#define LWR_DATA_BUS_IO_8            (3 << LWR_DATA_BUS_OFS)
#define LWR_DATA_BUS_DDR_EN          (4 << LWR_DATA_BUS_OFS)

#define LWR_ADDR_BUS_OFS             3
#define LWR_ADDR_BUS_MASK            (7 << LWR_ADDR_BUS_OFS)
#define LWR_ADDR_BUS_IO_1            (0 << LWR_ADDR_BUS_OFS)
#define LWR_ADDR_BUS_IO_2            (1 << LWR_ADDR_BUS_OFS)
#define LWR_ADDR_BUS_IO_4            (2 << LWR_ADDR_BUS_OFS)
#define LWR_ADDR_BUS_IO_8            (3 << LWR_ADDR_BUS_OFS)
#define LWR_ADDR_BUS_DDR_EN          (4 << LWR_ADDR_BUS_OFS)

#define LWR_CMD_BUS_OFS             0
#define LWR_CMD_BUS_MASK            (7 << LWR_CMD_BUS_OFS)
#define LWR_CMD_BUS_IO_1            (0 << LWR_CMD_BUS_OFS)
#define LWR_CMD_BUS_IO_2            (1 << LWR_CMD_BUS_OFS)
#define LWR_CMD_BUS_IO_4            (2 << LWR_CMD_BUS_OFS)
#define LWR_CMD_BUS_IO_8            (3 << LWR_CMD_BUS_OFS)
#define LWR_CMD_BUS_DDR_EN          (4 << LWR_CMD_BUS_OFS)

/*
* LWR Mode Control Register (LWR_CTRL, 0x84)
*/
#define LWR_CTRL_EN                    BIT(31)

#define LWR_CTRL_SLV_ACT_OFS         21
#define LWR_CTRL_SLV_ACT_MASK        (3 << LWR_CTRL_SLV_ACT_OFS)
#define LWR_CTRL_SLV_ACT_LOW         (0 << LWR_CTRL_SLV_ACT_OFS)
#define LWR_CTRL_SLV_ACT_UP          (1 << LWR_CTRL_SLV_ACT_OFS)

#define LWR_CTRL_CMD_CODE_2ND_OFS     8
#define LWR_CTRL_CMD_CODE_2ND_MASK    (0xFF << LWR_CTRL_CMD_CODE_2ND_OFS)

#define LWR_CTRL_CMD_CODE_1ST_OFS     0
#define LWR_CTRL_CMD_CODE_1ST_MASK    (0xFF << LWR_CTRL_CMD_CODE_1ST_OFS)

/*
* LWR Mode Start Mapping Address Register (LWR_ADDR, 0x88)
*/
#define LWR_ADDR_MASK                0xFFFFFFFF

/*
* LWR Mode Start Mapping Range Register (LWR_RNG, 0x8c)
*/
#define LWR_RNG_MASK                0xFFFFFFFF

/*
* LWR Suspend Control Register (LWR_SUSP_CTRL, 0x90)
*/
#define LWR_SUSP_CTRL_EN            BIT(31)

/*
* DMAC Write Configuration Register (DMAC_WR_CONFIG, 0x94)
*/
#define DMAC_WR_PERIPH_EN           BIT(31)
#define DMAC_WR_ALL_FLUSH_EN        BIT(30)
#define DMAC_WR_LAST_FLUSH_EN       BIT(29)

#define DMAC_WR_QE_OFS                 16
#define DMAC_WR_QE_MASK                (0xf << DMAC_WR_QE_OFS)

#define DMAC_WR_BRST_LEN_OFS         12
#define DMAC_WR_BRST_LEN_MASK        (0xf << DMAC_WR_BRST_LEN_OFS)

#define DMAC_WR_BRST_SIZE_OFS         8
#define DMAC_WR_BRST_SIZE_MASK        (7 << DMAC_RD_BRST_SIZE_OFS)
#define DMAC_WR_BRST_SIZE_4B          (2 << DMAC_RD_BRST_SIZE_OFS)

#define DMAC_WR_DD_OFS                 1
#define DMAC_WR_DD_MASK                (1 << DMAC_WR_DD_OFS)
#define DMAC_WR_DD_WRITE               (0 << DMAC_WR_DD_OFS)
#define DMAC_WR_DD_READ                (1 << DMAC_WR_DD_OFS)

#define DMAC_WR_START                BIT(0)

/*
* DMAC Write Byte Count Register (DMAC_WR_CNT, 0x98)
*/

#define DMAC_WR_CNT_MASK                0xFFFFFFFF

/*
* DMA Slave COntrol Register (DMAS_CTRL, 0x9c)
*/
#define DMAS_CTRL_DD_OFS             31
#define DMAS_CTRL_DD_MASK            (1 << DMAS_CTRL_DD_OFS)
#define DMAS_CTRL_DD_WRITE           (0 << DMAS_CTRL_DD_OFS)
#define DMAS_CTRL_DD_READ            (1 << DMAS_CTRL_DD_OFS)

#define DMAS_CTRL_EN                BIT(30)

#define DMAS_CTRL_DATA_BUS_OFS     6
#define DMAS_CTRL_DATA_BUS_MASK    (0x7 << DMAS_CTRL_DATA_BUS_OFS)
#define DMAS_CTRL_DATA_BUS_IO_1    (0 << DMAS_CTRL_DATA_BUS_OFS)
#define DMAS_CTRL_DATA_BUS_IO_2    (1 << DMAS_CTRL_DATA_BUS_OFS)
#define DMAS_CTRL_DATA_BUS_IO_4    (2 << DMAS_CTRL_DATA_BUS_OFS)
#define DMAS_CTRL_DATA_BUS_IO_8    (3 << DMAS_CTRL_DATA_BUS_OFS)
#define DMAS_CTRL_DATA_BUS_DDR_EN  (4 << DMAS_CTRL_DATA_BUS_OFS)

/*
* Data Strobe COnfigration Register (DATA_STROB, 0xa0)
*/
#define DATA_STROB_EDO_EN           BIT(2)
#define DATA_STROB_POLARITY         BIT(1)
#define DATA_STROB_CYCLE            BIT(0)

/*
* Input Delay Code Register 0 (IDLY_CODE_0, 0xa4)
*/
#define NDQS0_IDLY_3_OFS             24
#define NDQS0_IDLY_3_MASK            (0x1F << NDQS0_IDLY_3_OFS)
#define NDQS0_IDLY_2_OFS             16
#define NDQS0_IDLY_2_MASK            (0x1F << NDQS0_IDLY_2_OFS)
#define NDQS0_IDLY_1_OFS             8
#define NDQS0_IDLY_1_MASK            (0x1F << NDQS0_IDLY_1_OFS)
#define NDQS0_IDLY_0_OFS             0
#define NDQS0_IDLY_0_MASK            (0x1F << NDQS0_IDLY_0_OFS)

/*
* Input Delay Code Register 1 (IDLY_CODE_1, 0xa8)
*/
#define NDQS0_IDLY_7_OFS             24
#define NDQS0_IDLY_7_MASK            (0x1F << NDQS0_IDLY_7_OFS)
#define NDQS0_IDLY_6_OFS             16
#define NDQS0_IDLY_6_MASK            (0x1F << NDQS0_IDLY_6_OFS)
#define NDQS0_IDLY_5_OFS             8
#define NDQS0_IDLY_5_MASK            (0x1F << NDQS0_IDLY_5_OFS)
#define NDQS0_IDLY_4_OFS             0
#define NDQS0_IDLY_4_MASK            (0x1F << NDQS0_IDLY_4_OFS)

/*
* Input Delay Code Register 2 (IDLY_CODE_2, 0xac)
*/
#define NDQS1_IDLY_3_OFS             24
#define NDQS1_IDLY_3_MASK            (0x1F << NDQS1_IDLY_3_OFS)
#define NDQS1_IDLY_2_OFS             16
#define NDQS1_IDLY_2_MASK            (0x1F << NDQS1_IDLY_2_OFS)
#define NDQS1_IDLY_1_OFS             8
#define NDQS1_IDLY_1_MASK            (0x1F << NDQS1_IDLY_1_OFS)
#define NDQS1_IDLY_0_OFS             0
#define NDQS1_IDLY_0_MASK            (0x1F << NDQS1_IDLY_0_OFS)

/*
* Input Delay Code Register 3 (IDLY_CODE_3, 0xb0)
*/
#define NDQS1_IDLY_7_OFS             24
#define NDQS1_IDLY_7_MASK            (0x1F << NDQS1_IDLY_7_OFS)
#define NDQS1_IDLY_6_OFS             16
#define NDQS1_IDLY_6_MASK            (0x1F << NDQS1_IDLY_6_OFS)
#define NDQS1_IDLY_5_OFS             8
#define NDQS1_IDLY_5_MASK            (0x1F << NDQS1_IDLY_5_OFS)
#define NDQS1_IDLY_4_OFS             0
#define NDQS1_IDLY_4_MASK            (0x1F << NDQS1_IDLY_4_OFS)

/*
* Input Delay Code Register 0 (IDLY_CODE_4, 0xb4)
*/
#define DQS0_IDLY_3_OFS             24
#define DQS0_IDLY_3_MASK            (0x1F << DQS0_IDLY_3_OFS)
#define DQS0_IDLY_2_OFS             16
#define DQS0_IDLY_2_MASK            (0x1F << DQS0_IDLY_2_OFS)
#define DQS0_IDLY_1_OFS             8
#define DQS0_IDLY_1_MASK            (0x1F << DQS0_IDLY_1_OFS)
#define DQS0_IDLY_0_OFS             0
#define DQS0_IDLY_0_MASK            (0x1F << DQS0_IDLY_0_OFS)

/*
* Input Delay Code Register 1 (IDLY_CODE_5, 0xb8)
*/
#define DQS0_IDLY_7_OFS             24
#define DQS0_IDLY_7_MASK            (0x1F << DQS0_IDLY_7_OFS)
#define DQS0_IDLY_6_OFS             16
#define DQS0_IDLY_6_MASK            (0x1F << DQS0_IDLY_6_OFS)
#define DQS0_IDLY_5_OFS             8
#define DQS0_IDLY_5_MASK            (0x1F << DQS0_IDLY_5_OFS)
#define DQS0_IDLY_4_OFS             0
#define DQS0_IDLY_4_MASK            (0x1F << DQS0_IDLY_4_OFS)

/*
* Input Delay Code Register 2 (IDLY_CODE_6, 0xbc)
*/
#define DQS1_IDLY_3_OFS             24
#define DQS1_IDLY_3_MASK            (0x1F << DQS1_IDLY_3_OFS)
#define DQS1_IDLY_2_OFS             16
#define DQS1_IDLY_2_MASK            (0x1F << DQS1_IDLY_2_OFS)
#define DQS1_IDLY_1_OFS             8
#define DQS1_IDLY_1_MASK            (0x1F << DQS1_IDLY_1_OFS)
#define DQS1_IDLY_0_OFS             0
#define DQS1_IDLY_0_MASK            (0x1F << DQS1_IDLY_0_OFS)

/*
* Input Delay Code Register 3 (IDLY_CODE_7, 0xc0)
*/
#define DQS1_IDLY_7_OFS             24
#define DQS1_IDLY_7_MASK            (0x1F << DQS1_IDLY_7_OFS)
#define DQS1_IDLY_6_OFS             16
#define DQS1_IDLY_6_MASK            (0x1F << DQS1_IDLY_6_OFS)
#define DQS1_IDLY_5_OFS             8
#define DQS1_IDLY_5_MASK            (0x1F << DQS1_IDLY_5_OFS)
#define DQS1_IDLY_4_OFS             0
#define DQS1_IDLY_4_MASK            (0x1F << DQS1_IDLY_4_OFS)

/*
* General Purpose Inputs and Outputs Register (GPIO, 0xc4)
*/
#define GPIO_PT1                    BIT(19)
#define GPIO_RESET_1                BIT(18)
#define GPIO_HOLDB_1                BIT(17)
#define GPIO_WPB_1                  BIT(16)
#define GPIO_PT0                    BIT(3)
#define GPIO_RESET_0                BIT(2)
#define GPIO_HOLDB_0                BIT(1)
#define GPIO_WPB_0                  BIT(0)

/*
* Host Controller Version Register (HC_VER, 0xd0)
*/
#define HC_VERS_HW_OFS             0
#define HC_VERS_HW_MASK            (0xFF << HC_VERS_HW_OFS)
#define HC_VERS_HW_ONFI_OFS        8
#define HC_VERS_HW_ONFI_MASK       (0xFF << HC_VERS_HW_ONFI_OFS)
#define HC_VERS_HW_SPEC_OFS        16
#define HC_VERS_HW_SPEC_MASK       (0xFF << HC_VERS_HW_SPEC_OFS)
#define HC_VERS_HW_VIVADO_OFS      24
#define HC_VERS_HW_VIVADO_MASK     (0xFF << HC_VERS_HW_VIVADO_OFS)

/*
* Host Controller Test Configuration Register 0 (HC_TEST_0, 0xe0)
*/
#define HC_TEST_0_MAX_LNR_SUSP_MASK    0xFFFFFFFF

/*
* Host Controller Test Configuration Register 1 (HC_TEST_1, 0xe4)
*/
#define HC_TEST_1_MASK                0xFFFFFFFF

/*
* Host Controller Test Configuration Register 2 (HC_TEST_2, 0xe8)
*/
#define HC_TEST_2_MASK                0xFFFFFFFF

/*
* Host Controller Test Configuration Register 3 (HC_TEST_3, 0xec)
*/
#define HC_TEST_3_MASK                0xFFFFFFFF

/*
* Host Controller Test Configuration Register 4 (HC_TEST_4, 0xf0)
*/
#define HC_TEST_4_MASK                0xFFFFFFFF

/*
* Host Controller Test Configuration Register 5 (HC_TEST_5, 0xf4)
*/
#define HC_TEST_5_MASK                0xFFFFFFFF

/*
* Host Controller Test Configuration Register 6 (HC_TEST_6, 0xf8)
*/
#define HC_TEST_6_MASK                0xFFFFFFFF

/*
* Host Controller Test Configuration Register 7 (HC_TEST_7, 0xfc)
*/
#define HC_TEST_7_MASK                0xFFFFFFFF

    typedef struct {
        u32    Ssr;               //0x000
        u32 Sr;                   //0x004
        u8  reserved2[0x1f8];     //0x008~0x11FF
        u32 Ccr00;                //0x200
        u32 Ccr01;                //0x204
        u32 Ccr02;                //0x208
        u32 Ccr03;                //0x20c
        u32 Ccr04;                //0x210
        u32 Ccr05;                //0x214
        u32 Ccr06;                //0x218
        u32 Ccr07;                //0x21c
        u32 Ccr08;                //0x220
        u32 Ccr09;                //0x224
        u32 Ccr10;                //0x228
        u32 Ccr11;                //0x22c
        u32 Ccr12;                //0x230
        u32 Ccr13;                //0x234
        u32 Ccr14;                //0x238
        u32 Ccr15;                //0x23c
        u32 Ccr16;                //0x240
        u32 Ccr17;                //0x244
        u32 Ccr18;                //0x248
        u32 Ccr19;                //0x24c
        u32 Ccr20;                //0x250
        u32 Ccr21;                //0x254
        u32 Ccr22;                //0x258
        u32 Ccr23;                //0x25c
    }MxHcClkRegs;

    typedef struct {
        u32 HcConfig;         //0x000
        u32 IntrSts;          //0x004
        u32 IntrStsEn;        //0x008
        u32 IntrSigEn;        //0x00c
        u32 HcEn;             //0x010
        u32 Txd0;             //0x014
        u32 Txd1;             //0x018
        u32 Txd2;             //0x01c
        u32 Txd3;             //0x020
        u32 Rxd;              //0x024
        u32 Reserved0;        //0x028
        u32 Reserved1;        //0x02c
        u32 Ss0Ctrl;          //0x030
        u32 Ss1Ctrl;          //0x034
        u32 OctaCrc;          //0x038
        u32 ONFI0DinCnt;      //0x03c
        u32 ONFI1DinCnt;      //0x040
        u32 LrdConfig;        //0x044
        u32 LrdCtrl;          //0x048
        u32 LrdAddr;          //0x04c
        u32 LrdRng;           //0x050
        u32 AxiSlaveAddr;     //0x054
        u32 DmacConfig;       //0x058
        u32 DmacCnt;          //0x05c
        u32 SdmaAddr;         //0x060
        u32 DmaMConfig;       //0x064
        u32 DmaMCnt;          //0x068
        u32 LnrTimerTh;       //0x06c
        u32 RwwConfig;        //0x070
        u32 RwwCtrl;          //0x074
        u32 RdmConfig0;       //0x078
        u32 RdmConfig1;       //0x07c
        u32 LwrConfig;        //0x080
        u32 LwrCtrl;          //0x084
        u32 LwrAddr;          //0x088
        u32 LwrRng;           //0x08c
        u32 LwrSuspCtrl;      //0x090
        u32 DmacWrConfig;     //0x094
        u32 DmacWrCnt;        //0x098
        u32 DmasCtrl;         //0x09c
        u32 DataStrob;        //0x0a0
        u32 IdlyCode0;        //0x0a4
        u32 IdlyCode1;        //0x0a8
        u32 IdlyCode2;        //0x0ac
        u32 IdlyCode3;        //0x0b0
        u32 IdlyCode4;        //0x0b4
        u32 IdlyCode5;        //0x0b8
        u32 IdlyCode6;        //0x0bc
        u32 IdlyCode7;        //0x0c0
        u32 Gpio;             //0x0c4
        u32 Reserved2;        //0x0c8
        u32 Reserved3;        //0x0cc
        u32 HcVer;            //0x0d0
        u32 Reserved4;        //0x0d4
        u32 Reserved5;        //0x0d8
        u32 Reserved6;        //0x0dc
        u32 HcTest0;          //0x0e0
        u32 HcTest1;          //0x0e4
        u32 HcTest2;          //0x0e8
        u32 HcTest3;          //0x0ec
        u32 HcTest4;          //0x0f0
        u32 HcTest5;          //0x0f4
        u32 HcTest6;          //0x0f8
        u32 HcTest7;          //0x0fc
    } MxHcRegs;
#else

/* Definition for QSPI clock resources */
#define OSPI_CLK_ENABLE()           __HAL_RCC_OSPI2_CLK_ENABLE()
#define OSPI_CLK_DISABLE()          __HAL_RCC_OSPI2_CLK_DISABLE()
#define OSPIM_CLK_ENABLE()          __HAL_RCC_OSPIM_CLK_ENABLE()
#define OSPIM_CLK_DISABLE()         __HAL_RCC_OSPIM_CLK_DISABLE()
#define OSPI_CS_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOG_CLK_ENABLE()
#define OSPI_CLK_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOI_CLK_ENABLE()
#define OSPI_D0_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOI_CLK_ENABLE()
#define OSPI_D1_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOI_CLK_ENABLE()
#define OSPI_D2_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOI_CLK_ENABLE()
#define OSPI_D3_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOH_CLK_ENABLE()
#define OSPI_D4_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOH_CLK_ENABLE()
#define OSPI_D5_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOH_CLK_ENABLE()
#define OSPI_D6_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOG_CLK_ENABLE()
#define OSPI_D7_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOG_CLK_ENABLE()
#define OSPI_DQS_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOG_CLK_ENABLE()
#define OSPI_DMA_CLK_ENABLE()       __HAL_RCC_DMA1_CLK_ENABLE()
#define OSPI_DMAMUX_CLK_ENABLE()    __HAL_RCC_DMAMUX1_CLK_ENABLE()

#define OSPI_FORCE_RESET()          __HAL_RCC_OSPI2_FORCE_RESET()
#define OSPI_RELEASE_RESET()        __HAL_RCC_OSPI2_RELEASE_RESET()

/* Definition for QSPI Pins */
#define OSPI_CS_PIN             GPIO_PIN_12
#define OSPI_CS_GPIO_PORT       GPIOG
#define OSPI_CLK_PIN            GPIO_PIN_6
#define OSPI_CLK_GPIO_PORT      GPIOI
#define OSPI_D0_PIN             GPIO_PIN_11
#define OSPI_D0_GPIO_PORT       GPIOI
#define OSPI_D1_PIN             GPIO_PIN_10
#define OSPI_D1_GPIO_PORT       GPIOI
#define OSPI_D2_PIN             GPIO_PIN_9
#define OSPI_D2_GPIO_PORT       GPIOI
#define OSPI_D3_PIN             GPIO_PIN_8
#define OSPI_D3_GPIO_PORT       GPIOH
#define OSPI_D4_PIN             GPIO_PIN_9
#define OSPI_D4_GPIO_PORT       GPIOH
#define OSPI_D5_PIN             GPIO_PIN_10
#define OSPI_D5_GPIO_PORT       GPIOH
#define OSPI_D6_PIN             GPIO_PIN_9
#define OSPI_D6_GPIO_PORT       GPIOG
#define OSPI_D7_PIN             GPIO_PIN_10
#define OSPI_D7_GPIO_PORT       GPIOG
#define OSPI_DQS_PIN            GPIO_PIN_15
#define OSPI_DQS_GPIO_PORT      GPIOG

/* Definition for OSPI DMA */
#define OSPI_DMA_CHANNEL        DMA1_Channel1
#define OSPI_DMA_REQUEST        DMA_REQUEST_OCTOSPI2
#define OSPI_DMA_IRQ            DMA1_Channel1_IRQn
#define OSPI_DMA_IRQ_HANDLER    DMA1_Channel1_IRQHandler
#endif

/********************************************************************************
 *               Function Declaration                   *
 ********************************************************************************/

#ifdef CONTR_25F0A
    u32 inline MxRd32(u32 *BaseAddr);
    void inline MxWr32(u32 *BaseAddr, u32 Val);
#elif PLATFORM_ST
void STM_Set_CSPin_QSPI(void);
void STM_Set_CSPin_GPIO(void);
void STM_Set_CSPin_High(void);
void STM_Set_CSPin_Low(void);
#endif

int MxHardwareInit(MxSpi *Spi);
int MxPolledTransfer(MxSpi *Spi, u8 *WrBuf, u8 *RdBuf, u32 ByteCount);
int MxLnrModeWrite(MxSpi *Spi, u8 *WrBuf, u32 Address, u32 ByteCount, u8 WriteCmd);
int MxLnrModeRead(MxSpi *Spi, u8 *RdBuf, u32 Address, u32 ByteCount, u8 ReadCmd);
void MxSetDeviceFreq(u32 SetDevFreq);
int MxGetHcVer(MxSpi *Spi);

#endif /* MXIC_HC_H_ */
