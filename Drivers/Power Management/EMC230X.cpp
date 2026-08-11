/*******************************************************************************
 * @file      EMC230X.cpp
 * @brief     Implementation of the Microchip EMC2301/2/3/5 fan controller driver.
 *
 * @details   See EMC230X.hpp for the API and usage. Points worth knowing when
 *            working in this file:
 *
 *            - The 13-bit tach count is left-justified in its register pair
 *              (FxTT/FxTR[12:5] high byte, [4:0] in low-byte bits 7:3). Reads shift
 *              right by 3 as each fan's pair completes; writes shift left by 3.
 *              Never split that shift away from the byte assembly -- a failure part
 *              way through would otherwise leave earlier fans scaled by 8.
 *
 *            - Per-fan registers are only touched for channels the detected part
 *              implements; ensureIdentified() reads the product ID on first use.
 *
 *            - No exceptions, no heap, no platform headers. Errors are reported
 *              through EMC230X_STATUS, which is a set of bit flags.
 *
 * @version   1.0
 * @date      Created  8 November 2024
 * @date      Modified 11 August 2026
 *
 * @author    Michael Margolese
 * @copyright Copyright (c) 2024-2026 Tenuvah Designs. All rights reserved.
 ******************************************************************************/

#include <EMC230X.hpp>

namespace EMC230X
{

EMC230X::EMC230X(uint8_t addr_, smbus_write_register write_func, smbus_read_register read_func) : EMC230X()
{
	i2cAddr = addr_;
	smbus_write_reg = write_func;
	smbus_read_reg = read_func;
}

EMC230X::EMC230X()
{
	// Nothing is touched on the chip here -- this only clears the local cache. No I2C
	// happens until setI2CDriver() has been called and a get/set is issued.
	//
	// The pole fields default to FANPOLE_2 to match the EMC230x reset value of the
	// EDG field (01b, 5 edges), which is the standard 4-wire fan giving 2 tach pulses
	// per revolution. getFanConfig1() overwrites these with what the chip reports.
	i2cAddr                 = I2C_ADDRESS; // 0x2F; overridden by setI2CDriver()
	smbus_write_reg         = nullptr;
	smbus_read_reg          = nullptr;
	emc230xState_           = {0};
	fanControllers_         = FANCNTRL_1;  // until getChipInfo() identifies the part
	chipIdentified_         = false;
	fan1Poles_               = EMC230X_FANPOLES::FANPOLE_2;
	fan2Poles_               = EMC230X_FANPOLES::FANPOLE_2;
	fan3Poles_               = EMC230X_FANPOLES::FANPOLE_2;
	fan4Poles_               = EMC230X_FANPOLES::FANPOLE_2;
	fan5Poles_               = EMC230X_FANPOLES::FANPOLE_2;
	tachFan1PolesMultiplier_ = 2;          // poles = FANPOLE enum value + 1
	tachFan2PolesMultiplier_ = 2;
	tachFan3PolesMultiplier_ = 2;
	tachFan4PolesMultiplier_ = 2;
	tachFan5PolesMultiplier_ = 2;
}

EMC230X::~EMC230X()
{
}

/* Reads the product ID once so fanControllers_ can be trusted. The per-fan getters call
   this first: registers belonging to a fan the part does not implement are reserved, and
   reading them is what made this driver misbehave on the smaller parts. getChipInfo()
   touches only chip-level registers, so there is no recursion back into here. */
EMC230X_STATUS EMC230X::ensureIdentified()
{
	if (chipIdentified_)
	{
		return EMC230X_STATUS::EMC230X_STATUS_OK;
	}

	return getChipInfo();
}

EMC230X_STATUS EMC230X::getChipState()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	EMC230X_STATUS status = EMC230X_STATUS::EMC230X_STATUS_OK;
	status = (EMC230X_STATUS) (status | getChipInfo());
	status = (EMC230X_STATUS) (status | getChipConfig());
	status = (EMC230X_STATUS) (status | getSoftWareLock());
	status = (EMC230X_STATUS) (status | getFanStatus());
	status = (EMC230X_STATUS) (status | getFanStallStatus());
	status = (EMC230X_STATUS) (status | getFanSpinStatus());
	status = (EMC230X_STATUS) (status | getDriveFailStatus());
	status = (EMC230X_STATUS) (status | getFanInterruptEnable());
	status = (EMC230X_STATUS) (status | getPWMPolarity());
	status = (EMC230X_STATUS) (status | getPWMOutput());
	status = (EMC230X_STATUS) (status | getPWMBaseFreq());
	status = (EMC230X_STATUS) (status | getPWMDivider());
	status = (EMC230X_STATUS) (status | getFanDriveSettings());
	status = (EMC230X_STATUS) (status | getFanConfig1());
	status = (EMC230X_STATUS) (status | getFanConfig2());
	status = (EMC230X_STATUS) (status | getFanSpinUpConfig());
	status = (EMC230X_STATUS) (status | getFanMaxStep());
	status = (EMC230X_STATUS) (status | getFanMinDrive());
	status = (EMC230X_STATUS) (status | getFanValidTachCount());
	status = (EMC230X_STATUS) (status | getDriveFailBand());
	status = (EMC230X_STATUS) (status | getFanTachTarget());
	status = (EMC230X_STATUS) (status | getFanTachReading());
	return status;
}

