/*
 * mcp23s18.h
 *
 *  Created on: Aug 27, 2024
 *      Author: Michael Margolese
 */
#pragma once

#ifndef MCP23S18_H_
#define MCP23S18_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "spi.h"

//BANK = 1
/*
#define IODIRA_ADDR   0x00
#define IPOLA_ADDR    0x01
#define GPINTENA_ADDR 0x02
#define DEFVALA_ADDR  0x03
#define INTCONA_ADDR  0x04
#define IOCONA_ADDR   0x05
#define GPPUA_ADDR    0x06
#define INTFA_ADDR    0x07
#define INTCAPA_ADDR  0x08
#define GPIOA_ADDR    0x09
#define OLATA_ADDR    0x0A

#define IODIRB_ADDR   0x10
#define IPOLB_ADDR    0x11
#define GPINTENB_ADDR 0x12
#define DEFVALB_ADDR  0x13
#define INTCONB_ADDR  0x14
#define IOCONB_ADDR   0x15
#define GPPUB_ADDR    0x16
#define INTFB_ADDR    0x17
#define INTCAB_ADDR   0x18
#define GPIOB_ADDR    0x19
#define OLATB_ADDR    0x1A
*/

// BANK 0 MODE
#define IODIRA_ADDR   0x00
#define IODIRB_ADDR   0x01

#define IPOLA_ADDR    0x02
#define IPOLB_ADDR    0x03

#define GPINTENA_ADDR 0x04
#define GPINTENB_ADDR 0x05

#define DEFVALA_ADDR  0x06
#define DEFVALB_ADDR  0x07

#define INTCONA_ADDR  0x08
#define INTCONB_ADDR  0x09

#define IOCONA_ADDR   0x0A
#define IOCONB_ADDR   0x0B

#define GPPUA_ADDR    0x0C
#define GPPUB_ADDR    0x0D

#define INTFA_ADDR    0x0E
#define INTFB_ADDR    0x0F

#define INTCAPA_ADDR  0x10
#define INTCAB_ADDR   0x11

#define GPIOA_ADDR    0x12
#define GPIOB_ADDR    0x13

#define OLATA_ADDR    0x14
#define OLATB_ADDR    0x15

typedef void (*nss_spi_ptr)(void);

typedef struct
{
  SPI_HandleTypeDef *spi;
  nss_spi_ptr cs_select;
  nss_spi_ptr cs_unselect;

  uint8_t rIODIRA;
  uint8_t rIPOLA;
  uint8_t rGPINTENA;
  uint8_t rDEFVALA;
  uint8_t rINTCONA;
  uint8_t rIOCONA;
  uint8_t rGPPUA;
  uint8_t rINTFA;
  uint8_t rINTCAPA;
  uint8_t rGPIOA;
  uint8_t rOLATA;
  uint8_t rIODIRB;
  uint8_t rIPOLB;
  uint8_t rGPINTENB;
  uint8_t rDEFVALB;
  uint8_t rINTCONB;
  uint8_t rIOCONB;
  uint8_t rGPPUB;
  uint8_t rINTFB;
  uint8_t rINTCAB;
  uint8_t rGPIOB;
  uint8_t rOLATB;

} sMCP23S18_Instance;

void Init_MCP23S18(sMCP23S18_Instance* inst, SPI_HandleTypeDef *spi_port, nss_spi_ptr cs_select_func, nss_spi_ptr cs_unselect_func);
void MCP23S18_WriteReg(sMCP23S18_Instance* inst, uint8_t addr, uint8_t data);
uint8_t MCP23S18_ReadReg(sMCP23S18_Instance* inst, uint8_t addr);

#ifdef __cplusplus
}
#endif

#endif // MCP23S18_H_
