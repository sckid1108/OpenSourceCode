/*
 * mcp23s18.c
 *
 *  Created on: Aug 27, 2024
 *      Author: Michael Margolese
 */

#include <string.h>
#include "MCP23S18.h"

const uint8_t WRITE_CMD = ((0x20 << 1) | 0x0);
const uint8_t READ_CMD  = ((0x20 << 1) | 0x1);

void MCP23S18_WriteReg(sMCP23S18_Instance* inst, uint8_t addr, uint8_t data)
{
  inst->cs_select();

  if ( HAL_SPI_Transmit(inst->spi, &WRITE_CMD, 1, HAL_MAX_DELAY) != HAL_OK)
  {
      Error_Handler((uint8_t *)__FILE__, __LINE__);
  }

  if ( HAL_SPI_Transmit(inst->spi, &addr, 1, HAL_MAX_DELAY) != HAL_OK)
  {
      Error_Handler((uint8_t *)__FILE__, __LINE__);
  }

  if ( HAL_SPI_Transmit(inst->spi, &data, 1, HAL_MAX_DELAY) != HAL_OK)
  {
      Error_Handler((uint8_t *)__FILE__, __LINE__);
  }

  inst->cs_unselect();
}

uint8_t MCP23S18_ReadReg(sMCP23S18_Instance* inst, uint8_t addr)
{
  uint8_t data = 0;

  inst->cs_select();

  if ( HAL_SPI_Transmit(inst->spi, &READ_CMD, 1, HAL_MAX_DELAY) != HAL_OK)
  {
      Error_Handler((uint8_t *)__FILE__, __LINE__);
  }

  if ( HAL_SPI_Transmit(inst->spi, &addr, 1, HAL_MAX_DELAY) != HAL_OK)
  {
      Error_Handler((uint8_t *)__FILE__, __LINE__);
  }

  if ( HAL_SPI_Receive(inst->spi, &data, 1, HAL_MAX_DELAY) != HAL_OK)
  {
      Error_Handler((uint8_t *)__FILE__, __LINE__);
  }

  inst->cs_unselect();

  return data;
}


void Init_MCP23S18(sMCP23S18_Instance* inst, SPI_HandleTypeDef *spi_port, nss_spi_ptr cs_select_func, nss_spi_ptr cs_unselect_func)
{
  memset(inst, 0, sizeof(sMCP23S18_Instance));

  inst->spi = spi_port;
  inst->cs_select = cs_select_func;
  inst->cs_unselect = cs_unselect_func;

  // Configure IOCON - IO Expander Configuration Register
  //inst->rIOCONA = MCP23S18_ReadReg(inst, IOCONA_ADDR);
  //MCP23S18_WriteReg(inst, IOCONA_ADDR, 0xE0); // BANK 1, MIRROR (INT WIREDOR), SEQ DISABLED, x, x, ACTIVE DRIVE, ACTIVE LOW, READ GPIO CLEAR INTERRUPT
  //inst->rIOCONA = MCP23S18_ReadReg(inst, IOCONA_ADDR);

  //inst->rIOCONB = MCP23S18_ReadReg(inst, IOCONB_ADDR);
  //MCP23S18_WriteReg(inst, IOCONB_ADDR, 0xE0);
  //inst->rIOCONB = MCP23S18_ReadReg(inst, IOCONB_ADDR);

  // Read all registers default values
  /*
  MCP23S18_WriteReg(inst, IODIRA_ADDR, 255);
  MCP23S18_WriteReg(inst, IPOLA_ADDR, 0);
  MCP23S18_WriteReg(inst, GPINTENA_ADDR, 0);
  MCP23S18_WriteReg(inst, DEFVALA_ADDR, 0);
  MCP23S18_WriteReg(inst, INTCONA_ADDR, 0);
  MCP23S18_WriteReg(inst, GPPUA_ADDR, 0);
  MCP23S18_WriteReg(inst, INTFA_ADDR, 0);
  MCP23S18_WriteReg(inst, INTCAPA_ADDR, 0);
  MCP23S18_WriteReg(inst, GPIOA_ADDR, 0);
  MCP23S18_WriteReg(inst, OLATA_ADDR, 0);
  MCP23S18_WriteReg(inst, IODIRB_ADDR, 255);
  MCP23S18_WriteReg(inst, IPOLB_ADDR, 0);
  MCP23S18_WriteReg(inst, GPINTENB_ADDR, 0);
  MCP23S18_WriteReg(inst, INTCONB_ADDR, 0);
  MCP23S18_WriteReg(inst, GPPUB_ADDR, 0);
  MCP23S18_WriteReg(inst, INTFB_ADDR, 0);
  MCP23S18_WriteReg(inst, INTCAB_ADDR, 0);
  MCP23S18_WriteReg(inst, GPIOB_ADDR, 0);
  MCP23S18_WriteReg(inst, OLATB_ADDR, 0);
  */

  inst->rIODIRA   = MCP23S18_ReadReg(inst, IODIRA_ADDR);
  inst->rIPOLA    = MCP23S18_ReadReg(inst, IPOLA_ADDR);
  inst->rGPINTENA = MCP23S18_ReadReg(inst, GPINTENA_ADDR);
  inst->rDEFVALA  = MCP23S18_ReadReg(inst, DEFVALA_ADDR);
  inst->rINTCONA  = MCP23S18_ReadReg(inst, INTCONA_ADDR);
  inst->rGPPUA    = MCP23S18_ReadReg(inst, GPPUA_ADDR);
  inst->rINTFA    = MCP23S18_ReadReg(inst, INTFA_ADDR);
  inst->rINTCAPA  = MCP23S18_ReadReg(inst, INTCAPA_ADDR);
  inst->rGPIOA    = MCP23S18_ReadReg(inst, GPIOA_ADDR);
  inst->rOLATA    = MCP23S18_ReadReg(inst, OLATA_ADDR);
  inst->rIODIRB   = MCP23S18_ReadReg(inst, IODIRB_ADDR);
  inst->rIPOLB    = MCP23S18_ReadReg(inst, IPOLB_ADDR);
  inst->rGPINTENB = MCP23S18_ReadReg(inst, GPINTENB_ADDR);
  inst->rDEFVALB  = MCP23S18_ReadReg(inst, DEFVALB_ADDR);
  inst->rINTCONB  = MCP23S18_ReadReg(inst, INTCONB_ADDR);
  inst->rGPPUB    = MCP23S18_ReadReg(inst, GPPUB_ADDR);
  inst->rINTFB    = MCP23S18_ReadReg(inst, INTFB_ADDR);
  inst->rINTCAB   = MCP23S18_ReadReg(inst, INTCAB_ADDR);
  inst->rGPIOB    = MCP23S18_ReadReg(inst, GPIOB_ADDR);
  inst->rOLATB    = MCP23S18_ReadReg(inst, OLATB_ADDR);

}

