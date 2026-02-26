#include "M95320.h"

static inline HAL_StatusTypeDef M95320_RequireCS(const M95320_HandleTypeDef *m)
{
    if (!m || !m->spi || !m->cs_port) return HAL_ERROR;
    return HAL_OK;
}

static inline void M95320_ChipSelectLow(M95320_HandleTypeDef *m)
{
	HAL_GPIO_WritePin(m->cs_port,m->cs_pin,GPIO_PIN_RESET);
}

static inline void M95320_ChipSelectHigh(M95320_HandleTypeDef *m)
{
	HAL_GPIO_WritePin(m->cs_port,m->cs_pin,GPIO_PIN_SET);
}

HAL_StatusTypeDef M95320_Hold(M95320_HandleTypeDef *m, m95320_hold_t hold)
{
	if (!m || !m->hold_port) return HAL_ERROR;
	
	if (HAL_GPIO_ReadPin(m->cs_port, m->cs_pin) == GPIO_PIN_RESET) return HAL_BUSY;
	
	if(hold == DEVICE_HOLD){
		
		HAL_GPIO_WritePin(m->hold_port,m->hold_pin,GPIO_PIN_RESET);
		
	} else if (hold == DEVICE_RELEASE){
		
		HAL_GPIO_WritePin(m->hold_port,m->hold_pin,GPIO_PIN_SET);
		
	} else {
		
		return HAL_ERROR;
		
	}
	
	return HAL_OK;
}

HAL_StatusTypeDef M95320_WriteProtect(M95320_HandleTypeDef *m, m95320_write_protect_t wp)
{
	if (M95320_RequireCS(m) != HAL_OK) return HAL_ERROR;

  if (!m->write_protect_port) return HAL_ERROR;
	
	if (HAL_GPIO_ReadPin(m->cs_port, m->cs_pin) == GPIO_PIN_RESET) return HAL_BUSY;

	if (wp == READ_ONLY){
		
		HAL_GPIO_WritePin(m->write_protect_port,m->write_protect_pin,GPIO_PIN_RESET);
		
	} else if (wp == WRITE) {
		
		HAL_GPIO_WritePin(m->write_protect_port,m->write_protect_pin,GPIO_PIN_SET);
		
	} else {
		
		return HAL_ERROR;
		
	}
	
	return HAL_OK;
}

HAL_StatusTypeDef M95320_ReadStatusRegister(M95320_HandleTypeDef *m, uint8_t *status)
{
    uint8_t cmd = M95320_CMD_RDSR;
	
	  if (M95320_RequireCS(m) != HAL_OK || !status) return HAL_ERROR;

    M95320_ChipSelectLow(m);

    if (HAL_SPI_Transmit(m->spi, &cmd, 1, M95320_SPI_TIMEOUT) != HAL_OK || HAL_SPI_Receive(m->spi, status, 1, M95320_SPI_TIMEOUT) != HAL_OK)
    {
        M95320_ChipSelectHigh(m);
			
        return HAL_ERROR;
    }

    M95320_ChipSelectHigh(m);
		
    return HAL_OK;
}

static HAL_StatusTypeDef M95320_WriteDisable(M95320_HandleTypeDef *m)
{
    uint8_t cmd = M95320_CMD_WRDI;

    if (M95320_RequireCS(m) != HAL_OK) return HAL_ERROR;

    M95320_ChipSelectLow(m);
	
    if (HAL_SPI_Transmit(m->spi, &cmd, 1, M95320_SPI_TIMEOUT) != HAL_OK)
    {
        M95320_ChipSelectHigh(m);
			
        return HAL_ERROR;
    }
		
    M95320_ChipSelectHigh(m);

    return HAL_OK;
}

static HAL_StatusTypeDef M95320_WriteEnable(M95320_HandleTypeDef *m){
	uint8_t cmd = M95320_CMD_WREN;
	
	M95320_ChipSelectLow(m);
	
	if (HAL_SPI_Transmit(m->spi, &cmd, 1, M95320_SPI_TIMEOUT) != HAL_OK)
	{
			M95320_ChipSelectHigh(m);
		
			return HAL_ERROR;
	}
	
	M95320_ChipSelectHigh(m);
	
	return HAL_OK;
}

static HAL_StatusTypeDef M95320_WaitWhileBusy(M95320_HandleTypeDef *m)
{
    uint8_t status;
    uint32_t start;

    if (M95320_RequireCS(m) != HAL_OK) return HAL_ERROR;

    start = HAL_GetTick();

    while (1)
    {
        if (M95320_ReadStatusRegister(m, &status) != HAL_OK) return HAL_ERROR;

        if (!(status & M95320_SR_WIP))
            break;

        if ((HAL_GetTick() - start) > M95320_WRITE_TIMEOUT_MS) return HAL_TIMEOUT;
				
				HAL_Delay(1);
    }

    return HAL_OK;
}

