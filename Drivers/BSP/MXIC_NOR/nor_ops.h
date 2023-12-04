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

#ifndef NOR_OPS_H_
#define NOR_OPS_H_

#include "nor_cmd.h"

/*
 * Advanced Protection mode
 */
enum MX_ASP_MODE {
    ASP_SPB_LOCK = 0,
    ASP_DPB_LOCK,
    ASP_DPB_UNLOCK,
    ASP_SBLK,
    ASP_SBULK,
    ASP_PLOCK, /* Permanent lock 0x64 */
    ASP_BWL_LOCK, /* Block write lock protection 0xE2 */
    ASP_SPB,
    ASP_DPB,
    ASP_SB,
    ASP_PERM, /* permanent lock 0x3f */
    ASP_BWL /* block write lock */
};

enum FlashLockStatus {
    FLASH_IS_UNLOCKED = 14, FLASH_IS_LOCKED = 15, FLASH_IS_PARTIALLY_LOCKED = 16,
};

enum FlashCurProtectMode {
    BP_MODE = 10, ASP_SOLID_MODE = 11, ASP_PWD_MODE = 12, SBP_MODE = 13,
};
/*
 * here is used to check boundary of 64K/4K protection area
 */
#define MX_64KB                         (1 << 16)
#define MX_4KB                          (1 << 12)
#define MX_WP64K_FIRST                  MX_64KB
#define MX_WP64K_LAST(mtd_sz)           ((mtd_sz) - MX_64KB)

#define MX_SINGLE_WP4K_NUMBER           (MX_64KB / MX_4KB)
#define MX_WP64K_NUMBER(Chipsz)         ((Chipsz) / MX_64KB)
#define MX_ADVANCED_LOCK_NUMBER(Chipsz) (MX_WP64K_NUMBER(Chipsz) + 2 * MX_SINGLE_WP4K_NUMBER)

int MxSoftwareInit(MxChip *Mxic);
int MxScanMode(MxChip *Mxic);
int MxChangeMode(MxChip *Mxic, u32 SetMode, u32 SetAddrMode);
int MxChipReset(MxChip *Mxic);

int MxLock(MxChip *Mxic, u32 Addr, u64 Len);
int MxUnlock(MxChip *Mxic, u32 Addr, u64 Len);
int MxIsLocked(MxChip *Mxic, u32 Addr, u64 Len);
int MxSingleBlockLock(MxChip *Mxic, u32 Addr, u64 Len);
int MxSingleBlockUnlock(MxChip *Mxic, u32 Addr, u64 Len);
int MxSingleBlockIsLocked(MxChip *Mxic, u32 Addr, u64 Len);

int MxQryLockMode(MxChip *Mxic);
int MxAspWpselLock(MxChip *Mxic);
int MxIsSPBLocked(MxChip *Mxic);
int MxAspLock(MxChip *Mxic, u32 Addr, u64 Len);
int MxAspUnlock(MxChip *Mxic, u32 Addr, u64 Len);
int MxAspIsLocked(MxChip *Mxic, u32 Addr, u64 Len);
int MxPwdLockSpb(MxChip *Mxic, u32 Addr, u64 Len);
int MxPwdUnlockSpb(MxChip *Mxic, u32 Addr, u64 Len);
int MxPwdSpbLk(MxChip *Mxic);
int MxPwdAuth(MxChip *Mxic, u8 *Password);
int MxDpbLock(MxChip *Mxic, u32 Addr, u64 Len);
int MxDpbUnlock(MxChip *Mxic, u32 Addr, u64 Len);
int MxDpbIsLocked(MxChip *Mxic, u32 Addr, u64 Len);
#endif /* NOR_OPS_H_ */
