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

/* Includes ------------------------------------------------------------------*/
#include <rwwee1.h>
#include "main.h"
#include "mx_define.h"
#include "cmsis_os.h"

/** @addtogroup STM32L4xx_HAL_Examples
 * @{
 */
extern __IO JOYState_TypeDef JoyState;
extern uint8_t key_enable;
/** @addtogroup BSP
 * @{
 */
extern uint16_t WrData[PAGE_SZ * 5], RdData[PAGE_SZ * 5];
uint8_t test_process = 0;

void NonRWW_LED(uint8_t r, uint8_t w, uint8_t e) {
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(r == 0 ? LCD_COLOR_LIGHTGRAY : LCD_COLOR_GREEN);
    BSP_LCD_FillCircle(155, BSP_LCD_GetYSize() / 2 - 35, 10);
    BSP_LCD_SetTextColor(w == 0 ? LCD_COLOR_LIGHTGRAY : LCD_COLOR_RED);
    BSP_LCD_FillCircle(225, BSP_LCD_GetYSize() / 2 - 35, 10);
    BSP_LCD_SetTextColor(e == 0 ? LCD_COLOR_LIGHTGRAY : LCD_COLOR_ORANGE);
    BSP_LCD_FillCircle(295, BSP_LCD_GetYSize() / 2 - 35, 10);
    BSP_LCD_Refresh();
}

#define INC_DEGREE  3
#define MAX_RANGE  (100/INC_DEGREE)
void NonRWW_ProcessBar(uint8_t inc) {
    char process_string[20];
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_FillRect(25, BSP_LCD_GetYSize() / 2 - 5, BSP_LCD_GetXSize() - 20, 10);
    BSP_LCD_SetTextColor(LCD_COLOR_LIGHTRED);
    BSP_LCD_DrawRect(21, BSP_LCD_GetYSize() / 2 - 9, 288, 18);
    if ((inc % MAX_RANGE) == (MAX_RANGE - 1)) {
        BSP_LCD_FillRect(25, BSP_LCD_GetYSize() / 2 - 5, 280, 10);
        sprintf(process_string, "%d%% ", 100);
    } else {
        BSP_LCD_FillRect(25, BSP_LCD_GetYSize() / 2 - 5, (inc % MAX_RANGE) * (280 / (MAX_RANGE - 1)), 10);
        sprintf(process_string, "%d%% ", (inc % MAX_RANGE) * INC_DEGREE);
    }

    BSP_LCD_DisplayStringAt(315, BSP_LCD_GetYSize() / 2 - 10, (uint8_t*) process_string, LEFT_MODE);
    BSP_LCD_Refresh();

}

void RWW_LED(uint8_t r, uint8_t w, uint8_t e) {
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(r == 0 ? LCD_COLOR_LIGHTGRAY : LCD_COLOR_GREEN);
    BSP_LCD_FillCircle(155, BSP_LCD_GetYSize() / 2 + 45, 10);
    BSP_LCD_SetTextColor(w == 0 ? LCD_COLOR_LIGHTGRAY : LCD_COLOR_RED);
    BSP_LCD_FillCircle(225, BSP_LCD_GetYSize() / 2 + 45, 10);
    BSP_LCD_SetTextColor(e == 0 ? LCD_COLOR_LIGHTGRAY : LCD_COLOR_ORANGE);
    BSP_LCD_FillCircle(295, BSP_LCD_GetYSize() / 2 + 45, 10);
    BSP_LCD_Refresh();
}
SemaphoreHandle_t led_mutex;
void R_LED(uint8_t r) {
    xSemaphoreTake(led_mutex, portMAX_DELAY);
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(r == 0 ? LCD_COLOR_LIGHTGRAY : LCD_COLOR_GREEN);
    BSP_LCD_FillCircle(155, BSP_LCD_GetYSize() / 2 + 45, 10);
    BSP_LCD_Refresh();
    xSemaphoreGive(led_mutex);
}

