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
#include <rwwee.h>
#include "main.h"
#include "cmsis_os.h"

#include "usart.h"
#include "mx_define.h"
#include "nor_cmd.h"
#include "app.h"

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define MFX_IRQ_PENDING_GPIO    0x01
#define MFX_IRQ_PENDING_IDD     0x02
#define MFX_IRQ_PENDING_ERROR   0x04
#define MFX_IRQ_PENDING_TS_DET  0x08
#define MFX_IRQ_PENDING_TS_NE   0x10
#define MFX_IRQ_PENDING_TS_TH   0x20
#define MFX_IRQ_PENDING_TS_FULL 0x40
#define MFX_IRQ_PENDING_TS_OVF  0x80

/* Private define ------------------------------------------------------------*/
#define LTDC_ACTIVE_LAYER     LTDC_DEFAULT_ACTIVE_LAYER
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t DemoIndex = 0;
__IO uint8_t NbLoop = 1;

#if defined(LCD_DIMMING)
RTC_HandleTypeDef RTCHandle;
__IO uint32_t display_dimmed     = 0;     /* LCD dimming status */
__IO uint32_t maintain_display   = 0;     /* Flag to prevent LCD dimming or turning off */
#if defined(LCD_OFF_AFTER_DIMMING)
__IO uint32_t display_turned_off = 0;     /* LCD turn off status */
#endif /* defined(LCD_OFF_AFTER_DIMMING) */
TIM_HandleTypeDef    TimHandle;
static void Timer_Init(TIM_HandleTypeDef * TimHandle);
uint32_t brightness = 0;
extern DSI_HandleTypeDef    hdsi_discovery;
#endif /* defined(LCD_DIMMING) */

uint32_t AudioPlayback = 0;

/* Components Initialization Status */
FlagStatus JoyInitialized = RESET;
FlagStatus LcdInitialized = RESET;
FlagStatus TsInitialized = RESET;
FlagStatus LedInitialized = RESET;

/* Player Pause/Resume Status */
__IO uint32_t PauseResumeStatus = IDLE_STATUS;

/* Counter for Sel Joystick pressed */
__IO uint32_t PressCount = 0;

/* Joystick Status */
__IO JOYState_TypeDef JoyState = JOY_NONE;

/* MFX Interrupt Status */
__IO FlagStatus MfxItOccurred = RESET;

/* Touch event */
__IO uint32_t TouchEvent = 0;

/* IDD event */
__IO uint32_t IddEvent = 0;
#if (DEBUG_ON == 1)
__IO uint32_t IddEventCounter = 0;
#endif

/* Volume of the audio playback */
/* Initial volume level (from 0% (Mute) to 100% (Max)) */
__IO uint8_t Volume = 100;
__IO uint8_t VolumeChange = 0;

uint8_t key_enable = 0;

/* Private function prototypes -----------------------------------------------*/
static void Display_DemoDescription(void);

/* Private functions ---------------------------------------------------------*/

static void eeprom_rww_demo(void const *argument);
TaskHandle_t eeprom_rww_demo_handle;
/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
u16 WrData[PAGE_SZ * 5], RdData[PAGE_SZ * 5] = { 0 };

extern uint8_t skip_len;

int main(void) {
    /* STM32L4xx HAL library initialization:
     - Configure the Flash prefetch
     - Systick timer is configured by default as source of time base, but user
     can eventually implement his proper time base source (a general purpose
     timer for example or other time source), keeping in mind that Time base
     duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
     handled in milliseconds basis.
     - Set NVIC Group Priority to 4
     - Low Level Initialization
     */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* System common Hardware components initialization (Leds, joystick, LCD and touchscreen) */
    SystemHardwareInit();
#ifdef RWW_DRIVER_SUPPORT
    STM32_Timer_Init();
#endif
    BSP_USART1_Init();

    while (BSP_LCD_IsFrameBufferAvailable() != LCD_OK);
    Display_DemoDescription();
    BSP_LCD_Refresh();

#if defined(LCD_DIMMING)
  Dimming_Timer_Enable(&RTCHandle);
#endif
    xTaskCreate(eeprom_rww_demo, "eeprom_rww_demo", configMINIMAL_STACK_SIZE,
        NULL, osPriorityNormal, &eeprom_rww_demo_handle);

    for (int n = 0; n < PAGE_SZ * 5; n++)
        WrData[n] = n % 65536;

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    for (;;);

}

