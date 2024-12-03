/*
 * mcp4725.h
 *
 *  Created on: Sep 27, 2024
 *      Author: Michael Margolese
 */

#pragma once

#ifndef INC_MCP4725_H_
#define INC_MCP4725_H_

#pragma message ("Compiling " __FILE__ )

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MCP4725_I2CADDR_DEFAULT    (0x62) // Default I2C address, Alternate is 0x60
#define MCP4725_CMD_WRITEDAC       (0x40) // Writes data to the DAC
#define MCP4725_CMD_WRITEDACEEPROM (0x60) // Writes data to the DAC and the EEPROM (persisting the assigned value after reset)

#define MCP4725_EEPROM_WRITETIME 50 // 50 milli-seconds time to write EEPROM

#define MCP4725_READY 1 // EEPROM Write Complete
#define MCP4725_BUSY  0 // EEPROM Write Busy / Not Complete

typedef void (*write_i2c)(uint8_t addr, uint8_t* data, uint16_t len);
typedef void (*read_i2c) (uint8_t addr, uint8_t* data, uint16_t len);

typedef enum __attribute__ ((__packed__))
{
  MSP4725_NORMAL = 0, // Normal Mode
  MSP4725_1K     = 1, // 1K Resistor to Ground (VOUT), Vout Offm, Internal Circuits Powered Down
  MSP4725_100K   = 2, // 100K Resistor to Ground (VOUT), Vout Offm, Internal Circuits Powered Down
  MSP4725_500K   = 3  // 500K Resistor to Ground (VOUT), Vout Offm, Internal Circuits Powered Down
} eMSP4725_POWERMODE;

typedef union
{
  struct
  {
    uint8_t rsvd0    : 1;
    uint8_t pwrmode0 : 1;
    uint8_t pwrmode1 : 1;
    uint8_t rsvd1    : 1;
    uint8_t rsvd2    : 1;
    uint8_t rsvd3    : 1;
    uint8_t por      : 1;
    uint8_t rdy_busy : 1;
  };
  uint8_t status;
}mcp4725Status;

typedef struct __attribute__ ((packed))
{
  uint8_t addr;
  mcp4725Status status;
  uint16_t dacval;

  write_i2c write_func;
  read_i2c read_func;

} sMCP4725;


void MCP4725_Init(uint8_t addr, sMCP4725 *device, write_i2c writefunc, read_i2c readfunc);
void MCP4725_WriteDAC(sMCP4725 *device, eMSP4725_POWERMODE pwrmode, uint16_t dacval);
uint16_t MCP4725_ReadDAC(sMCP4725 *device);
void MCP4725_WriteEEPROM(sMCP4725 *device, eMSP4725_POWERMODE pwrmode, uint16_t promval);


#ifdef __cplusplus
}
#endif

#endif /* INC_MCP4725_H_ */
