/*
 * EMC230X.cpp
 *
 *  Created on: Nov 8, 2024
 *      Author: Michael Margolese
 */

#include <EMC230X.hpp>
#include <cassert>

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
	// Base config for a fan with 2 poles and 500 min RPM.
	// Fan starts in off state.
	i2cAddr                 = I2C_ADDRESS;
	smbus_write_reg         = nullptr;
	smbus_read_reg          = nullptr;
	emc230xState_           = {0};
	fanControllers_         = EMC230X_FAN::FANCNTRL_1;
	fan1Poles_               = EMC230X_FANPOLES::FANPOLE_1;
	fan2Poles_               = EMC230X_FANPOLES::FANPOLE_1;
	fan3Poles_               = EMC230X_FANPOLES::FANPOLE_1;
	fan4Poles_               = EMC230X_FANPOLES::FANPOLE_1;
	fan5Poles_               = EMC230X_FANPOLES::FANPOLE_1;
	tachFan1PolesMultiplier_ = 0;
	tachFan2PolesMultiplier_ = 0;
	tachFan3PolesMultiplier_ = 0;
	tachFan4PolesMultiplier_ = 0;
	tachFan5PolesMultiplier_ = 0;
}

EMC230X::~EMC230X()
{
}

EMC230X_STATUS EMC230X::getChipState()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

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
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	emc230xState_.ChipInfo.ProductFeatures = 0;
	emc230xState_.ChipInfo.ProductID       = 0;
	emc230xState_.ChipInfo.ManufacturerID  = 0;
	emc230xState_.ChipInfo.Revision        = 0;
	int state = 0;
	uint8_t udata = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_PRODUCTFEATURES, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.ChipInfo.ProductFeatures = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_PRODUCTID, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.ChipInfo.ProductID = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_MANUFACTURERID, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.ChipInfo.ManufacturerID = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_CHIPREVISION, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.ChipInfo.Revision = udata;

	switch ((EMC230X_PRODUCTID)emc230xState_.ChipInfo.ProductID)
	{
		case EMC230X_PRODUCTID::EMC2301_ID: fanControllers_ = EMC230X_FAN::FANCNTRL_1; break;
		case EMC230X_PRODUCTID::EMC2302_ID: fanControllers_ = EMC230X_FAN::FANCNTRL_2; break;
		case EMC230X_PRODUCTID::EMC2303_ID: fanControllers_ = EMC230X_FAN::FANCNTRL_3; break;
		case EMC230X_PRODUCTID::EMC2305_ID: fanControllers_ = EMC230X_FAN::FANCNTRL_5; break;
		default: fanControllers_ = EMC230X_FAN::FANCNTRL_1;
	}

	return EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getChipConfig()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.Configuration.Config = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_CONFIGURATION, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Configuration.Config = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanStatus()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanStatus.fanStatus = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_STATUS, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanStatus.fanStatus = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanStallStatus()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanStallStatus.fanStallStatus = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_STALLSTATUS, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanStallStatus.fanStallStatus = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanSpinStatus()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanSpinStatus.fanSpinStatus = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_SPINSTATUS, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanSpinStatus.fanSpinStatus = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getDriveFailStatus()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.DriveFailStatus.driveFailStatus = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_DRIVEFAILSTATUS, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.DriveFailStatus.driveFailStatus = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanInterruptEnable()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanInterruptEnable.fanInterruptEnable = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_INTERUPTENABLE, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanInterruptEnable.fanInterruptEnable = (uint8_t)(udata & 0x1F);

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getPWMPolarity()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.PWMPolarity.pwmPolarity = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_PWMPOLARITY, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.PWMPolarity.pwmPolarity = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getPWMOutput()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.PWMOutputConfig.pwmOutputConfig = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_PWMOUTPUT, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.PWMOutputConfig.pwmOutputConfig = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getPWMBaseFreq()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.PWMBaseF123.pwmBaseF123 = 0;
	emc230xState_.PWMBaseF45.pwmBaseF45 = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_PWMBASEFREQ45, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.PWMBaseF45.pwmBaseF45 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_PWMBASEFREQ123, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.PWMBaseF123.pwmBaseF123 = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getPWMDivider()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.PWMDividers.PWM1Div = 0;
	emc230xState_.PWMDividers.PWM2Div = 0;
	emc230xState_.PWMDividers.PWM3Div = 0;
	emc230xState_.PWMDividers.PWM4Div = 0;
	emc230xState_.PWMDividers.PWM5Div = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN1PWMDIVIDE, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.PWMDividers.PWM1Div = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN2PWMDIVIDE, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.PWMDividers.PWM2Div = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN3PWMDIVIDE, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.PWMDividers.PWM3Div = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN4PWMDIVIDE, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.PWMDividers.PWM4Div = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN5PWMDIVIDE, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.PWMDividers.PWM5Div = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanDriveSettings()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanDriveSetting.PWM1 = 0;
	emc230xState_.FanDriveSetting.PWM2 = 0;
	emc230xState_.FanDriveSetting.PWM3 = 0;
	emc230xState_.FanDriveSetting.PWM4 = 0;
	emc230xState_.FanDriveSetting.PWM5 = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN1DRIVESETTING, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanDriveSetting.PWM1 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN2DRIVESETTING, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanDriveSetting.PWM2 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN3DRIVESETTING, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanDriveSetting.PWM3 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN4DRIVESETTING, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanDriveSetting.PWM4 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN5DRIVESETTING, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanDriveSetting.PWM5 = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanConfig1()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.Fan1Config1.fanConfig1 = 0;
	emc230xState_.Fan2Config1.fanConfig1 = 0;
	emc230xState_.Fan3Config1.fanConfig1 = 0;
	emc230xState_.Fan4Config1.fanConfig1 = 0;
	emc230xState_.Fan5Config1.fanConfig1 = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN1CONFIG1, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Fan1Config1.fanConfig1 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN2CONFIG1, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Fan2Config1.fanConfig1 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN3CONFIG1, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Fan3Config1.fanConfig1 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN4CONFIG1, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Fan4Config1.fanConfig1 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN5CONFIG1, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Fan5Config1.fanConfig1 = udata;

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
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.Fan1Config2.fanConfig2 = 0;
	emc230xState_.Fan2Config2.fanConfig2 = 0;
	emc230xState_.Fan3Config2.fanConfig2 = 0;
	emc230xState_.Fan4Config2.fanConfig2 = 0;
	emc230xState_.Fan5Config2.fanConfig2 = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN1CONFIG2, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Fan1Config2.fanConfig2 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN2CONFIG2, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Fan2Config2.fanConfig2 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN3CONFIG2, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Fan3Config2.fanConfig2 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN4CONFIG2, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Fan4Config2.fanConfig2 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN5CONFIG2, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Fan5Config2.fanConfig2 = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getPIDGain()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.Fan1PIDGain.pidGain = 0;
	emc230xState_.Fan2PIDGain.pidGain = 0;
	emc230xState_.Fan3PIDGain.pidGain = 0;
	emc230xState_.Fan4PIDGain.pidGain = 0;
	emc230xState_.Fan5PIDGain.pidGain = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN1PIDGAIN, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Fan1PIDGain.pidGain = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN2PIDGAIN, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Fan2PIDGain.pidGain = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN3PIDGAIN, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Fan3PIDGain.pidGain = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN4PIDGAIN, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Fan4PIDGain.pidGain = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN5PIDGAIN, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.Fan5PIDGain.pidGain = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanSpinUpConfig()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanSpinUp1.fanSpinUpConfig = 0;
	emc230xState_.FanSpinUp2.fanSpinUpConfig = 0;
	emc230xState_.FanSpinUp3.fanSpinUpConfig = 0;
	emc230xState_.FanSpinUp4.fanSpinUpConfig = 0;
	emc230xState_.FanSpinUp5.fanSpinUpConfig = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN1SPINUP, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanSpinUp1.fanSpinUpConfig = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN2SPINUP, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanSpinUp2.fanSpinUpConfig = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN3SPINUP, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanSpinUp3.fanSpinUpConfig = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN4SPINUP, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanSpinUp4.fanSpinUpConfig = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN5SPINUP, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanSpinUp5.fanSpinUpConfig = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanMaxStep()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanDrvMaxStepSize1 = 0;
	emc230xState_.FanDrvMaxStepSize2 = 0;
	emc230xState_.FanDrvMaxStepSize3 = 0;
	emc230xState_.FanDrvMaxStepSize4 = 0;
	emc230xState_.FanDrvMaxStepSize5 = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN1MAXSTEP, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanDrvMaxStepSize1 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN2MAXSTEP, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanDrvMaxStepSize2 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN3MAXSTEP, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanDrvMaxStepSize3 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN4MAXSTEP, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanDrvMaxStepSize4 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN5MAXSTEP, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanDrvMaxStepSize5 = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanMinDrive()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.FanDrvMinStepSize1 = 0;
	emc230xState_.FanDrvMinStepSize2 = 0;
	emc230xState_.FanDrvMinStepSize3 = 0;
	emc230xState_.FanDrvMinStepSize4 = 0;
	emc230xState_.FanDrvMinStepSize5 = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN1MINDRIVE, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanDrvMinStepSize1 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN2MINDRIVE, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanDrvMinStepSize2 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN3MINDRIVE, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanDrvMinStepSize3 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN4MINDRIVE, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanDrvMinStepSize4 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN5MINDRIVE, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.FanDrvMinStepSize5 = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanValidTachCount()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.ValidTachCount1 = 0;
	emc230xState_.ValidTachCount2 = 0;
	emc230xState_.ValidTachCount3 = 0;
	emc230xState_.ValidTachCount4 = 0;
	emc230xState_.ValidTachCount5 = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN1VALTACHCOUNT, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.ValidTachCount1 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN2VALTACHCOUNT, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.ValidTachCount2 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN3VALTACHCOUNT, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.ValidTachCount3 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN4VALTACHCOUNT, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.ValidTachCount4 = udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN5VALTACHCOUNT, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.ValidTachCount5 = udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getDriveFailBand()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.DriveFailBand1 = 0;
	emc230xState_.DriveFailBand2 = 0;
	emc230xState_.DriveFailBand3 = 0;
	emc230xState_.DriveFailBand4 = 0;
	emc230xState_.DriveFailBand5 = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN1DRVFAILMSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.DriveFailBand1 = udata << 8;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN1DRVFAILLSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.DriveFailBand1 += udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN2DRVFAILMSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.DriveFailBand2 = udata << 8;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN2DRVFAILLSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.DriveFailBand2 += udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN3DRVFAILMSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.DriveFailBand3 = udata << 8;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN3DRVFAILLSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.DriveFailBand3 += udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN4DRVFAILMSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.DriveFailBand4 = udata << 8;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN4DRVFAILLSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.DriveFailBand4 += udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN5DRVFAILMSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.DriveFailBand5 = udata << 8;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN5DRVFAILLSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.DriveFailBand5 += udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanTachTarget()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.TachTarget1 = 0;
	emc230xState_.TachTarget2 = 0;
	emc230xState_.TachTarget3 = 0;
	emc230xState_.TachTarget4 = 0;
	emc230xState_.TachTarget5 = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN1TACHTARGETMSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachTarget1 = udata << 8;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN1TACHTARGETLSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachTarget1 += udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN2TACHTARGETMSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachTarget2 = udata << 8;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN2TACHTARGETLSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachTarget2 += udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN3TACHTARGETMSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachTarget3 = udata << 8;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN3TACHTARGETLSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachTarget3 += udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN4TACHTARGETMSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachTarget4 = udata << 8;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN4TACHTARGETLSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachTarget4 += udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN5TACHTARGETMSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachTarget5 = udata << 8;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN5TACHTARGETLSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachTarget5 += udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getFanTachReading()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.TachCurrent1 = 0;
	emc230xState_.TachCurrent2 = 0;
	emc230xState_.TachCurrent3 = 0;
	emc230xState_.TachCurrent4 = 0;
	emc230xState_.TachCurrent5 = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN1TACHREADMSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachCurrent1 = udata << 8;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN1TACHREADLSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachCurrent1 += udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN2TACHREADMSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachCurrent2 = udata << 8;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN2TACHREADLSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachCurrent2 += udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN3TACHREADMSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachCurrent3 = udata << 8;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN3TACHREADLSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachCurrent3 += udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN4TACHREADMSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachCurrent4 = udata << 8;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN4TACHREADLSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachCurrent4 += udata;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN5TACHREADMSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachCurrent5 = udata << 8;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_FAN5TACHREADLSB, &udata, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;
	else emc230xState_.TachCurrent5 += udata;

	return EMC230X_STATUS::EMC230X_STATUS_OK;
}