static void eeprom_rww_demo(void const *argument) {
    (void) argument;

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    /* Enable the GPIO_LED Clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Configure the GPIO_LED pin */
    GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_0 | GPIO_PIN_5 | GPIO_PIN_1
        | GPIO_PIN_4;
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

    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);

    __reinit:
#if 1
    if (mx_eeprom_init()) {
        mx_eeprom_format();
        goto __reinit;
    }
    printf("mx eeprom init successfully\r\n");
#endif
    uint8_t demo_step = 0;
    while (1) {
        if (demo_step < 2) {
            if (demo_step == 0)
                skip_len = 1;
            else if (demo_step == 1)
                skip_len = 2;
            AudioRecord_demo();
            demo_step++;
        } else if (demo_step == 2) {
            rww_perf_demo();
            demo_step++;
        } else if (demo_step == 3) {
            eeprom_perf_demo();
            demo_step = 0;
        }

    }

    vTaskSuspend(NULL);
}

/**
 * @brief  System Clock Configuration
 *         The system Clock is configured as follows :
 *            System Clock source            = PLL (MSI)
 *            SYSCLK(Hz)                     = 120000000
 *            HCLK(Hz)                       = 120000000
 *            AHB Prescaler                  = 1
 *            APB1 Prescaler                 = 1
 *            APB2 Prescaler                 = 1
 *            MSI Frequency(Hz)              = 4000000
 *            PLL_M                          = 1
 *            PLL_N                          = 60
 *            PLL_Q                          = 2
 *            PLL_R                          = 2
 *            PLL_P                          = 7
 *            Flash Latency(WS)              = 5
 * @param  None
 * @retval None
 */
void SystemClock_Config(void) {
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };

    /* Enable voltage range 1 boost mode for frequency above 80 Mhz (120Mhz) */
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);
    __HAL_RCC_PWR_CLK_DISABLE();

    /* Enable MSI Oscillator and activate PLL with MSI as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 60;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLP = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        /* Initialization Error */
        while (1);
    }

    /* To avoid undershoot due to maximum frequency, select PLL as system clock source */
    /* with AHB prescaler divider 2 as first step */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
        | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) {
        /* Initialization Error */
        while (1);
    }

    /* AHB prescaler divider at 1 as second step */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        /* Initialization Error */
        while (1);
    }
}

/**
 * @brief  System Clock Configuration
 *         The system Clock is configured as follow :
 *            System Clock source            = MSI
 *            SYSCLK(Hz)                     = 2000000
 *            HCLK(Hz)                       = 2000000
 *            AHB Prescaler                  = 1
 *            APB1 Prescaler                 = 1
 *            APB2 Prescaler                 = 1
 *            MSI Frequency(Hz)              = 2000000
 *            PLL_M                          = 1
 *            PLL_N                          = 80
 *            PLL_R                          = 2
 *            PLL_P                          = 7
 *            PLL_Q                          = 4
 *            Flash Latency(WS)              = 4
 * @param  None
 * @retval None
 */
void SystemLowClock_Config(void) {
    /* oscillator and clocks configs */
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    uint32_t flatency = 0;

    /* Retrieve clock parameters */
    HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &flatency);

    /* switch SYSCLK to MSI in order to modify PLL divider */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, flatency) != HAL_OK) {
        /* Initialization Error */
        Error_Handler();
    }

    /* Retrieve oscillator parameters */
    HAL_RCC_GetOscConfig(&RCC_OscInitStruct);

    /* turn off PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        /* Initialization Error */
        Error_Handler();
    }

    /* Enable voltage range 2 mode for lower frequency */
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);
    __HAL_RCC_PWR_CLK_DISABLE();
}

/**
 * @brief  Recover System Clock Configuration after Stop mode
 *         The system Clock is configured as follows :
 *            System Clock source            = PLL (MSI)
 *            SYSCLK(Hz)                     = 120000000
 *            HCLK(Hz)                       = 120000000
 *            AHB Prescaler                  = 1
 *            APB1 Prescaler                 = 1
 *            APB2 Prescaler                 = 1
 *            MSI Frequency(Hz)              = 4000000
 *            PLL_M                          = 1
 *            PLL_N                          = 60
 *            PLL_Q                          = 2
 *            PLL_R                          = 2
 *            PLL_P                          = 7
 *            Flash Latency(WS)              = 5
 * @param  None
 * @retval None
 */