void W_LED(uint8_t w) {
    xSemaphoreTake(led_mutex, portMAX_DELAY);
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(w == 0 ? LCD_COLOR_LIGHTGRAY : LCD_COLOR_RED);
    BSP_LCD_FillCircle(225, BSP_LCD_GetYSize() / 2 + 45, 10);
    BSP_LCD_Refresh();
    xSemaphoreGive(led_mutex);
}

void E_LED(uint8_t e) {
    xSemaphoreTake(led_mutex, portMAX_DELAY);
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(e == 0 ? LCD_COLOR_LIGHTGRAY : LCD_COLOR_ORANGE);
    BSP_LCD_FillCircle(295, BSP_LCD_GetYSize() / 2 + 45, 10);
    BSP_LCD_Refresh();
    xSemaphoreGive(led_mutex);
}

void R_LED2(uint8_t r) {
    xSemaphoreTake(led_mutex, portMAX_DELAY);
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(r == 0 ? LCD_COLOR_LIGHTGRAY : LCD_COLOR_GREEN);
    BSP_LCD_FillCircle(155, BSP_LCD_GetYSize() / 2 - 35, 10);
    BSP_LCD_Refresh();
    xSemaphoreGive(led_mutex);
}

void W_LED2(uint8_t w) {
    xSemaphoreTake(led_mutex, portMAX_DELAY);
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(w == 0 ? LCD_COLOR_LIGHTGRAY : LCD_COLOR_RED);
    BSP_LCD_FillCircle(225, BSP_LCD_GetYSize() / 2 - 35, 10);
    BSP_LCD_Refresh();
    xSemaphoreGive(led_mutex);
}

void E_LED2(uint8_t e) {
    xSemaphoreTake(led_mutex, portMAX_DELAY);
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(e == 0 ? LCD_COLOR_LIGHTGRAY : LCD_COLOR_ORANGE);
    BSP_LCD_FillCircle(295, BSP_LCD_GetYSize() / 2 - 35, 10);
    BSP_LCD_Refresh();
    xSemaphoreGive(led_mutex);
}

void RWW_ProcessBar(uint8_t inc) {
    char process_string[20];
    xSemaphoreTake(led_mutex, portMAX_DELAY);
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_FillRect(25, BSP_LCD_GetYSize() / 2 + 70, BSP_LCD_GetXSize() - 20, 10);
    BSP_LCD_SetTextColor(LCD_COLOR_LIGHTRED);
    BSP_LCD_DrawRect(21, BSP_LCD_GetYSize() / 2 + 66, 288, 18);
    if ((inc % MAX_RANGE) == (MAX_RANGE - 1)) {
        BSP_LCD_FillRect(25, BSP_LCD_GetYSize() / 2 + 70, 280, 10);
        sprintf(process_string, "%d%% ", 100);
    } else {
        BSP_LCD_FillRect(25, BSP_LCD_GetYSize() / 2 + 70,
            (inc % MAX_RANGE) * (280 / (MAX_RANGE - 1)), 10);
        sprintf(process_string, "%d%% ", (inc % MAX_RANGE) * INC_DEGREE);
    }

    BSP_LCD_DisplayStringAt(315, BSP_LCD_GetYSize() / 2 + 70, (uint8_t*) process_string, LEFT_MODE);
    BSP_LCD_Refresh();
    xSemaphoreGive(led_mutex);

}

