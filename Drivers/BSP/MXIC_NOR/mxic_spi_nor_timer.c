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

/* Includes -----------------------------------------------------------------------*/
#include "mxic_spi_nor_timer.h"

/* Exported Variables -------------------------------------------------------------*/
TIM_HandleTypeDef MXICSPINOR_TimHandle;

/*
 * Timer Initialization for MXIC SPI NOR BSP Timer Function Implement
 */
HAL_StatusTypeDef MXICSPINORTimer_Init(void) {
    MXIC_SPI_NOR_TIM_CLK_ENABLE();

    MXICSPINOR_TimHandle.Instance = MXIC_SPI_NOR_TIM;
    MXICSPINOR_TimHandle.Init.Period = 0xffffffff - 1;
    MXICSPINOR_TimHandle.Init.Prescaler = (uint32_t)(SystemCoreClock / 1000000 - 1); //1us count
    MXICSPINOR_TimHandle.Init.ClockDivision = 0;
    MXICSPINOR_TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;

    if (HAL_TIM_Base_Init(&MXICSPINOR_TimHandle) != HAL_OK) {
        return HAL_ERROR;
    }

    if (HAL_TIM_Base_Start(&MXICSPINOR_TimHandle) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/*
 * Timer Deinitialization for MXIC SPI NOR BSP Timer Function Implement
 */
HAL_StatusTypeDef MXICSPINORTimer_Deinit(void) {
    if (HAL_TIM_Base_Stop(&MXICSPINOR_TimHandle) != HAL_OK) {
        return HAL_ERROR;
    }

    if (HAL_TIM_Base_DeInit(&MXICSPINOR_TimHandle) != HAL_OK) {
        return HAL_ERROR;
    }

    MXIC_SPI_NOR_TIM_CLK_DISABLE();

    return HAL_OK;
}

/*
 * Delay Us for MXIC SPI NOR BSP Timer Function Implement
 */
void MXIC_SPI_NOR_HAL_Delay_Us(uint32_t Delay) {
    WRITE_REG(MXIC_SPI_NOR_TIM->CNT, 0);

    while ((MXIC_SPI_NOR_TIM->CNT) < Delay) {
    }
}