void SystemClock_ConfigFromLowPower(void) {
    /* oscillator and clocks configs */
    RCC_OscInitTypeDef RCC_OscInitStruct;

    HAL_RCC_GetOscConfig(&RCC_OscInitStruct);

    /* Enable main PLL */
    if (RCC_OscInitStruct.PLL.PLLState != RCC_PLL_ON) {
        SystemClock_Config();
    }
}

/**
 * @brief  Hardware components main initialization
 * @param  None
 * @retval None
 */
void SystemHardwareInit(void) {
    /* Init LED 1 to 4  */
    if (LedInitialized != SET) {
        BSP_LED_Init(LED1);
        BSP_LED_Init(LED2);
        LedInitialized = SET;
    }

    /* Init Joystick in interrupt mode */
    if (JoyInitialized != SET) {
        BSP_JOY_Init(JOY_MODE_EXTI);
        JoyInitialized = SET;
    }

    /* Initialize the LCD */
    if (LcdInitialized != SET) {
        /* Initialize the LCD */
        if (BSP_LCD_Init() != LCD_OK) {
            Error_Handler();
        }

        LcdInitialized = SET;
    }

    /* For external component access over I2C */
    __HAL_RCC_I2C1_CLK_ENABLE();
}

/**
 * @brief  Hardware components main de-initialization
 * @param  None
 * @retval None
 */
void SystemHardwareDeInit(void) {
    if (LedInitialized != RESET) {
        BSP_LED_DeInit(LED1);
        BSP_LED_DeInit(LED2);
        LedInitialized = RESET;
    }

    if (JoyInitialized != RESET) {
        BSP_JOY_DeInit();
        JoyInitialized = RESET;
    }

    if (LcdInitialized != RESET) {
        BSP_LCD_DisplayOff();
        BSP_LCD_DeInit();
        LcdInitialized = RESET;
    }

    /* Disable remaining clocks */
    __HAL_RCC_PWR_CLK_DISABLE();
    __HAL_RCC_FLASH_CLK_DISABLE();
    __HAL_RCC_SYSCFG_CLK_DISABLE();
    __HAL_RCC_GPIOB_CLK_DISABLE();
    __HAL_RCC_GPIOC_CLK_DISABLE();
    __HAL_RCC_GPIOD_CLK_DISABLE();
    __HAL_RCC_GPIOE_CLK_DISABLE();
    __HAL_RCC_GPIOF_CLK_DISABLE();
    __HAL_RCC_GPIOG_CLK_DISABLE();
    __HAL_RCC_GPIOH_CLK_DISABLE();
    __HAL_RCC_GPIOI_CLK_DISABLE();

    __HAL_RCC_I2C1_CLK_DISABLE();

    RCC->AHB1SMENR = 0x0;
    RCC->AHB2SMENR = 0x0;
    RCC->AHB3SMENR = 0x0;
    RCC->APB1SMENR1 = 0x0;
    RCC->APB1SMENR2 = 0x0;
    RCC->APB2SMENR = 0x0;
}

/**
 * @brief  Read a value from fixed location in RTC Backup memory (DR0)
 * @param  None
 * @retval Value
 */
uint32_t SystemRtcBackupRead(void) {
    RTC_HandleTypeDef hrtc;
    hrtc.Instance = RTC;

    /* Enable PWR clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* enable Back up register access */
    HAL_PWR_EnableBkUpAccess();

    return (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0));
}

/**
 * @brief  Write a value at fixed location in  RTC Backup memory (DR0)
 * @param  SaveIndex  Index value to save
 * @retval None
 */
void SystemRtcBackupWrite(uint32_t SaveIndex) {
    RTC_HandleTypeDef hrtc;
    hrtc.Instance = RTC;

    /* enable Back up register access */
    HAL_PWR_EnableBkUpAccess();

    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, SaveIndex);
}

/**
 * @brief  Display main demo messages
 * @param  None
 * @retval None
 */
