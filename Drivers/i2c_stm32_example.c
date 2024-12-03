// For use with I2C based drivers.  You can follow the same pattern for PIC, ARDUINO, RASPBI PICO, etc.
#include <stdint.h>
#include <i2c.h>

int i2c1_write(uint16_t addr, uint8_t reg, uint8_t* data, uint8_t len)
{	
	HAL_StatusTypeDef state = HAL_I2C_Mem_Write(&hi2c1, (addr << 1), reg, I2C_MEMADD_SIZE_8BIT, data, len, 100);
	return (int) state;
}

int i2c1_read(uint16_t addr, uint8_t reg, uint8_t* data, uint8_t len)
{	
	HAL_StatusTypeDef state = HAL_I2C_Mem_Read(&hi2c1, (addr << 1), reg, I2C_MEMADD_SIZE_8BIT, data, len, 100);
	return (int) state;
}
