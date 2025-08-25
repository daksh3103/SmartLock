/* USER CODE BEGIN Header */
/**
******************************************************************************
* @file           : main.c
* @brief          : Main program body
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "keypad.h"
#include "rc522.h"
#include "i2c-lcd.h"
#include <stdio.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi2;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
#define PIN_LENGTH 4
#define MAX_ATTEMPTS 3
#define LOCKOUT_PERIOD_MS 30000 // 30 seconds

/* --- 'EEPROM' simulation --- */
char valid_pin[PIN_LENGTH + 1] = "1234";
uint8_t valid_uid[5] = {0xA1, 0xB2, 0xC3, 0xD4, 0xE5};

/* --- State tracking --- */
char pin_entered[PIN_LENGTH + 1];
uint8_t pin_index = 0;
uint8_t fail_count = 0;
uint32_t lockout_ms = 0;
uint8_t rfid_authenticated = 0;
uint8_t pin_authenticated = 0;

/* --- GPIO definitions --- */
#define RELAY_GPIO_Port GPIOC
#define RELAY_Pin GPIO_PIN_4
#define LOCK_STATUS_LED_GPIO_Port GPIOA
#define LOCK_STATUS_LED_Pin GPIO_PIN_5
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI2_Init(void);

/* USER CODE BEGIN PFP */
void smartlock_success();
void smartlock_fail();
void reset_auth();
void lockout_mode();

