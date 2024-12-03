/*
 * MCP9808.cpp
 *
 *  Created on: Nov 26, 2024
 *      Author: Michael Margolese
 */

#include <math.h>
#include <MCP9808.hpp>

namespace MCP9808
{

static constexpr float degCtoF(float degc)
{
   return degc * (9.0f / 5.0f) + 32.0f;
}

float MCP9808::FixedToFloat(uint16_t val) // signed 2's complement, Q8.2 format
{
	float res = 0.0f;
	bool isneg = ((val & 0x1000) == 0) ? false : true; // 10th bit is sign bit after shift 2 bits right
	val = (uint16_t)(val & 0x1FFF);

	// Page 25 of datasheet, Temp = msb*16 + lsb*0.0625
	res = 0.0625f * (val & 0x00FF);
	res += (16.0f * (val >> 8));
	if (isneg == true) res = 256.0f - res; // Temp = 256 - (msb*16 + lsb*0.0625)

	return res;
}

// https://embeddedartistry.com/blog/2018/07/12/simple-fixed-point-conversion-in-c/
uint16_t MCP9808::FloatToFixed(float val, uint8_t fracbits)
{
	uint16_t res = 0;
	bool isneg = false;

	if (val < 0.0f) { isneg = true; val = -1.0f*val; }

	res = (uint16_t)(val * (1 << fracbits)); // will signed extend if negative

	if (isneg == true) res = (uint16_t) (res | (0xF << 12));

	return res;
	//inline fixed_point_t double_to_fixed(double input)
	//{
	//    return (fixed_point_t)(input * (1 << FIXED_POINT_FRACTIONAL_BITS));
	//}
}

bool MCP9808::init()
{
	assert(I2Cwrite != nullptr && I2Cread != nullptr);

	uint8_t data[2]; // data[0]->MSB, data[1]->LSB

	// Read manufacturer and device id
	if ( I2Cread(i2cAddr, MCP9808_REG_MANUF_ID, data, 2) != 0 ) return false;
	ManufacturerID = (uint16_t)((data[0] << 8) + data[1]);

	// Read device id and chip revision
	if ( I2Cread(i2cAddr, MCP9808_REG_DEVICE_ID, data, 2) != 0 ) return false;
	DeviceID   = data[0];
	RevisionID = data[1];

	// Clear configuration register
	data[0] = 0;
	if ( I2Cwrite(i2cAddr, MCP9808_REG_DEVICE_ID, data, 1) != 0 ) return false;

	getConfiguration(Configuration);
	getResolution(adcResolution);
	readTempC(TempC);
	getTemperatureLimits(CriticalLimit, UpperLimit, LowerLimit);

	return true;
}

bool MCP9808::readTempC(float& temp)
{
	assert(I2Cwrite != nullptr && I2Cread != nullptr);

	temp = NAN;
	uint8_t data[2];
	uint16_t tempcounts;

	if ( I2Cread(i2cAddr, MCP9808_REG_AMBIENT_TEMP, data, 2) != 0 ) return false;
	tempcounts = (uint16_t)((data[0] << 8) + data[1]);

	// upper 3 bits used for alert state
	if (tempcounts != 0xFFFF)
	{
		temp = tempcounts & 0x1FFF; // remove upper 3 bits
		temp = temp / 16.0f;
		if (tempcounts & 0x1000) // account for sign bit
		  temp -= 256.0f;
	}

	// set alert status
	uint8_t status = (uint8_t)(tempcounts >> 13);

	alertStatus = eMCP9808_AlertStatus::ALERT_NONE;

	eMCP9808_AlertStatus TL = ((status & 0x1) == 0) ? eMCP9808_AlertStatus::ALERT_TA_GT_TL : eMCP9808_AlertStatus::ALERT_TA_LT_TL;
	eMCP9808_AlertStatus TU = ((status & 0x2) == 0) ? eMCP9808_AlertStatus::ALERT_TA_LT_TU : eMCP9808_AlertStatus::ALERT_TA_GT_TU;
	eMCP9808_AlertStatus TC = ((status & 0x4) == 0) ? eMCP9808_AlertStatus::ALERT_TA_LT_TC : eMCP9808_AlertStatus::ALERT_TA_GT_TC;

	alertStatus = (eMCP9808_AlertStatus)(TL|TU|TC);

	TempC = temp;

	return true;
}

bool MCP9808::readTempF(float& temp) { return degCtoF(readTempC(temp));}

bool MCP9808::getResolution(eMCP9808_Resolution& res)
{
	assert(I2Cwrite != nullptr && I2Cread != nullptr);

	res = eMCP9808_Resolution::RES_0_5DEGC;
	uint8_t data;
	if ( I2Cread(i2cAddr, MCP9808_REG_RESOLUTION, &data, 1) != 0 ) return false;
	res = (eMCP9808_Resolution) data;
	adcResolution = res;
	return true;
}

bool MCP9808::setResolution(eMCP9808_Resolution res)
{
	assert(I2Cwrite != nullptr && I2Cread != nullptr);

	uint8_t data = (uint8_t)(res & 0x3);
	if ( I2Cwrite(i2cAddr, MCP9808_REG_RESOLUTION, &data, 1) != 0) return false;
	getResolution(adcResolution);
	return true;
}

bool MCP9808::setConfiguration(MCP9808Config &config)
{
	assert(I2Cwrite != nullptr && I2Cread != nullptr);

	uint8_t data[2];

	data[0] = (uint8_t) (config.Config >> 8);
	data[1] = (uint8_t) (config.Config & 0xFF);
	if ( I2Cwrite(i2cAddr, MCP9808_REG_CONFIG, data, 2) != 0) return false;
	return getConfiguration(Configuration);
}

bool MCP9808::getConfiguration(MCP9808Config &config)
{
	assert(I2Cwrite != nullptr && I2Cread != nullptr);

	uint8_t data[2];
	if ( I2Cread(i2cAddr, MCP9808_REG_CONFIG, data, 2) != 0 ) return false;
	config.Config = (uint16_t)((data[0] << 8) + data[1]);
	return true;
}

bool MCP9808::setTemperatureLimits(float tcrit, float tupper, float tlower)
{
	uint8_t data[2];
	uint16_t tempcounts;

	tempcounts = FloatToFixed(tupper, 4);
	tempcounts = tempcounts & 0x1FFF;
	data[0] = (uint8_t)(tempcounts >> 8);   //data[0] = 0x05; // +90C
	data[1] = (uint8_t)(tempcounts & 0xFF); //data[1] = 0xA0;

	if ( I2Cwrite(i2cAddr, MCP9808_REG_UPPER_TEMP, data, 2) != 0 ) return false;

	tempcounts = FloatToFixed(tlower, 4);
	tempcounts = tempcounts & 0x1FFF;
	data[0] = (uint8_t)(tempcounts >> 8);
	data[1] = (uint8_t)(tempcounts & 0xFF);

	if ( I2Cwrite(i2cAddr, MCP9808_REG_LOWER_TEMP, data, 2) != 0 ) return false;

	tempcounts = FloatToFixed(tcrit, 4);
	tempcounts = tempcounts & 0x1FFF;
	data[0] = (uint8_t)(tempcounts >> 8);
	data[1] = (uint8_t)(tempcounts & 0xFF);

	if ( I2Cwrite(i2cAddr, MCP9808_REG_CRIT_TEMP, data, 2) != 0 ) return false;

	getTemperatureLimits(CriticalLimit, UpperLimit, LowerLimit);

	return true;
}

bool MCP9808::getTemperatureLimits(float& tcrit, float& tupper, float& tlower)
{
	assert(I2Cwrite != nullptr && I2Cread != nullptr);

	tcrit  = NAN;
	tupper = NAN;
	tlower = NAN;
	uint8_t data[2];
	uint16_t tempcounts;

	// signed 2's complement, Q8.2 format
	if ( I2Cread(i2cAddr, MCP9808_REG_UPPER_TEMP, data, 2) != 0 ) return false;
	tempcounts = (uint16_t)((data[0] << 8) + data[1]);
	tupper = FixedToFloat(tempcounts);

	if ( I2Cread(i2cAddr, MCP9808_REG_LOWER_TEMP, data, 2) != 0 ) return false;
	tempcounts = (uint16_t)((data[0] << 8) + data[1]);
	tlower = FixedToFloat(tempcounts);

	if ( I2Cread(i2cAddr, MCP9808_REG_CRIT_TEMP, data, 2) != 0 ) return false;
	tempcounts = (uint16_t)((data[0] << 8) + data[1]);
	tcrit = FixedToFloat(tempcounts);

	CriticalLimit = tcrit;
	UpperLimit = tupper;
	LowerLimit = tlower;

	return true;
}

} // end namespace
