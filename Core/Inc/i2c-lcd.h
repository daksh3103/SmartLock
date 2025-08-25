#ifndef I2C_LCD_H
#define I2C_LCD_H

#include "main.h"
#include "stm32f4xx_hal.h"


void lcd_init(I2C_HandleTypeDef *hi2c);
void lcd_send_string(char *str);
void lcd_put_cur(uint8_t row, uint8_t col);
void lcd_clear(void);

#endif