EMC230X_STATUS EMC230X::getChipInfo()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	emc230xState_.ChipInfo.ProductFeatures = 0;
	emc230xState_.ChipInfo.ProductID       = 0;
	emc230xState_.ChipInfo.ManufacturerID  = 0;
	emc230xState_.ChipInfo.Revision        = 0;
	int state = 0;
	uint8_t udata = 0;

	state = smbus_read_reg(i2cAddr, EMC230X_REG_PRODUCTFEATURES, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.ChipInfo.ProductFeatures = udata;

	state = smbus_read_reg(i2cAddr, EMC230X_REG_PRODUCTID, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.ChipInfo.ProductID = udata;

	state = smbus_read_reg(i2cAddr, EMC230X_REG_MANUFACTURERID, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.ChipInfo.ManufacturerID = udata;

	state = smbus_read_reg(i2cAddr, EMC230X_REG_CHIPREVISION, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.ChipInfo.Revision = udata;

	// Mask of the channels this part has, not just the highest one -- an EMC2303 has
	// fans 1, 2 and 3, so 0x07. Callers use it to avoid touching absent channels;
	// registers for a fan the part does not implement are reserved.
	switch ((EMC230X_PRODUCTID)emc230xState_.ChipInfo.ProductID)
	{
		case EMC230X_PRODUCTID::EMC2301_ID: fanControllers_ = FANCNTRL_1; break;
		case EMC230X_PRODUCTID::EMC2302_ID: fanControllers_ = FANCNTRL_1 | FANCNTRL_2; break;
		case EMC230X_PRODUCTID::EMC2303_ID: fanControllers_ = FANCNTRL_1 | FANCNTRL_2 | FANCNTRL_3; break;
		case EMC230X_PRODUCTID::EMC2305_ID: fanControllers_ = FANCNTRL_1 | FANCNTRL_2 | FANCNTRL_3 |
		                                                      FANCNTRL_4 | FANCNTRL_5; break;
		default: fanControllers_ = FANCNTRL_1; break; // unrecognised part: assume one fan
	}

	chipIdentified_ = true;

	return EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getChipConfig()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.Configuration.Config = 0;

	state = smbus_read_reg(i2cAddr, EMC230X_REG_CONFIGURATION, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Configuration.Config = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanStatus()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanStatus.fanStatus = 0;

	state = smbus_read_reg(i2cAddr, EMC230X_REG_STATUS, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanStatus.fanStatus = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanStallStatus()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanStallStatus.fanStallStatus = 0;

	state = smbus_read_reg(i2cAddr, EMC230X_REG_STALLSTATUS, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanStallStatus.fanStallStatus = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanSpinStatus()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanSpinStatus.fanSpinStatus = 0;

	state = smbus_read_reg(i2cAddr, EMC230X_REG_SPINSTATUS, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanSpinStatus.fanSpinStatus = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getDriveFailStatus()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.DriveFailStatus.driveFailStatus = 0;

	state = smbus_read_reg(i2cAddr, EMC230X_REG_DRIVEFAILSTATUS, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.DriveFailStatus.driveFailStatus = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanInterruptEnable()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanInterruptEnable.fanInterruptEnable = 0;

	state = smbus_read_reg(i2cAddr, EMC230X_REG_INTERRUPTENABLE, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanInterruptEnable.fanInterruptEnable = (uint8_t)(udata & 0x1F);

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getPWMPolarity()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.PWMPolarity.pwmPolarity = 0;

	state = smbus_read_reg(i2cAddr, EMC230X_REG_PWMPOLARITY, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.PWMPolarity.pwmPolarity = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getPWMOutput()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.PWMOutputConfig.pwmOutputConfig = 0;

	state = smbus_read_reg(i2cAddr, EMC230X_REG_PWMOUTPUT, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.PWMOutputConfig.pwmOutputConfig = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getPWMBaseFreq()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.PWMBaseF123.pwmBaseF123 = 0;
	emc230xState_.PWMBaseF45.pwmBaseF45 = 0;

	state = smbus_read_reg(i2cAddr, EMC230X_REG_PWMBASEFREQ45, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.PWMBaseF45.pwmBaseF45 = udata;

	state = smbus_read_reg(i2cAddr, EMC230X_REG_PWMBASEFREQ123, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.PWMBaseF123.pwmBaseF123 = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getPWMDivider()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	// Identify the part before touching per-fan registers -- the ones belonging to a
	// fan this device does not implement are reserved.
	EMC230X_STATUS ident = ensureIdentified();
	if (ident != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return ident;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.PWMDividers.PWM1Div = 0;
	emc230xState_.PWMDividers.PWM2Div = 0;
	emc230xState_.PWMDividers.PWM3Div = 0;
	emc230xState_.PWMDividers.PWM4Div = 0;
	emc230xState_.PWMDividers.PWM5Div = 0;

	if (fanControllers_ & FANCNTRL_1)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN1PWMDIVIDE, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.PWMDividers.PWM1Div = udata;

	}

	if (fanControllers_ & FANCNTRL_2)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN2PWMDIVIDE, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.PWMDividers.PWM2Div = udata;

	}

	if (fanControllers_ & FANCNTRL_3)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN3PWMDIVIDE, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.PWMDividers.PWM3Div = udata;

	}

	if (fanControllers_ & FANCNTRL_4)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN4PWMDIVIDE, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.PWMDividers.PWM4Div = udata;

	}

	if (fanControllers_ & FANCNTRL_5)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN5PWMDIVIDE, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.PWMDividers.PWM5Div = udata;

	}

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanDriveSettings()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	// Identify the part before touching per-fan registers -- the ones belonging to a
	// fan this device does not implement are reserved.
	EMC230X_STATUS ident = ensureIdentified();
	if (ident != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return ident;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanDriveSetting.PWM1 = 0;
	emc230xState_.FanDriveSetting.PWM2 = 0;
	emc230xState_.FanDriveSetting.PWM3 = 0;
	emc230xState_.FanDriveSetting.PWM4 = 0;
	emc230xState_.FanDriveSetting.PWM5 = 0;

	if (fanControllers_ & FANCNTRL_1)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN1DRIVESETTING, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanDriveSetting.PWM1 = udata;

	}

	if (fanControllers_ & FANCNTRL_2)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN2DRIVESETTING, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanDriveSetting.PWM2 = udata;

	}

	if (fanControllers_ & FANCNTRL_3)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN3DRIVESETTING, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanDriveSetting.PWM3 = udata;

	}

	if (fanControllers_ & FANCNTRL_4)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN4DRIVESETTING, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanDriveSetting.PWM4 = udata;

	}

	if (fanControllers_ & FANCNTRL_5)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN5DRIVESETTING, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanDriveSetting.PWM5 = udata;

	}

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanConfig1()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	// Identify the part before touching per-fan registers -- the ones belonging to a
	// fan this device does not implement are reserved.
	EMC230X_STATUS ident = ensureIdentified();
	if (ident != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return ident;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.Fan1Config1.fanConfig1 = 0;
	emc230xState_.Fan2Config1.fanConfig1 = 0;
	emc230xState_.Fan3Config1.fanConfig1 = 0;
	emc230xState_.Fan4Config1.fanConfig1 = 0;
	emc230xState_.Fan5Config1.fanConfig1 = 0;

	if (fanControllers_ & FANCNTRL_1)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN1CONFIG1, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.Fan1Config1.fanConfig1 = udata;

	}

	if (fanControllers_ & FANCNTRL_2)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN2CONFIG1, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.Fan2Config1.fanConfig1 = udata;

	}

	if (fanControllers_ & FANCNTRL_3)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN3CONFIG1, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.Fan3Config1.fanConfig1 = udata;

	}

	if (fanControllers_ & FANCNTRL_4)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN4CONFIG1, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.Fan4Config1.fanConfig1 = udata;

	}

	if (fanControllers_ & FANCNTRL_5)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN5CONFIG1, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.Fan5Config1.fanConfig1 = udata;

	}

	fan1Poles_ = (EMC230X_FANPOLES) emc230xState_.Fan1Config1.EDG;
	fan2Poles_ = (EMC230X_FANPOLES) emc230xState_.Fan2Config1.EDG;
	fan3Poles_ = (EMC230X_FANPOLES) emc230xState_.Fan3Config1.EDG;
	fan4Poles_ = (EMC230X_FANPOLES) emc230xState_.Fan4Config1.EDG;
	fan5Poles_ = (EMC230X_FANPOLES) emc230xState_.Fan5Config1.EDG;

	tachFan1PolesMultiplier_ = fan1Poles_ + 1;
	tachFan2PolesMultiplier_ = fan2Poles_ + 1;
	tachFan3PolesMultiplier_ = fan3Poles_ + 1;
	tachFan4PolesMultiplier_ = fan4Poles_ + 1;
	tachFan5PolesMultiplier_ = fan5Poles_ + 1;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanConfig2()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	// Identify the part before touching per-fan registers -- the ones belonging to a
	// fan this device does not implement are reserved.
	EMC230X_STATUS ident = ensureIdentified();
	if (ident != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return ident;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.Fan1Config2.fanConfig2 = 0;
	emc230xState_.Fan2Config2.fanConfig2 = 0;
	emc230xState_.Fan3Config2.fanConfig2 = 0;
	emc230xState_.Fan4Config2.fanConfig2 = 0;
	emc230xState_.Fan5Config2.fanConfig2 = 0;

	if (fanControllers_ & FANCNTRL_1)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN1CONFIG2, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.Fan1Config2.fanConfig2 = udata;

	}

	if (fanControllers_ & FANCNTRL_2)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN2CONFIG2, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.Fan2Config2.fanConfig2 = udata;

	}

	if (fanControllers_ & FANCNTRL_3)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN3CONFIG2, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.Fan3Config2.fanConfig2 = udata;

	}

	if (fanControllers_ & FANCNTRL_4)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN4CONFIG2, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.Fan4Config2.fanConfig2 = udata;

	}

	if (fanControllers_ & FANCNTRL_5)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN5CONFIG2, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.Fan5Config2.fanConfig2 = udata;

	}

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getPIDGain()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	// Identify the part before touching per-fan registers -- the ones belonging to a
	// fan this device does not implement are reserved.
	EMC230X_STATUS ident = ensureIdentified();
	if (ident != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return ident;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.Fan1PIDGain.pidGain = 0;
	emc230xState_.Fan2PIDGain.pidGain = 0;
	emc230xState_.Fan3PIDGain.pidGain = 0;
	emc230xState_.Fan4PIDGain.pidGain = 0;
	emc230xState_.Fan5PIDGain.pidGain = 0;

	if (fanControllers_ & FANCNTRL_1)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN1PIDGAIN, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.Fan1PIDGain.pidGain = udata;

	}

	if (fanControllers_ & FANCNTRL_2)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN2PIDGAIN, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.Fan2PIDGain.pidGain = udata;

	}

	if (fanControllers_ & FANCNTRL_3)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN3PIDGAIN, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.Fan3PIDGain.pidGain = udata;

	}

	if (fanControllers_ & FANCNTRL_4)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN4PIDGAIN, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.Fan4PIDGain.pidGain = udata;

	}

	if (fanControllers_ & FANCNTRL_5)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN5PIDGAIN, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.Fan5PIDGain.pidGain = udata;

	}

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanSpinUpConfig()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	// Identify the part before touching per-fan registers -- the ones belonging to a
	// fan this device does not implement are reserved.
	EMC230X_STATUS ident = ensureIdentified();
	if (ident != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return ident;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanSpinUp1.fanSpinUpConfig = 0;
	emc230xState_.FanSpinUp2.fanSpinUpConfig = 0;
	emc230xState_.FanSpinUp3.fanSpinUpConfig = 0;
	emc230xState_.FanSpinUp4.fanSpinUpConfig = 0;
	emc230xState_.FanSpinUp5.fanSpinUpConfig = 0;

	if (fanControllers_ & FANCNTRL_1)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN1SPINUP, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanSpinUp1.fanSpinUpConfig = udata;

	}

	if (fanControllers_ & FANCNTRL_2)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN2SPINUP, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanSpinUp2.fanSpinUpConfig = udata;

	}

	if (fanControllers_ & FANCNTRL_3)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN3SPINUP, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanSpinUp3.fanSpinUpConfig = udata;

	}

	if (fanControllers_ & FANCNTRL_4)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN4SPINUP, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanSpinUp4.fanSpinUpConfig = udata;

	}

	if (fanControllers_ & FANCNTRL_5)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN5SPINUP, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanSpinUp5.fanSpinUpConfig = udata;

	}

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanMaxStep()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	// Identify the part before touching per-fan registers -- the ones belonging to a
	// fan this device does not implement are reserved.
	EMC230X_STATUS ident = ensureIdentified();
	if (ident != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return ident;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanDrvMaxStepSize1 = 0;
	emc230xState_.FanDrvMaxStepSize2 = 0;
	emc230xState_.FanDrvMaxStepSize3 = 0;
	emc230xState_.FanDrvMaxStepSize4 = 0;
	emc230xState_.FanDrvMaxStepSize5 = 0;

	if (fanControllers_ & FANCNTRL_1)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN1MAXSTEP, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanDrvMaxStepSize1 = udata;

	}

	if (fanControllers_ & FANCNTRL_2)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN2MAXSTEP, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanDrvMaxStepSize2 = udata;

	}

	if (fanControllers_ & FANCNTRL_3)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN3MAXSTEP, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanDrvMaxStepSize3 = udata;

	}

	if (fanControllers_ & FANCNTRL_4)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN4MAXSTEP, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanDrvMaxStepSize4 = udata;

	}

	if (fanControllers_ & FANCNTRL_5)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN5MAXSTEP, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanDrvMaxStepSize5 = udata;

	}

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanMinDrive()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	// Identify the part before touching per-fan registers -- the ones belonging to a
	// fan this device does not implement are reserved.
	EMC230X_STATUS ident = ensureIdentified();
	if (ident != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return ident;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanDrvMinStepSize1 = 0;
	emc230xState_.FanDrvMinStepSize2 = 0;
	emc230xState_.FanDrvMinStepSize3 = 0;
	emc230xState_.FanDrvMinStepSize4 = 0;
	emc230xState_.FanDrvMinStepSize5 = 0;

	if (fanControllers_ & FANCNTRL_1)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN1MINDRIVE, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanDrvMinStepSize1 = udata;

	}

	if (fanControllers_ & FANCNTRL_2)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN2MINDRIVE, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanDrvMinStepSize2 = udata;

	}

	if (fanControllers_ & FANCNTRL_3)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN3MINDRIVE, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanDrvMinStepSize3 = udata;

	}

	if (fanControllers_ & FANCNTRL_4)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN4MINDRIVE, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanDrvMinStepSize4 = udata;

	}

	if (fanControllers_ & FANCNTRL_5)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN5MINDRIVE, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.FanDrvMinStepSize5 = udata;

	}

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanValidTachCount()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	// Identify the part before touching per-fan registers -- the ones belonging to a
	// fan this device does not implement are reserved.
	EMC230X_STATUS ident = ensureIdentified();
	if (ident != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return ident;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.ValidTachCount1 = 0;
	emc230xState_.ValidTachCount2 = 0;
	emc230xState_.ValidTachCount3 = 0;
	emc230xState_.ValidTachCount4 = 0;
	emc230xState_.ValidTachCount5 = 0;

	if (fanControllers_ & FANCNTRL_1)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN1VALTACHCOUNT, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.ValidTachCount1 = udata;

	}

	if (fanControllers_ & FANCNTRL_2)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN2VALTACHCOUNT, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.ValidTachCount2 = udata;

	}

	if (fanControllers_ & FANCNTRL_3)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN3VALTACHCOUNT, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.ValidTachCount3 = udata;

	}

	if (fanControllers_ & FANCNTRL_4)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN4VALTACHCOUNT, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.ValidTachCount4 = udata;

	}

	if (fanControllers_ & FANCNTRL_5)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN5VALTACHCOUNT, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.ValidTachCount5 = udata;

	}

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getDriveFailBand()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	// Identify the part before touching per-fan registers -- the ones belonging to a
	// fan this device does not implement are reserved.
	EMC230X_STATUS ident = ensureIdentified();
	if (ident != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return ident;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.DriveFailBand1 = 0;
	emc230xState_.DriveFailBand2 = 0;
	emc230xState_.DriveFailBand3 = 0;
	emc230xState_.DriveFailBand4 = 0;
	emc230xState_.DriveFailBand5 = 0;

	if (fanControllers_ & FANCNTRL_1)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN1DRVFAILMSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.DriveFailBand1 = udata << 8;

		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN1DRVFAILLSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.DriveFailBand1 += udata;

	}

	if (fanControllers_ & FANCNTRL_2)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN2DRVFAILMSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.DriveFailBand2 = udata << 8;

		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN2DRVFAILLSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.DriveFailBand2 += udata;

	}

	if (fanControllers_ & FANCNTRL_3)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN3DRVFAILMSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.DriveFailBand3 = udata << 8;

		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN3DRVFAILLSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.DriveFailBand3 += udata;

	}

	if (fanControllers_ & FANCNTRL_4)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN4DRVFAILMSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.DriveFailBand4 = udata << 8;

		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN4DRVFAILLSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.DriveFailBand4 += udata;

	}

	if (fanControllers_ & FANCNTRL_5)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN5DRVFAILMSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.DriveFailBand5 = udata << 8;

		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN5DRVFAILLSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.DriveFailBand5 += udata;

	}

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanTachTarget()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	// Identify the part before touching per-fan registers -- the ones belonging to a
	// fan this device does not implement are reserved.
	EMC230X_STATUS ident = ensureIdentified();
	if (ident != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return ident;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.TachTarget1 = 0;
	emc230xState_.TachTarget2 = 0;
	emc230xState_.TachTarget3 = 0;
	emc230xState_.TachTarget4 = 0;
	emc230xState_.TachTarget5 = 0;

	if (fanControllers_ & FANCNTRL_1)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN1TACHTARGETMSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachTarget1 = udata << 8;

		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN1TACHTARGETLSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachTarget1 = (uint16_t)((emc230xState_.TachTarget1 + udata) >> 3);

	}

	if (fanControllers_ & FANCNTRL_2)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN2TACHTARGETMSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachTarget2 = udata << 8;

		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN2TACHTARGETLSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachTarget2 = (uint16_t)((emc230xState_.TachTarget2 + udata) >> 3);

	}

	if (fanControllers_ & FANCNTRL_3)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN3TACHTARGETMSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachTarget3 = udata << 8;

		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN3TACHTARGETLSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachTarget3 = (uint16_t)((emc230xState_.TachTarget3 + udata) >> 3);

	}

	if (fanControllers_ & FANCNTRL_4)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN4TACHTARGETMSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachTarget4 = udata << 8;

		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN4TACHTARGETLSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachTarget4 = (uint16_t)((emc230xState_.TachTarget4 + udata) >> 3);

	}

	if (fanControllers_ & FANCNTRL_5)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN5TACHTARGETMSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachTarget5 = udata << 8;

		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN5TACHTARGETLSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachTarget5 = (uint16_t)((emc230xState_.TachTarget5 + udata) >> 3);

	}


	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanTachReading()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	// Identify the part before touching per-fan registers -- the ones belonging to a
	// fan this device does not implement are reserved.
	EMC230X_STATUS ident = ensureIdentified();
	if (ident != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return ident;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.TachCurrent1 = 0;
	emc230xState_.TachCurrent2 = 0;
	emc230xState_.TachCurrent3 = 0;
	emc230xState_.TachCurrent4 = 0;
	emc230xState_.TachCurrent5 = 0;

	if (fanControllers_ & FANCNTRL_1)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN1TACHREADMSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachCurrent1 = udata << 8;

		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN1TACHREADLSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachCurrent1 = (uint16_t)((emc230xState_.TachCurrent1 + udata) >> 3);

	}

	if (fanControllers_ & FANCNTRL_2)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN2TACHREADMSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachCurrent2 = udata << 8;

		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN2TACHREADLSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachCurrent2 = (uint16_t)((emc230xState_.TachCurrent2 + udata) >> 3);

	}

	if (fanControllers_ & FANCNTRL_3)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN3TACHREADMSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachCurrent3 = udata << 8;

		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN3TACHREADLSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachCurrent3 = (uint16_t)((emc230xState_.TachCurrent3 + udata) >> 3);

	}

	if (fanControllers_ & FANCNTRL_4)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN4TACHREADMSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachCurrent4 = udata << 8;

		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN4TACHREADLSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachCurrent4 = (uint16_t)((emc230xState_.TachCurrent4 + udata) >> 3);

	}

	if (fanControllers_ & FANCNTRL_5)
	{
		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN5TACHREADMSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachCurrent5 = udata << 8;

		state = smbus_read_reg(i2cAddr, EMC230X_REG_FAN5TACHREADLSB, &udata, 1);
		if (state != 0) return EMC230X_STATUS_FAIL;
		else emc230xState_.TachCurrent5 = (uint16_t)((emc230xState_.TachCurrent5 + udata) >> 3);

	}


	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getSoftWareLock()
{
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.SoftwareLock = 0;

	state = smbus_read_reg(i2cAddr, EMC230X_REG_SOFTWARELOCK, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.SoftwareLock = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_FANPOLES EMC230X::getFanPoles(EMC230X_FAN fan)
{
	switch (fan)
	{
		case EMC230X_FAN::FANCNTRL_1 : return fan1Poles_;
		case EMC230X_FAN::FANCNTRL_2 : return fan2Poles_;
		case EMC230X_FAN::FANCNTRL_3 : return fan3Poles_;
		case EMC230X_FAN::FANCNTRL_4 : return fan4Poles_;
		case EMC230X_FAN::FANCNTRL_5 : return fan5Poles_;
		default : return fan1Poles_;
	}
}

uint16_t EMC230X::getFanTachReading(EMC230X_FAN fan)
{
	if (!isValidFan(fan)) return 0;
	switch (fan)
	{
		case EMC230X_FAN::FANCNTRL_1 : return emc230xState_.TachCurrent1;
		case EMC230X_FAN::FANCNTRL_2 : return emc230xState_.TachCurrent2;
		case EMC230X_FAN::FANCNTRL_3 : return emc230xState_.TachCurrent3;
		case EMC230X_FAN::FANCNTRL_4 : return emc230xState_.TachCurrent4;
		case EMC230X_FAN::FANCNTRL_5 : return emc230xState_.TachCurrent5;
	}
	return 0;
}

//Sets the PWM output polarity
//1 = PWM x drive setting of 00h produces 100% duty cycle, drive setting of FFh produces 0% duty cycle.
//0 = PWM x drive setting of 00h produces 0% duty cycle, drive setting of FFh produces 100% duty cycle.
EMC230X_STATUS EMC230X::setPWMPolarity(EMC230X_FAN fan, uint8_t polarity)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}
	if (polarity > 1) polarity = 1;

	EMC230X_STATUS state = getPWMPolarity();

	// These setters are read-modify-write: bail if the read failed, otherwise we
	// would write the zeroed cache back over the chip's real configuration.
	if (state != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return state;
	}

	if (fan == EMC230X_FAN::FANCNTRL_1) emc230xState_.PWMPolarity.POLARITY1 = polarity;
	if (fan == EMC230X_FAN::FANCNTRL_2) emc230xState_.PWMPolarity.POLARITY2 = polarity;
	if (fan == EMC230X_FAN::FANCNTRL_3) emc230xState_.PWMPolarity.POLARITY3 = polarity;
	if (fan == EMC230X_FAN::FANCNTRL_4) emc230xState_.PWMPolarity.POLARITY4 = polarity;
	if (fan == EMC230X_FAN::FANCNTRL_5) emc230xState_.PWMPolarity.POLARITY5 = polarity;

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, EMC230X_REG_PWMPOLARITY, &emc230xState_.PWMPolarity.pwmPolarity, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	state = getPWMPolarity();
	return state;
}

