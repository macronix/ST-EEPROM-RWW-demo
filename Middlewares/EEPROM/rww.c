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

#include <rwwee1.h>
#include "mx_define.h"
#include "nor_cmd.h"
#include "app.h"
#include "cmsis_os.h"
/* fred: For testing */
extern uint32_t waitStick;

MxChip Mxic;

SemaphoreHandle_t xBinarySemaphore;

/* RWW structure */
static struct rww_info mx_rww = { .initialized = false, };

#ifdef MX_EEPROM_ECC_CHECK
/**
    * @brief    Check NOR flash on-die ECC status.
    * @retval Status
    */
static int mx_ee_check_ecc(void)
{
    uint32_t status[2];

    /* Read ECC status */
    if (BSP_OSPI_NOR_Check_ECC(status))
    {
        mx_err("mxee_ckecc: fail to read ecc status\r\n");
        mx_err("mxee_ckecc: QSPI HC state 0x%08lx, error 0x%08lx\r\n",
                        status[0], status[1]);

        return MX_EIO;
    }

    /* No ECC failure */
    if (status[0] == 0 || status[0] == MX25LM51245G_CR2_ECCFS_ONE)
        return MX_OK;

    /* Detected double program */
    if (status[0] & MX25LM51245G_CR2_ECCFS_ERR)
        mx_err("mxee_ckecc: detected double program\r\n");

    /* Detected 2 bit-flips */
    if (status[0] & MX25LM51245G_CR2_ECCFS_TWO)
        mx_err("mxee_ckecc: detected 2 bit-flips\r\n");

    /* Corrected 1 bit-flip */
    if (status[0] & MX25LM51245G_CR2_ECCFS_ONE)
        mx_err("mxee_ckecc: corrected 1 bit-flip\r\n");

    mx_err("mxee_ckecc: ECC failure cnt %d\r\n",
                    status[0] & MX25LM51245G_CR2_ECCCNT_MASK);

    return MX_EECC;
}
#endif

/* Private functions ---------------------------------------------------------*/
#ifdef RWW_DRIVER_SUPPORT
SemaphoreHandle_t xBinarySempahore;

#define TIMx                                       TIM2
#define TIMx_CLK_ENABLE()                          __HAL_RCC_TIM2_CLK_ENABLE()
#define TIMx_CLK_DISABLE()                         __HAL_RCC_TIM2_CLK_DISABLE()
#define TIMx_IRQn                                  TIM2_IRQn
TIM_HandleTypeDef TimHandle_Busy;
uint32_t ticks = 0;
uint32_t delay_timing = 0;
HAL_StatusTypeDef STM32_Timer_Init() {
    TIMx_CLK_ENABLE();

    TimHandle_Busy.Instance = TIMx;
    TimHandle_Busy.Init.Period = 100 - 1; //100us
    TimHandle_Busy.Init.Prescaler = (uint32_t)(SystemCoreClock / 1000000 - 1);
    TimHandle_Busy.Init.ClockDivision = 0;
    TimHandle_Busy.Init.CounterMode = TIM_COUNTERMODE_UP;

    if (HAL_TIM_Base_Init(&TimHandle_Busy) != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_NVIC_SetPriority(TIMx_IRQn, 0x0c, 0);
    HAL_NVIC_EnableIRQ(TIMx_IRQn);
    return HAL_OK;
}

HAL_StatusTypeDef STM32_Timer_Config(uint32_t delay) {
    delay_timing = delay;
    ticks = 0;

    if (HAL_TIM_Base_Start_IT(&TimHandle_Busy) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef STM32_Timer_DeInit(void) {

    if (HAL_TIM_Base_Stop_IT(&TimHandle_Busy) != HAL_OK) {
        return HAL_ERROR;
    }

    __HAL_TIM_SET_COUNTER(&TimHandle_Busy, 0);

    return HAL_OK;
}

void Handler_Thread(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (ticks >= delay_timing) {
        STM32_Timer_DeInit();
        xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);
        ticks = delay_timing = 0;
        busy_bank &= 0x80;
        portYIELD_FROM_ISR(pdTRUE);
        return;
    }

    ticks++;
    __HAL_TIM_CLEAR_FLAG(&TimHandle_Busy, TIM_FLAG_UPDATE);
}
#endif
/**
 * @brief    Read NOR flash.
 * @param    addr: Read start address
 * @param    len: Read length
 * @param    buf: Data buffer
 * @retval Status
 */
static int readcnt1 = 0, errcnt1 = 0;

int mx_ee_rww_read(uint32_t addr, uint32_t len, void *buf) {
    int ret;
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    uint8_t *data = (uint8_t*) buf;

    ret = MxRead(&Mxic, addr, len, buf);

    readcnt1++;
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    return (!ret ? MX_OK : MX_EIO);
}

/**
 * @brief    Write NOR flash.
 * @param    addr: Write start address
 * @param    len: Write length
 * @param    buf: Data buffer
 * @retval Status
 */
int mx_ee_rww_write(uint32_t addr, uint32_t len, void *buf) {
    int ret;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

    ret = MxWrite(&Mxic, addr, len, buf);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    return (!ret ? MX_OK : MX_EIO);
}

/**
 * @brief    Erase NOR flash.
 * @param    addr: Erase start address
 * @param    len: Erase length
 * @retval Status
 */
int mx_ee_rww_erase(uint32_t addr, uint32_t len) {
    int ret;

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
    ret = MxErase(&Mxic, addr, len / MX_FLASH_SECTOR_SIZE);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
    return (!ret ? MX_OK : MX_EIO);
}

/**
 * @brief    Initialize RWW layer.
 * @retval Status
 */
int mx_ee_rww_init(void) {
    int ret = 0;
#ifdef RWW_DRIVER_SUPPORT
    xBinarySemaphore = xSemaphoreCreateBinary();
    xReadMutex = xSemaphoreCreateMutex();
    xWriteMutex = xSemaphoreCreateMutex();
    xBusMutex = xSemaphoreCreateMutex();
    xCommandMutex = xSemaphoreCreateMutex();
    xBufferMutex = xSemaphoreCreateMutex();
#endif
    ret = MxInit(&Mxic);
    return ret;
}

/**
 * @brief    Deinit RWW layer.
 */
void mx_ee_rww_deinit(void) {
#ifdef RWW_DRIVER_SUPPORT
    vSemaphoreDelete(xBinarySemaphore);
    vSemaphoreDelete(xReadMutex);
    vSemaphoreDelete(xWriteMutex);
    vSemaphoreDelete(xBusMutex);
    vSemaphoreDelete(xCommandMutex);
    vSemaphoreDelete(xBufferMutex);
#endif
}
