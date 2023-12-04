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
#include "main.h"

/* Private define ------------------------------------------------------------*/
/* Audio file size and start address are defined here since the Audio file is 
 stored in Flash memory as a constant table of 16-bit data */
#define AUDIO_FILE_ADDRESS    0x08100000     /* Audio file address */
#define AUDIO_FILE_SIZE        (180*1024*10)

#define PLAY_BUFF_SIZE         (4096-4)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint32_t AudioPlayback = 0;

extern __IO JOYState_TypeDef JoyState;

extern __IO uint8_t UserPressButton;
extern __IO uint32_t PauseResumeStatus;

/* Variable used to replay audio sample (from play or record test)*/

uint16_t PlayBuff[PLAY_BUFF_SIZE];
__IO uint32_t PlaybackPosition;

uint32_t PlayBackHalfBuffCplt = 0;
uint32_t PlayBackBuffCplt = 0;

extern __IO uint8_t Volume;
extern __IO uint8_t VolumeChange;

/* Private function prototypes -----------------------------------------------*/
static void AudioPlay_SetHint(void);
static void AudioPlay_DisplayInfos(WAVE_FormatTypeDef *format);

static void AUDIO_TransferComplete_CallBack(void);
static void AUDIO_HalfTransfer_CallBack(void);
static void AUDIO_Error_CallBack(void);
/* Private functions ---------------------------------------------------------*/

/**
 * @brief Test Audio Hardware.
 *     The main objective of this test is to check the hardware connection of the
 *     Audio peripheral.
 * @param    None
 * @retval None
 */