static void Display_DemoDescription(void) {
    /* Set LCD Foreground Layer  */
    BSP_LCD_SelectLayer(LTDC_ACTIVE_LAYER);

    BSP_LCD_SetFont(&Font20);

    /* Clear the LCD */
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_Clear(LCD_COLOR_WHITE);

    /* Set the LCD Text Color */
    BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);

    /* Display LCD messages */
    BSP_LCD_DisplayStringAt(0, 75, (uint8_t*) "EEPROM and RWW Driver",
        CENTER_MODE);
    BSP_LCD_DisplayStringAt(0, 120, (uint8_t*) "On STM32L4R9I Demo",
        CENTER_MODE);

    /* Draw Bitmap */
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 100,
        (uint8_t*) "Macronix EEPROM Emulation", CENTER_MODE);
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 70,
        (uint8_t*) "Macronix RWW Octa Flash", CENTER_MODE);

    BSP_LCD_SetFont(&Font20);
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_FillRect(0, BSP_LCD_GetYSize() / 2, BSP_LCD_GetXSize(), 90);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 + 10,
        (uint8_t*) "1.Right Key-Rec While Play", CENTER_MODE);
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 + 35,
        (uint8_t*) "2.Up Key-RWW Driver PERF", CENTER_MODE);
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 + 60,
        (uint8_t*) "3.Down Key-EEPROM PERF", CENTER_MODE);

}

/**
 * @brief converts a 32bit unsined int into ASCII
 * @caller several callers for display values
 * @param Number digit to displays
 *  p_tab values in array in ASCII
 * @retval None
 */
void Convert_IntegerIntoChar(uint32_t number, uint16_t *p_tab) {
    uint16_t units = 0, tens = 0, hundreds = 0, thousands = 0, tenthousand = 0, hundredthousand = 0;

    units = ((((number % 100000) % 10000) % 1000) % 100) % 10;
    tens = (((((number - units) / 10) % 10000) % 1000) % 100) % 10;
    hundreds = ((((number - tens - units) / 100) % 1000) % 100) % 10;
    thousands = (((number - hundreds - tens - units) / 1000) % 100) % 10;
    tenthousand = ((number - thousands - hundreds - tens - units) / 10000) % 10;
    hundredthousand = ((number - tenthousand - thousands - hundreds - tens - units) / 100000);

    *(p_tab + 5) = units + 0x30;
    *(p_tab + 4) = tens + 0x30;
    *(p_tab + 3) = hundreds + 0x30;
    *(p_tab + 2) = thousands + 0x30;
    *(p_tab + 1) = tenthousand + 0x30;
    *(p_tab + 0) = hundredthousand + 0x30;
}

/**
 * @brief  EXTI line detection callbacks.
 * @param GPIO_Pin: Specifies the pins connected EXTI line
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    /* Check if interrupt comes from MFX */
    if (GPIO_Pin == MFX_INT_PIN) {
        /* Disable MFX interrupt to manage current one */
        /* MFX interrupt will be re-enable after processing in Mfx_Event() */
        HAL_NVIC_DisableIRQ(MFX_INT_EXTI_IRQn);

        MfxItOccurred = SET;
    }
    /* Check if interrupt comes Joystick SEL */
    else if (GPIO_Pin == SEL_JOY_PIN) {
        JoyState = JOY_SEL;
        if (!key_enable)
            JoyState = JOY_NONE;
        if (AudioPlayback == 1) {
            /* SEL is used to pause and resume the audio playback */
            if (PressCount == 1) {
                /* Resume playing Wave status */
                PauseResumeStatus = RESUME_STATUS;
                PressCount = 0;
            } else {
                /* Pause playing Wave status */
                PauseResumeStatus = PAUSE_STATUS;
                PressCount = 1;
            }
        }
    } else {
    }
}

/**
 * @brief  MFX event management.
 * @retval None
 */
