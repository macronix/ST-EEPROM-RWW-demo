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
/** @addtogroup STM32L4xx_HAL_Examples
 * @{
 */

extern __IO JOYState_TypeDef JoyState;

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define REC_SAMPLE_RATE   AUDIO_FREQUENCY_22K
#define PLAY_SAMPLE_RATE  AUDIO_FREQUENCY_22K
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

#define WAVE_FILE_SIZE (50*1024*1024)

#define BUFF_SIZE    (4096-4)
#define REC_START_ADDR    0
#define DELAY_BANK_CNT    63
int16_t RecordBuff[BUFF_SIZE];
int16_t PlaybackBuff[BUFF_SIZE];
uint32_t RecHalfBuffCplt = 0;
uint32_t RecBuffCplt = 0;
uint32_t PlayHalfBuffCplt = 0;
uint32_t PlayBuffCplt = 0;
uint32_t PlaybackStarted = 0;
uint16_t CurrentInputDevice = INPUT_DEVICE_DIGITAL_MIC;

/* Private function prototypes -----------------------------------------------*/
static void AudioRecord_SetHint(void);
static void Record_Init(void);
static void Playback_Init(void);
void Audio_Record_RxHalfCpltCallback(void);
void Audio_Record_RxCpltCallback(void);
void Audio_Record_ErrorCallback(void);
void Audio_Playback_TxHalfCpltCallback(void);
void Audio_Playback_TxCpltCallback(void);
void Audio_Playback_ErrorCallback(void);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Test BSP AUDIO for record.
 * @param  None
 * @retval None
 */
extern __IO uint8_t Volume;
extern __IO uint8_t VolumeChange;
extern uint32_t AudioPlayback;
extern uint8_t key_enable;

uint32_t rec_addr = REC_START_ADDR;
uint32_t play_addr = REC_START_ADDR;
uint32_t play_len = BUFF_SIZE;
uint32_t wave_size = 0;

uint8_t skip_len = 0;
TaskHandle_t playback_handle;

SemaphoreHandle_t xPlaybackSemaphore;

static void Playback_Thread(void const *argument) {
#if 1
    play_len = skip_len * BUFF_SIZE;
    if (((rec_addr / BUFF_SIZE) % 4) == ((play_addr / BUFF_SIZE) % 4)) {
        mx_eeprom_read(play_addr + BUFF_SIZE, BUFF_SIZE, (uint8_t*) PlaybackBuff);
    } else {
        mx_eeprom_read(play_addr, BUFF_SIZE, (uint8_t*) PlaybackBuff);
    }
    HAL_Delay(10);
    if (AUDIO_OK != BSP_AUDIO_OUT_Play((uint16_t*) PlaybackBuff, BUFF_SIZE)) {
        Error_Handler();
    }
    play_addr += play_len;

    while (1) {
        if (PlayHalfBuffCplt == 1) {
            PlayHalfBuffCplt = 0;
            if (((rec_addr / BUFF_SIZE) % 4) == ((play_addr / BUFF_SIZE) % 4)) {
                mx_eeprom_read(play_addr + BUFF_SIZE, BUFF_SIZE, (uint8_t*) PlaybackBuff);
            } else {
                mx_eeprom_read(play_addr, BUFF_SIZE, (uint8_t*) PlaybackBuff);
            }
            play_addr += play_len;
            if (play_addr >= wave_size) //(4092*30))
                    {
                play_addr = REC_START_ADDR;
            }
        }

        if (PlayBuffCplt == 1) {
            PlayBuffCplt = 0;
            if (((rec_addr / BUFF_SIZE) % 4) == ((play_addr / BUFF_SIZE) % 4)) {
                mx_eeprom_read(play_addr + BUFF_SIZE, BUFF_SIZE, (uint8_t*) PlaybackBuff + BUFF_SIZE);
            } else {
                mx_eeprom_read(play_addr, BUFF_SIZE, (uint8_t*) PlaybackBuff + BUFF_SIZE);
            }

            play_addr += play_len;
            if (play_addr >= wave_size) //(4092*30))
            {
                play_addr = REC_START_ADDR;
            }
        }
    }
#endif

}