void RWW_ProcessBar2(uint8_t inc) {
    char process_string[20];
    xSemaphoreTake(led_mutex, portMAX_DELAY);
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_FillRect(25, BSP_LCD_GetYSize() / 2 - 5, BSP_LCD_GetXSize() - 20, 10);
    BSP_LCD_SetTextColor(LCD_COLOR_LIGHTRED);
    BSP_LCD_DrawRect(21, BSP_LCD_GetYSize() / 2 - 9, 288, 18);
    if ((inc % 17) == (17 - 1)) {
        BSP_LCD_FillRect(25, BSP_LCD_GetYSize() / 2 - 5, 280, 10);
        sprintf(process_string, "%d%% ", 100);
    } else {
        BSP_LCD_FillRect(25, BSP_LCD_GetYSize() / 2 - 5, (inc % 17) * (280 / (17 - 1)), 10);
        sprintf(process_string, "%d%% ", (inc % 17) * 6);
    }

    BSP_LCD_DisplayStringAt(315, BSP_LCD_GetYSize() / 2 - 10, (uint8_t*) process_string, LEFT_MODE);
    BSP_LCD_Refresh();
    xSemaphoreGive(led_mutex);

}

void EEPROM_ProcessBar(uint8_t inc) {
    char process_string[20];
    xSemaphoreTake(led_mutex, portMAX_DELAY);
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_FillRect(25, BSP_LCD_GetYSize() / 2 + 70, BSP_LCD_GetXSize() - 20, 10);
    BSP_LCD_SetTextColor(LCD_COLOR_LIGHTRED);
    BSP_LCD_DrawRect(21, BSP_LCD_GetYSize() / 2 + 66, 288, 18);
    if ((inc % 17) == (17 - 1)) {
        BSP_LCD_FillRect(25, BSP_LCD_GetYSize() / 2 + 70, 280, 10);
        sprintf(process_string, "%d%% ", 100);
    } else {
        BSP_LCD_FillRect(25, BSP_LCD_GetYSize() / 2 + 70, (inc % 17) * (280 / (17 - 1)), 10);
        sprintf(process_string, "%d%% ", (inc % 17) * 6);
    }

    BSP_LCD_DisplayStringAt(315, BSP_LCD_GetYSize() / 2 + 70, (uint8_t*) process_string, LEFT_MODE);
    BSP_LCD_Refresh();
    xSemaphoreGive(led_mutex);

}