__IO uint32_t errorSrc = 0;
__IO uint32_t errorMsg = 0;
void Mfx_Event(void) {
    uint32_t irqPending;

    /* Reset joystick state */
    JoyState = JOY_NONE;

    irqPending = MFX_IO_Read(IO_I2C_ADDRESS, MFXSTM32L152_REG_ADR_IRQ_PENDING);

    /* GPIO IT from MFX */
    if (irqPending & MFX_IRQ_PENDING_GPIO) {
        uint32_t JoystickStatus;
        uint32_t statusGpio = BSP_IO_ITGetStatus( RIGHT_JOY_PIN | LEFT_JOY_PIN | UP_JOY_PIN | DOWN_JOY_PIN
            | TS_INT_PIN | SD_DETECT_PIN);

        TouchEvent = statusGpio & TS_INT_PIN;

        JoystickStatus = statusGpio & (RIGHT_JOY_PIN | LEFT_JOY_PIN | UP_JOY_PIN | DOWN_JOY_PIN);

        if (JoystickStatus != 0) {
            if (JoystickStatus == RIGHT_JOY_PIN) {
                JoyState = JOY_RIGHT;
            } else if (JoystickStatus == LEFT_JOY_PIN) {
                JoyState = JOY_LEFT;
            } else if (JoystickStatus == UP_JOY_PIN) {
                JoyState = JOY_UP;
                if (AudioPlayback == 1) {
                    /* UP is used to increment the volume of the audio playback */
                    Volume += 5;
                    if (Volume > 100) {
                        Volume = 100;
                    }
                    VolumeChange = 1;
                }
            } else if (JoystickStatus == DOWN_JOY_PIN) {
                JoyState = JOY_DOWN;
                if (AudioPlayback == 1) {
                    /* DOWN is used to decrement the volume of the audio playback */
                    Volume -= 5;
                    if ((int8_t) Volume < 70) {
                        Volume = 70;
                    }
                    VolumeChange = 1;
                }
            } else {
                JoyState = JOY_NONE;
            }
        }

        /* Insert a little delay to avoid debounce */
        HAL_Delay(500);

        /* Clear IO Expander IT */
        BSP_IO_ITClear(statusGpio);
    }
    if (irqPending & MFX_IRQ_PENDING_IDD) {
        IddEvent = 1;
#if (DEBUG_ON == 1)
        IddEventCounter++;
#endif
    }
    if (irqPending & MFX_IRQ_PENDING_ERROR) {
        errorSrc = MFX_IO_Read(IO_I2C_ADDRESS, MFXSTM32L152_REG_ADR_ERROR_SRC);
        errorMsg = MFX_IO_Read(IO_I2C_ADDRESS, MFXSTM32L152_REG_ADR_ERROR_MSG);
    }

    /* Ack all IRQ pending except GPIO if any */
    irqPending &= ~MFX_IRQ_PENDING_GPIO;
    if (irqPending) {
        MFX_IO_Write(IO_I2C_ADDRESS, MFXSTM32L152_REG_ADR_IRQ_ACK, irqPending);
    }

    MfxItOccurred = RESET;

    /* Re-enable MFX interrupt */
    HAL_NVIC_EnableIRQ(MFX_INT_EXTI_IRQn);
}

/**
 * @brief Toggle Leds
 * @param  None
 * @retval None
 */