// 1 - PWM x is Push-Pull Type
// 0 - PWM x is Open Drain Type
EMC230X_STATUS EMC230X::setPWMOutput(EMC230X_FAN fan, uint8_t iotype)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}
	if (iotype > 1) iotype = 1;

	EMC230X_STATUS state = getPWMOutput();

	// These setters are read-modify-write: bail if the read failed, otherwise we
	// would write the zeroed cache back over the chip's real configuration.
	if (state != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return state;
	}

	if (fan == EMC230X_FAN::FANCNTRL_1) emc230xState_.PWMOutputConfig.PWMOUT1 = iotype;
	if (fan == EMC230X_FAN::FANCNTRL_2) emc230xState_.PWMOutputConfig.PWMOUT2 = iotype;
	if (fan == EMC230X_FAN::FANCNTRL_3) emc230xState_.PWMOutputConfig.PWMOUT3 = iotype;
	if (fan == EMC230X_FAN::FANCNTRL_4) emc230xState_.PWMOutputConfig.PWMOUT4 = iotype;
	if (fan == EMC230X_FAN::FANCNTRL_5) emc230xState_.PWMOutputConfig.PWMOUT5 = iotype;

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, EMC230X_REG_PWMPOLARITY, &emc230xState_.PWMOutputConfig.pwmOutputConfig, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	state = getPWMOutput();
	return state;
}