/* Private typedef -----------------------------------------------------------*/
static void rww_perf_demo_display(void) {
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    /* Clear the LCD */
    BSP_LCD_Clear(LCD_COLOR_WHITE);

    /* Set Audio Demo description */
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_FillRect(0, 0, BSP_LCD_GetXSize(), 100);
    BSP_LCD_FillRect(0, BSP_LCD_GetYSize() - 70, BSP_LCD_GetXSize(), 70);

    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
    BSP_LCD_SetFont(&Font24);
    BSP_LCD_DisplayStringAt(5, 35, (uint8_t*) "NonRWW Driver", CENTER_MODE);
    BSP_LCD_DisplayStringAt(10, 70, (uint8_t*) "VS RWW Driver", CENTER_MODE);
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 60, (uint8_t*) "Press any key to next", CENTER_MODE);
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 40, (uint8_t*) "MXIC 2019.08", CENTER_MODE);

    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetFont(&Font20);
    BSP_LCD_DisplayStringAt(10, BSP_LCD_GetYSize() / 2 - 40, (uint8_t*) "NonRWW", LEFT_MODE);
    BSP_LCD_DisplayStringAt(10, BSP_LCD_GetYSize() / 2 + 40, (uint8_t*) "RwW/RwE", LEFT_MODE);

    BSP_LCD_DisplayStringAt(130, BSP_LCD_GetYSize() / 2 - 40, (uint8_t*) "R", LEFT_MODE);
    BSP_LCD_DisplayStringAt(200, BSP_LCD_GetYSize() / 2 - 40, (uint8_t*) "W", LEFT_MODE);
    BSP_LCD_DisplayStringAt(270, BSP_LCD_GetYSize() / 2 - 40, (uint8_t*) "E", LEFT_MODE);

    BSP_LCD_DisplayStringAt(130, BSP_LCD_GetYSize() / 2 + 40, (uint8_t*) "R", LEFT_MODE);
    BSP_LCD_DisplayStringAt(200, BSP_LCD_GetYSize() / 2 + 40, (uint8_t*) "W", LEFT_MODE);
    BSP_LCD_DisplayStringAt(270, BSP_LCD_GetYSize() / 2 + 40, (uint8_t*) "E", LEFT_MODE);
    BSP_LCD_Refresh();

    NonRWW_LED(0, 0, 0);
    RWW_LED(0, 0, 0);

}
#if 1
void nonRWW_testflow(void) {
    uint32_t addr = 0;
    uint32_t start_t, end_t;
    char timing[20];
    test_process = 0;
    NonRWW_ProcessBar(test_process++);

    start_t = xTaskGetTickCount();

    for (uint8_t i = 0; i < 4; i++) {

        for (uint8_t j = 0; j < 2; j++) {
            NonRWW_LED(0, 0, 1);
            mx_ee_rww_erase(addr + 64 * 1024 * j, 64 * 1024);
            NonRWW_ProcessBar(test_process++);

            NonRWW_LED(1, 0, 0);
            for (uint32_t i = 0; i < 256 * 1024; i += 256) {
                mx_ee_rww_read(addr + 256 * 1024 * j + i, 256, RdData);
            }
            NonRWW_ProcessBar(test_process++);

            NonRWW_LED(0, 1, 0);
            for (uint32_t i = 0; i < 64 * 1024; i += 256) {
                mx_ee_rww_write(addr + 64 * 1024 * j + i, 256, WrData);
            }
            NonRWW_ProcessBar(test_process++);

            NonRWW_LED(1, 0, 0);
            for (uint32_t i = 0; i < 256 * 1024; i += 256) {
                mx_ee_rww_read(addr + 256 * 1024 * j + i, 256, RdData);
            }

            NonRWW_ProcessBar(test_process++);
        }

        addr += 0x1000000;
        if (addr == 0x3000000)
            addr = 0;
    }

    end_t = xTaskGetTickCount();

    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_FillRect(0, BSP_LCD_GetYSize() / 2 - 50, BSP_LCD_GetXSize(), 60);

    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetFont(&Font24);
    BSP_LCD_DisplayStringAt(20, BSP_LCD_GetYSize() / 2 - 40, (uint8_t*) "NonRWW", LEFT_MODE);
    BSP_LCD_SetTextColor(LCD_COLOR_LIGHTRED);

    BSP_LCD_SetFont(&Font32);
    sprintf(timing, "%d.%d", (end_t - start_t) / 100, ((end_t - start_t) - 100 * ((end_t - start_t) / 100)) / 10);
    BSP_LCD_Display32StringAt(200, BSP_LCD_GetYSize() / 2 - 75, (uint8_t*) timing, LEFT_MODE);

    BSP_LCD_SetFont(&Font24);
    BSP_LCD_DisplayStringAt(300, BSP_LCD_GetYSize() / 2 - 40, "S", LEFT_MODE);
    BSP_LCD_Refresh();

}
#endif
TaskHandle_t read_handle;
uint8_t read_complete = 0;
static void Read_Thread(void const *argument) {
    uint8_t time = 16;
    read_complete = 0;
    while (time--) {
        R_LED(1);
        for (uint32_t j = 0; j < 256 * 1024; j += 256) {
            mx_ee_rww_read(0x3000000 + j, 256, RdData);
        }
        R_LED(0);
        RWW_ProcessBar(test_process++);
    }
    read_complete = 1;
    printf("about to delete read_thread\r\n");
    vTaskDelete(read_handle);
}

