#ifndef KEYPAD_H
#define KEYPAD_H

#include "main.h"

/* Updated pin assignments - avoiding PA4, PB4, PB5, PB6 */
/* Rows now use mixed ports */
#define KEYPAD_ROW_PORT_A GPIOA
#define KEYPAD_ROW_PORT_C GPIOC

// Columns also use mixed ports
#define KEYPAD_COL_PORT_B GPIOB
#define KEYPAD_COL_PORT_C GPIOC

void keypad_init(void);
char keypad_getkey(void);

#endif