// Printf redirection to UART
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
* @brief  The application entry point.
* @retval int
*/
int main(void)
{
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_I2C1_Init();
    MX_SPI2_Init();

    /* USER CODE BEGIN 2 */
    printf("\r\n=== Smart Door Lock System Started ===\r\n");
    printf("System: Initializing peripherals...\r\n");

    // Initialize LCD
    printf("System: Initializing LCD...\r\n");
    lcd_init(&hi2c1);
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("Smart Door Lock");
    lcd_put_cur(1, 0);
    lcd_send_string("Ready");
    printf("LCD: Display initialized and ready message shown\r\n");

    // Initialize RFID
    printf("System: Initializing RFID RC522...\r\n");
    MFRC522_Init();
    printf("RFID: RC522 initialized successfully\r\n");

    // Reset authentication state
    reset_auth();
    printf("Auth: Authentication state reset\r\n");

    printf("System: Valid PIN = %s\r\n", valid_pin);
    printf("System: Valid UID = %02X:%02X:%02X:%02X:%02X\r\n",
           valid_uid[0], valid_uid[1], valid_uid[2], valid_uid[3], valid_uid[4]);
    printf("System: Max attempts = %d, Lockout time = %ld ms\r\n", MAX_ATTEMPTS, LOCKOUT_PERIOD_MS);
    printf("System: Ready for operation!\r\n\r\n");
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */

        // Handle lockout state
        if (fail_count >= MAX_ATTEMPTS) {
            printf("Security: LOCKOUT MODE - %d failed attempts\r\n", fail_count);
            lockout_mode();
            continue;
        }

        // 1. Keypad check
        char key = keypad_getkey();
        if (key != 0) {
            printf("Keypad: Key pressed = '%c'\r\n", key);

            // Accept numeric or * as clear; # as enter
            if (key >= '0' && key <= '9' && pin_index < PIN_LENGTH) {
                pin_entered[pin_index++] = key;
                printf("Keypad: PIN entry [%d/%d]: ", pin_index, PIN_LENGTH);
                for(int i = 0; i < pin_index; i++) printf("*");
                printf("\r\n");

                lcd_put_cur(1, 0);
                for (uint8_t i = 0; i < pin_index; ++i) lcd_send_string("*");
                for (uint8_t i = pin_index; i < PIN_LENGTH; ++i) lcd_send_string(" ");
            }

            if (key == '*') {
                printf("Keypad: Clear PIN entry\r\n");
                pin_index = 0;
                memset(pin_entered, 0, sizeof(pin_entered));
                lcd_put_cur(1, 0);
                lcd_send_string("    "); // Clear display
            }

            if (key == '#') {
                pin_entered[pin_index] = 0;
                printf("Keypad: PIN submitted = '%s'\r\n", pin_entered);

                if (strcmp(pin_entered, valid_pin) == 0) {
                    pin_authenticated = 1;
                    printf("Auth: PIN CORRECT! Authentication granted\r\n");
                    lcd_put_cur(0,0);
                    lcd_send_string("PIN OK        ");
                } else {
                    pin_authenticated = 0;
                    printf("Auth: PIN INCORRECT! Expected: %s, Got: %s\r\n", valid_pin, pin_entered);
                    smartlock_fail();
                }

                pin_index = 0;
                memset(pin_entered, 0, sizeof(pin_entered));
            }
        }

        // 2. RFID check
        uint8_t card_uid[5];
        if (MFRC522_Check(card_uid) == MI_OK) {
            printf("RFID: Card detected! UID = %02X:%02X:%02X:%02X:%02X\r\n",
                   card_uid[0], card_uid[1], card_uid[2], card_uid[3], card_uid[4]);

            if (memcmp(card_uid, valid_uid, 5) == 0) {
                rfid_authenticated = 1;
                printf("Auth: RFID CORRECT! Authentication granted\r\n");
                lcd_put_cur(0,0);
                lcd_send_string("RFID OK       ");
            } else {
                rfid_authenticated = 0;
                printf("Auth: RFID INCORRECT! Valid UID = %02X:%02X:%02X:%02X:%02X\r\n",
                       valid_uid[0], valid_uid[1], valid_uid[2], valid_uid[3], valid_uid[4]);
                smartlock_fail();
            }

            HAL_Delay(800); // avoid repeated trigger
        }

        // 3. Dual authentication logic
        if (pin_authenticated && rfid_authenticated) {
            printf("Auth: DUAL AUTHENTICATION SUCCESS! Both PIN and RFID verified\r\n");
            smartlock_success();
            reset_auth();
            continue;
        }

        HAL_Delay(50); // polling rate

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/* --- Success/failure routines --- */
void smartlock_success() {
    printf("Door: ACCESS GRANTED! Unlocking door...\r\n");
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("Access Granted!");

    // Turn ON user LED (LD2) to show unlock
    HAL_GPIO_WritePin(LOCK_STATUS_LED_GPIO_Port, LOCK_STATUS_LED_Pin, GPIO_PIN_SET);
    printf("Hardware: Status LED ON\r\n");

    // Unlock relay (Active LOW)
    HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_RESET);
    printf("Hardware: Relay UNLOCKED (LOW)\r\n");

    printf("Door: Door will remain unlocked for 2 seconds...\r\n");
    HAL_Delay(2000);

    // Lock relay and turn off status LED
    HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LOCK_STATUS_LED_GPIO_Port, LOCK_STATUS_LED_Pin, GPIO_PIN_RESET);
    printf("Hardware: Relay LOCKED (HIGH), Status LED OFF\r\n");

    lcd_clear();
    lcd_put_cur(0,0);
    lcd_send_string("Ready         ");
    printf("Door: Door locked and system ready\r\n\r\n");
}

void smartlock_fail() {
    fail_count++;
    printf("Security: ACCESS DENIED! Fail count = %d/%d\r\n", fail_count, MAX_ATTEMPTS);

    lcd_clear();
    lcd_put_cur(0,0);
    lcd_send_string("Access Denied!");

    // Blink the onboard LED for access denied
    printf("Hardware: Blinking status LED for denial indication\r\n");
    for(int i=0; i<4; i++) {
        HAL_GPIO_TogglePin(LOCK_STATUS_LED_GPIO_Port, LOCK_STATUS_LED_Pin);
        HAL_Delay(150);
    }
    HAL_GPIO_WritePin(LOCK_STATUS_LED_GPIO_Port, LOCK_STATUS_LED_Pin, GPIO_PIN_RESET);

    lcd_clear();
    lcd_put_cur(0,0);
    lcd_send_string("Ready         ");

    if (fail_count >= MAX_ATTEMPTS) {
        printf("Security: MAX ATTEMPTS REACHED! Lockout will begin...\r\n");
    }
    printf("\r\n");
}