EMC230X_STATUS EMC230X::getSoftWareLock()
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	int state = 0;
	uint8_t udata = 0;
	emc230xState_.SoftwareLock = 0;

	state = smbus_read_reg(I2C_ADDRESS, EMC230X_REG_SOFTWARELOCK, &udata, 1);
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
	if (polarity > 1) polarity = 1;

	EMC230X_STATUS state = getPWMPolarity();

	if (fan == EMC230X_FAN::FANCNTRL_1) emc230xState_.PWMPolarity.POLARITY1 = polarity;
	if (fan == EMC230X_FAN::FANCNTRL_2) emc230xState_.PWMPolarity.POLARITY2 = polarity;
	if (fan == EMC230X_FAN::FANCNTRL_3) emc230xState_.PWMPolarity.POLARITY3 = polarity;
	if (fan == EMC230X_FAN::FANCNTRL_4) emc230xState_.PWMPolarity.POLARITY4 = polarity;
	if (fan == EMC230X_FAN::FANCNTRL_5) emc230xState_.PWMPolarity.POLARITY5 = polarity;

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, EMC230X_REG_PWMPOLARITY, &emc230xState_.PWMPolarity.pwmPolarity, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = getPWMPolarity();
	return state;
}

// 1 - PWM x is Push-Pull Type
// 0 - PWM x is Open Drain Type
EMC230X_STATUS EMC230X::setPWMOutput(EMC230X_FAN fan, uint8_t iotype)
{
	if (iotype > 1) iotype = 1;

	EMC230X_STATUS state = getPWMOutput();

	if (fan == EMC230X_FAN::FANCNTRL_1) emc230xState_.PWMOutputConfig.PWMOUT1 = iotype;
	if (fan == EMC230X_FAN::FANCNTRL_2) emc230xState_.PWMOutputConfig.PWMOUT2 = iotype;
	if (fan == EMC230X_FAN::FANCNTRL_3) emc230xState_.PWMOutputConfig.PWMOUT3 = iotype;
	if (fan == EMC230X_FAN::FANCNTRL_4) emc230xState_.PWMOutputConfig.PWMOUT4 = iotype;
	if (fan == EMC230X_FAN::FANCNTRL_5) emc230xState_.PWMOutputConfig.PWMOUT5 = iotype;

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, EMC230X_REG_PWMPOLARITY, &emc230xState_.PWMOutputConfig.pwmOutputConfig, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = getPWMOutput();
	return state;
}