void AudioRecord_demo(void) {
    uint32_t i;
    WAVE_FormatTypeDef waveformat;
    uint8_t Volume_string[20] = { 0 };

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    /* Enable the GPIO_LED Clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Configure the GPIO_LED pin */
    GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_0 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

    /* Display test informations */
    AudioRecord_SetHint();

    /* Reset JoyState */
    JoyState = JOY_NONE;
    rec_addr = REC_START_ADDR;
    play_addr = REC_START_ADDR;
    play_len = BUFF_SIZE;
    wave_size = 0;
    uint8_t bank_delay = 0;

    /* Display current microphone */
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    if (CurrentInputDevice == INPUT_DEVICE_DIGITAL_MIC) {
        BSP_LCD_DisplayStringAt(0, 150, (uint8_t*) "Current mic is digital", CENTER_MODE);
    } else {
        BSP_LCD_DisplayStringAt(0, 150, (uint8_t*) "Current mic is headset", CENTER_MODE);
    }
    BSP_LCD_Refresh();

    /* Initialize record */
    Record_Init();

    /* Start record */
    if (AUDIO_OK != BSP_AUDIO_IN_Record((uint16_t*) RecordBuff, BUFF_SIZE)) {
        Error_Handler();
    }

    RecHalfBuffCplt = 0;
    RecBuffCplt = 0;
    /* Reset JoyState */
    JoyState = JOY_NONE;
    Playback_Init();

    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    BSP_LCD_DisplayStringAt(0, 200, (uint8_t*) "Recording", CENTER_MODE);
    BSP_LCD_Refresh();
    key_enable = 0;
    JoyState = JOY_NONE;

    while (JoyState == JOY_NONE) {
        if (MfxItOccurred == SET) {
            Mfx_Event();
            if (!key_enable)
                JoyState = JOY_NONE;
        }

        if (RecHalfBuffCplt == 1) {
            /* Store values on Play buff */
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);

            if (bank_delay < DELAY_BANK_CNT)
                bank_delay++;
            if (bank_delay > 12) {
                mx_eeprom_sync_write(rec_addr, BUFF_SIZE,
                        (uint8_t*) RecordBuff);

                rec_addr += BUFF_SIZE;
                if (wave_size < (WAVE_FILE_SIZE - 2 * BUFF_SIZE))
                    wave_size += BUFF_SIZE;

                if (rec_addr >= WAVE_FILE_SIZE)
                    rec_addr = REC_START_ADDR;
            }
            RecHalfBuffCplt = 0;
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
        }
        if (RecBuffCplt == 1) {

            /* Store values on Play buff */
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);

            if (bank_delay < DELAY_BANK_CNT)
                bank_delay++;
            if (bank_delay > 12) {
                mx_eeprom_sync_write(rec_addr, BUFF_SIZE, (uint8_t*) RecordBuff + BUFF_SIZE);

                rec_addr += BUFF_SIZE;
                if (rec_addr >= WAVE_FILE_SIZE)
                    rec_addr = REC_START_ADDR;

                if (wave_size < (WAVE_FILE_SIZE - 2 * BUFF_SIZE))
                    wave_size += BUFF_SIZE;
            }

            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);

            RecBuffCplt = 0;
        }

        if (bank_delay == DELAY_BANK_CNT) {
#if 1
            if (PlaybackStarted == 0) {
                /* Insert a little delay before starting playback to be sure that data are available for playback */
                /* Without this delay, potential noise when optimization high is selected for compiler */
                while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
                BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
                BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
                BSP_LCD_DisplayStringAt(0, 225, (uint8_t*) "While", CENTER_MODE);
                BSP_LCD_SetTextColor(LCD_COLOR_LIGHTRED);
                if (skip_len == 1)
                    BSP_LCD_DisplayStringAt(0, 250, (uint8_t*) "1X Playing Back", CENTER_MODE);
                else if (skip_len == 2)
                    BSP_LCD_DisplayStringAt(0, 250, (uint8_t*) "2X Playing Back", CENTER_MODE);
                BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
                sprintf((char*) Volume_string, " Volume : %d%% ", Volume);
                BSP_LCD_DisplayStringAt(0, 300, Volume_string, CENTER_MODE);
                BSP_LCD_Refresh();

                xTaskCreate(Playback_Thread, "Playback_Thread", 512, NULL, osPriorityNormal, &playback_handle);
                PlaybackStarted = 1;
                key_enable = 1;
            }
#endif

        }

    }
    key_enable = 0;
    BSP_AUDIO_OUT_SetVolume(0);

    /* Stop playback */
    if (BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW) != AUDIO_OK) {
        Error_Handler();
    }
    printf("exit s0\r\n");
    if (PlaybackStarted)
        vTaskDelete(playback_handle);
    printf("play handle: %x\r\n", playback_handle);
    /* Stop record */
    if (BSP_AUDIO_IN_Stop() != AUDIO_OK) {
        Error_Handler();
    }

    /* De-initialize playback */
    if (BSP_AUDIO_OUT_DeInit() != AUDIO_OK) {
        Error_Handler();
    }

    /* De-initialize record */
    if (BSP_AUDIO_IN_DeInit() != AUDIO_OK) {
        Error_Handler();
    }

    /* Restore GPIOH clock used for LED */
    __HAL_RCC_GPIOH_CLK_ENABLE();

    /* Reset global variables */
    RecHalfBuffCplt = 0;
    RecBuffCplt = 0;
    PlaybackStarted = 0;
    printf("exit audio demo\r\n");

    /* Reset JoyState */
    JoyState = JOY_NONE;
}

