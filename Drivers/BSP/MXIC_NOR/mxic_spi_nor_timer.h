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


#ifndef MXIC_SPI_NOR_TIMER_H_
#define MXIC_SPI_NOR_TIMER_H_

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Macro Definition-------------------------------------------------------------*/
#define MXIC_SPI_NOR_TIM  TIM4

#define MXIC_SPI_NOR_TIM_CLK_ENABLE()    __TIM4_CLK_ENABLE()
#define MXIC_SPI_NOR_TIM_CLK_DISABLE()   __TIM4_CLK_DISABLE()

/* Exported Variables -------------------------------------------------------------*/
extern TIM_HandleTypeDef MXICSPINOR_TimHandle;

/* Exported Functions-------------------------------------------------------------*/
HAL_StatusTypeDef MXICSPINORTimer_Init(void);
void MXIC_SPI_NOR_HAL_Delay_Us(uint32_t Delay);
HAL_StatusTypeDef MXICSPINORTimer_Deinit(void);

#endif /* MXIC_SPI_NOR_TIMER_H_ */
