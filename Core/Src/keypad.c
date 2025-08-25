#include "keypad.h"

// Key map for 4x4
static const char keymap[4][4] = {
    { '1','2','3','A'},
    { '4','5','6','B'},
    { '7','8','9','C'},
    { '*','0','#','D'}
};

// Row configuration (mixed ports)
typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
} gpio_pin_t;

static const gpio_pin_t row_pins[4] = {
    {GPIOC, GPIO_PIN_3},  // Row 0 - PC3
    {GPIOA, GPIO_PIN_9},  // Row 1 - PA9
    {GPIOA, GPIO_PIN_6},  // Row 2 - PA6
    {GPIOA, GPIO_PIN_7}   // Row 3 - PA7
};

// Column configuration (mixed ports)
static const gpio_pin_t col_pins[4] = {
    {GPIOB, GPIO_PIN_3},  // Col 0 - PB3
    {GPIOC, GPIO_PIN_1},  // Col 1 - PC1
    {GPIOB, GPIO_PIN_9},  // Col 2 - PB9
    {GPIOC, GPIO_PIN_2}   // Col 3 - PC2
};

void keypad_init(void) {
    // GPIO setup handled by CubeMX
    // Rows: Output, initially HIGH
    // Cols: Input with Pull-up
}

char keypad_getkey(void) {
    for (int row = 0; row < 4; row++) {
        // Set all rows HIGH first
        for (int r = 0; r < 4; r++) {
            HAL_GPIO_WritePin(row_pins[r].port, row_pins[r].pin, GPIO_PIN_SET);
        }

        // Set current row LOW
        HAL_GPIO_WritePin(row_pins[row].port, row_pins[row].pin, GPIO_PIN_RESET);

        // Small delay for signal to settle
        HAL_Delay(1);

        // Check all columns
        for (int col = 0; col < 4; col++) {
            GPIO_PinState state = HAL_GPIO_ReadPin(col_pins[col].port, col_pins[col].pin);
            if (state == GPIO_PIN_RESET) {  // Button pressed (pulled low)
                HAL_Delay(50);
                // Wait for button release
                while (HAL_GPIO_ReadPin(col_pins[col].port, col_pins[col].pin) == GPIO_PIN_RESET) {
                    HAL_Delay(10);
                }
                return keymap[row][col];
            }
        }
    }
    return 0; // No key pressed
}