EMC230X_STATUS EMC230X::setPWMBaseFreq(EMC230X_FAN fan, EMMC230X_PWMFREQ freq)
{
	EMC230X_STATUS state = getPWMBaseFreq();

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_PWMBASEFREQ123; emc230xState_.PWMBaseF123.PMB1 = freq; reg = emc230xState_.PWMBaseF123.pwmBaseF123;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_PWMBASEFREQ123; emc230xState_.PWMBaseF123.PMB2 = freq; reg = emc230xState_.PWMBaseF123.pwmBaseF123;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_PWMBASEFREQ123; emc230xState_.PWMBaseF123.PMB3 = freq; reg = emc230xState_.PWMBaseF123.pwmBaseF123;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_PWMBASEFREQ45;  emc230xState_.PWMBaseF45.PMB4 = freq; reg = emc230xState_.PWMBaseF45.pwmBaseF45;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_PWMBASEFREQ45;  emc230xState_.PWMBaseF45.PMB5 = freq; reg = emc230xState_.PWMBaseF45.pwmBaseF45;}

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, addr, &reg, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = getPWMBaseFreq();
	return state;
}

EMC230X_STATUS EMC230X::setPWMDivider(EMC230X_FAN fan, uint8_t divider)
{
	EMC230X_STATUS state = getPWMDivider();

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1PWMDIVIDE; emc230xState_.PWMDividers.PWM1Div = divider; reg = emc230xState_.PWMDividers.PWM1Div;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2PWMDIVIDE; emc230xState_.PWMDividers.PWM2Div = divider; reg = emc230xState_.PWMDividers.PWM2Div;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3PWMDIVIDE; emc230xState_.PWMDividers.PWM3Div = divider; reg = emc230xState_.PWMDividers.PWM3Div;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4PWMDIVIDE; emc230xState_.PWMDividers.PWM4Div = divider; reg = emc230xState_.PWMDividers.PWM4Div;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5PWMDIVIDE; emc230xState_.PWMDividers.PWM5Div = divider; reg = emc230xState_.PWMDividers.PWM5Div;}

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, addr, &reg, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = getPWMDivider();
	return state;
}

EMC230X_STATUS EMC230X::setFanDriveSettings(EMC230X_FAN fan, uint8_t speed)
{
	EMC230X_STATUS state = getFanDriveSettings();

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1DRIVESETTING; emc230xState_.FanDriveSetting.PWM1 = speed; reg = emc230xState_.FanDriveSetting.PWM1;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2DRIVESETTING; emc230xState_.FanDriveSetting.PWM2 = speed; reg = emc230xState_.FanDriveSetting.PWM2;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3DRIVESETTING; emc230xState_.FanDriveSetting.PWM3 = speed; reg = emc230xState_.FanDriveSetting.PWM3;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4DRIVESETTING; emc230xState_.FanDriveSetting.PWM4 = speed; reg = emc230xState_.FanDriveSetting.PWM4;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5DRIVESETTING; emc230xState_.FanDriveSetting.PWM5 = speed; reg = emc230xState_.FanDriveSetting.PWM5;}

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, addr, &reg, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = getFanDriveSettings();
	return state;
}

EMC230X_STATUS EMC230X::setFanInterrupt(EMC230X_FAN fan, bool enabled)
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	EMC230X_STATUS state = getFanInterruptEnable();

	if (enabled == true) emc230xState_.FanInterruptEnable.fanInterruptEnable |= (uint8_t) fan;
	else emc230xState_.FanInterruptEnable.fanInterruptEnable &= (uint8_t) (~fan);

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, EMC230X_REG_INTERUPTENABLE, &emc230xState_.FanInterruptEnable.fanInterruptEnable, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = (EMC230X_STATUS)(state | getFanInterruptEnable());
	return state;
}