EMC230X_STATUS EMC230X::setPWMBaseFreq(EMC230X_FAN fan, EMC230X_PWMFREQ freq)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}
	EMC230X_STATUS state = getPWMBaseFreq();

	// These setters are read-modify-write: bail if the read failed, otherwise we
	// would write the zeroed cache back over the chip's real configuration.
	if (state != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return state;
	}

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_PWMBASEFREQ123; emc230xState_.PWMBaseF123.PMB1 = freq; reg = emc230xState_.PWMBaseF123.pwmBaseF123;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_PWMBASEFREQ123; emc230xState_.PWMBaseF123.PMB2 = freq; reg = emc230xState_.PWMBaseF123.pwmBaseF123;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_PWMBASEFREQ123; emc230xState_.PWMBaseF123.PMB3 = freq; reg = emc230xState_.PWMBaseF123.pwmBaseF123;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_PWMBASEFREQ45;  emc230xState_.PWMBaseF45.PMB4 = freq; reg = emc230xState_.PWMBaseF45.pwmBaseF45;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_PWMBASEFREQ45;  emc230xState_.PWMBaseF45.PMB5 = freq; reg = emc230xState_.PWMBaseF45.pwmBaseF45;}

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, addr, &reg, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	state = getPWMBaseFreq();
	return state;
}