void rww_testflow(void) {
    uint32_t addr = 0;
    uint32_t start_t, end_t;
    char timing[20];
    test_process = 0;

    RWW_ProcessBar(test_process++);

    start_t = xTaskGetTickCount();

    xTaskCreate(Read_Thread, "Read_Thread", 1024, NULL, osPriorityNormal, &read_handle);
    printf("read_handle: %x\r\n");
#if 1
    for (uint32_t i = 0; i < 2; i++) {
        E_LED(1);
        mx_ee_rww_erase(i * 64 * 1024, 64 * 1024);
        E_LED(0);
        RWW_ProcessBar(test_process++);
#if 1
        W_LED(1);
        for (uint32_t j = 0; j < 64 * 1024; j += 256) {
            mx_ee_rww_write(i * 64 * 1024 + j, 256, WrData);
        }
        W_LED(0);
        RWW_ProcessBar(test_process++);
#endif
        E_LED(1);
        mx_ee_rww_erase(0x1000000 + i * 64 * 1024, 64 * 1024);
        E_LED(0);
        RWW_ProcessBar(test_process++);
#if 1
        W_LED(1);
        for (uint32_t j = 0; j < 64 * 1024; j += 256) {
            mx_ee_rww_write(0x1000000 + i * 64 * 1024 + j, 256, WrData);
        }
        W_LED(0);
        RWW_ProcessBar(test_process++);
#endif
#if 1
        E_LED(1);
        mx_ee_rww_erase(0x2000000 + i * 64 * 1024, 64 * 1024);
        E_LED(0);
        RWW_ProcessBar(test_process++);
#if 1
        W_LED(1);
        for (uint32_t j = 0; j < 64 * 1024; j += 256) {
            mx_ee_rww_write(0x2000000 + i * 64 * 1024 + j, 256, WrData);
        }
        W_LED(0);
        RWW_ProcessBar(test_process++);
#endif
        E_LED(1);
        mx_ee_rww_erase(i * 64 * 1024, 64 * 1024);
        E_LED(0);
        RWW_ProcessBar(test_process++);
#if 1
        W_LED(1);
        for (uint32_t j = 0; j < 64 * 1024; j += 256) {
            mx_ee_rww_write(i * 64 * 1024 + j, 256, WrData);
        }
        W_LED(0);
        RWW_ProcessBar(test_process++);
#endif
#endif
    }
#endif

    while (!read_complete) {
        taskYIELD();
    }

    end_t = xTaskGetTickCount();

    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_FillRect(0, BSP_LCD_GetYSize() / 2 + 30, BSP_LCD_GetXSize(), 60);

    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetFont(&Font24);
    BSP_LCD_DisplayStringAt(20, BSP_LCD_GetYSize() / 2 + 40, (uint8_t*) "RwW/RwE", LEFT_MODE);

    BSP_LCD_SetTextColor(LCD_COLOR_LIGHTRED);
    BSP_LCD_SetFont(&Font32);
    sprintf(timing, "%d.%d", (end_t - start_t) / 100, ((end_t - start_t) - 100 * ((end_t - start_t) / 100)) / 10);
    BSP_LCD_Display32StringAt(200, BSP_LCD_GetYSize() / 2, (uint8_t*) timing, LEFT_MODE);
    BSP_LCD_SetFont(&Font24);
    BSP_LCD_DisplayStringAt(300, BSP_LCD_GetYSize() / 2 + 40, (uint8_t*) "S", LEFT_MODE);

    BSP_LCD_Refresh();
}
/* Private define ------------------------------------------------------------*/

void rww_perf_demo(void) {
    led_mutex = xSemaphoreCreateMutex();
    rww_perf_demo_display();
    nonRWW_testflow();
    rww_testflow();
    if (MfxItOccurred == SET) {
        Mfx_Event();
    }

    JoyState = JOY_NONE;
    key_enable = TRUE;
    while (JoyState == JOY_NONE) {
        if (MfxItOccurred == SET) {
            Mfx_Event();
        }
    }
    key_enable = 0;
    vSemaphoreDelete(led_mutex);
    /* Reset JoyState */
    JoyState = JOY_NONE;
}

