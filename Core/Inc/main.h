/* USER CODE BEGIN Header */
/**
******************************************************************************
* @file           : main.h
* @brief          : Header for main.c file.
*                   This file contains the common defines of the application.
******************************************************************************
* @attention
*
* Copyright (c) 2025 STMicroelectronics.
* All rights reserved.
*
* This software is licensed under terms that can be found in the LICENSE file
* in the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
******************************************************************************
*/
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define B1_EXTI_IRQn EXTI15_10_IRQn
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* Keypad Pin Definitions */
#define KEYPAD_ROW_PORT_A GPIOA
#define KEYPAD_ROW_PORT_C GPIOC
#define KEYPAD_COL_PORT_B GPIOB
#define KEYPAD_COL_PORT_C GPIOC

/* RFID Pin Definitions */
#define RC522_NSS_PORT GPIOB
#define RC522_NSS_PIN GPIO_PIN_12
#define RC522_RST_PORT GPIOC
#define RC522_RST_PIN GPIO_PIN_0

/* Relay and Status LED Definitions */
#define RELAY_GPIO_Port GPIOC
#define RELAY_Pin GPIO_PIN_4
#define LOCK_STATUS_LED_GPIO_Port GPIOA
#define LOCK_STATUS_LED_Pin GPIO_PIN_5

/* LCD I2C Address */
#define LCD_ADDR (0x27 << 1)

/* Application Configuration */
#define PIN_LENGTH 4
#define MAX_ATTEMPTS 3
#define LOCKOUT_PERIOD_MS 30000UL

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
