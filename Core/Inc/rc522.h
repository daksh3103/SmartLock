#ifndef __RC522_H
#define __RC522_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

extern SPI_HandleTypeDef hspi2; // make sure hspi2 is declared in main.c

// Pin mapping - change if needed
#define RC522_NSS_PORT     GPIOB
#define RC522_NSS_PIN      GPIO_PIN_12
#define RC522_RST_PORT     GPIOB
#define RC522_RST_PIN      GPIO_PIN_1

// MFRC522 commands
#define PCD_IDLE           0x00
#define PCD_AUTHENT        0x0E
#define PCD_RECEIVE        0x08
#define PCD_TRANSMIT       0x04
#define PCD_TRANSCEIVE     0x0C
#define PCD_RESETPHASE     0x0F
#define PCD_CALCCRC        0x03

// MFRC522 Registers (subset used)
#define CommandReg         0x01
#define CommIEnReg         0x02
#define DivIEnReg          0x03
#define CommIrqReg         0x04
#define DivIrqReg          0x05
#define ErrorReg           0x06
#define Status1Reg         0x07
#define Status2Reg         0x08
#define FIFODataReg        0x09
#define FIFOLevelReg       0x0A
#define ControlReg         0x0C
#define BitFramingReg      0x0D
#define ModeReg            0x11
#define TxModeReg          0x12
#define RxModeReg          0x13
#define TxControlReg       0x14
#define TxASKReg           0x15
#define TModeReg           0x2A
#define TPrescalerReg      0x2B
#define TReloadRegH        0x2C
#define TReloadRegL        0x2D
#define CRCResultRegL      0x22
#define CRCResultRegH      0x21
#define VersionReg         0x37

// Mifare commands
#define PICC_REQIDL        0x26
#define PICC_REQALL        0x52
#define PICC_ANTICOLL      0x93
#define PICC_SElECTTAG     0x93
#define PICC_AUTHENT1A     0x60
#define PICC_AUTHENT1B     0x61
#define PICC_READ          0x30
#define PICC_WRITE         0xA0
#define PICC_HALT          0x50

// Status
#define MI_OK              0
#define MI_NOTAGERR        1
#define MI_ERR             2

// API
void MFRC522_Init(void);
void MFRC522_Reset(void);
void MFRC522_AntennaOn(void);
uint8_t MFRC522_Request(uint8_t reqMode, uint8_t *TagType);
uint8_t MFRC522_Anticoll(uint8_t *serNum);
uint8_t MFRC522_Check(uint8_t *id);

#endif /* __RC522_H */