HAL_StatusTypeDef M95320_WriteStatusRegister(M95320_HandleTypeDef *m, m95320_block_protect_t block)
{
	uint8_t status;
	
	if (M95320_RequireCS(m) != HAL_OK) return HAL_ERROR;
	
	if (m->write_protect_port && HAL_GPIO_ReadPin(m->write_protect_port, m->write_protect_pin) == GPIO_PIN_RESET) return HAL_ERROR;
	
	if (M95320_ReadStatusRegister(m, &status) != HAL_OK) return HAL_ERROR;
	
	status &= ~(M95320_SR_BP0 | M95320_SR_BP1);
	
  status |= ((block & 0x03) << 2);
	
	M95320_WriteEnable(m);
	
	uint8_t tx[2] = {M95320_CMD_WRSR, status};
	
	M95320_ChipSelectLow(m);
	
	if (HAL_SPI_Transmit(m->spi, tx, 2, M95320_SPI_TIMEOUT) != HAL_OK)
	{
			M95320_ChipSelectHigh(m);
		
			return HAL_ERROR;
	}
	
	M95320_ChipSelectHigh(m);
	
	if (M95320_WaitWhileBusy(m) != HAL_OK) return HAL_TIMEOUT;

	if (M95320_WriteDisable(m) != HAL_OK) return HAL_ERROR;
	
	return HAL_OK;
}

HAL_StatusTypeDef M95320_ReadMemory(M95320_HandleTypeDef *m, uint16_t address,uint8_t *data,uint16_t length)
{
	
	uint8_t tx[3];
	
	if (M95320_RequireCS(m) != HAL_OK || !data || !length) return HAL_ERROR;
	
	if ((address + length) > M95320_SIZE) return HAL_ERROR;
	
	tx[0] = M95320_CMD_READ;
	tx[1] = (uint8_t)(address >> 8);
	tx[2] = (uint8_t)(address & 0xFF);
	
	M95320_ChipSelectLow(m);
	
	if (HAL_SPI_Transmit(m->spi, tx, 3, M95320_SPI_TIMEOUT) != HAL_OK)
	{
			M95320_ChipSelectHigh(m);
			return HAL_ERROR;
	}
	
	if(HAL_SPI_Receive(m->spi,data,length,M95320_SPI_TIMEOUT) != HAL_OK)
	{
		M95320_ChipSelectHigh(m);
			return HAL_ERROR;
	}
	
	M95320_ChipSelectHigh(m);
	
	return HAL_OK;
}

HAL_StatusTypeDef M95320_WriteMemory(M95320_HandleTypeDef *m, uint16_t address,uint8_t *data,uint16_t length){
	
	uint8_t tx[3];
	uint8_t status;
	
	if (M95320_RequireCS(m) != HAL_OK || !data || !length) return HAL_ERROR;
	
	if ((address + length) > M95320_SIZE) return HAL_ERROR;
	
	if ((address % M95320_PAGE_SIZE) + length > M95320_PAGE_SIZE) return HAL_ERROR;
	
	if (m->write_protect_port && HAL_GPIO_ReadPin(m->write_protect_port, m->write_protect_pin) == GPIO_PIN_RESET) return HAL_ERROR;
	
	if (M95320_ReadStatusRegister(m, &status) != HAL_OK) return HAL_ERROR;
	
	uint8_t bp = (status >> 2) & 0x03;
	
	if ((bp == WHOLE_MEMORY) || (bp == UPPER_HALF && address < 0x800) || (bp == UPPER_QUARTER && address < 0xC00)) return HAL_ERROR;

	if (status & M95320_SR_WIP) return HAL_BUSY;
	
	M95320_WriteEnable(m);
	
	if (M95320_ReadStatusRegister(m, &status) != HAL_OK) return HAL_ERROR;

	if (!(status & M95320_SR_WEL)) return HAL_ERROR;
	
	tx[0] = M95320_CMD_WRITE;
	tx[1] = (uint8_t)(address >> 8);
	tx[2] = (uint8_t)(address & 0xFF);
	
	M95320_ChipSelectLow(m);
	
	if (HAL_SPI_Transmit(m->spi, tx, 3, M95320_SPI_TIMEOUT) != HAL_OK || HAL_SPI_Transmit(m->spi,data,length,M95320_SPI_TIMEOUT) != HAL_OK)
	{
			M95320_ChipSelectHigh(m);
			return HAL_ERROR;
	}
	
	M95320_ChipSelectHigh(m);
	
	if (M95320_WaitWhileBusy(m) != HAL_OK) return HAL_TIMEOUT;
	
	if (M95320_WriteDisable(m) != HAL_OK) return HAL_ERROR;
	
	return HAL_OK;
}
	
HAL_StatusTypeDef M95320_ReadIDPage(M95320_HandleTypeDef *m, uint8_t offset, uint8_t *data, uint16_t length)
{
    uint8_t tx[3];

    if (M95320_RequireCS(m) != HAL_OK || !data || !length) return HAL_ERROR;

    if ((offset + length) > M95320_ID_PAGE_SIZE) return HAL_ERROR;

    tx[0] = M95320_CMD_RDID;
    tx[1] = M95320_ID_ADDR_MSB;
    tx[2] = M95320_ID_ADDR_LSB + offset;

    M95320_ChipSelectLow(m);

    if (HAL_SPI_Transmit(m->spi, tx, 3, M95320_SPI_TIMEOUT) != HAL_OK)
		{
			M95320_ChipSelectHigh(m);
			
      return HAL_ERROR;
		}   

    if (HAL_SPI_Receive(m->spi, data, length, M95320_SPI_TIMEOUT) != HAL_OK)
		{
      M95320_ChipSelectHigh(m);
			
			return HAL_ERROR;
		}

    M95320_ChipSelectHigh(m);
    return HAL_OK;
}