void reset_auth() {
    rfid_authenticated = 0;
    pin_authenticated = 0;
    pin_index = 0;
    memset(pin_entered, 0, sizeof(pin_entered));
    printf("Auth: Authentication flags reset (PIN: %d, RFID: %d)\r\n",
           pin_authenticated, rfid_authenticated);
}

void lockout_mode() {
    printf("Security: *** LOCKOUT MODE ACTIVATED ***\r\n");
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("LOCKED OUT!");

    uint32_t remaining = LOCKOUT_PERIOD_MS / 1000; // in seconds
    printf("Security: Lockout duration = %ld seconds\r\n", remaining);

    for(uint32_t t = 0; t < remaining; ++t) {
        uint32_t time_left = remaining - t;
        printf("Security: Lockout remaining: %ld seconds\r\n", time_left);

        lcd_put_cur(1, 0);
        char buf[17];
        snprintf(buf, sizeof(buf), "Wait %lus       ", time_left);
        lcd_send_string(buf);

        // Blink LED during lockout
        HAL_GPIO_WritePin(LOCK_STATUS_LED_GPIO_Port, LOCK_STATUS_LED_Pin, GPIO_PIN_SET); // ON
        HAL_Delay(500);
        HAL_GPIO_WritePin(LOCK_STATUS_LED_GPIO_Port, LOCK_STATUS_LED_Pin, GPIO_PIN_RESET); // OFF
        HAL_Delay(500);
    }

    // After lockout period, turn off LED and show Ready
    HAL_GPIO_WritePin(LOCK_STATUS_LED_GPIO_Port, LOCK_STATUS_LED_Pin, GPIO_PIN_RESET);
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("Ready         ");
    fail_count = 0; // Reset the failed-attempts counter here

    printf("Security: LOCKOUT ENDED - System ready, fail count reset\r\n\r\n");
}

/**
* @brief System Clock Configuration
* @retval None
*/
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
    */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
* @brief I2C1 Initialization Function
* @param None
* @retval None
*/
static void MX_I2C1_Init(void)
{
    /* USER CODE BEGIN I2C1_Init 0 */
    /* USER CODE END I2C1_Init 0 */

    /* USER CODE BEGIN I2C1_Init 1 */
    /* USER CODE END I2C1_Init 1 */
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN I2C1_Init 2 */
    /* USER CODE END I2C1_Init 2 */
}

/**
* @brief SPI2 Initialization Function
* @param None
* @retval None
*/
static void MX_SPI2_Init(void)
{
    /* USER CODE BEGIN SPI2_Init 0 */
    /* USER CODE END SPI2_Init 0 */

    /* USER CODE BEGIN SPI2_Init 1 */
    /* USER CODE END SPI2_Init 1 */
    /* SPI2 parameter configuration*/
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi2) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN SPI2_Init 2 */
    /* USER CODE END SPI2_Init 2 */
}

/**
* @brief USART2 Initialization Function
* @param None
* @retval None
*/
static void MX_USART2_UART_Init(void)
{
    /* USER CODE BEGIN USART2_Init 0 */
    /* USER CODE END USART2_Init 0 */

    /* USER CODE BEGIN USART2_Init 1 */
    /* USER CODE END USART2_Init 1 */
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN USART2_Init 2 */
    /* USER CODE END USART2_Init 2 */
}

/**
* @brief GPIO Initialization Function
* @param None
* @retval None
*/
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* USER CODE BEGIN MX_GPIO_Init_1 */
    /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0|GPIO_PIN_4, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_9, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

    /*Configure GPIO pin : B1_Pin */
    GPIO_InitStruct.Pin = B1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : PC0 PC4 */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*Configure GPIO pins : PA5 PA6 PA7 PA9 */
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pin : PB12 */
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : PB3 */
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    /* USER CODE BEGIN MX_GPIO_Init_2 */
    /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
* @brief  This function is executed in case of error occurrence.
* @retval None
*/
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

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
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