WAVE_FormatTypeDef playwaveformat;
void AudioPlay_demo(void) {
    uint8_t status = AUDIO_OK;

    uint8_t Volume_string[20] = { 0 };

    PlaybackPosition = (2 * PLAY_BUFF_SIZE) + PLAY_HEADER;

    AudioPlay_SetHint();

    mx_eeprom_read(0, PLAY_HEADER, (uint8_t*) &playwaveformat);

    printf("waveformat.ChunkID: %x\r\n", playwaveformat.ChunkID);
    printf("waveformat.FileSize: %x\r\n", playwaveformat.FileSize);
    printf("waveformat.FileFormat: %x\r\n", playwaveformat.FileFormat);
    printf("waveformat.SubChunk1ID: %x\r\n", playwaveformat.SubChunk1ID);
    printf("waveformat.SubChunk1Size: %x\r\n", playwaveformat.SubChunk1Size);
    printf("waveformat.AudioFormat: %x\r\n", playwaveformat.AudioFormat);
    printf("waveformat.NbrChannels: %x\r\n", playwaveformat.NbrChannels);
    printf("waveformat.SampleRate: %x\r\n", playwaveformat.SampleRate);
    printf("waveformat.ByteRate: %x\r\n", playwaveformat.ByteRate);
    printf("waveformat.BlockAlign: %x\r\n", playwaveformat.BlockAlign);
    printf("waveformat.BitPerSample: %x\r\n", playwaveformat.BitPerSample);
    printf("waveformat.SubChunk2ID: %x\r\n", playwaveformat.SubChunk2ID);
    printf("waveformat.SubChunk2Size: %x\r\n", playwaveformat.SubChunk2Size);
    AudioPlay_DisplayInfos(&playwaveformat);

    /* Initialize Audio Device */
    if (BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, Volume,
            playwaveformat.SampleRate) != AUDIO_OK) {
        while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
        BSP_LCD_SetTextColor(LCD_COLOR_RED);
        BSP_LCD_DisplayStringAt(0, 180, (uint8_t*) "Initialization problem",
                CENTER_MODE);
        BSP_LCD_DisplayStringAt(0, 195, (uint8_t*) "Audio Codec not detected",
                CENTER_MODE);
        BSP_LCD_Refresh();
        status = AUDIO_ERROR;
    } else {
        /* Register audio BSP drivers callbacks */
        BSP_AUDIO_OUT_RegisterCallbacks(AUDIO_Error_CallBack,
                AUDIO_HalfTransfer_CallBack, AUDIO_TransferComplete_CallBack);

        /* Initialize Volume */
        if (BSP_AUDIO_OUT_SetVolume(Volume) != AUDIO_OK) {
            status = AUDIO_ERROR;
        } else {
            /* Initialize the data buffer */
            mx_eeprom_read(PLAY_HEADER, PLAY_BUFF_SIZE * 2,
                    (uint8_t*) PlayBuff);

            /* Start the audio player */
            if (AUDIO_OK
                    != BSP_AUDIO_OUT_Play((uint16_t*) PlayBuff,
                            PLAY_BUFF_SIZE)) {
                status = AUDIO_ERROR;
            } else {
                while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
                BSP_LCD_DisplayStringAt(0, 180, (uint8_t*) "Playback on-going",
                        CENTER_MODE);
                sprintf((char*) Volume_string, " Volume : %d%% ", Volume);
                BSP_LCD_DisplayStringAt(0, 195, Volume_string, CENTER_MODE);
                BSP_LCD_Refresh();

                /* Reset JoyState and set audio playback variable */
                AudioPlayback = 1;
            }
        }
    }

    printf("start to play\r\n");
    printf("total size: %x\r\n", playwaveformat.SubChunk2Size);
    JoyState = JOY_NONE;
    /* Infinite loop */
    while (JoyState != JOY_RIGHT) {
        if (MfxItOccurred == SET) {
            Mfx_Event();
        }

        if (status == AUDIO_OK) {
            if (PauseResumeStatus == PAUSE_STATUS) {
                /* Pause playing */
                if (BSP_AUDIO_OUT_Pause() != AUDIO_OK) {
                    Error_Handler();
                }
                while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
                BSP_LCD_DisplayStringAt(0, 180,
                        (uint8_t*) "Playback paused", CENTER_MODE);
                BSP_LCD_Refresh();
                PauseResumeStatus = IDLE_STATUS;
                } else if (PauseResumeStatus == RESUME_STATUS) {
                    /* Resume playing */
                    if (BSP_AUDIO_OUT_Resume() != AUDIO_OK) {
                        Error_Handler();
                    }
                    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
                    BSP_LCD_DisplayStringAt(0, 180, (uint8_t*) "Playback on-going",
                            CENTER_MODE);
                    BSP_LCD_Refresh();
                    PauseResumeStatus = IDLE_STATUS;
                }

            if (VolumeChange != 0) {
                VolumeChange = 0;
                if (BSP_AUDIO_OUT_SetVolume(Volume) != AUDIO_OK) {
                    Error_Handler();
                }
                while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
                sprintf((char*) Volume_string, " Volume : %d%% ", Volume);
                BSP_LCD_DisplayStringAt(0, 195, Volume_string, CENTER_MODE);
                BSP_LCD_Refresh();
            }
        }
        if (PlayBackBuffCplt == 1) {
            mx_eeprom_read(PlaybackPosition, PLAY_BUFF_SIZE,
                    (uint8_t*) (((uint8_t*) PlayBuff) + PLAY_BUFF_SIZE));
            PlaybackPosition += PLAY_BUFF_SIZE;
            /* check the end of the file */
            if ((PlaybackPosition + PLAY_BUFF_SIZE)
                    > playwaveformat.SubChunk2Size) {
                PlaybackPosition = PLAY_HEADER;
            }
            PlayBackBuffCplt = 0;
        }
        if (PlayBackHalfBuffCplt == 1) {
            mx_eeprom_read(PlaybackPosition, PLAY_BUFF_SIZE,
                    (uint8_t*) PlayBuff);
            PlaybackPosition += PLAY_BUFF_SIZE;

            /* check the end of the file */
            if ((PlaybackPosition + PLAY_BUFF_SIZE)
                    > playwaveformat.SubChunk2Size) {
                PlaybackPosition = PLAY_HEADER;
            }
            PlayBackHalfBuffCplt = 0;
        }
    }

    /* Reset JoyState and audio playback variable */
    PlayBackBuffCplt = 0;
    PlayBackHalfBuffCplt = 0;
    AudioPlayback = 0;
    JoyState = JOY_NONE;

    if(status == AUDIO_OK)
    {
        /* Stop Player before close Test */
        if (BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW) != AUDIO_OK)
        {
            /* Audio Stop error */
            Error_Handler();
        }

        /* De-initialize playback */
        if(BSP_AUDIO_OUT_DeInit() != AUDIO_OK)
        {
            Error_Handler();
        }
    }
}