EMC230X_STATUS EMC230X::setPWMDivider(EMC230X_FAN fan, uint8_t divider)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}
	EMC230X_STATUS state = getPWMDivider();

	// These setters are read-modify-write: bail if the read failed, otherwise we
	// would write the zeroed cache back over the chip's real configuration.
	if (state != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return state;
	}

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1PWMDIVIDE; emc230xState_.PWMDividers.PWM1Div = divider; reg = emc230xState_.PWMDividers.PWM1Div;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2PWMDIVIDE; emc230xState_.PWMDividers.PWM2Div = divider; reg = emc230xState_.PWMDividers.PWM2Div;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3PWMDIVIDE; emc230xState_.PWMDividers.PWM3Div = divider; reg = emc230xState_.PWMDividers.PWM3Div;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4PWMDIVIDE; emc230xState_.PWMDividers.PWM4Div = divider; reg = emc230xState_.PWMDividers.PWM4Div;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5PWMDIVIDE; emc230xState_.PWMDividers.PWM5Div = divider; reg = emc230xState_.PWMDividers.PWM5Div;}

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, addr, &reg, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	state = getPWMDivider();
	return state;
}

EMC230X_STATUS EMC230X::setFanDriveSettings(EMC230X_FAN fan, uint8_t speed)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}
	EMC230X_STATUS state = getFanDriveSettings();

	// These setters are read-modify-write: bail if the read failed, otherwise we
	// would write the zeroed cache back over the chip's real configuration.
	if (state != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return state;
	}

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1DRIVESETTING; emc230xState_.FanDriveSetting.PWM1 = speed; reg = emc230xState_.FanDriveSetting.PWM1;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2DRIVESETTING; emc230xState_.FanDriveSetting.PWM2 = speed; reg = emc230xState_.FanDriveSetting.PWM2;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3DRIVESETTING; emc230xState_.FanDriveSetting.PWM3 = speed; reg = emc230xState_.FanDriveSetting.PWM3;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4DRIVESETTING; emc230xState_.FanDriveSetting.PWM4 = speed; reg = emc230xState_.FanDriveSetting.PWM4;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5DRIVESETTING; emc230xState_.FanDriveSetting.PWM5 = speed; reg = emc230xState_.FanDriveSetting.PWM5;}

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, addr, &reg, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	state = getFanDriveSettings();
	return state;
}

EMC230X_STATUS EMC230X::setFanInterrupt(EMC230X_FAN fan, bool enabled)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	EMC230X_STATUS state = getFanInterruptEnable();

	// These setters are read-modify-write: bail if the read failed, otherwise we
	// would write the zeroed cache back over the chip's real configuration.
	if (state != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return state;
	}

	if (enabled == true) emc230xState_.FanInterruptEnable.fanInterruptEnable |= (uint8_t) fan;
	else emc230xState_.FanInterruptEnable.fanInterruptEnable &= (uint8_t) (~fan);

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, EMC230X_REG_INTERRUPTENABLE, &emc230xState_.FanInterruptEnable.fanInterruptEnable, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	state = (EMC230X_STATUS)(state | getFanInterruptEnable());
	return state;
}

EMC230X_STATUS EMC230X::setFanConfig1(EMC230X_FAN fan, EMC230X_FanConfig1 cfg)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}
	EMC230X_STATUS state;

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1CONFIG1; emc230xState_.Fan1Config1 = cfg; reg = emc230xState_.Fan1Config1.fanConfig1;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2CONFIG1; emc230xState_.Fan2Config1 = cfg; reg = emc230xState_.Fan2Config1.fanConfig1;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3CONFIG1; emc230xState_.Fan3Config1 = cfg; reg = emc230xState_.Fan3Config1.fanConfig1;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4CONFIG1; emc230xState_.Fan4Config1 = cfg; reg = emc230xState_.Fan4Config1.fanConfig1;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5CONFIG1; emc230xState_.Fan5Config1 = cfg; reg = emc230xState_.Fan5Config1.fanConfig1;}

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, addr, &reg, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	state = getFanConfig1();
	return state;
}

EMC230X_STATUS EMC230X::setFanConfig2(EMC230X_FAN fan, EMC230X_FanConfig2 cfg)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}
	EMC230X_STATUS state;

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1CONFIG2; emc230xState_.Fan1Config2 = cfg; reg = emc230xState_.Fan1Config2.fanConfig2;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2CONFIG2; emc230xState_.Fan2Config2 = cfg; reg = emc230xState_.Fan2Config2.fanConfig2;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3CONFIG2; emc230xState_.Fan3Config2 = cfg; reg = emc230xState_.Fan3Config2.fanConfig2;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4CONFIG2; emc230xState_.Fan4Config2 = cfg; reg = emc230xState_.Fan4Config2.fanConfig2;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5CONFIG2; emc230xState_.Fan5Config2 = cfg; reg = emc230xState_.Fan5Config2.fanConfig2;}

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, addr, &reg, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	state = getFanConfig2();
	return state;
}

EMC230X_STATUS EMC230X::setPIDGain(EMC230X_FAN fan, EMC230X_PIDGain gain)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}
	EMC230X_STATUS state;

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1PIDGAIN; emc230xState_.Fan1PIDGain = gain; reg = emc230xState_.Fan1PIDGain.pidGain;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2PIDGAIN; emc230xState_.Fan2PIDGain = gain; reg = emc230xState_.Fan2PIDGain.pidGain;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3PIDGAIN; emc230xState_.Fan3PIDGain = gain; reg = emc230xState_.Fan3PIDGain.pidGain;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4PIDGAIN; emc230xState_.Fan4PIDGain = gain; reg = emc230xState_.Fan4PIDGain.pidGain;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5PIDGAIN; emc230xState_.Fan5PIDGain = gain; reg = emc230xState_.Fan5PIDGain.pidGain;}

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, addr, &reg, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	state = getPIDGain();
	return state;
}