/**
 * @brief  Display audio record demo hint
 * @param  None
 * @retval None
 */
static void AudioRecord_SetHint(void) {
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    /* Clear the LCD */
    BSP_LCD_Clear(LCD_COLOR_WHITE);

    /* Set Audio Demo description */
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_FillRect(0, 0, BSP_LCD_GetXSize(), 130);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
    BSP_LCD_SetFont(&Font24);
    BSP_LCD_DisplayStringAt(0, 40, (uint8_t*) "Audio Record", CENTER_MODE);
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_DisplayStringAt(0, 100, (uint8_t*) "Press any key to exit record", CENTER_MODE);

    /* Set the LCD Text Color */
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_DrawRect(40, 140, BSP_LCD_GetXSize() - 80, BSP_LCD_GetYSize() - 240);
    BSP_LCD_DrawRect(41, 141, BSP_LCD_GetXSize() - 82, BSP_LCD_GetYSize() - 242);

    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_Refresh();
}

/**
 * @brief  Record initialization
 * @param  None
 * @retval None
 */
static void Record_Init(void) {
    if (CurrentInputDevice == INPUT_DEVICE_DIGITAL_MIC) {
        if (AUDIO_OK != BSP_AUDIO_IN_InitEx(INPUT_DEVICE_DIGITAL_MIC, REC_SAMPLE_RATE, 16, 2)) {
            Error_Handler();
        }
    } else {
        if (AUDIO_OK != BSP_AUDIO_IN_InitEx(INPUT_DEVICE_ANALOG_MIC, REC_SAMPLE_RATE, 16, 1)) {
            Error_Handler();
        }
    }

    BSP_AUDIO_IN_RegisterCallbacks(Audio_Record_ErrorCallback,
        Audio_Record_RxHalfCpltCallback, Audio_Record_RxCpltCallback);
}

/**
 * @brief  Playback initialization
 * @param  None
 * @retval None
 */
static void Playback_Init(void) {
    if (CurrentInputDevice == INPUT_DEVICE_ANALOG_MIC) {
        if (AUDIO_OK != BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, 100, PLAY_SAMPLE_RATE)) {
            Error_Handler();
        }

        BSP_AUDIO_OUT_ChangeAudioConfig(
                BSP_AUDIO_OUT_CIRCULARMODE | BSP_AUDIO_OUT_MONOMODE);
    } else {
        if (AUDIO_OK != BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, 100, PLAY_SAMPLE_RATE)) {
            Error_Handler();
        }
    }

    BSP_AUDIO_OUT_RegisterCallbacks(Audio_Playback_ErrorCallback,
        Audio_Playback_TxHalfCpltCallback, Audio_Playback_TxCpltCallback);
}

/**
 * @brief  Half record complete callback.
 * @param  None.
 * @retval None.
 */
void Audio_Record_RxHalfCpltCallback(void) {
    RecHalfBuffCplt = 1;
}

/**
 * @brief  Record complete callback.
 * @param  None.
 * @retval None.
 */
void Audio_Record_RxCpltCallback(void) {
    RecBuffCplt = 1;
}

/**
 * @brief  Record error callback.
 * @param  None.
 * @retval None.
 */
void Audio_Record_ErrorCallback(void) {
    Error_Handler();
}

/**
 * @brief  Half playback complete callback.
 * @param  None.
 * @retval None.
 */
void Audio_Playback_TxHalfCpltCallback(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    PlayHalfBuffCplt = 1;
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
}

/**
 * @brief  Playback complete callback.
 * @param  None.
 * @retval None.
 */
void Audio_Playback_TxCpltCallback(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    PlayBuffCplt = 1;
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
}

/**
 * @brief  Playback error callback.
 * @param  None.
 * @retval None.
 */
void Audio_Playback_ErrorCallback(void) {
    Error_Handler();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
