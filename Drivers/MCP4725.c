/*
 * mcp4725.c
 *
 *  Created on: Sep 27, 2024
 *      Author: Michael Margolese
 */

#include "mcp4725.h"

void MCP4725_Init(uint8_t addr, sMCP4725 *device, write_i2c writefunc, read_i2c readfunc)
{
  device->addr = addr << 1;
  device->dacval = 0;
  device->status.status = 0;
  device->write_func = writefunc;
  device->read_func = readfunc;
}

void MCP4725_WriteDAC(sMCP4725 *device, eMSP4725_POWERMODE pwrmode, uint16_t dacval)
{
  uint8_t data[3] = {MCP4725_CMD_WRITEDAC | (pwrmode << 1), 0, 0};
  dacval = dacval << 4;
  data[1] = (uint8_t)((dacval & 0xFF00) >> 8);
  data[2] = (uint8_t)(dacval & 0x00FF);
  device->write_func(device->addr, data, 3);
}

uint16_t MCP4725_ReadDAC(sMCP4725 *device)
{
  uint8_t data[3] = {0, 0, 0};
  device->read_func(device->addr, data, 3);
  device->status.status = data[0];
  device->dacval = (uint16_t)((data[1] << 8) | data[2]);
  device->dacval = device->dacval >> 4;
  return device->dacval;
}

void MCP4725_WriteEEPROM(sMCP4725 *device, eMSP4725_POWERMODE pwrmode, uint16_t promval)
{
  uint8_t data[3] = {MCP4725_CMD_WRITEDACEEPROM | (pwrmode << 1), 0, 0};
  promval = promval << 4;
  data[1] = (uint8_t)((promval & 0xFF00) >> 8);
  data[2] = (uint8_t)(promval & 0x00FF);
  device->write_func(device->addr, data, 3);
}