EMC230X_STATUS EMC230X::setFanSpinUpConfig(EMC230X_FAN fan, EMC230X_FanSpinUpConfig spinup)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}
	EMC230X_STATUS state;

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1SPINUP; emc230xState_.FanSpinUp1 = spinup; reg = emc230xState_.FanSpinUp1.fanSpinUpConfig;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2SPINUP; emc230xState_.FanSpinUp2 = spinup; reg = emc230xState_.FanSpinUp2.fanSpinUpConfig;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3SPINUP; emc230xState_.FanSpinUp3 = spinup; reg = emc230xState_.FanSpinUp3.fanSpinUpConfig;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4SPINUP; emc230xState_.FanSpinUp4 = spinup; reg = emc230xState_.FanSpinUp4.fanSpinUpConfig;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5SPINUP; emc230xState_.FanSpinUp5 = spinup; reg = emc230xState_.FanSpinUp5.fanSpinUpConfig;}

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, addr, &reg, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	state = getFanSpinUpConfig();
	return state;
}

EMC230X_STATUS EMC230X::setFanMaxStep(EMC230X_FAN fan, uint8_t maxstep)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}
	EMC230X_STATUS state;

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1MAXSTEP; emc230xState_.FanDrvMaxStepSize1 = maxstep; reg = emc230xState_.FanDrvMaxStepSize1;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2MAXSTEP; emc230xState_.FanDrvMaxStepSize2 = maxstep; reg = emc230xState_.FanDrvMaxStepSize2;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3MAXSTEP; emc230xState_.FanDrvMaxStepSize3 = maxstep; reg = emc230xState_.FanDrvMaxStepSize3;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4MAXSTEP; emc230xState_.FanDrvMaxStepSize4 = maxstep; reg = emc230xState_.FanDrvMaxStepSize4;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5MAXSTEP; emc230xState_.FanDrvMaxStepSize5 = maxstep; reg = emc230xState_.FanDrvMaxStepSize5;}

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, addr, &reg, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	state = getFanMaxStep();
	return state;
}

EMC230X_STATUS EMC230X::setFanMinDrive(EMC230X_FAN fan, uint8_t minstep)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}
	EMC230X_STATUS state;

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1MINDRIVE; emc230xState_.FanDrvMinStepSize1 = minstep; reg = emc230xState_.FanDrvMinStepSize1;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2MINDRIVE; emc230xState_.FanDrvMinStepSize2 = minstep; reg = emc230xState_.FanDrvMinStepSize2;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3MINDRIVE; emc230xState_.FanDrvMinStepSize3 = minstep; reg = emc230xState_.FanDrvMinStepSize3;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4MINDRIVE; emc230xState_.FanDrvMinStepSize4 = minstep; reg = emc230xState_.FanDrvMinStepSize4;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5MINDRIVE; emc230xState_.FanDrvMinStepSize5 = minstep; reg = emc230xState_.FanDrvMinStepSize5;}

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, addr, &reg, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	state = getFanMinDrive();
	return state;
}

EMC230X_STATUS EMC230X::setDriveFailBand(EMC230X_FAN fan, uint16_t fail)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}
	EMC230X_STATUS state;

	uint8_t regmsb = (uint8_t)(fail >> 8);
	uint8_t reglsb = (uint8_t)(fail & 0xFF);

	uint8_t addrmsb = 0;
	uint8_t addrlsb = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) { addrmsb = EMC230X_REG_FAN1DRVFAILMSB; addrlsb = EMC230X_REG_FAN1DRVFAILLSB; }

	if (fan == EMC230X_FAN::FANCNTRL_2) { addrmsb = EMC230X_REG_FAN2DRVFAILMSB; addrlsb = EMC230X_REG_FAN2DRVFAILLSB; }

	if (fan == EMC230X_FAN::FANCNTRL_3) { addrmsb = EMC230X_REG_FAN3DRVFAILMSB; addrlsb = EMC230X_REG_FAN3DRVFAILLSB; }

	if (fan == EMC230X_FAN::FANCNTRL_4) { addrmsb = EMC230X_REG_FAN4DRVFAILMSB; addrlsb = EMC230X_REG_FAN4DRVFAILLSB; }

	if (fan == EMC230X_FAN::FANCNTRL_5) { addrmsb = EMC230X_REG_FAN5DRVFAILMSB; addrlsb = EMC230X_REG_FAN5DRVFAILLSB; }

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, addrlsb, &reglsb, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, addrmsb, &regmsb, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	state = getDriveFailBand();
	return state;
}

EMC230X_STATUS EMC230X::setFanTachTarget(EMC230X_FAN fan, uint16_t tach)
{ // Always write LSB first pg. 45 datasheet
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}

	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	// The 13-bit count is left-justified in the register pair (bits 15:3), so shift
	// the whole value before splitting. Shifting the masked LSB alone leaves the MSB
	// a factor of 8 short.
	uint16_t raw = (uint16_t)(tach << 3);

	uint8_t lsb = (uint8_t)(raw & 0x00FF);
	uint8_t msb = (uint8_t)(raw >> 8);

	uint8_t addrmsb = 0;
	uint8_t addrlsb = 0;

	EMC230X_STATUS state = EMC230X_STATUS::EMC230X_STATUS_OK;

	if (fan == EMC230X_FAN::FANCNTRL_1) { addrmsb = EMC230X_REG_FAN1TACHTARGETMSB; addrlsb = EMC230X_REG_FAN1TACHTARGETLSB; }
	if (fan == EMC230X_FAN::FANCNTRL_2) { addrmsb = EMC230X_REG_FAN2TACHTARGETMSB; addrlsb = EMC230X_REG_FAN2TACHTARGETLSB; }
	if (fan == EMC230X_FAN::FANCNTRL_3) { addrmsb = EMC230X_REG_FAN3TACHTARGETMSB; addrlsb = EMC230X_REG_FAN3TACHTARGETLSB; }
	if (fan == EMC230X_FAN::FANCNTRL_4) { addrmsb = EMC230X_REG_FAN4TACHTARGETMSB; addrlsb = EMC230X_REG_FAN4TACHTARGETLSB; }
	if (fan == EMC230X_FAN::FANCNTRL_5) { addrmsb = EMC230X_REG_FAN5TACHTARGETMSB; addrlsb = EMC230X_REG_FAN5TACHTARGETLSB; }

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, addrlsb, &lsb, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, addrmsb, &msb, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	state = getFanTachTarget();
	return state;
}

EMC230X_STATUS EMC230X::setFanValidTachCount(EMC230X_FAN fan, uint8_t count)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}
	// The caller must install an SMBus driver via setI2CDriver() first. Reported
	// rather than asserted so release builds (NDEBUG) fail safe instead of
	// calling through a null pointer.
	if (smbus_read_reg == nullptr || smbus_write_reg == nullptr)
	{
		return EMC230X_STATUS::EMC230X_SMBUS_DRIVER_NULL;
	}

	uint8_t addr=0;

	EMC230X_STATUS state = EMC230X_STATUS::EMC230X_STATUS_OK;

	if (fan == EMC230X_FAN::FANCNTRL_1) addr = EMC230X_REG_FAN1VALTACHCOUNT;
	if (fan == EMC230X_FAN::FANCNTRL_2) addr = EMC230X_REG_FAN2VALTACHCOUNT;
	if (fan == EMC230X_FAN::FANCNTRL_3) addr = EMC230X_REG_FAN3VALTACHCOUNT;
	if (fan == EMC230X_FAN::FANCNTRL_4) addr = EMC230X_REG_FAN4VALTACHCOUNT;
	if (fan == EMC230X_FAN::FANCNTRL_5) addr = EMC230X_REG_FAN5VALTACHCOUNT;

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, addr, &count, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	state = getFanValidTachCount();
	return state;
}

