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

#ifndef APP_H_
#define APP_H_

#include "nor_ops.h"

int MxInit(MxChip *Mxic);

int MxSetMode(MxChip *Mxic, u32 SetMode,u32 SetAddrMode);
void MxEnterLinearMode(MxChip *Mxic);
void MxEnterIOMode(MxChip *Mxic);
void MxEnterSDmaMode(MxChip *Mxic);

int MxSetMX25RPowerMode(MxChip *Mxic, u8 SetMode);
int MxRead(MxChip *Mxic, u32 Addr, u32 SectCnt, u8 *Buf);
int MxWrite(MxChip *Mxic, u32 Addr, u32 SectCnt, u8 *Buf);
int MxErase(MxChip *Mxic, u32 Addr, u32 SecCnt);
int MxGetCurLockMode(MxChip *Mxic);
int MxSetLockMode(MxChip *Mxic, int LockMode);
int MxLockFlash(MxChip *Mxic, u32 Addr, u64 Len);
int MxUnlockFlash(MxChip *Mxic, u32 Addr, u64 Len);
int MxIsFlashLocked(MxChip *Mxic, u32 Addr, u64 Len);
int MxDPBLockFlash(MxChip *Mxic, u32 Addr, u64 Len);
int MxDPBLockFlash(MxChip *Mxic, u32 Addr, u64 Len);
int MxDPBUnlockFlash(MxChip *Mxic, u32 Addr, u64 Len);
int MxIsFlashDPBLocked(MxChip *Mxic, u32 Addr, u64 Len);
int MxChange2AspMode(MxChip *Mxic);
int MxChange2PwdMode(MxChip *Mxic);
int MxEnterOTP(MxChip *Mxic);
int MxExitOTP(MxChip *Mxic);
int MxDeepPowerDown(MxChip *Mxic);
int MxRealseDeepPowerDown(MxChip *Mxic);
int MxSuspend(MxChip *Mxic);
int MxResume(MxChip *Mxic);

int MxCalibration(MxChip *Mxic);

#endif /* APP_H_ */
