/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H
#define __USART_H

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
#include "stm32l4r9i_discovery.h"
#include "stdio.h"

/* Exported types ------------------------------------------------------------*/
extern UART_HandleTypeDef UartHandle;
/* Exported constants --------------------------------------------------------*/
/* User can use this section to tailor USARTx/UARTx instance used and associated
   resources */
///* Definition for USARTx clock resources */

//#define USARTx                           USART1
//#define USARTx_CLK_ENABLE()              __HAL_RCC_USART1_CLK_ENABLE();
//#define USARTx_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
//#define USARTx_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
//
//#define USARTx_FORCE_RESET()             __HAL_RCC_USART1_FORCE_RESET()
//#define USARTx_RELEASE_RESET()           __HAL_RCC_USART1_RELEASE_RESET()
//
///* Definition for USARTx Pins */
//#define USARTx_TX_PIN                    GPIO_PIN_9
//#define USARTx_TX_GPIO_PORT              GPIOA
//#define USARTx_TX_AF                     GPIO_AF7_USART1
//#define USARTx_RX_PIN                    GPIO_PIN_10
//#define USARTx_RX_GPIO_PORT              GPIOA
//#define USARTx_RX_AF                     GPIO_AF7_USART1

#define USARTx                           USART2
#define USARTx_CLK_ENABLE()              __HAL_RCC_USART2_CLK_ENABLE()
#define USARTx_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()

#define USARTx_FORCE_RESET()             __HAL_RCC_USART2_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __HAL_RCC_USART2_RELEASE_RESET()

/* Definition for USARTx Pins */
#define USARTx_TX_PIN                    GPIO_PIN_2
#define USARTx_TX_GPIO_PORT              GPIOA
#define USARTx_TX_AF                     GPIO_AF7_USART2
#define USARTx_RX_PIN                    GPIO_PIN_3
#define USARTx_RX_GPIO_PORT              GPIOA
#define USARTx_RX_AF                     GPIO_AF7_USART2


/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void BSP_USART1_Init();
int __serial_io_putchar(int ch);
int __serial_io_getchar(void);

/* Private function prototypes -----------------------------------------------*/
#endif /* __MAIN_H */