// The number of fan poles would also affect the tachometer counting because
// different fans generate different amount of pulses per rotation. Most fans
// are 2 poles, but this info should be obtained directly from the fan datasheet.
EMC230X_STATUS EMC230X::setFanPoles(EMC230X_FAN fan, EMC230X_FANPOLES poles)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}
	uint8_t multiplier = 0;
	uint8_t reg = 0;
	uint8_t configreg = 0;

	if ( poles == EMC230X_FANPOLES::FANPOLE_1 ) multiplier = 1; // 0.5 * 2 pg 39
	if ( poles == EMC230X_FANPOLES::FANPOLE_2 ) multiplier = 2; //   1 * 2 pg 39
	if ( poles == EMC230X_FANPOLES::FANPOLE_3 ) multiplier = 3; // 1.5 * 2 pg 39
	if ( poles == EMC230X_FANPOLES::FANPOLE_4 ) multiplier = 4; //   2 * 2 pg 39

	// To avoid doubles, we first multiply the fan pole multiplier by 2 to make it an integer.
	// It will be divided by 2 in the equations relating RPM to tachometer readings.

	EMC230X_STATUS status = getFanConfig1(); // get latest

	// These setters are read-modify-write: bail if the read failed, otherwise we
	// would write the zeroed cache back over the chip's real configuration.
	if (status != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return status;
	}

	switch (fan)
	{
		case EMC230X_FAN::FANCNTRL_1 :
		{
			fan1Poles_ = poles;
			tachFan1PolesMultiplier_ = multiplier;
			reg = EMC230X_REG_FAN1CONFIG1;
			emc230xState_.Fan1Config1.EDG = poles;
			configreg = emc230xState_.Fan1Config1.fanConfig1;
			break;
		}
		case EMC230X_FAN::FANCNTRL_2 :
		{
			fan2Poles_ = poles;
			tachFan2PolesMultiplier_ = multiplier;
			reg = EMC230X_REG_FAN2CONFIG1;
			emc230xState_.Fan2Config1.EDG = poles;
			configreg = emc230xState_.Fan2Config1.fanConfig1;
			break;
		}
		case EMC230X_FAN::FANCNTRL_3 :
		{
			fan3Poles_ = poles;
			tachFan3PolesMultiplier_ = multiplier;
			reg = EMC230X_REG_FAN3CONFIG1;
			emc230xState_.Fan3Config1.EDG = poles;
			configreg = emc230xState_.Fan3Config1.fanConfig1;
			break;
		}
		case EMC230X_FAN::FANCNTRL_4 :
		{
			fan4Poles_ = poles;
			tachFan4PolesMultiplier_ = multiplier;
			reg = EMC230X_REG_FAN4CONFIG1;
			emc230xState_.Fan4Config1.EDG = poles;
			configreg = emc230xState_.Fan4Config1.fanConfig1;
			break;
		}
		case EMC230X_FAN::FANCNTRL_5 :
		{
			fan5Poles_ = poles;
			tachFan5PolesMultiplier_ = multiplier;
			reg = EMC230X_REG_FAN5CONFIG1;
			emc230xState_.Fan5Config1.EDG = poles;
			configreg = emc230xState_.Fan5Config1.fanConfig1;
			break;
		}
	}

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, reg, &configreg, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	status = getFanConfig1(); // get latest

	return status;
}

// Toggles the RPM-based Fan Speed Control Algorithm, whereby the fan speed will be controlled
// by the EMC230X using a PID algorithm based on the target tachometer reading given by user.
EMC230X_STATUS EMC230X::toggleControlAlgorithm(EMC230X_FAN fan, bool enable)
{
	// Reject anything that is not exactly one valid fan selector. Without this the
	// if-chains below leave the register address at 0 and write to register 0x00.
	if (!isValidFan(fan))
	{
		return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	}

	EMC230X_STATUS status = getFanConfig1(); // get latest

	// These setters are read-modify-write: bail if the read failed, otherwise we
	// would write the zeroed cache back over the chip's real configuration.
	if (status != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return status;
	}

	uint8_t reg = 0;
	uint8_t configreg = 0;

	switch (fan)
	{
		case EMC230X_FAN::FANCNTRL_1 :
		{
			reg = EMC230X_REG_FAN1CONFIG1;
			emc230xState_.Fan1Config1.ENAG = (enable == true) ? 1 : 0;
			configreg = emc230xState_.Fan1Config1.fanConfig1;
			break;
		}
		case EMC230X_FAN::FANCNTRL_2 :
		{
			reg = EMC230X_REG_FAN2CONFIG1;
			emc230xState_.Fan2Config1.ENAG = (enable == true) ? 1 : 0;
			configreg = emc230xState_.Fan2Config1.fanConfig1;
			break;
		}
		case EMC230X_FAN::FANCNTRL_3 :
		{
			reg = EMC230X_REG_FAN3CONFIG1;
			emc230xState_.Fan3Config1.ENAG = (enable == true) ? 1 : 0;
			configreg = emc230xState_.Fan3Config1.fanConfig1;
			break;
		}
		case EMC230X_FAN::FANCNTRL_4 :
		{
			reg = EMC230X_REG_FAN4CONFIG1;
			emc230xState_.Fan4Config1.ENAG = (enable == true) ? 1 : 0;
			configreg = emc230xState_.Fan4Config1.fanConfig1;
			break;
		}
		case EMC230X_FAN::FANCNTRL_5 :
		{
			reg = EMC230X_REG_FAN5CONFIG1;
			emc230xState_.Fan5Config1.ENAG = (enable == true) ? 1 : 0;
			configreg = emc230xState_.Fan5Config1.fanConfig1;
			break;
		}
	}

	// The driver returns its own int error code, which is not an EMC230X_STATUS --
	// map it rather than casting a foreign value into the enum.
	if (smbus_write_reg(i2cAddr, reg, &configreg, 1) != 0)
	{
		return EMC230X_STATUS::EMC230X_STATUS_FAIL;
	}

	status = getFanConfig1(); // get latest

	return status;
}