void EEPROM_LED(uint8_t r, uint8_t w) {
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(r == 0 ? LCD_COLOR_LIGHTGRAY : LCD_COLOR_GREEN);
    BSP_LCD_FillCircle(155, BSP_LCD_GetYSize() / 2 + 45, 10);
    BSP_LCD_SetTextColor(w == 0 ? LCD_COLOR_LIGHTGRAY : LCD_COLOR_RED);
    BSP_LCD_FillCircle(225, BSP_LCD_GetYSize() / 2 + 45, 10);
    BSP_LCD_Refresh();
}

/* Private typedef -----------------------------------------------------------*/
static void eeprom_perf_demo_display(void) {
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    /* Clear the LCD */
    BSP_LCD_Clear(LCD_COLOR_WHITE);

    /* Set Audio Demo description */
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_FillRect(0, 0, BSP_LCD_GetXSize(), 100);
    BSP_LCD_FillRect(0, BSP_LCD_GetYSize() - 70, BSP_LCD_GetXSize(), 70);

    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
    BSP_LCD_SetFont(&Font24);
    BSP_LCD_DisplayStringAt(5, 35, (uint8_t*) "RWW Driver", CENTER_MODE);
    BSP_LCD_DisplayStringAt(10, 70, (uint8_t*) "VS EEPROM Driver", CENTER_MODE);
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 60, (uint8_t*) "Press any key to next", CENTER_MODE);
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 35, (uint8_t*) "MXIC 2019.08", CENTER_MODE);

    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetFont(&Font20);
    BSP_LCD_DisplayStringAt(10, BSP_LCD_GetYSize() / 2 - 40, (uint8_t*) "RWW", LEFT_MODE);
    BSP_LCD_DisplayStringAt(10, BSP_LCD_GetYSize() / 2 + 40, (uint8_t*) "EEPROM", LEFT_MODE);

    BSP_LCD_DisplayStringAt(130, BSP_LCD_GetYSize() / 2 - 40, (uint8_t*) "R", LEFT_MODE);
    BSP_LCD_DisplayStringAt(200, BSP_LCD_GetYSize() / 2 - 40, (uint8_t*) "W", LEFT_MODE);
    BSP_LCD_DisplayStringAt(270, BSP_LCD_GetYSize() / 2 - 40, (uint8_t*) "E", LEFT_MODE);

    BSP_LCD_DisplayStringAt(130, BSP_LCD_GetYSize() / 2 + 40, (uint8_t*) "R", LEFT_MODE);
    BSP_LCD_DisplayStringAt(200, BSP_LCD_GetYSize() / 2 + 40, (uint8_t*) "W", LEFT_MODE);
    BSP_LCD_Refresh();

    NonRWW_LED(0, 0, 0);
    EEPROM_LED(0, 0);

}

TaskHandle_t read_handle2;

static void Read_Thread2(void const *argument) {
    {
        R_LED2(1);
        for (uint32_t j = 0; j < 4096; j += 256) {
            mx_ee_rww_read(0x1000000 + j, 256, RdData);
        }
        R_LED2(0);
        RWW_ProcessBar2(test_process++);
    }
    read_complete++;
    vTaskDelete(read_handle);
}