EMC230X_STATUS EMC230X::setFanConfig1(EMC230X_FAN fan, EMC230X_FanConfig1 cfg)
{
	EMC230X_STATUS state;

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1CONFIG1; emc230xState_.Fan1Config1 = cfg; reg = emc230xState_.Fan1Config1.fanConfig1;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2CONFIG1; emc230xState_.Fan2Config1 = cfg; reg = emc230xState_.Fan2Config1.fanConfig1;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3CONFIG1; emc230xState_.Fan3Config1 = cfg; reg = emc230xState_.Fan3Config1.fanConfig1;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4CONFIG1; emc230xState_.Fan4Config1 = cfg; reg = emc230xState_.Fan4Config1.fanConfig1;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5CONFIG1; emc230xState_.Fan5Config1 = cfg; reg = emc230xState_.Fan5Config1.fanConfig1;}

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, addr, &reg, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = getFanConfig1();
	return state;
}

EMC230X_STATUS EMC230X::setFanConfig2(EMC230X_FAN fan, EMC230X_FanConfig2 cfg)
{
	EMC230X_STATUS state;

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1CONFIG2; emc230xState_.Fan1Config2 = cfg; reg = emc230xState_.Fan1Config2.fanConfig2;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2CONFIG2; emc230xState_.Fan2Config2 = cfg; reg = emc230xState_.Fan2Config2.fanConfig2;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3CONFIG2; emc230xState_.Fan3Config2 = cfg; reg = emc230xState_.Fan3Config2.fanConfig2;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4CONFIG2; emc230xState_.Fan4Config2 = cfg; reg = emc230xState_.Fan4Config2.fanConfig2;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5CONFIG2; emc230xState_.Fan5Config2 = cfg; reg = emc230xState_.Fan5Config2.fanConfig2;}

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, addr, &reg, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = getFanConfig2();
	return state;
}

EMC230X_STATUS EMC230X::setPIDGain(EMC230X_FAN fan, EMC230X_PIDGain gain)
{
	EMC230X_STATUS state;

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1PIDGAIN; emc230xState_.Fan1PIDGain = gain; reg = emc230xState_.Fan1PIDGain.pidGain;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2PIDGAIN; emc230xState_.Fan2PIDGain = gain; reg = emc230xState_.Fan2PIDGain.pidGain;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3PIDGAIN; emc230xState_.Fan3PIDGain = gain; reg = emc230xState_.Fan3PIDGain.pidGain;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4PIDGAIN; emc230xState_.Fan4PIDGain = gain; reg = emc230xState_.Fan4PIDGain.pidGain;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5PIDGAIN; emc230xState_.Fan5PIDGain = gain; reg = emc230xState_.Fan5PIDGain.pidGain;}

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, addr, &reg, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = getPIDGain();
	return state;
}

EMC230X_STATUS EMC230X::setFanSpinUpConfig(EMC230X_FAN fan, EMC230X_FanSpinUpConfig spinup)
{
	EMC230X_STATUS state;

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1SPINUP; emc230xState_.FanSpinUp1 = spinup; reg = emc230xState_.FanSpinUp1.fanSpinUpConfig;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2SPINUP; emc230xState_.FanSpinUp2 = spinup; reg = emc230xState_.FanSpinUp2.fanSpinUpConfig;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3SPINUP; emc230xState_.FanSpinUp3 = spinup; reg = emc230xState_.FanSpinUp3.fanSpinUpConfig;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4SPINUP; emc230xState_.FanSpinUp4 = spinup; reg = emc230xState_.FanSpinUp4.fanSpinUpConfig;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5SPINUP; emc230xState_.FanSpinUp5 = spinup; reg = emc230xState_.FanSpinUp5.fanSpinUpConfig;}

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, addr, &reg, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = getFanSpinUpConfig();
	return state;
}

EMC230X_STATUS EMC230X::setFanMaxStep(EMC230X_FAN fan, uint8_t maxstep)
{
	EMC230X_STATUS state;

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1MAXSTEP; emc230xState_.FanDrvMaxStepSize1 = maxstep; reg = emc230xState_.FanDrvMaxStepSize1;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2MAXSTEP; emc230xState_.FanDrvMaxStepSize2 = maxstep; reg = emc230xState_.FanDrvMaxStepSize2;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3MAXSTEP; emc230xState_.FanDrvMaxStepSize3 = maxstep; reg = emc230xState_.FanDrvMaxStepSize3;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4MAXSTEP; emc230xState_.FanDrvMaxStepSize4 = maxstep; reg = emc230xState_.FanDrvMaxStepSize4;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5MAXSTEP; emc230xState_.FanDrvMaxStepSize5 = maxstep; reg = emc230xState_.FanDrvMaxStepSize5;}

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, addr, &reg, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = getFanMaxStep();
	return state;
}

EMC230X_STATUS EMC230X::setFanMinDrive(EMC230X_FAN fan, uint8_t minstep)
{
	EMC230X_STATUS state;

	uint8_t reg = 0;
	uint8_t addr = 0;

	if (fan == EMC230X_FAN::FANCNTRL_1) {addr = EMC230X_REG_FAN1MINDRIVE; emc230xState_.FanDrvMinStepSize1 = minstep; reg = emc230xState_.FanDrvMinStepSize1;}
	if (fan == EMC230X_FAN::FANCNTRL_2) {addr = EMC230X_REG_FAN2MINDRIVE; emc230xState_.FanDrvMinStepSize2 = minstep; reg = emc230xState_.FanDrvMinStepSize2;}
	if (fan == EMC230X_FAN::FANCNTRL_3) {addr = EMC230X_REG_FAN3MINDRIVE; emc230xState_.FanDrvMinStepSize3 = minstep; reg = emc230xState_.FanDrvMinStepSize3;}
	if (fan == EMC230X_FAN::FANCNTRL_4) {addr = EMC230X_REG_FAN4MINDRIVE; emc230xState_.FanDrvMinStepSize4 = minstep; reg = emc230xState_.FanDrvMinStepSize4;}
	if (fan == EMC230X_FAN::FANCNTRL_5) {addr = EMC230X_REG_FAN5MINDRIVE; emc230xState_.FanDrvMinStepSize5 = minstep; reg = emc230xState_.FanDrvMinStepSize5;}

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, addr, &reg, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = getFanMinDrive();
	return state;
}

EMC230X_STATUS EMC230X::setDriveFailBand(EMC230X_FAN fan, uint16_t fail)
{
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

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, addrlsb, &reglsb, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, addrmsb, &regmsb, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = getDriveFailBand();
	return state;
}

