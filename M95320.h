#ifndef INC_M95320_H_
#define INC_M95320_H_

#if defined(STM32F412Vx) || defined(STM32F446xx)
    #include "stm32f4xx_hal.h"
#elif defined(STM32G030xx) || defined(STM32G0B0xx) || defined(STM32G070xx)
    #include "stm32g0xx_hal.h"
#elif defined(STM32G431xx)
    #include "stm32g4xx_hal.h"
#else
	#error "MCU not supported. Contact for support"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define M95320_CMD_WREN     0x06  /* Write Enable */
#define M95320_CMD_WRDI     0x04  /* Write Disable */
#define M95320_CMD_RDSR     0x05  /* Read Status Register */
#define M95320_CMD_WRSR     0x01  /* Write Status Register */
#define M95320_CMD_READ     0x03  /* Read from Memory Array */
#define M95320_CMD_WRITE    0x02  /* Write to Memory Array */

/* -------------------------------------------------
 * Identification Page Instructions
 * (Available only for M95320-D)
 * ------------------------------------------------- */
 
#define M95320_CMD_RDID     0x83  /* Read Identification Page */
#define M95320_CMD_WRID     0x82  /* Write Identification Page */

#define M95320_SR_WIP   (1U << 0)
#define M95320_SR_WEL   (1U << 1)
#define M95320_SR_BP0   (1U << 2)
#define M95320_SR_BP1   (1U << 3)

#define M95320_PAGE_SIZE  32
#define M95320_SIZE       4096

#define M95320_WRITE_TIMEOUT_MS  8

#define M95320_ID_ADDR_MSB     0x00
#define M95320_ID_ADDR_LSB     0x00   

#define M95320_LS_ADDR_MSB     0x04
#define M95320_LS_ADDR_LSB     0x00

#define M95320_ID_PAGE_SIZE   32

#define M95320_SPI_TIMEOUT 100

typedef enum{
	DEVICE_HOLD = 0,
	DEVICE_RELEASE = 1,
}m95320_hold_t;

typedef enum{
	NONE = 0x00,
	UPPER_QUARTER = 0x01, /* 0C00h - 0FFFh */
	UPPER_HALF = 0x02, /* 0800h - 0FFFh */
	WHOLE_MEMORY = 0x03, /* 0000h - 0FFFh */
}m95320_block_protect_t;

typedef enum{
	READ_ONLY = 0,
	WRITE = 1,
}m95320_write_protect_t;

typedef struct{
	SPI_HandleTypeDef *spi;
	
	GPIO_TypeDef *cs_port;
  uint16_t      cs_pin;
	
	GPIO_TypeDef *write_protect_port;
	uint16_t write_protect_pin;
	
	GPIO_TypeDef *hold_port;
	uint16_t hold_pin;
}M95320_HandleTypeDef;


HAL_StatusTypeDef M95320_Hold(M95320_HandleTypeDef *m, m95320_hold_t hold);
HAL_StatusTypeDef M95320_WriteProtect(M95320_HandleTypeDef *m, m95320_write_protect_t wp);

HAL_StatusTypeDef M95320_ReadStatusRegister(M95320_HandleTypeDef *m, uint8_t *status);
HAL_StatusTypeDef M95320_WriteStatusRegister(M95320_HandleTypeDef *m,m95320_block_protect_t block);

HAL_StatusTypeDef M95320_ReadMemory(M95320_HandleTypeDef *m, uint16_t address, uint8_t *data, uint16_t length);
HAL_StatusTypeDef M95320_WriteMemory(M95320_HandleTypeDef *m, uint16_t address, uint8_t *data, uint16_t length);

/* -----------------------------
 * Identification page (M95320-D)
 * ----------------------------- */
HAL_StatusTypeDef M95320_ReadIDPage(M95320_HandleTypeDef *m, uint8_t offset, uint8_t *data, uint16_t length);
HAL_StatusTypeDef M95320_WriteIDPage(M95320_HandleTypeDef *m, uint8_t offset, uint8_t *data, uint16_t length);
HAL_StatusTypeDef M95320_ReadIDLockStatus(M95320_HandleTypeDef *m, uint8_t *locked);
HAL_StatusTypeDef M95320_LockIDPage(M95320_HandleTypeDef *m);

#endif