void rww_testflow2(void) {
    uint32_t addr = 0;
    uint32_t start_t, end_t;
    char timing[20];
    test_process = 0;
    RWW_ProcessBar2(test_process++);
    read_complete = 0;
    start_t = xTaskGetTickCount();

    for (uint8_t i = 0; i < 8; i++) {
        xTaskCreate(Read_Thread2, "Read_Thread2", 1024, NULL, osPriorityNormal, &read_handle2);
        E_LED2(1);
        mx_ee_rww_erase(addr, 4096);
        E_LED2(0);
        W_LED2(1);
        for (uint32_t j = 0; j < 512; j += 256) {
            mx_ee_rww_write(addr + j, 256, WrData);
        }
        W_LED2(0);
        RWW_ProcessBar2(test_process++);
    }
    while (read_complete < 8) {
        taskYIELD();
    }
    end_t = xTaskGetTickCount();
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);

    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_FillRect(0, BSP_LCD_GetYSize() / 2 - 50, BSP_LCD_GetXSize(), 60);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetFont(&Font24);

    BSP_LCD_DisplayStringAt(20, BSP_LCD_GetYSize() / 2 - 40, (uint8_t*) "RWW", LEFT_MODE);
    BSP_LCD_SetTextColor(LCD_COLOR_LIGHTRED);

    BSP_LCD_SetFont(&Font32);
    sprintf(timing, "%d", (end_t - start_t) * 10); //, ((end_t-start_t)-100*((end_t-start_t)/100))/10);
    BSP_LCD_Display32StringAt(200, BSP_LCD_GetYSize() / 2 - 75, (uint8_t*) timing, LEFT_MODE);

    BSP_LCD_SetFont(&Font24);
    BSP_LCD_DisplayStringAt(335, BSP_LCD_GetYSize() / 2 - 40, "ms", LEFT_MODE);
    BSP_LCD_Refresh();
}

TaskHandle_t read_handle3;

static void Read_Thread3(void const *argument) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
    for (uint32_t i = 0; i < 8; i++) {
        R_LED(1);
        for (uint32_t j = 0; j < 8; j++) {
            mx_eeprom_read(0x80000200 - 4, 0x200 - 4, RdData);
        }
        R_LED(0);
        EEPROM_ProcessBar(test_process++);
    }
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    read_complete++;
    vTaskDelete(read_handle);
}

void eeprom_testflow(void) {
    uint32_t addr = 0;
    uint32_t start_t, end_t;
    char timing[20];
    test_process = 0;
    read_complete = 0;
    EEPROM_ProcessBar(test_process++);

    start_t = xTaskGetTickCount();
    xTaskCreate(Read_Thread3, "Read_Thread3", 1024, NULL, osPriorityNormal, &read_handle3);
    printf("read_handle: %x\r\n");

    for (uint8_t i = 0; i < 8; i++) {
        W_LED(1);
        mx_eeprom_sync_write(0x80000000, 512 - 4, WrData);
        W_LED(0);
        EEPROM_ProcessBar(test_process++);
    }
    while (!read_complete) {
        taskYIELD();
    }

    end_t = xTaskGetTickCount();
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_FillRect(0, BSP_LCD_GetYSize() / 2 + 30, BSP_LCD_GetXSize(), 60);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetFont(&Font24);

    BSP_LCD_DisplayStringAt(20, BSP_LCD_GetYSize() / 2 + 40, (uint8_t*) "EEPROM", LEFT_MODE);
    BSP_LCD_SetTextColor(LCD_COLOR_LIGHTRED);
    BSP_LCD_SetFont(&Font32);
    sprintf(timing, "%d", (end_t - start_t) * 10); //, ((end_t-start_t)-100*((end_t-start_t)/100))/10);
    BSP_LCD_Display32StringAt(200, BSP_LCD_GetYSize() / 2, (uint8_t*) timing, LEFT_MODE);

    BSP_LCD_SetFont(&Font24);
    BSP_LCD_DisplayStringAt(300, BSP_LCD_GetYSize() / 2 + 40, "ms", LEFT_MODE);

    BSP_LCD_Refresh();
}

void eeprom_perf_demo(void) {
    led_mutex = xSemaphoreCreateMutex();
    eeprom_perf_demo_display();
    rww_testflow2();
    eeprom_testflow();

    if (MfxItOccurred == SET) {
        Mfx_Event();
    }
    JoyState = JOY_NONE;
    key_enable = TRUE;
    while (JoyState == JOY_NONE) {
        if (MfxItOccurred == SET) {
            Mfx_Event();
        }
    }
    key_enable = FALSE;
    /* Reset JoyState */
    JoyState = JOY_NONE;
    vSemaphoreDelete(led_mutex);
}

/**
 * @}
 */

/**
 * @}
 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