EMC230X_STATUS EMC230X::setFanTachTarget(EMC230X_FAN fan, uint16_t tach)
{ // Always write LSB first pg. 45 datasheet
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	uint8_t lsb = (uint8_t)(tach & 0x00FF);
	uint8_t msb = (uint8_t)(tach >> 8);

	uint8_t addrmsb = 0;
	uint8_t addrlsb = 0;

	EMC230X_STATUS state = EMC230X_STATUS::EMC230X_STATUS_OK;

	if (fan == EMC230X_FAN::FANCNTRL_1) { addrmsb = EMC230X_REG_FAN1TACHTARGETMSB; addrlsb = EMC230X_REG_FAN1TACHTARGETLSB; }
	if (fan == EMC230X_FAN::FANCNTRL_2) { addrmsb = EMC230X_REG_FAN2TACHTARGETMSB; addrlsb = EMC230X_REG_FAN2TACHTARGETLSB; }
	if (fan == EMC230X_FAN::FANCNTRL_3) { addrmsb = EMC230X_REG_FAN3TACHTARGETMSB; addrlsb = EMC230X_REG_FAN3TACHTARGETLSB; }
	if (fan == EMC230X_FAN::FANCNTRL_4) { addrmsb = EMC230X_REG_FAN4TACHTARGETMSB; addrlsb = EMC230X_REG_FAN4TACHTARGETLSB; }
	if (fan == EMC230X_FAN::FANCNTRL_5) { addrmsb = EMC230X_REG_FAN5TACHTARGETMSB; addrlsb = EMC230X_REG_FAN5TACHTARGETLSB; }

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, addrlsb, &lsb, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, addrmsb, &msb, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = getFanTachTarget();
	return state;
}

EMC230X_STATUS EMC230X::setFanValidTachCount(EMC230X_FAN fan, uint8_t count)
{
	assert(smbus_read_reg != nullptr);
	assert(smbus_write_reg != nullptr);

	uint8_t addr=0;

	EMC230X_STATUS state = EMC230X_STATUS::EMC230X_STATUS_OK;

	if (fan == EMC230X_FAN::FANCNTRL_1) addr = EMC230X_REG_FAN1VALTACHCOUNT;
	if (fan == EMC230X_FAN::FANCNTRL_2) addr = EMC230X_REG_FAN2VALTACHCOUNT;
	if (fan == EMC230X_FAN::FANCNTRL_3) addr = EMC230X_REG_FAN3VALTACHCOUNT;
	if (fan == EMC230X_FAN::FANCNTRL_4) addr = EMC230X_REG_FAN4VALTACHCOUNT;
	if (fan == EMC230X_FAN::FANCNTRL_5) addr = EMC230X_REG_FAN5VALTACHCOUNT;

	state = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, addr, &count, 1);
	if (state != 0) return EMC230X_STATUS_FAIL;

	state = getFanValidTachCount();
	return state;
}

// The number of fan poles would also affect the tachometer counting because
// different fans generate different amount of pulses per rotation. Most fans
// are 2 poles, but this info should be obtained directly from the fan datasheet.
EMC230X_STATUS EMC230X::setFanPoles(EMC230X_FAN fan, EMC230X_FANPOLES poles)
{
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

	status = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, reg, &configreg, 1);
	if (status != 0) return EMC230X_STATUS_FAIL;

	status = getFanConfig1(); // get latest

	return status;
}

// Toggles the RPM-based Fan Speed Control Algorithm, whereby the fan speed will be controlled
// by the EMC230X using a PID algorithm based on the target tachometer reading given by user.
EMC230X_STATUS EMC230X::toggleControlAlgorithm(EMC230X_FAN fan, bool enable)
{

	EMC230X_STATUS status = getFanConfig1(); // get latest

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

	status = (EMC230X_STATUS) smbus_write_reg(I2C_ADDRESS, reg, &configreg, 1);
	if (status != 0) return EMC230X_STATUS_FAIL;

	status = getFanConfig1(); // get latest

	return status;
}

// Obtain the tachometer reading, convert to RPM, and store in a private variable.
// Get the fan speed (RPM) in RPM
uint16_t EMC230X::getFanRPM(EMC230X_FAN fan)
{
	uint16_t rpm = 0;
	uint16_t tach = 1;
	uint8_t edges = 0;
	float invpoles = 1.0f;
	float invmult = 1.0f;
	float ftach = 32.768E3f;
	EMC230X_FanConfig1 configReg = {0};
	EMC230X_FANPOLES poleSetting;

	getFanConfig1();
	getFanTachReading();

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

	float frpm = invpoles * (edges / (1.0f*tach*invmult)) * ftach * 60.0f;

	return rpm = (uint16_t) frpm;

}

void EMC230X::runControlAlgorithm(EMC230X_FAN fan, uint16_t tachTarget)
{
	toggleControlAlgorithm(fan, false);
	EMC230X_FanConfig1 cfg = {0};
	setFanPoles(fan, EMC230X_FANPOLES::FANPOLE_2);
	cfg.UDT = 3;
	cfg.EDG = 1;  //5 edges
	cfg.FRNG = 0; //500rpm min, mult = 1
	cfg.ENAG = 0;
	setFanConfig1(fan, cfg);
	setFanDriveSettings(fan, 0);
	setPWMOutput(fan, 0);
	getFanTachReading();
	getChipState();

	EMC230X_FanSpinUpConfig fspuc;
	fspuc.DFC  = 3; // 64 periods
	fspuc.NKCK = 1; // do not drive to 100% PWM
	fspuc.SPLV = 7; // 65%
	fspuc.SPT  = 3; // 2000ms spin up time
	setFanSpinUpConfig(fan, fspuc);
	setFanMaxStep(fan, 32);
	setFanMinDrive(fan, 8);
	setFanValidTachCount(fan, 0xFE);
	// RPM = (3932160*1)/TachTarget, Ex 524 RPM = (3932160*1)/7500, 5000 is about 50% duty cycle and loud
	setFanTachTarget(fan, (uint16_t)(tachTarget) );
	toggleControlAlgorithm(fan, true);
}

} // end namespace