HAL_StatusTypeDef M95320_WriteIDPage(M95320_HandleTypeDef *m, uint8_t offset, uint8_t *data, uint16_t length)
{
    uint8_t tx[3];
	  uint8_t status;

    if (M95320_RequireCS(m) != HAL_OK || !data || !length) return HAL_ERROR;

    if ((offset + length) > M95320_ID_PAGE_SIZE) return HAL_ERROR;

    if (m->write_protect_port && HAL_GPIO_ReadPin(m->write_protect_port, m->write_protect_pin) == GPIO_PIN_RESET) return HAL_ERROR;
	
		if (M95320_WaitWhileBusy(m) != HAL_OK) return HAL_BUSY;
	
		if (M95320_WriteEnable(m) != HAL_OK) return HAL_ERROR;
	
		if (M95320_ReadStatusRegister(m, &status) != HAL_OK) return HAL_ERROR;

		if (!(status & M95320_SR_WEL)) return HAL_ERROR;

    tx[0] = M95320_CMD_WRID;
    tx[1] = M95320_ID_ADDR_MSB;
    tx[2] = M95320_ID_ADDR_LSB + offset;

    M95320_ChipSelectLow(m);
		
    if (HAL_SPI_Transmit(m->spi, tx, 3, M95320_SPI_TIMEOUT) != HAL_OK)
		{
      M95320_ChipSelectHigh(m);
			
			return HAL_ERROR;
		}

    if (HAL_SPI_Transmit(m->spi, data, length, M95320_SPI_TIMEOUT) != HAL_OK)
		{
      M95320_ChipSelectHigh(m);
			
			return HAL_ERROR;
		}

    M95320_ChipSelectHigh(m);

		if (M95320_WaitWhileBusy(m) != HAL_OK) return HAL_TIMEOUT;
		
		if (M95320_WriteDisable(m) != HAL_OK) return HAL_ERROR;

    return HAL_OK;

}

HAL_StatusTypeDef M95320_ReadIDLockStatus(M95320_HandleTypeDef *m, uint8_t *locked)
{
    uint8_t tx[3];
    uint8_t rx;

    if (M95320_RequireCS(m) != HAL_OK || (locked == NULL)) return HAL_ERROR;

    tx[0] = M95320_CMD_RDID;
    tx[1] = M95320_LS_ADDR_MSB;
    tx[2] = M95320_LS_ADDR_LSB;

    M95320_ChipSelectLow(m);

    if (HAL_SPI_Transmit(m->spi, tx, 3, M95320_SPI_TIMEOUT) != HAL_OK)
		{
      M95320_ChipSelectHigh(m);
			
			return HAL_ERROR;
		}

    if (HAL_SPI_Receive(m->spi, &rx, 1, M95320_SPI_TIMEOUT) != HAL_OK)
		{
      M95320_ChipSelectHigh(m);
			
			return HAL_ERROR;
		}

    M95320_ChipSelectHigh(m);

    *locked = (rx & 0x01);
		
    return HAL_OK;
}

HAL_StatusTypeDef M95320_LockIDPage(M95320_HandleTypeDef *m)
{
    uint8_t tx[3];
		uint8_t status;

    if (M95320_RequireCS(m) != HAL_OK) return HAL_ERROR;
	
	  if (m->write_protect_port && HAL_GPIO_ReadPin(m->write_protect_port, m->write_protect_pin) == GPIO_PIN_RESET) return HAL_ERROR;
	
		if (M95320_WaitWhileBusy(m) != HAL_OK) return HAL_BUSY;

    if (M95320_WriteEnable(m) != HAL_OK) return HAL_ERROR;
	
		if (M95320_ReadStatusRegister(m, &status) != HAL_OK) return HAL_ERROR;

    if (!(status & M95320_SR_WEL)) return HAL_ERROR;

    tx[0] = M95320_CMD_WRID;
    tx[1] = M95320_LS_ADDR_MSB;
    tx[2] = M95320_LS_ADDR_LSB;

    M95320_ChipSelectLow(m);
    if (HAL_SPI_Transmit(m->spi, tx, 3, M95320_SPI_TIMEOUT) != HAL_OK)
		{
      M95320_ChipSelectHigh(m);
			
			return HAL_ERROR;
		}
    M95320_ChipSelectHigh(m);
		
		if (M95320_WaitWhileBusy(m) != HAL_OK) return HAL_TIMEOUT;
		
		if (M95320_WriteDisable(m) != HAL_OK) return HAL_ERROR;

    return HAL_OK;
}