void Toggle_Leds(void) {
    static uint8_t ticks = 0;

    if (ticks++ > 100) {
        BSP_LED_Toggle(LED_ORANGE);
        ticks = 0;
    }
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
void Error_Handler(void) {
    /* Turn LED REDon */
    BSP_LED_On(LED_ORANGE);
    while (1) {
    }
}

/**
 * @brief  Application implementation of weak function.
 * @param  None
 * @retval None
 */
void BSP_ErrorHandler(void) {
    Error_Handler();
}

#if defined(LCD_DIMMING)
void Dimming_Timer_Enable(RTC_HandleTypeDef *RTCHandle) {
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_PeriphCLKInitTypeDef rtcclk;

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    rtcclk.PeriphClockSelection = RCC_PERIPHCLK_RTC;

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    rtcclk.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
    HAL_RCCEx_PeriphCLKConfig(&rtcclk);

    __HAL_RCC_RTC_ENABLE();

    /* Enable and set Wake-Up Timer to the lowest priority */
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0x0F, 0x0F);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);

    RTCHandle->Instance = RTC;

    RTCHandle->Init.HourFormat = RTC_HOURFORMAT_12;
    RTCHandle->Init.AsynchPrediv = 0x7F /*RTC_ASYNCH_PREDIV*/;
    RTCHandle->Init.SynchPrediv = 0xF9 /*RTC_SYNCH_PREDIV*/;
    RTCHandle->Init.OutPut = RTC_OUTPUT_DISABLE;
    RTCHandle->Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    RTCHandle->Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    RTCHandle->Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if (HAL_RTC_Init(RTCHandle) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_RTCEx_SetWakeUpTimer_IT(RTCHandle, DIMMING_COUNTDOWN,
            RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief  At each RTC wake-up timer expiration, initiate LCD screen dimming (or turn off) if possible
 * @param  hrtc: RTC handle parameter
 */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) {
#if defined(LCD_OFF_AFTER_DIMMING)
    static uint32_t dimming_period_counter = 0;
#endif /* defined(LCD_OFF_AFTER_DIMMING) */

#if defined(LCD_OFF_AFTER_DIMMING)
  if (display_turned_off == 0)
  {
    /* If LCD is not turned off, check whether or not it is dimmed;
       if is not dimmed and if no condition prevents it, dim the screen */
#endif /* defined(LCD_OFF_AFTER_DIMMING) */
    /* If the screen is not dimmed */
    if (display_dimmed == 0) {
        if (maintain_display == 0) {
            brightness = BRIGHTNESS_MAX;
            Timer_Init(&TimHandle);
            display_dimmed = 1; /* Screen is dimmed */
        }
    }
#if defined(LCD_OFF_AFTER_DIMMING)
    else
    {
        /* If screen is dimmed, wait for LCD_OFF_COUNTDOWN RTC wake-up timer expirations
         before turning off the LCD */
        dimming_period_counter++;
        if (dimming_period_counter == LCD_OFF_COUNTDOWN) {
            dimming_period_counter = 0;
            display_dimmed = 0; /* Screen is not dimmed */
            display_turned_off = 1; /* Screen is turned off */
            BSP_LCD_DisplayOff();
        }
    }
  }
#endif /* defined(LCD_OFF_AFTER_DIMMING) */

    /* Reset flag preventing screen dimming or turning off */
    maintain_display = 0;

}

static void Timer_Init(TIM_HandleTypeDef *TimHandle) {
    /* Prescaler declaration */
    uint32_t uwPrescalerValue = 0;

    /* Compute the prescaler value to have TIMx counter clock equal to 10000 Hz */
    uwPrescalerValue = (uint32_t)(SystemCoreClock / 10000) - 1;

    /* Set TIM3 instance */
    TimHandle->Instance = TIM3;

    /* Initialize TIM3 peripheral as follows:
     + Period = 10000 - 1
     + Prescaler = (SystemCoreClock/10000) - 1
     + ClockDivision = 0
     + Counter direction = Up
     */
    TimHandle->Init.Period = 100 - 1; /* Want 10 ms period */
    TimHandle->Init.Prescaler = uwPrescalerValue;
    TimHandle->Init.ClockDivision = 0;
    TimHandle->Init.CounterMode = TIM_COUNTERMODE_UP;
    TimHandle->Init.RepetitionCounter = 0;

    if (HAL_TIM_Base_Init(TimHandle) != HAL_OK) {
        /* Initialization Error */
        Error_Handler();
    }

    /*##-2- Start the TIM Base generation in interrupt mode ####################*/
    /* Start Channel1 */
    if (HAL_TIM_Base_Start_IT(TimHandle) != HAL_OK) {
        /* Starting Error */
        Error_Handler();
    }

}

/**
 * @brief TIM MSP Initialization
 *        This function configures the hardware resources used in this example:
 *           - Peripheral's clock enable
 * @param htim: TIM handle pointer
 * @retval None
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim) {
    /*##-1- Enable peripheral clock #################################*/
    /* TIM3 Peripheral clock enable */
    __HAL_RCC_TIM3_CLK_ENABLE();

    /*##-2- Configure the NVIC for TIM3 ########################################*/
    /* Set the TIM3 priority */
    HAL_NVIC_SetPriority(TIM3_IRQn, 0x0F, 0x0F /*3, 0*/);

    /* Enable the TIM3 global Interrupt */
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    HAL_DSI_ShortWrite(&hdsi_discovery, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0x51,
            brightness);
    brightness--;
    if (brightness < BRIGHTNESS_MIN) {
        HAL_TIM_Base_Stop_IT(htim);
    }
}
#endif /* defined(LCD_DIMMING) */

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif /* USE_FULL_ASSERT */

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