/*

// Since the tachometer register on the EMC230X has an upper limit, it is necessary to apply multipliers
// for fans with high RPMs. This is done by adjusting the minimum RPM expected for the fan.
// The function is written such that the min RPM will be forced to the closest lower RPM.
EMC230X_STATUS EMC230X::setTachMinRPM(uint16_t minRPM)
{
  uint8_t writeByte;
  if (minRPM < 1000)
  {
    writeByte = EMC230X_REG_FANCONFIG1_MINRPM_500;
    tachMinRPMMultiplier_ = 1;
  }
  else if (minRPM < 2000)
  {
    writeByte = EMC230X_REG_FANCONFIG1_MINRPM_1000;
    tachMinRPMMultiplier_ = 2;
  }
  else if (minRPM < 4000)
  {
    writeByte = EMC230X_REG_FANCONFIG1_MINRPM_2000;
    tachMinRPMMultiplier_ = 4;
  }
  else
  {
    writeByte = EMC230X_REG_FANCONFIG1_MINRPM_4000;
    tachMinRPMMultiplier_ = 8;
  }

  return writeRegisterBits(EMC230X_REG_FANCONFIG1, EMC230X_REG_FANCONFIG1_MINRPM_CLEAR, writeByte);
}



// Adjusts the period between subsequent PWM drive updates.
// This is only used if toggleRampControl() was enabled.
// The function is written such that the period will be forced to the closest lower period.
EMC230X_STATUS EMC230X::setDriveUpdatePeriod(uint16_t periodMs)
{
  uint8_t writeByte;
  if (periodMs < 200)
  {
    writeByte = EMC230X_REG_FANCONFIG1_UPDATE_100;
  }
  else if (periodMs < 300)
  {
    writeByte = EMC230X_REG_FANCONFIG1_UPDATE_200;
  }
  else if (periodMs < 400)
  {
    writeByte = EMC230X_REG_FANCONFIG1_UPDATE_300;
  }
  else if (periodMs < 500)
  {
    writeByte = EMC230X_REG_FANCONFIG1_UPDATE_400;
  }
  else if (periodMs < 800)
  {
    writeByte = EMC230X_REG_FANCONFIG1_UPDATE_500;
  }
  else if (periodMs < 1200)
  {
    writeByte = EMC230X_REG_FANCONFIG1_UPDATE_800;
  }
  else if (periodMs < 1600)
  {
    writeByte = EMC230X_REG_FANCONFIG1_UPDATE_1200;
  }
  else
  {
    writeByte = EMC230X_REG_FANCONFIG1_UPDATE_1600;
  }

  return writeRegisterBits(EMC230X_REG_FANCONFIG1, EMC230X_REG_FANCONFIG1_UPDATE_CLEAR, writeByte);
}

// Toggle ramp control for DIRECT CONTROL MODE, whereby the fan speed will be increased gradually.
// Disabling this will allow fan speed to be changed instantly.
// Note that enabling the RPM-based Fan Speed Control will automatically use the
// ramp control, regardless of the status of this function.
EMC230X_STATUS EMC230X::toggleRampControl(bool enable)
{
  if (enable)
  {
    return writeRegisterBits(EMC230X_REG_FANCONFIG2, ~EMC230X_REG_FANCONFIG2_RAMPCONTROL, EMC230X_REG_FANCONFIG2_RAMPCONTROL);
  }
  else
  {
    return writeRegisterBits(EMC230X_REG_FANCONFIG2, ~EMC230X_REG_FANCONFIG2_RAMPCONTROL, 0);
  }
}

// Toggle the glitch filter, which removes high frequency noise
// from the TACH pin.
EMC230X_STATUS EMC230X::toggleGlitchFilter(bool enable)
{
  if (enable)
  {
    return writeRegisterBits(EMC230X_REG_FANCONFIG2, ~EMC230X_REG_FANCONFIG2_GLITCHFILTER, EMC230X_REG_FANCONFIG2_GLITCHFILTER);
  }
  else
  {
    return writeRegisterBits(EMC230X_REG_FANCONFIG2, ~EMC230X_REG_FANCONFIG2_GLITCHFILTER, 0);
  }
}

// Change the type of derivative used in the PID algorithm for RPM-based speed control.
// Refer to Table 5.15 at pg 31 of the datasheet.
EMC230X_STATUS EMC230X::setDerivativeMode(uint8_t modeType)
{
  uint8_t writeByte;

  switch (modeType)
  {
  case 0:
    writeByte = EMC230X_REG_FANCONFIG2_DEROPT_NONE;
    break;
  case 1:
    writeByte = EMC230X_REG_FANCONFIG2_DEROPT_BASIC;
    break;
  case 2:
    writeByte = EMC230X_REG_FANCONFIG2_DEROPT_STEP;
    break;
  case 3:
    writeByte = EMC230X_REG_FANCONFIG2_DEROPT_BOTH;
  default:
    return EMC230X_STATUS_INVALIDARG;
    break;
  }

  return writeRegisterBits(EMC230X_REG_FANCONFIG2, EMC230X_REG_FANCONFIG2_DEROPT_CLEAR, writeByte);
}

// Since the tachometer has an accuracy rating, it is not expected that the
// RPM readings will be constant even if the PWM drive is constant. Therefore,
// it may be desirable to tell the EMC230X to stop changing PWM drive as long as
// the RPM reading is within a tolerance of the target. This function does that.
// The function is written such that the error range will be forced to the closest higher range.
// The argument should be a positive number.
EMC230X_STATUS EMC230X::setControlErrRange(uint8_t errorRangeRPM)
{
  uint8_t writeByte;
  if (errorRangeRPM < 0.01) // Account for doubles sometimes not being exactly 0
  {
    writeByte = EMC230X_REG_FANCONFIG2_ERRRANGE_0;
  }
  else if (errorRangeRPM <= 50)
  {
    writeByte = EMC230X_REG_FANCONFIG2_ERRRANGE_50;
  }
  else if (errorRangeRPM <= 100)
  {
    writeByte = EMC230X_REG_FANCONFIG2_ERRRANGE_100;
  }
  else
  {
    writeByte = EMC230X_REG_FANCONFIG2_ERRRANGE_200;
  }

  return writeRegisterBits(EMC230X_REG_FANCONFIG2, EMC230X_REG_FANCONFIG2_ERRRANGE_CLEAR, writeByte);
}

// Toggle max spin up, whereby the fan is set to 100% duty cycle for 1/4th of the
// time during the spin up routine.
EMC230X_STATUS EMC230X::toggleSpinUpMax(bool enable)
{
  if (enable)
  {
    return writeRegisterBits(EMC230X_REG_FANSPINUP, ~EMC230X_REG_FANSPINUP_NOKICK, EMC230X_REG_FANSPINUP_NOKICK);
  }
  else
  {
    return writeRegisterBits(EMC230X_REG_FANSPINUP, ~EMC230X_REG_FANSPINUP_NOKICK, 0);
  }
}

// Set the drive level that should be used during the spin up routine.
// The function is written such that the drive will be forced to the closest lower drive.
EMC230X_STATUS EMC230X::setSpinUpDrive(uint8_t drivePercent)
{
  uint8_t writeByte;
  if (drivePercent < 35)
  {
    writeByte = EMC230X_REG_FANSPINUP_SPINLVL_30;
  }
  else if (drivePercent < 40)
  {
    writeByte = EMC230X_REG_FANSPINUP_SPINLVL_35;
  }
  else if (drivePercent < 45)
  {
    writeByte = EMC230X_REG_FANSPINUP_SPINLVL_40;
  }
  else if (drivePercent < 50)
  {
    writeByte = EMC230X_REG_FANSPINUP_SPINLVL_45;
  }
  else if (drivePercent < 55)
  {
    writeByte = EMC230X_REG_FANSPINUP_SPINLVL_50;
  }
  else if (drivePercent < 60)
  {
    writeByte = EMC230X_REG_FANSPINUP_SPINLVL_55;
  }
  else if (drivePercent < 65)
  {
    writeByte = EMC230X_REG_FANSPINUP_SPINLVL_60;
  }
  else
  {
    writeByte = EMC230X_REG_FANSPINUP_SPINLVL_65;
  }

  return writeRegisterBits(EMC230X_REG_FANSPINUP, EMC230X_REG_FANSPINUP_SPINLVL_CLEAR, writeByte);
}

// Determine the duration of the spin up routine.
// The function is written such that the time will be forced to the closest shorter time.
EMC230X_STATUS EMC230X::setSpinUpTime(uint16_t timeMs)
{
  uint8_t writeByte;
  if (timeMs < 500)
  {
    writeByte = EMC230X_REG_FANSPINUP_SPINUPTIME_250;
  }
  else if (timeMs < 1000)
  {
    writeByte = EMC230X_REG_FANSPINUP_SPINUPTIME_500;
  }
  else if (timeMs < 2000)
  {
    writeByte = EMC230X_REG_FANSPINUP_SPINUPTIME_1000;
  }
  else
  {
    writeByte = EMC230X_REG_FANSPINUP_SPINUPTIME_2000;
  }

  return writeRegisterBits(EMC230X_REG_FANSPINUP, EMC230X_REG_FANSPINUP_SPINUPTIME_CLEAR, writeByte);
}

// Set the maximum change in fan drive that could be performed over a single
// update period. Maximum is 0b00111111 (aka 63 or 0x3F)
EMC230X_STATUS EMC230X::setControlMaxStep(uint8_t stepSize)
{
  if (stepSize > EMC230X_REG_FANMAXSTEP_MAX)
  {
    stepSize = EMC230X_REG_FANMAXSTEP_MAX;
  }

  uint8_t data[] = {EMC230X_REG_FANMAXSTEP, stepSize};
  if (HAL_I2C_Master_Transmit(i2cWire_, I2C_ADDRESS, data, 2, 100) == HAL_OK)
  //if (i2cWire_->write(I2C_ADDRESS, EMC230X_REG_FANMAXSTEP, stepSize) == I2C_STATUS_OK)
  {
    return EMC230X_STATUS_OK;
  }
  else
  {
    return EMC230X_STATUS_FAIL;
  }
}

// Sets the minimum allowable drive for the RPM-based Fan Speed Control algorithm.
// The algorithm will not drive the fan at a level lower than this unless the
// tachometer target is specifically set to 0xFF.
// This is extremely useful for fans that would stop spinning if the PWM signal
// is low but not zero because once the PWM signal is low enough, the fan stops spinning
// and the tachometer readings become zero causing the algorithm to drive high and restart
// the fan, but now the tachometer is above the target. The fan is then driven to a halt again
// and this process is repeated indefinitely, causing the fan to on-off-on-off......
// Having a minimum drive prevents this from happening.
EMC230X_STATUS EMC230X::setFanMinDrive(double minDrivePercent)
{
  // Convert the percent to byte format
  if (minDrivePercent <= 0.0d) minDrivePercent = 0.0d;
  if (minDrivePercent >= 100.0d) minDrivePercent = 100.0d;

  uint8_t writeByte = (uint8_t) (minDrivePercent / 100 * 255);

  uint8_t data[] = {EMC230X_REG_FANMINDRIVE, writeByte};
  if (HAL_I2C_Master_Transmit(i2cWire_, I2C_ADDRESS, data, 2, 100) == HAL_OK)
  //if (i2cWire_->write(I2C_ADDRESS, EMC230X_REG_FANMINDRIVE, writeByte) == I2C_STATUS_OK)
  {
    return EMC230X_STATUS_OK;
  }
  else
  {
    return EMC230X_STATUS_FAIL;
  }
}

// Sets the minimum RPM which is checked at the end of the spin up routine to decide if the fan is actually
// moving or if it is stalled.
// Internally, the function converts the min RPM to tachometer count that will be written
// to the appropriate register.
// NOTE: this value shouldn't be the absolute minimum RPM because it only serves as a check
// for the spin up routine. Absolute minimum speed should be set at setFanMinDrive(), although
// that function accepts percentage, not RPM.
// Also NOTE: the min value is dependent on what was set in setTachMinRPM(). This function will automatically
// increase the RPM to the lower limit if the given RPM is lower than the one set in setTachMinRPM().
EMC230X_STATUS EMC230X::setMinValidRPM(uint16_t minRPM)
{
  // Ensure the given min RPM is not below the limits of the tachometer.
  uint16_t tachMinRPM;
  switch (tachMinRPMMultiplier_)
  {
  case 1:
    tachMinRPM = 500;
    break;
  case 2:
    tachMinRPM = 1000;
    break;
  case 3:
    tachMinRPM = 2000;
    break;
  default:
  case 4:
    tachMinRPM = 4000;
    break;
  }

  if (minRPM < tachMinRPM)
  {
    minRPM = tachMinRPM;
  }

  // To avoid doubles, the fan pole multiplier was multiplied by 2 to make it an integer.
  // Here, we divide it (and the -1 in the bracket) by 2 to bring it back to its proper value.
  uint8_t maxTachCount_ = 60 * tachMinRPMMultiplier_ * TACHO_FREQUENCY * (tachFanPolesMultiplier_ - 2) / 2 / fanPoleCount_ / minRPM;

  uint8_t data[] = {EMC230X_REG_FANVALTACHCOUNT, maxTachCount_};
  if (HAL_I2C_Master_Transmit(i2cWire_, I2C_ADDRESS, data, 2, 100) == HAL_OK)
  //if (i2cWire_->write(I2C_ADDRESS, EMC230X_REG_FANVALTACHCOUNT, maxTachCount_) == I2C_STATUS_OK)
  {
    return EMC230X_STATUS_OK;
  }
  else
  {
    return EMC230X_STATUS_FAIL;
  }
}

// Based on the given target RPM, calculate the appropriate target tachometer reading and
// write it to the appropriate register.
// Sanity check for targetRPM to ensure it doesn't cause the calculation to overflow
// the max value of uint16_t (65535).
EMC230X_STATUS EMC230X::setRPMTarget(uint16_t targetRPM)
{
  // To avoid doubles, the fan pole multiplier was multiplied by 2 to make it an integer.
  // Here, we divide it (and the -1 in the bracket) by 2 to bring it back to its proper value.
  uint32_t temp = 60 * tachMinRPMMultiplier_ * TACHO_FREQUENCY * (tachFanPolesMultiplier_ - 2) / 2 / fanPoleCount_ / targetRPM;
  targetTachCount_ = (temp > 65535) ? 65535 : (uint16_t) temp;
  return writeTachoTarget(targetTachCount_);
}

// Turn the fan on (to the most recently known target RPM) or turn it off
EMC230X_STATUS EMC230X::toggleFan(bool enable)
{
  if (enable)
  {
    return writeTachoTarget(targetTachCount_);
  }
  else
  {
    return writeTachoTarget(TACHO_OFF);
  }
}

// Obtain the tachometer reading, convert to RPM, and store in a private variable.
// Get the fan speed (RPM) by calling getFanSpeed().
EMC230X_STATUS EMC230X::fetchFanSpeed()
{
  fanSpeed_ = 0;

  uint8_t data[] = {EMC230X_REG_TACHREADMSB, (uint8_t) 1};
  if (HAL_I2C_Master_Transmit(i2cWire_, I2C_ADDRESS, data, 2, HAL_MAX_DELAY) != HAL_OK)
  //if (i2cWire_->read(I2C_ADDRESS, EMC230X_REG_TACHREADMSB, (uint8_t) 1) == I2C_STATUS_OK)
  {
	HAL_I2C_Master_Receive(i2cWire_, I2C_ADDRESS, data, 1, 100);

	//uint16_t tachoCount = (i2cWire_->getByte()) << 8;
	uint16_t tachoCount = (data[0]) << 8;

    data[0] = EMC230X_REG_TACHREADLSB;
    data[1] = 1;
    if (HAL_I2C_Master_Transmit(i2cWire_, I2C_ADDRESS, data, 2, 100) != HAL_OK)
    //if (i2cWire_->read(I2C_ADDRESS, EMC230X_REG_TACHREADLSB, (uint8_t)1) == I2C_STATUS_OK)
    {
      HAL_I2C_Master_Receive(i2cWire_, I2C_ADDRESS, data, 1, 100);

      tachoCount |= data[0];
      //tachoCount |= i2cWire_->getByte();
      tachoCount = tachoCount >> 3;

      // To avoid doubles, the fan pole multiplier was multiplied by 2 to make it an integer.
      // Here, we divide it (and the -1 in the bracket) by 2 to bring it back to its proper value.
      fanSpeed_ = 60 * tachMinRPMMultiplier_ * TACHO_FREQUENCY * (tachFanPolesMultiplier_ - 2) / 2 / fanPoleCount_ / tachoCount;
    }
    else
    {
      return EMC230X_STATUS_FAIL;
    }
  }
  else
  {
    return EMC230X_STATUS_FAIL;
  }
  return EMC230X_STATUS_OK;
}

// Writes specific bits in the given register, such that the other bits in the register
// are unaffected. This is done by reading the register first, masking the appropriate
// bits, and then writing this modified byte into the register.
EMC230X_STATUS EMC230X::writeRegisterBits(uint8_t registerAddress, uint8_t clearingMask, uint8_t byteToWrite)
{
  uint8_t data[] = {registerAddress, (uint8_t) 1};
  if (HAL_I2C_Master_Transmit(i2cWire_, I2C_ADDRESS, data, 2, 100) == HAL_OK)
  //if (i2cWire_->read(I2C_ADDRESS, registerAddress, (uint8_t) 1) == I2C_STATUS_OK)
  {
    //uint8_t registerContents = i2cWire_->getByte();
	uint8_t registerContents;
    HAL_I2C_Master_Receive(i2cWire_, I2C_ADDRESS, &registerContents, 1, 100);

    registerContents &= clearingMask; // Reset the bits at the location of interest
    registerContents |= byteToWrite;  // Write bits to the location of interest

    uint8_t data[] = {registerAddress, registerContents};
    if (HAL_I2C_Master_Transmit(i2cWire_, I2C_ADDRESS, data, 2, 100) == HAL_OK)
    //if (i2cWire_->write(I2C_ADDRESS, registerAddress, registerContents) == I2C_STATUS_OK)
    {
      return EMC230X_STATUS_OK;
    }
    else
    {
      return EMC230X_STATUS_FAIL;
    }
  }
  else
  {
    return EMC230X_STATUS_FAIL;
  }
  return EMC230X_STATUS_FAIL;
}

EMC230X_STATUS EMC230X::writeTachoTarget(uint16_t tachoTarget)
{
  uint8_t tachCountLSB = (tachoTarget << 3) & 0xF8;
  uint8_t tachCountMSB = (tachoTarget >> 5) & 0xFF;

  // The low byte must be written before the high byte, because
  // the target is officially changed once the high byte is written (pg 36 of datasheet).
  uint8_t data[] = {EMC230X_REG_TACHTARGETLSB, tachCountLSB};

  if (HAL_I2C_Master_Transmit(i2cWire_, I2C_ADDRESS, data, 2, 100) == HAL_OK)
  //if (i2cWire_->write(I2C_ADDRESS, EMC230X_REG_TACHTARGETLSB, tachCountLSB) == I2C_STATUS_OK)
  {
	uint8_t data[] = {EMC230X_REG_TACHTARGETMSB, tachCountMSB};

	if (HAL_I2C_Master_Transmit(i2cWire_, I2C_ADDRESS, data, 2, 100) == HAL_OK)
	//if (i2cWire_->write(I2C_ADDRESS, EMC230X_REG_TACHTARGETMSB, tachCountMSB) == I2C_STATUS_OK)
    {
      return EMC230X_STATUS_OK;
    }
    else
    {
      return EMC230X_STATUS_FAIL;
    }
  }
  else
  {
    return EMC230X_STATUS_FAIL;
  }
  return EMC230X_STATUS_FAIL;
}
*/






