# M95320-EEPROM-Library-STM32
M95320 EEPROM Library for STM32 Platform

Example Usage:
```
#include "string.h"
#include "M95320.h"

SPI_HandleTypeDef hspi1;
M95320_HandleTypeDef eeprom = {
    .spi = &hspi1,
    .cs_port = GPIOA,
    .cs_pin = GPIO_PIN_4,
    .write_protect_port = GPIOA,
    .write_protect_pin = GPIO_PIN_5,
    .hold_port = GPIOA,
    .hold_pin = GPIO_PIN_6
};

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();

    M95320_Hold(&eeprom, DEVICE_RELEASE);
    M95320_WriteProtect(&eeprom, WRITE);
  
    uint8_t tx_data[32] = "M95320 EEPROM";
    uint8_t rx_data[32] = {0};

    uint8_t test_done = 0;

    while (1)
    {
     if(!test_done && M95320_WriteMemory(&eeprom, 0x0000, tx_data, strlen((char*)tx_data)) == HAL_OK && M95320_ReadMemory(&eeprom, 0x0000, rx_data, strlen((char*)tx_data)) == HAL_OK) test_done = 1;

      HAL_Delay(1000); 
    }
}
```
For questions, improvements, or bug reports, please feel free to reach out.

