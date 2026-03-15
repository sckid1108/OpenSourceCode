/*
 * MCP9808.hpp
 *
 *  Created on: Nov 8, 2024
 *      Author: Michael Margolese
 */

#pragma once

#ifndef MCP9808_HPP_
#define MCP9808_HPP_

#include <stdbool.h>
#include <stdint.h>
#include <cassert>
#include <i2c.h>

namespace MCP9808
{
typedef enum : uint8_t
{
	RES_0_5DEGC    = 0x0, // +0.5 degC, Tconv = 30ms, 33 samples/sec
	RES_0_25DEGC   = 0x1, // +0.25 degC, Tconv = 65ms, 15 samples/sec
	RES_0_125DEGC  = 0x2, // +0.125 degC, Tconv = 130ms, 7 samples/sec
	RES_0_0625DEGC = 0x3  // +0.0625 degC, Tconv = 250ms, 4 samples/sec
} eMCP9808_Resolution;

typedef enum : uint8_t // Flags
{
	ALERT_NONE     = 0x00,
	ALERT_TA_GT_TC = 0x01, // Tambient >= Tcritical
	ALERT_TA_LT_TC = 0x02, // Tambient <  Tcritical
	ALERT_TA_GT_TU = 0x04, // Tambient >  Tupper
	ALERT_TA_LT_TU = 0x08, // Tambient <= Tupper
	ALERT_TA_GT_TL = 0x10, // Tambient >= Tlower
	ALERT_TA_LT_TL = 0x20  // Tambient <  Tlower
} eMCP9808_AlertStatus;

typedef union
{
	struct
	{
		uint16_t AlertOutputMode     : 1; // 0 - Comparator output, 1 - Interrupt output
		uint16_t AlertOutputPolarity : 1; // 0 - Active low, 1 - Active high
		uint16_t AlertOutputSelect   : 1; // 0 - Alert Out for Tupper, Tlower, and Tcrit, 1 - Ta over Tcrit only
		uint16_t AlertOutputControl  : 1; // 0 - Alerts Disabled, 1 - Alerts Enabled
		uint16_t AlertOutputStatus   : 1; // 0 - Alert is NOT asserted, 1 - Alert Asserted
		uint16_t InterruptClear      : 1; // 0 - Does nothing, 1 - Clears Alert Interrupt reads back as 0
		uint16_t WindowLock          : 1; // 0 - Unlocked Tupper,Tlower writable, 1 - Tupper, Tlower Locked
		uint16_t CriticalLock        : 1; // 0 - Unlocked Tcrit, 1 - Locked Tcrit
		uint16_t Shutdown            : 1; // 0 - Continuous Conversion, 1 - Shutdown
		uint16_t Hysteressis         : 2; // Thyst for Tupper and Tlower, 00 - 0degC, 01 - 1.5degC, 10 - 3degC, 11 - 6degC
		uint16_t RSVD                : 5;
	};
	uint16_t Config;
} MCP9808Config;

class MCP9808
{
private:
	/// Prototype of user supplied I2C write_byte or write_word function. Should return 0 on success and a non-0 error code on failure.
	typedef int (*i2c_write)(uint16_t addr, uint8_t reg, uint8_t* data, uint8_t len);

	/// Prototype of user supplied I2C read_byte or read_word function. Should return 0 on success and a non-0 error code on failure. */
	typedef int (*i2c_read)(uint16_t addr, uint8_t reg, uint8_t* data, uint8_t len);

	uint16_t i2cAddr;
	uint16_t ManufacturerID;
	uint8_t  DeviceID;
	uint8_t  RevisionID;
	i2c_write I2Cwrite;
	i2c_read  I2Cread;
	eMCP9808_AlertStatus alertStatus;
	MCP9808Config Configuration;
	eMCP9808_Resolution adcResolution;
	float TempC;
	float UpperLimit, LowerLimit, CriticalLimit;

	float FixedToFloat(uint16_t val);
	uint16_t FloatToFixed(float val, uint8_t fracbits);

public :

	static constexpr uint16_t MCP9808_I2CADDR_DEFAULT = 0x18; ///< Default I2C address (A0=A1=A2=0)
	static constexpr uint8_t  MCP9808_REG_CONFIG     = 0x01;   ///< MCP9808 config register
	static constexpr uint8_t  MCP9808_REG_UPPER_TEMP = 0x02;   ///< upper alert boundary
	static constexpr uint8_t  MCP9808_REG_LOWER_TEMP = 0x03;   ///< lower alert boundary
	static constexpr uint8_t  MCP9808_REG_CRIT_TEMP  = 0x04;   ///< critical temperature
	static constexpr uint8_t  MCP9808_REG_AMBIENT_TEMP = 0x05; ///< ambient temperature
	static constexpr uint8_t  MCP9808_REG_MANUF_ID   = 0x06;   ///< manufacture ID
	static constexpr uint8_t  MCP9808_REG_DEVICE_ID  = 0x07;   ///< device ID
	static constexpr uint8_t  MCP9808_REG_RESOLUTION = 0x08;   ///< resolution

	static constexpr uint16_t MCP9808_MANUFACTURER_ID = 0x0054;
	static constexpr uint8_t  MCP9808_DEVICE_ID       = 0x04;

	static constexpr float UPPER_TEMP_LIMIT = 125.0f; // +125.0 degC
	static constexpr float LOWER_TEMP_LIMIT = -40.0f; //  -40.0 degC

	MCP9808(uint16_t addr_, i2c_write write_, i2c_read read_) : i2cAddr(addr_), I2Cwrite(write_), I2Cread(read_) {}
	MCP9808() : i2cAddr(0), I2Cwrite(nullptr), I2Cread(nullptr) {}
	~MCP9808() { i2cAddr = 0; I2Cwrite = nullptr; I2Cread = nullptr;}

	void setI2CDriver(uint8_t addr_, i2c_write write_, i2c_read read_) { i2cAddr = addr_; I2Cwrite = write_; I2Cread = read_; }

	bool init();
	bool readTempC(float& temp);
	bool readTempF(float& temp);
	bool getResolution(eMCP9808_Resolution& res);
	bool setResolution(eMCP9808_Resolution res);
	bool setConfiguration(MCP9808Config &config);
	bool getConfiguration(MCP9808Config &config);
	bool setTemperatureLimits(float tcrit = UPPER_TEMP_LIMIT, float tupper = UPPER_TEMP_LIMIT, float tlower = LOWER_TEMP_LIMIT); // in DegC
	bool getTemperatureLimits(float& tcrit, float& tupper, float& tlower); // in DegC
};

} // end namespace MCP9808

#endif /* MCP9808_HPP_ */
