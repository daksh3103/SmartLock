#include "rc522.h"
#include <string.h>

extern SPI_HandleTypeDef hspi2; // ensure main.c defines hspi2

// Helper macros for CS/RST
#define RC522_CS_LOW()   HAL_GPIO_WritePin(RC522_NSS_PORT, RC522_NSS_PIN, GPIO_PIN_RESET)
#define RC522_CS_HIGH()  HAL_GPIO_WritePin(RC522_NSS_PORT, RC522_NSS_PIN, GPIO_PIN_SET)
#define RC522_RST_LOW()  HAL_GPIO_WritePin(RC522_RST_PORT, RC522_RST_PIN, GPIO_PIN_RESET)
#define RC522_RST_HIGH() HAL_GPIO_WritePin(RC522_RST_PORT, RC522_RST_PIN, GPIO_PIN_SET)

// Low-level read/write register
static void RC522_WriteReg(uint8_t addr, uint8_t val) {
    uint8_t buf[2];
    buf[0] = (addr << 1) & 0x7E; // address format for write
    buf[1] = val;
    RC522_CS_LOW();
    HAL_SPI_Transmit(&hspi2, buf, 2, HAL_MAX_DELAY);
    RC522_CS_HIGH();
}

static uint8_t RC522_ReadReg(uint8_t addr) {
    uint8_t tx = ((addr << 1) & 0x7E) | 0x80; // read address
    uint8_t rx = 0;
    RC522_CS_LOW();
    HAL_SPI_Transmit(&hspi2, &tx, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi2, &rx, 1, HAL_MAX_DELAY);
    RC522_CS_HIGH();
    return rx;
}

static void RC522_SetBitMask(uint8_t reg, uint8_t mask) {
    RC522_WriteReg(reg, RC522_ReadReg(reg) | mask);
}
static void RC522_ClearBitMask(uint8_t reg, uint8_t mask) {
    RC522_WriteReg(reg, RC522_ReadReg(reg) & (~mask));
}

// Calculate CRC using MFRC522's built-in CRC calculator
static void RC522_CalculateCRC(uint8_t *data, uint8_t len, uint8_t *result) {
    RC522_ClearBitMask(DivIrqReg, 0x04);      // Clear CRCIRq
    RC522_SetBitMask(FIFOLevelReg, 0x80);     // Clear FIFO pointer
    for (uint8_t i = 0; i < len; i++) {
        RC522_WriteReg(FIFODataReg, data[i]);
    }
    RC522_WriteReg(CommandReg, PCD_CALCCRC);
    // wait for CRC calculation
    uint16_t i = 5000;
    uint8_t n;
    do {
        n = RC522_ReadReg(DivIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x04));
    result[0] = RC522_ReadReg(CRCResultRegL);
    result[1] = RC522_ReadReg(CRCResultRegH);
}

// Core transceive function
static uint8_t MFRC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint16_t *backLen) {
    uint8_t status = MI_ERR;
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
    uint8_t lastBits;
    uint8_t n;
    uint16_t i;

    if (command == PCD_AUTHENT) {
        irqEn = 0x12;
        waitIRq = 0x10;
    }
    if (command == PCD_TRANSCEIVE) {
        irqEn = 0x77;
        waitIRq = 0x30;
    }

    RC522_WriteReg(CommIEnReg, irqEn | 0x80);  // Allow interrupts
    RC522_ClearBitMask(CommIrqReg, 0x80);      // Clear IRQ
    RC522_SetBitMask(FIFOLevelReg, 0x80);      // Flush FIFO

    // Write data to FIFO
    for (i = 0; i < sendLen; i++) {
        RC522_WriteReg(FIFODataReg, sendData[i]);
    }

    RC522_WriteReg(CommandReg, command);
    if (command == PCD_TRANSCEIVE) {
        RC522_SetBitMask(BitFramingReg, 0x80); // StartSend=1, transmission of data starts
    }

    // Wait for completion
    i = 2000;
    do {
        n = RC522_ReadReg(CommIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

    RC522_ClearBitMask(BitFramingReg, 0x80);

    if (i != 0) {
        if (!(RC522_ReadReg(ErrorReg) & 0x1B)) {
            status = MI_OK;
            if (n & irqEn & 0x01) {
                status = MI_NOTAGERR;
            }
            if (command == PCD_TRANSCEIVE) {
                n = RC522_ReadReg(FIFOLevelReg);
                lastBits = RC522_ReadReg(ControlReg) & 0x07;
                if (lastBits) {
                    *backLen = (n - 1) * 8 + lastBits;
                } else {
                    *backLen = n * 8;
                }
                if (n == 0) n = 1;
                if (n > 16) n = 16;
                for (i = 0; i < n; i++) {
                    backData[i] = RC522_ReadReg(FIFODataReg);
                }
            }
        } else {
            status = MI_ERR;
        }
    }
    return status;
}

// Public functions
void MFRC522_Reset(void) {
    RC522_WriteReg(CommandReg, PCD_RESETPHASE);
}

void MFRC522_AntennaOn(void) {
    uint8_t temp = RC522_ReadReg(TxControlReg);
    if (!(temp & 0x03)) {
        RC522_SetBitMask(TxControlReg, 0x03);
    }
}

void MFRC522_Init(void) {
    // Hardware reset: toggle RST pin if available
    RC522_RST_LOW();
    HAL_Delay(50);
    RC522_RST_HIGH();
    HAL_Delay(50);

    MFRC522_Reset();

    RC522_WriteReg(TModeReg, 0x8D);
    RC522_WriteReg(TPrescalerReg, 0x3E);
    RC522_WriteReg(TReloadRegL, 30);
    RC522_WriteReg(TReloadRegH, 0);
    RC522_WriteReg(TxASKReg, 0x40);
    RC522_WriteReg(ModeReg, 0x3D);

    MFRC522_AntennaOn();
}

// Request to find cards in the field
uint8_t MFRC522_Request(uint8_t reqMode, uint8_t *TagType) {
    uint8_t status;
    uint16_t backBits;
    uint8_t buf[1];

    RC522_WriteReg(BitFramingReg, 0x07); // TxLastBits = 7

    buf[0] = reqMode;
    status = MFRC522_ToCard(PCD_TRANSCEIVE, buf, 1, TagType, &backBits);
    if ((status != MI_OK) || (backBits != 0x10)) {
        status = MI_ERR;
    }
    return status;
}

// Anti-collision to get UID
uint8_t MFRC522_Anticoll(uint8_t *serNum) {
    uint8_t status;
    uint8_t i;
    uint8_t serNumCheck = 0;
    uint16_t unLen;
    uint8_t buf[2];

    RC522_WriteReg(BitFramingReg, 0x00); // Reset bit framing

    buf[0] = PICC_ANTICOLL;
    buf[1] = 0x20;

    status = MFRC522_ToCard(PCD_TRANSCEIVE, buf, 2, serNum, &unLen);

    if (status == MI_OK) {
        // serNum contains 5 bytes, last is BCC
        for (i = 0; i < 4; i++) {
            serNumCheck ^= serNum[i];
        }
        if (serNumCheck != serNum[4]) {
            status = MI_ERR;
        }
    }
    return status;
}

// High-level check (Request + Anticollision) to get UID
uint8_t MFRC522_Check(uint8_t *id) {
    uint8_t status;
    uint8_t TagType[2];

    status = MFRC522_Request(PICC_REQIDL, TagType);
    if (status == MI_OK) {
        status = MFRC522_Anticoll(id);
    }
    return status;
}