// Obtain the tachometer reading, convert to RPM, and store in a private variable.
// Get the fan speed (RPM) in RPM
// DONT USE THIS USE THE INTEGER - getFanRPMReading
EMC230X_STATUS EMC230X::getFanRPM(EMC230X_FAN fan, uint16_t *rpm)
{
	if (rpm == nullptr)  return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	if (!isValidFan(fan)) return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;

	*rpm = 0;

	uint16_t tach = 1;
	uint8_t edges = 0;
	float invpoles = 1.0f;
	float invmult = 1.0f;
	float ftach = 32.768E3f;
	EMC230X_FanConfig1 configReg = {0};
	EMC230X_FANPOLES poleSetting;

	// Both refresh the cache this function reads. Propagate their failure instead of
	// computing a speed from whatever the cache happens to hold.
	EMC230X_STATUS status = getFanConfig1();
	if (status != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return status;
	}

	status = getFanTachReading();
	if (status != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		return status;
	}

	switch (fan)
	{
		case EMC230X_FAN::FANCNTRL_1 : { configReg = emc230xState_.Fan1Config1;  tach = emc230xState_.TachCurrent1; poleSetting = fan1Poles_; break; }
		case EMC230X_FAN::FANCNTRL_2 : { configReg = emc230xState_.Fan2Config1;  tach = emc230xState_.TachCurrent2; poleSetting = fan2Poles_; break; }
		case EMC230X_FAN::FANCNTRL_3 : { configReg = emc230xState_.Fan3Config1;  tach = emc230xState_.TachCurrent3; poleSetting = fan3Poles_; break; }
		case EMC230X_FAN::FANCNTRL_4 : { configReg = emc230xState_.Fan4Config1;  tach = emc230xState_.TachCurrent4; poleSetting = fan4Poles_; break; }
		case EMC230X_FAN::FANCNTRL_5 : { configReg = emc230xState_.Fan5Config1;  tach = emc230xState_.TachCurrent5; poleSetting = fan5Poles_; break; }
	}

	switch (poleSetting)
	{
		case EMC230X_FANPOLES::FANPOLE_1 : { invpoles = 1.0f; edges = 3 - 1; break; }
		case EMC230X_FANPOLES::FANPOLE_2 : { invpoles = 0.5f; edges = 5 - 1; break; }
		case EMC230X_FANPOLES::FANPOLE_3 : { invpoles = 0.33333f; edges = 7 - 1; break; }
		case EMC230X_FANPOLES::FANPOLE_4 : { invpoles = 0.25f; edges = 9 - 1; break; }
	}

	if ( configReg.FRNG == 0 ) invmult = 1.0f;
	if ( configReg.FRNG == 1 ) invmult = 0.5f;
	if ( configReg.FRNG == 2 ) invmult = 0.25f;
	if ( configReg.FRNG == 3 ) invmult = 0.125f;

	// Full scale means no tach edges arrived -- a stopped or disconnected fan. Valid
	// observation, so 0 RPM with an OK status. A count of 0 is physically impossible
	// and would divide by zero (inf cast to uint16_t is undefined behaviour), so it can
	// only mean the cache is unpopulated; report that as a failure.
	if (tach >= TACHO_OFF) return EMC230X_STATUS::EMC230X_STATUS_OK;
	if (tach == 0)         return EMC230X_STATUS::EMC230X_STATUS_FAIL;

	float frpm = invpoles * (edges / (1.0f*tach*invmult)) * ftach * 60.0f;

	*rpm = (uint16_t) frpm;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

/* RPM from the cached TACH reading, integer only -- Equation 4-3 of the datasheet,
   RPM = 3932160 * m / COUNT, with m taken from the cached RNG bits.

   Uses cached state and does NO I2C. Call getFanTachReading() (the no-argument
   overload) first to refresh, otherwise this returns whatever was last read.

   Returns 0 for a stopped or disconnected fan: a full-scale count means no tach
   edges arrived in the measurement window, which is not a real speed. */
EMC230X_STATUS EMC230X::getFanRPMReading(EMC230X_FAN fan, uint16_t *rpm)
{
	if (rpm == nullptr)  return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;
	if (!isValidFan(fan)) return EMC230X_STATUS::EMC230X_STATUS_INVALIDARG;

	*rpm = 0;

	uint16_t tach = getFanTachReading(fan);

	// A full-scale count means no tach edges arrived in the measurement window, i.e. the
	// fan is stopped or disconnected. That is a valid observation, not an error, so it
	// reports OK with 0 RPM. A count of 0 cannot occur physically and means the cache was
	// never populated -- report that as a failure rather than as a stopped fan.
	if (tach >= TACHO_OFF) return EMC230X_STATUS::EMC230X_STATUS_OK;
	if (tach == 0)         return EMC230X_STATUS::EMC230X_STATUS_FAIL;

	uint8_t rng = 0;
	switch (fan)
	{
		case EMC230X_FAN::FANCNTRL_1 : rng = emc230xState_.Fan1Config1.FRNG; break;
		case EMC230X_FAN::FANCNTRL_2 : rng = emc230xState_.Fan2Config1.FRNG; break;
		case EMC230X_FAN::FANCNTRL_3 : rng = emc230xState_.Fan3Config1.FRNG; break;
		case EMC230X_FAN::FANCNTRL_4 : rng = emc230xState_.Fan4Config1.FRNG; break;
		case EMC230X_FAN::FANCNTRL_5 : rng = emc230xState_.Fan5Config1.FRNG; break;
	}

	uint32_t m = 1u << rng; // RNG 00/01/10/11 -> multiplier 1/2/4/8

	*rpm = (uint16_t) ((TACH_RPM_CONST * m) / tach);

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

/* Convert a desired fan speed to a tach count for the settings applied by
   runControlAlgorithm() (2 pulses/rev, 5 edges, FRNG multiplier 1).
   0 RPM returns TACHO_OFF, which the chip reads as "drive to 0%".
   Anything slower than TACH_MIN_RPM is clamped -- the 13-bit count cannot
   represent it, and the chip would report a stall instead. */
uint16_t EMC230X::rpmToTachCount(uint16_t rpm)
{
	if (rpm == 0) return TACHO_OFF;
	if (rpm < TACH_MIN_RPM) rpm = TACH_MIN_RPM;

	uint32_t count = TACH_RPM_CONST / rpm;
	if (count > TACHO_OFF) count = TACHO_OFF;

	return (uint16_t) count;
}

/* targetRPM is a fan speed, not a raw tach count. 0 parks the fan. */
void EMC230X::runControlAlgorithm(EMC230X_FAN fan, uint16_t targetRPM)
{
	if (!isValidFan(fan))
	{
		return;
	}

	toggleControlAlgorithm(fan, false);

	setFanPoles(fan, EMC230X_FANPOLES::FANPOLE_2); // 2 tach pulses per revolution

	EMC230X_FanConfig1 cfg = {0};
	cfg.UDT  = 3; // 400 ms PID update -- must exceed the 250 ms measurement at 480 RPM
	cfg.EDG  = 1; // 5 edges, i.e. 2 pulses/rev. NOT the motor's pole count.
	cfg.FRNG = 0; // multiplier 1: only setting that measures down to 480 RPM
	cfg.ENAG = 0; // closed loop enabled at the end, once everything else is set
	setFanConfig1(fan, cfg);      // 0x8B once ENAG is toggled on

	EMC230X_FanConfig2 cfg2 = {0};
	cfg2.ERG  = 1; // 50 RPM error window: 1% at 5000 RPM, 5% at 1000. POR is 0 RPM,
	               // which never stops updating the drive.
	cfg2.DPT  = 1; // basic derivative -- damps overshoot on a low-inertia 40 mm rotor
	cfg2.GHEN = 1; // glitch filter on the tach input
	cfg2.ENRC = 0; // Register 6-14: "available only when ENAGx = 0". Inert in closed
	               // loop -- there, MaxStep + UDT ramp the drive automatically (4.10).
	setFanConfig2(fan, cfg2);     // 0x2A

	setFanDriveSettings(fan, 0);
	setPWMOutput(fan, 0);         // open drain -- the fan supplies the pull-up (Intel 4-wire)
	setPWMBaseFreq(fan, EMC230X_PWMFREQ::FREQ_26KHZ); // Intel 4-wire wants 21-28 kHz

	getFanTachReading();
	getChipState();

	EMC230X_FanSpinUpConfig fspuc;
	fspuc.DFC  = 3; // 64 periods
	fspuc.NKCK = 1; // do not drive to 100% PWM
	fspuc.SPLV = 7; // 65%
	fspuc.SPT  = 3; // 2000ms spin up time
	setFanSpinUpConfig(fan, fspuc);

	// 4.10 documents the range as 1..31 counts; the register is only F1MS[5:0].
	// 32 was outside it. 31 with UDT=400 ms ramps 0->100% in ~3.3 s. POR is 0x10.
	setFanMaxStep(fan, 31);
	// ponytail: provisional. Keeps the loop above the 480 RPM tach floor, assuming
	// duty scales roughly linearly from the 20% -> 1050 RPM datasheet point.
	// Sweep setFanDriveSettings() 0..64 with the loop off and set this from measurement.
	setFanMinDrive(fan, 26);
	setFanValidTachCount(fan, 0xFE); // stall below count 8128 ~= 484 RPM

	setFanTachTarget(fan, rpmToTachCount(targetRPM));

	toggleControlAlgorithm(fan, true);
}

} // end namespace
