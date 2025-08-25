#include "i2c-lcd.h"
#include "stm32f4xx_hal.h"

#define LCD_ADDR (0x27 << 1) 

extern I2C_HandleTypeDef hi2c1;

static void lcd_send_cmd(char cmd);
static void lcd_send_data(char data);
static void lcd_send(uint8_t data, uint8_t mode);
static void lcd_write_nibble(uint8_t nibble, uint8_t mode);

void lcd_init(I2C_HandleTypeDef *hi2c) {
    HAL_Delay(50);
    lcd_send_cmd(0x30);
    HAL_Delay(5);
    lcd_send_cmd(0x30);
    HAL_Delay(1);
    lcd_send_cmd(0x30);
    HAL_Delay(10);
    lcd_send_cmd(0x20); // 4-bit mode
    HAL_Delay(10);
    lcd_send_cmd(0x28); // 2-line, 5x8 dots
    lcd_send_cmd(0x08); // Display off
    lcd_send_cmd(0x01); // Clear display
    HAL_Delay(2);
    lcd_send_cmd(0x06); // Entry mode
    lcd_send_cmd(0x0C); // Display ON, Cursor OFF
}

void lcd_send_string(char *str) {
    while (*str) lcd_send_data(*str++);
}

void lcd_put_cur(uint8_t row, uint8_t col) {
    uint8_t pos[] = {0x80, 0xC0};
    lcd_send_cmd(pos[row] + col);
}

void lcd_clear(void) {
    lcd_send_cmd(0x01);
    HAL_Delay(2);
}

static void lcd_send_cmd(char cmd) {
    lcd_send(cmd, 0);
}

static void lcd_send_data(char data) {
    lcd_send(data, 1);
}

static void lcd_send(uint8_t data, uint8_t mode) {
    lcd_write_nibble((data >> 4), mode);
    lcd_write_nibble((data & 0x0F), mode);
}

static void lcd_write_nibble(uint8_t nibble, uint8_t mode) {
    uint8_t data = (nibble << 4);
    data |= (mode << 0); // RS bit
    data |= 0x08; // Backlight ON

    uint8_t data_u = data | 0x04; // EN = 1
    uint8_t data_l = data & ~0x04; // EN = 0
    uint8_t buf[2] = {data_u, data_l};

    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, buf, 2, 10);
}