/**
 * @brief    Display audio play demo hint
 * @param    None
 * @retval None
 */
static void AudioPlay_SetHint(void) {
    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    /* Clear the LCD */
    BSP_LCD_Clear(LCD_COLOR_WHITE);

    /* Set Audio Demo description */
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_FillRect(0, 0, BSP_LCD_GetXSize(), 130);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
    BSP_LCD_SetFont(&Font24);
    BSP_LCD_DisplayStringAt(0, 30, (uint8_t*) "Audio Play", CENTER_MODE);
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_DisplayStringAt(0, 60, (uint8_t*) "Press RIGHT to stop play", CENTER_MODE);
    BSP_LCD_DisplayStringAt(0, 75, (uint8_t*) "SEL to pause/resume playback", CENTER_MODE);
    BSP_LCD_DisplayStringAt(0, 90, (uint8_t*) "UP/DOWN to change Volume", CENTER_MODE);

    /* Set the LCD Text Color */
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_DrawRect(40, 140, BSP_LCD_GetXSize() - 80, BSP_LCD_GetYSize() - 240);
    BSP_LCD_DrawRect(41, 141, BSP_LCD_GetXSize() - 82, BSP_LCD_GetYSize() - 242);

    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_Refresh();
}

/**
 * @brief    Display audio play demo hint
 * @param    format : structure containing informations of the file
 * @retval None
 */
static void AudioPlay_DisplayInfos(WAVE_FormatTypeDef *format) {
    uint8_t string[50] = { 0 };

    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    sprintf((char*) string, "Sampling frequency : %ld Hz", format->SampleRate);
    BSP_LCD_DisplayStringAt(0, 150, string, CENTER_MODE);

    if (format->NbrChannels == 2) {
        sprintf((char*) string, "Format : %d bits stereo", format->BitPerSample);
        BSP_LCD_DisplayStringAt(0, 165, string, CENTER_MODE);
    } else if (format->NbrChannels == 1) {
        sprintf((char*) string, "Format : %d bits mono", format->BitPerSample);
        BSP_LCD_DisplayStringAt(0, 165, string, CENTER_MODE);
    }
    BSP_LCD_Refresh();
}

/*--------------------------------
 Callbacks implementation:
 The callbacks prototypes are defined in the stm32l476g_eval_audio.h file
 and their implementation should be done in the user code if they are needed.
 Below some examples of callback implementations.
 --------------------------------------------------------*/
/**
 * @brief    AUDIO transfer complete function.
 * @param    None
 * @retval None
 */
static void AUDIO_TransferComplete_CallBack(void) {
    PlayBackBuffCplt = 1;
}

/**
 * @brief    AUDIO half transfer complete function.
 * @param    None
 * @retval None
 */
static void AUDIO_HalfTransfer_CallBack(void) {

    PlayBackHalfBuffCplt = 1;
}

/**
 * @brief    AUDIO error callback.
 * @param    None
 * @retval None
 */
static void AUDIO_Error_CallBack(void) {
/* Stop the program with an infinite loop */
    Error_Handler();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
