/*******************************************************************************
 * @file      EMC230X.hpp
 * @brief     Driver for the Microchip EMC2301/2/3/5 RPM-based PWM fan controllers.
 *
 * @details   Covers all four parts in the family. The device is identified from its
 *            product ID on first use and only the fan channels it actually implements
 *            are accessed -- the registers of an absent fan are reserved.
 *
 *            The driver is transport agnostic and makes no platform calls of its own.
 *            Supply an SMBus/I2C register read and write function through
 *            setI2CDriver(); both return 0 on success and non-zero on failure.
 *
 *            Typical use:
 * @code
 *              EMC230X::EMC230X fan;
 *              fan.setI2CDriver(0x2F, my_smbus_write, my_smbus_read);
 *
 *              if (fan.getChipInfo() == EMC230X::EMC230X_STATUS_OK)
 *              {
 *                  // closed-loop control at 2000 RPM on fan 1
 *                  fan.runControlAlgorithm(EMC230X::FANCNTRL_1, 2000);
 *
 *                  uint16_t rpm = 0;
 *                  fan.getFanTachReading();                       // refresh the cache
 *                  fan.getFanRPMReading(EMC230X::FANCNTRL_1, &rpm);
 *              }
 * @endcode
 *
 *            All speeds are in RPM. Tach counts, the register justification and the
 *            RPM conversion are handled internally; see the notes on TACH_RPM_CONST
 *            and EMC230X_FANPOLES for the arithmetic.
 *
 * @note      References: Microchip EMC2301/2/3/5 data sheet DS20006532A, and
 *            application note AN17.4 "RPM to TACH Counts Conversion".
 *
 * @version   1.0
 * @date      Created  8 November 2024
 * @date      Modified 11 August 2026
 *
 * @author    Michael Margolese
 * @copyright Copyright (c) 2024-2026 Tenuvah Designs. All rights reserved.
 ******************************************************************************/

#pragma once

#ifndef EMC230X_HPP_
#define EMC230X_HPP_

#include <stdbool.h>
#include <stdint.h>

namespace EMC230X
{

// Driver version, so a consumer can check what it is compiled against rather than
// relying on the comment block above. Bump the minor for additive changes and the
// major for anything that breaks an existing caller.
static constexpr uint8_t EMC230X_DRIVER_VERSION_MAJOR = 1;
static constexpr uint8_t EMC230X_DRIVER_VERSION_MINOR = 0;

// Statuses/Errors returned by the functions in this class.
//
// These are BIT FLAGS, not an ordered list. getChipState() ORs together the result
// of every individual read, so its return value can be any combination (3, 5, 6, 7)
// and will not equal a single enumerator. Test it with `== EMC230X_STATUS_OK` for
// "everything succeeded", or with `&` for a specific failure -- never with
// `== EMC230X_STATUS_FAIL`. Single-operation calls always return one value.
typedef enum : uint8_t
{
  EMC230X_STATUS_OK         = 0,  // No problemo.
  EMC230X_STATUS_FAIL       = 1,  // Something went wrong (SMBus read/write failed).
  EMC230X_STATUS_INVALIDARG = 2,  // The argument given to a function is invalid.
  EMC230X_SMBUS_DRIVER_NULL = 4   // setI2CDriver() has not been called.
} EMC230X_STATUS;

typedef enum : uint8_t
{
	FANCNTRL_1 = 0x1 << 0,
	FANCNTRL_2 = 0x1 << 1,
	FANCNTRL_3 = 0x1 << 2,
	FANCNTRL_4 = 0x1 << 3,
	FANCNTRL_5 = 0x1 << 4
} EMC230X_FAN;

typedef enum : uint8_t
{
	UNKNOWN_ID = 0x00,
	EMC2301_ID = 0x37,
	EMC2302_ID = 0x36,
	EMC2303_ID = 0x35,
	EMC2305_ID = 0x34,
} EMC230X_PRODUCTID;

typedef enum : uint8_t
{
	UNKNOWN_MANUFACTURER = 0,
	MICROCHIP_MANUFACTURER = 0x5D
} EMC230X_MANUFACTURERID;

// Fan pole count, encoded as the EDG field value rather than the pole count itself:
// poles = enum value + 1. The datasheet calls a fan "2-pole" when its tachometer
// emits 2 pulses per revolution, which is what a standard 4-wire PC fan does --
// that is FANPOLE_2, and it is the chip's reset default.
//
// Note this is the TACH pulses per revolution, NOT the motor's magnetic pole count.
// A 4-pole motor emitting 2 pulses/rev is FANPOLE_2. Per the datasheet the number of
// edges sampled must equal 2*poles + 1, so picking the wrong one scales every RPM
// reading by a constant factor.
typedef enum : uint8_t
{
	FANPOLE_1 = 0x0, // 3 edges sampled, 1 pulse/rev
	FANPOLE_2 = 0x1, // 5 edges sampled, 2 pulses/rev -- standard 4-wire fan, default
	FANPOLE_3 = 0x2, // 7 edges sampled, 3 pulses/rev
	FANPOLE_4 = 0x3  // 9 edges sampled, 4 pulses/rev
} EMC230X_FANPOLES;

typedef enum : uint8_t
{
	FREQ_26KHZ    = 0,
	FREQ_19_53KHZ = 1,
	FREQ_4_882KHZ = 2,
	FREQ_2_441KHZ = 3
} EMC230X_PWMFREQ;

// Data Structures
typedef union
{
	struct
	{
		uint8_t USECK  : 1; // 1 - use oscillator on clock pin, 0 - use internal osc. for tach measurements
		uint8_t DRECK  : 1; // 1 - clock pin acts as an output, 0 - clock acts as input
		uint8_t RSVD   : 3;
		uint8_t WD_EN  : 1; // 1 - Watchdog timer operates continuously
		uint8_t DIS_TO : 1; // 1 - Disables SMBus time out function
		uint8_t MASK   : 1; // 1 - Masks ALERT#
	};
	uint8_t Config;
} EMC230X_Configuration;

typedef union
{
	struct
	{
		uint8_t FNSTL  : 1; // 1 - Any bit in FAN STALL STATUS is set
		uint8_t FNSPIN : 1; // 1 - Any bit in FAN SPIN STATUS is set
		uint8_t DVFAIL : 1; // 1 - Any bit in FAN DRIVE FAIL is set
		uint8_t RSVD   : 4;
		uint8_t WATCH  : 1; // 1 - Watchdog expired
	};
	uint8_t fanStatus;
} EMC230X_FanStatus;

typedef union
{
	struct
	{
		uint8_t F1STL : 1; // 1 - Fan 1 tach count exceeded maximum valid TACH count -> stall
		uint8_t F2STL : 1; // 1 - Fan 2 tach count exceeded maximum valid TACH count -> stall
		uint8_t F3STL : 1; // 1 - Fan 3 tach count exceeded maximum valid TACH count -> stall
		uint8_t F4STL : 1; // 1 - Fan 4 tach count exceeded maximum valid TACH count -> stall
		uint8_t F5STL : 1; // 1 - Fan 5 tach count exceeded maximum valid TACH count -> stall
		uint8_t RSVD  : 3;
	};
	uint8_t fanStallStatus;
} EMC230X_FanStallStatus;

typedef union
{
	struct
	{
		uint8_t F1SPIN : 1; // 1 - Fan 1 spin up failed to start fan
		uint8_t F2SPIN : 1; // 1 - Fan 2 spin up failed to start fan
		uint8_t F3SPIN : 1; // 1 - Fan 3 spin up failed to start fan
		uint8_t F4SPIN : 1; // 1 - Fan 4 spin up failed to start fan
		uint8_t F5SPIN : 1; // 1 - Fan 5 spin up failed to start fan
		uint8_t RSVD   : 3;
	};
	uint8_t fanSpinStatus;
} EMC230X_FanSpinStatus;

typedef union
{
	struct
	{
		uint8_t F1DRV : 1; // 1 - Fan 1 unable to reach commanded RPM at 100% PWM
		uint8_t F2DRV : 1; // 1 - Fan 2 unable to reach commanded RPM at 100% PWM
		uint8_t F3DRV : 1; // 1 - Fan 3 unable to reach commanded RPM at 100% PWM
		uint8_t F4DRV : 1; // 1 - Fan 4 unable to reach commanded RPM at 100% PWM
		uint8_t F5DRV : 1; // 1 - Fan 5 unable to reach commanded RPM at 100% PWM
		uint8_t RSVD  : 3;
	};
	uint8_t driveFailStatus;
} EMC230X_DriveFailStatus;

typedef union
{
	struct
	{
		uint8_t F1ITEN : 1; // 1 - Fan 1 Allow ALERT# if error
		uint8_t F2ITEN : 1; // 1 - Fan 2 Allow ALERT# if error
		uint8_t F3ITEN : 1; // 1 - Fan 3 Allow ALERT# if error
		uint8_t F4ITEN : 1; // 1 - Fan 4 Allow ALERT# if error
		uint8_t F5ITEN : 1; // 1 - Fan 5 Allow ALERT# if error
		uint8_t RSVD   : 3;
	};
	uint8_t fanInterruptEnable;
} EMC230X_FanInterruptEnable;

typedef union
{
	struct
	{
		uint8_t POLARITY1 : 1; // 1 - Fan 1 drive 00h is 100% duty cycle, 0 - drive 00h is 0% duty cycle
		uint8_t POLARITY2 : 1; // 1 - Fan 2 drive 00h is 100% duty cycle, 0 - drive 00h is 0% duty cycle
		uint8_t POLARITY3 : 1; // 1 - Fan 3 drive 00h is 100% duty cycle, 0 - drive 00h is 0% duty cycle
		uint8_t POLARITY4 : 1; // 1 - Fan 4 drive 00h is 100% duty cycle, 0 - drive 00h is 0% duty cycle
		uint8_t POLARITY5 : 1; // 1 - Fan 5 drive 00h is 100% duty cycle, 0 - drive 00h is 0% duty cycle
		uint8_t RSVD      : 3;
	};
	uint8_t pwmPolarity;
} EMC230X_PWMPolarity;

typedef union
{
	struct
	{
		uint8_t PWMOUT1 : 1; // 1 - Fan 1 Push pull, 0 - open drain
		uint8_t PWMOUT2 : 1; // 1 - Fan 2 Push pull, 0 - open drain
		uint8_t PWMOUT3 : 1; // 1 - Fan 3 Push pull, 0 - open drain
		uint8_t PWMOUT4 : 1; // 1 - Fan 4 Push pull, 0 - open drain
		uint8_t PWMOUT5 : 1; // 1 - Fan 5 Push pull, 0 - open drain
		uint8_t RSVD    : 3;
	};
	uint8_t pwmOutputConfig;
} EMC230X_PWMOutputConfig;

typedef union
{
	struct
	{
		uint8_t PMB4 : 2; // 1 - Fan 4, 11 - 2.441KHz, 10 - 4.882KHz, 01 - 19.53KHz, 00 - 26KHz
		uint8_t PMB5 : 2; // 1 - Fan 5, 11 - 2.441KHz, 10 - 4.882KHz, 01 - 19.53KHz, 00 - 26KHz
		uint8_t RSVD : 4;
	};
	uint8_t pwmBaseF45; // Base PWM Frequency of Fan 4 and 5
} EMC230X_PWMBaseF45;

typedef union
{
	struct
	{
		uint8_t PMB1 : 2; // 1 - Fan 1, 11 - 2.441KHz, 10 - 4.882KHz, 01 - 19.53KHz, 00 - 26KHz
		uint8_t PMB2 : 2; // 1 - Fan 2, 11 - 2.441KHz, 10 - 4.882KHz, 01 - 19.53KHz, 00 - 26KHz
		uint8_t PMB3 : 2; // 1 - Fan 3, 11 - 2.441KHz, 10 - 4.882KHz, 01 - 19.53KHz, 00 - 26KHz
		uint8_t RSVD : 2;
	};
	uint8_t pwmBaseF123; // Base PWM Frequency of Fan 3,4, and 5
} EMC230X_PWMBaseF123;

typedef struct
{
	uint8_t PWM5;
	uint8_t PWM4;
	uint8_t PWM3;
	uint8_t PWM2;
	uint8_t PWM1;
} EMC230X_FanDriveSetting; // PWM = (VALUE/255)*100%

typedef struct
{
	uint8_t PWM5Div;
	uint8_t PWM4Div;
	uint8_t PWM3Div;
	uint8_t PWM2Div;
	uint8_t PWM1Div;
} EMC230X_PWMDividers; // PWM Base Frequency divided by this value 00->01

typedef union
{
	struct
	{
		uint8_t UDT  : 3; // Set PID Update rate for closed loop control
						  // 111 = 1600 ms update interval
						  // 110 = 1200 ms update interval
						  // 101 = 800 ms update interval
						  // 100 = 500 ms update interval
						  // 011 = 400 ms update interval
						  // 010 = 300 ms update interval
						  // 001 = 200 ms update interval
						  // 000 = 100 ms update interval
		uint8_t EDG  : 2; // Number of edges to use when calculating RPM
						  // 11 = 9 edges sampled (4 poles) - effective Tach multiplier is 2, based on two pole fans
						  // 10 = 7 edges sampled (3 poles) - effective Tach multiplier is 1.5, based on two pole fans
						  // 01 = 5 edges sampled (2 poles) - effective Tach multiplier is 1, based on two pole fans
						  // 00 = 3 edges sampled (1 pole)  - effective Tach multiplier is 0.5, based on two pole fans
		uint8_t FRNG : 2; // Sets min. fan speed measured
						  // 11 - 4000rpm min, TACH cnt mult = 8
						  // 10 - 2000rpm min, TACH cnt mult = 4
						  // 01 - 1000rpm min, TACH cnt mult = 2
						  // 00 -  500rpm min, TACH cnt mult = 1
		uint8_t ENAG : 1; //  1 - Enable closed loop algo, 0 - direct setting mode
	};
	uint8_t fanConfig1;
} EMC230X_FanConfig1; // General operation of RPM based fan speed control algorithm for Fan x

typedef union
{
	struct
	{
		uint8_t RSVD0 : 1;
		uint8_t ERG  : 2; // Error Window. Determines the range of the error window. When the measured fan speed
						  // is within the programmed error window around the target speed, then the fan drive
						  // setting is not updated. The algorithm will continue to monitor the fan speed and calculate necessary
						  // drive setting changes base on the error;
 						  // 11 = 200 RPM
						  // 10 = 100 RPM
						  // 01 =  50 RPM
						  // 00 =   0 RPM
		uint8_t DPT  : 2; // Fan Speed Derivative (00 NONE, 01 BASIC, 10 STEP, 11 BOTH)
		uint8_t GHEN : 1; // Glitch filter enable
		uint8_t ENRC : 1; // Enable ramp control
		uint8_t RSVD1 : 1;
	};
	uint8_t fanConfig2;
} EMC230X_FanConfig2; // Control tachometer and advanced features

typedef union
{
	struct
	{
		uint8_t GPR  : 2; // Proportional Gain, KP = 00 1x, 01 2x, 10 4x, 11 8x
		uint8_t GIN  : 2; // Integral Gain, KI = 00 1x, 01 2x, 10 4x, 11 8x
		uint8_t GDE  : 2; // Derivative Gain, KD = 00 1x, 01 2x, 10 4x, 11 8x
		uint8_t RSVD : 2;
	};
	uint8_t pidGain;
} EMC230X_PIDGain;

typedef union
{
	struct
	{
		uint8_t SPT  : 2; // Spin up time time spin-up routine will run before releasing the drive. 11 = 2s, 10 = 1s, 01 = 500 ms, 00 = 250 ms
		uint8_t SPLV : 3; // Spin up routine level, 111 = 65%, 110 = 60%, 101 = 55%
						  // 100 = 50%, 011 = 45%, 010 = 40%, 001 = 35%, 000 = 30%
		uint8_t NKCK : 1; // No Kick refers to the 100% drive for 1/4 of the spin-up time. 1 = Spin-Up will not drive to 100% PWM, 0 = Spin-Up will drive to 100% PWM
		uint8_t DFC  : 2; // Drive Fail Count (aging fan) 00 - Disabled, 01 - 16, 10 - 32, 11 - 64 periods
	};
	uint8_t fanSpinUpConfig;
} EMC230X_FanSpinUpConfig;

typedef struct
{
	uint8_t ProductFeatures : 8;
	uint8_t ProductID       : 8;
	uint8_t ManufacturerID  : 8;
	uint8_t Revision        : 8;
} EMC230X_ChipInfo;

typedef struct __attribute__((packed, aligned(sizeof(uint32_t))))
{
	EMC230X_Configuration Configuration;
	EMC230X_FanStatus FanStatus;
	EMC230X_FanStallStatus FanStallStatus;
	EMC230X_FanSpinStatus FanSpinStatus;
	EMC230X_DriveFailStatus DriveFailStatus;
	EMC230X_FanInterruptEnable FanInterruptEnable;

	EMC230X_PWMPolarity PWMPolarity;
	EMC230X_PWMOutputConfig PWMOutputConfig;
	EMC230X_PWMBaseF45 PWMBaseF45;
	EMC230X_PWMBaseF123 PWMBaseF123;
	EMC230X_PWMDividers PWMDividers;

	EMC230X_FanDriveSetting FanDriveSetting;

	EMC230X_FanConfig1 Fan1Config1; // General operation of RPM based fan speed control algorithm for Fan 1
	EMC230X_FanConfig1 Fan2Config1; // General operation of RPM based fan speed control algorithm for Fan 2
	EMC230X_FanConfig1 Fan3Config1; // General operation of RPM based fan speed control algorithm for Fan 3
	EMC230X_FanConfig1 Fan4Config1; // General operation of RPM based fan speed control algorithm for Fan 4
	EMC230X_FanConfig1 Fan5Config1; // General operation of RPM based fan speed control algorithm for Fan 5

	EMC230X_FanConfig2 Fan1Config2; // General operation of RPM based fan speed control algorithm for Fan 1
	EMC230X_FanConfig2 Fan2Config2; // General operation of RPM based fan speed control algorithm for Fan 2
	EMC230X_FanConfig2 Fan3Config2; // General operation of RPM based fan speed control algorithm for Fan 3
	EMC230X_FanConfig2 Fan4Config2; // General operation of RPM based fan speed control algorithm for Fan 4
	EMC230X_FanConfig2 Fan5Config2; // General operation of RPM based fan speed control algorithm for Fan 5

	EMC230X_PIDGain Fan1PIDGain;
	EMC230X_PIDGain Fan2PIDGain;
	EMC230X_PIDGain Fan3PIDGain;
	EMC230X_PIDGain Fan4PIDGain;
	EMC230X_PIDGain Fan5PIDGain;

	EMC230X_FanSpinUpConfig FanSpinUp1;
	EMC230X_FanSpinUpConfig FanSpinUp2;
	EMC230X_FanSpinUpConfig FanSpinUp3;
	EMC230X_FanSpinUpConfig FanSpinUp4;
	EMC230X_FanSpinUpConfig FanSpinUp5;

	uint8_t FanDrvMaxStepSize1; // Maximum step size for ramp rate control for Fan 1
	uint8_t FanDrvMaxStepSize2; // Maximum step size for ramp rate control for Fan 2
	uint8_t FanDrvMaxStepSize3; // Maximum step size for ramp rate control for Fan 3
	uint8_t FanDrvMaxStepSize4; // Maximum step size for ramp rate control for Fan 4
	uint8_t FanDrvMaxStepSize5; // Maximum step size for ramp rate control for Fan 5

	uint8_t FanDrvMinStepSize1; // Minimum drive setting for each RPM-based Fan Speed Control algorithm for Fan 1
	uint8_t FanDrvMinStepSize2; // Minimum drive setting for each RPM-based Fan Speed Control algorithm for Fan 2
	uint8_t FanDrvMinStepSize3; // Minimum drive setting for each RPM-based Fan Speed Control algorithm for Fan 3
	uint8_t FanDrvMinStepSize4; // Minimum drive setting for each RPM-based Fan Speed Control algorithm for Fan 4
	uint8_t FanDrvMinStepSize5; // Minimum drive setting for each RPM-based Fan Speed Control algorithm for Fan 5

	uint8_t ValidTachCount1; // Maximum TACH Reading register value to indicate that each fan is spinning properly for Fan1
	uint8_t ValidTachCount2; // Maximum TACH Reading register value to indicate that each fan is spinning properly for Fan2
	uint8_t ValidTachCount3; // Maximum TACH Reading register value to indicate that each fan is spinning properly for Fan3
	uint8_t ValidTachCount4; // Maximum TACH Reading register value to indicate that each fan is spinning properly for Fan4
	uint8_t ValidTachCount5; // Maximum TACH Reading register value to indicate that each fan is spinning properly for Fan5

	uint16_t DriveFailBand1; // Number of tach counts used by the Fan Drive Fail detection circuitry for Fan 1
	uint16_t DriveFailBand2; // Number of tach counts used by the Fan Drive Fail detection circuitry for Fan 2
	uint16_t DriveFailBand3; // Number of tach counts used by the Fan Drive Fail detection circuitry for Fan 3
	uint16_t DriveFailBand4; // Number of tach counts used by the Fan Drive Fail detection circuitry for Fan 4
	uint16_t DriveFailBand5; // Number of tach counts used by the Fan Drive Fail detection circuitry for Fan 5

	uint16_t TachTarget1; // Holds the target tachometer value that is maintained by the RPM-based Fan Speed Control algorithm for Fan 1
	uint16_t TachTarget2; // Holds the target tachometer value that is maintained by the RPM-based Fan Speed Control algorithm for Fan 2
	uint16_t TachTarget3; // Holds the target tachometer value that is maintained by the RPM-based Fan Speed Control algorithm for Fan 3
	uint16_t TachTarget4; // Holds the target tachometer value that is maintained by the RPM-based Fan Speed Control algorithm for Fan 4
	uint16_t TachTarget5; // Holds the target tachometer value that is maintained by the RPM-based Fan Speed Control algorithm for Fan 5

	uint16_t TachCurrent1; // Current Tachometer reading for Fan 1
	uint16_t TachCurrent2; // Current Tachometer reading for Fan 2
	uint16_t TachCurrent3; // Current Tachometer reading for Fan 3
	uint16_t TachCurrent4; // Current Tachometer reading for Fan 4
	uint16_t TachCurrent5; // Current Tachometer reading for Fan 5

	uint8_t SoftwareLock;  // All SWL registers are locked and read-only. Unlock occurs on power cycle.

	EMC230X_ChipInfo ChipInfo;

} sEMC230X_State;

class EMC230X
{

private:

	// Pg 12 of datasheet : The SMBus / I2C address is set at 0101_111(r/w)b(aka 47 or 0x2F)
	static constexpr uint8_t  I2C_ADDRESS = 0x2F;

	// Assume that we use internal clock for tachometer
	static constexpr uint16_t TACHO_FREQUENCY = 32768;

	// Max 13-bit count. Written to the tach target register it means "fan off".
	// A plain count -- setFanTachTarget() applies the <<3 register justification.
	static constexpr uint16_t TACHO_OFF = 0x1FFF;

	// Tach count <-> RPM for a 2-pole fan (2 pulses/rev), 5 edges, FRNG multiplier 1:
	//   RPM = (1/poles) * (edges-1) * m * f_TACH * 60 / count = 3932160 / count
	static constexpr uint32_t TACH_RPM_CONST = 3932160u;

	// The count saturates at 0x1FFF, so this is the slowest speed the chip can
	// measure at multiplier 1. Below it the tach reads stall whatever the fan does.
	static constexpr uint16_t TACH_MIN_RPM = 480; // 3932160 / 8191

	// List of registers
	static constexpr uint8_t EMC230X_REG_CONFIGURATION        = 0x20;
	static constexpr uint8_t EMC230X_REG_STATUS               = 0x24;
	static constexpr uint8_t EMC230X_REG_STALLSTATUS          = 0x25;
	static constexpr uint8_t EMC230X_REG_SPINSTATUS           = 0x26;
	static constexpr uint8_t EMC230X_REG_DRIVEFAILSTATUS      = 0x27;
	static constexpr uint8_t EMC230X_REG_INTERRUPTENABLE       = 0x29;
	static constexpr uint8_t EMC230X_REG_PWMPOLARITY          = 0x2A;
	static constexpr uint8_t EMC230X_REG_PWMOUTPUT            = 0x2B;
	static constexpr uint8_t EMC230X_REG_PWMBASEFREQ45        = 0x2C;
	static constexpr uint8_t EMC230X_REG_PWMBASEFREQ123       = 0x2D;

	static constexpr uint8_t EMC230X_REG_FAN1DRIVESETTING     = 0x30;
	static constexpr uint8_t EMC230X_REG_FAN1PWMDIVIDE        = 0x31;
	static constexpr uint8_t EMC230X_REG_FAN1CONFIG1          = 0x32;
	static constexpr uint8_t EMC230X_REG_FAN1CONFIG2          = 0x33;
	static constexpr uint8_t EMC230X_REG_FAN1PIDGAIN          = 0x35;
	static constexpr uint8_t EMC230X_REG_FAN1SPINUP           = 0x36;
	static constexpr uint8_t EMC230X_REG_FAN1MAXSTEP          = 0x37;
	static constexpr uint8_t EMC230X_REG_FAN1MINDRIVE         = 0x38;
	static constexpr uint8_t EMC230X_REG_FAN1VALTACHCOUNT     = 0x39;
	static constexpr uint8_t EMC230X_REG_FAN1DRVFAILLSB       = 0x3A;
	static constexpr uint8_t EMC230X_REG_FAN1DRVFAILMSB       = 0x3B;
	static constexpr uint8_t EMC230X_REG_FAN1TACHTARGETLSB    = 0x3C;
	static constexpr uint8_t EMC230X_REG_FAN1TACHTARGETMSB    = 0x3D;
	static constexpr uint8_t EMC230X_REG_FAN1TACHREADMSB      = 0x3E;
	static constexpr uint8_t EMC230X_REG_FAN1TACHREADLSB      = 0x3F;

	static constexpr uint8_t EMC230X_REG_FAN2DRIVESETTING     = 0x40;
	static constexpr uint8_t EMC230X_REG_FAN2PWMDIVIDE        = 0x41;
	static constexpr uint8_t EMC230X_REG_FAN2CONFIG1          = 0x42;
	static constexpr uint8_t EMC230X_REG_FAN2CONFIG2          = 0x43;
	static constexpr uint8_t EMC230X_REG_FAN2PIDGAIN          = 0x45;
	static constexpr uint8_t EMC230X_REG_FAN2SPINUP           = 0x46;
	static constexpr uint8_t EMC230X_REG_FAN2MAXSTEP          = 0x47;
	static constexpr uint8_t EMC230X_REG_FAN2MINDRIVE         = 0x48;
	static constexpr uint8_t EMC230X_REG_FAN2VALTACHCOUNT     = 0x49;
	static constexpr uint8_t EMC230X_REG_FAN2DRVFAILLSB       = 0x4A;
	static constexpr uint8_t EMC230X_REG_FAN2DRVFAILMSB       = 0x4B;
	static constexpr uint8_t EMC230X_REG_FAN2TACHTARGETLSB    = 0x4C;
	static constexpr uint8_t EMC230X_REG_FAN2TACHTARGETMSB    = 0x4D;
	static constexpr uint8_t EMC230X_REG_FAN2TACHREADMSB      = 0x4E;
	static constexpr uint8_t EMC230X_REG_FAN2TACHREADLSB      = 0x4F;

	static constexpr uint8_t EMC230X_REG_FAN3DRIVESETTING     = 0x50;
	static constexpr uint8_t EMC230X_REG_FAN3PWMDIVIDE        = 0x51;
	static constexpr uint8_t EMC230X_REG_FAN3CONFIG1          = 0x52;
	static constexpr uint8_t EMC230X_REG_FAN3CONFIG2          = 0x53;
	static constexpr uint8_t EMC230X_REG_FAN3PIDGAIN          = 0x55;
	static constexpr uint8_t EMC230X_REG_FAN3SPINUP           = 0x56;
	static constexpr uint8_t EMC230X_REG_FAN3MAXSTEP          = 0x57;
	static constexpr uint8_t EMC230X_REG_FAN3MINDRIVE         = 0x58;
	static constexpr uint8_t EMC230X_REG_FAN3VALTACHCOUNT     = 0x59;
	static constexpr uint8_t EMC230X_REG_FAN3DRVFAILLSB       = 0x5A;
	static constexpr uint8_t EMC230X_REG_FAN3DRVFAILMSB       = 0x5B;
	static constexpr uint8_t EMC230X_REG_FAN3TACHTARGETLSB    = 0x5C;
	static constexpr uint8_t EMC230X_REG_FAN3TACHTARGETMSB    = 0x5D;
	static constexpr uint8_t EMC230X_REG_FAN3TACHREADMSB      = 0x5E;
	static constexpr uint8_t EMC230X_REG_FAN3TACHREADLSB      = 0x5F;

	static constexpr uint8_t EMC230X_REG_FAN4DRIVESETTING     = 0x60;
	static constexpr uint8_t EMC230X_REG_FAN4PWMDIVIDE        = 0x61;
	static constexpr uint8_t EMC230X_REG_FAN4CONFIG1          = 0x62;
	static constexpr uint8_t EMC230X_REG_FAN4CONFIG2          = 0x63;
	static constexpr uint8_t EMC230X_REG_FAN4PIDGAIN          = 0x65;
	static constexpr uint8_t EMC230X_REG_FAN4SPINUP           = 0x66;
	static constexpr uint8_t EMC230X_REG_FAN4MAXSTEP          = 0x67;
	static constexpr uint8_t EMC230X_REG_FAN4MINDRIVE         = 0x68;
	static constexpr uint8_t EMC230X_REG_FAN4VALTACHCOUNT     = 0x69;
	static constexpr uint8_t EMC230X_REG_FAN4DRVFAILLSB       = 0x6A;
	static constexpr uint8_t EMC230X_REG_FAN4DRVFAILMSB       = 0x6B;
	static constexpr uint8_t EMC230X_REG_FAN4TACHTARGETLSB    = 0x6C;
	static constexpr uint8_t EMC230X_REG_FAN4TACHTARGETMSB    = 0x6D;
	static constexpr uint8_t EMC230X_REG_FAN4TACHREADMSB      = 0x6E;
	static constexpr uint8_t EMC230X_REG_FAN4TACHREADLSB      = 0x6F;

	static constexpr uint8_t EMC230X_REG_FAN5DRIVESETTING     = 0x70;
	static constexpr uint8_t EMC230X_REG_FAN5PWMDIVIDE        = 0x71;
	static constexpr uint8_t EMC230X_REG_FAN5CONFIG1          = 0x72;
	static constexpr uint8_t EMC230X_REG_FAN5CONFIG2          = 0x73;
	static constexpr uint8_t EMC230X_REG_FAN5PIDGAIN          = 0x75;
	static constexpr uint8_t EMC230X_REG_FAN5SPINUP           = 0x76;
	static constexpr uint8_t EMC230X_REG_FAN5MAXSTEP          = 0x77;
	static constexpr uint8_t EMC230X_REG_FAN5MINDRIVE         = 0x78;
	static constexpr uint8_t EMC230X_REG_FAN5VALTACHCOUNT     = 0x79;
	static constexpr uint8_t EMC230X_REG_FAN5DRVFAILLSB       = 0x7A;
	static constexpr uint8_t EMC230X_REG_FAN5DRVFAILMSB       = 0x7B;
	static constexpr uint8_t EMC230X_REG_FAN5TACHTARGETLSB    = 0x7C;
	static constexpr uint8_t EMC230X_REG_FAN5TACHTARGETMSB    = 0x7D;
	static constexpr uint8_t EMC230X_REG_FAN5TACHREADMSB      = 0x7E;
	static constexpr uint8_t EMC230X_REG_FAN5TACHREADLSB      = 0x7F;

	static constexpr uint8_t EMC230X_REG_SOFTWARELOCK         = 0xEF; // 1 - register writes are locked, unlocked on power up, 0 - registers are writeable
	static constexpr uint8_t EMC230X_REG_PRODUCTFEATURES      = 0xFC; // EMC2303 and EMC2305 - SMBBus Address and Default Fan PWM
	static constexpr uint8_t EMC230X_REG_PRODUCTID            = 0xFD;
	static constexpr uint8_t EMC230X_REG_MANUFACTURERID       = 0xFE;
	static constexpr uint8_t EMC230X_REG_CHIPREVISION         = 0xFF;

	/// Prototype of user supplied SMBus write_byte or write_word function. Should return 0 on success and a non-0 error code on failure.
	typedef int (*smbus_write_register)(uint16_t addr, uint8_t cmd, uint8_t* data, uint8_t len);

	/// Prototype of user supplied SMBus read_byte or read_word function. Should return 0 on success and a non-0 error code on failure. */
	typedef int (*smbus_read_register)(uint16_t addr, uint8_t cmd, uint8_t* data, uint8_t len);
	uint16_t i2cAddr;
	smbus_write_register smbus_write_reg;
	smbus_read_register smbus_read_reg;

	sEMC230X_State   emc230xState_;

	// Bit mask of the fan channels this part actually has, built from the product ID
	// by getChipInfo(): EMC2301 = 0x01, EMC2302 = 0x03, EMC2303 = 0x07, EMC2305 = 0x1F.
	// A mask, so it cannot be an EMC230X_FAN (those are single bits).
	uint8_t          fanControllers_;

	// Set once getChipInfo() has successfully read the product ID, so fanControllers_
	// can be trusted. The bulk getters consult ensureIdentified() rather than requiring
	// the caller to remember the ordering.
	bool             chipIdentified_;

	// Identify the part on first use. Registers for a fan the part does not implement
	// are reserved, so the bulk getters need the mask before they can skip them safely.
	// getChipInfo() touches only chip-level registers, so this cannot recurse.
	EMC230X_STATUS   ensureIdentified();
	EMC230X_FANPOLES fan1Poles_;
	EMC230X_FANPOLES fan2Poles_;
	EMC230X_FANPOLES fan3Poles_;
	EMC230X_FANPOLES fan4Poles_;
	EMC230X_FANPOLES fan5Poles_;

	uint8_t tachFan1PolesMultiplier_;
	uint8_t tachFan2PolesMultiplier_;
	uint8_t tachFan3PolesMultiplier_;
	uint8_t tachFan4PolesMultiplier_;
	uint8_t tachFan5PolesMultiplier_;

	// True only for exactly one of FANCNTRL_1..5. The setters index registers off a
	// single fan, so a mask of several fans (or 0) is not a legal argument.
	static bool isValidFan(EMC230X_FAN fan)
	{
		return (fan == EMC230X_FAN::FANCNTRL_1) || (fan == EMC230X_FAN::FANCNTRL_2) ||
		       (fan == EMC230X_FAN::FANCNTRL_3) || (fan == EMC230X_FAN::FANCNTRL_4) ||
		       (fan == EMC230X_FAN::FANCNTRL_5);
	}

public:

  // Low level I2C/SMBus Functions (processor specific)
  void setI2CDriver(uint8_t addr_, smbus_write_register write_func, smbus_read_register read_func) { i2cAddr = addr_; smbus_write_reg = write_func; smbus_read_reg = read_func;}

  // Set / Get EMC230X Fan Controller over SMBus
  EMC230X_STATUS getChipState(); // Runs all the "get" functions
  EMC230X_STATUS getChipInfo();
  EMC230X_STATUS getChipConfig();
  EMC230X_STATUS getSoftWareLock();

  EMC230X_STATUS getFanStatus();
  EMC230X_STATUS getFanStallStatus();
  EMC230X_STATUS getFanSpinStatus();
  EMC230X_STATUS getDriveFailStatus();

  EMC230X_STATUS getFanInterruptEnable();

  EMC230X_STATUS getPWMPolarity();
  EMC230X_STATUS getPWMOutput();
  EMC230X_STATUS getPWMBaseFreq();
  EMC230X_STATUS getPWMDivider();

  EMC230X_STATUS getFanDriveSettings(); // Fan Speed
  EMC230X_STATUS getFanConfig1();
  EMC230X_STATUS getFanConfig2();
  EMC230X_STATUS getPIDGain();
  EMC230X_STATUS getFanSpinUpConfig();
  EMC230X_STATUS getFanMaxStep();
  EMC230X_STATUS getFanMinDrive();
  EMC230X_STATUS getDriveFailBand();
  EMC230X_STATUS getFanValidTachCount();
  // Both of these refresh the whole cached state from the chip: every "get" reads all
  // five fans' registers whether the part has them or not, so each call is a burst of
  // blocking SMBus transactions. Call once per control cycle, not per fan.
  EMC230X_STATUS getFanTachTarget();
  EMC230X_STATUS getFanTachReading();

  // Both report speed through *rpm and the outcome through the return value, so a
  // stopped fan and a failed read stay distinguishable:
  //   EMC230X_STATUS_OK  and *rpm == 0  -> fan is genuinely stopped or disconnected
  //   EMC230X_STATUS_OK  and *rpm  > 0  -> measured speed
  //   anything else                     -> *rpm is not meaningful
  // A null rpm pointer returns EMC230X_STATUS_INVALIDARG.

  // Floating point, and re-reads config + all tach registers on every call (~15 SMBus
  // transactions).
  EMC230X_STATUS getFanRPM(EMC230X_FAN fan, uint16_t *rpm);

  // Integer equivalent of getFanRPM using only the cached tach count -- no I2C at all.
  // Call getFanTachReading() first to refresh, or this returns the previous reading.
  EMC230X_STATUS getFanRPMReading(EMC230X_FAN fan, uint16_t *rpm);

  EMC230X_STATUS setPWMPolarity(EMC230X_FAN fan, uint8_t polarity);
  EMC230X_STATUS setPWMOutput(EMC230X_FAN fan, uint8_t iotype);
  EMC230X_STATUS setPWMBaseFreq(EMC230X_FAN fan, EMC230X_PWMFREQ freq);
  EMC230X_STATUS setPWMDivider(EMC230X_FAN fan, uint8_t divider);

  EMC230X_STATUS setFanInterrupt(EMC230X_FAN fan, bool enabled);
  EMC230X_STATUS setFanConfig1(EMC230X_FAN fan, EMC230X_FanConfig1 cfg);
  EMC230X_STATUS setFanConfig2(EMC230X_FAN fan, EMC230X_FanConfig2 cfg);
  EMC230X_STATUS setPIDGain(EMC230X_FAN fan, EMC230X_PIDGain gain);

  EMC230X_STATUS setFanSpinUpConfig(EMC230X_FAN fan, EMC230X_FanSpinUpConfig spinup);
  EMC230X_STATUS setFanMaxStep(EMC230X_FAN fan, uint8_t maxstep);
  EMC230X_STATUS setFanMinDrive(EMC230X_FAN fan, uint8_t minstep);
  EMC230X_STATUS setFanDriveSettings(EMC230X_FAN fan, uint8_t speed);

  EMC230X_STATUS setDriveFailBand(EMC230X_FAN fan, uint16_t fail);
  EMC230X_STATUS setFanValidTachCount(EMC230X_FAN fan, uint8_t count);
  EMC230X_STATUS setFanTachTarget(EMC230X_FAN fan, uint16_t tach);

  EMC230X_STATUS toggleControlAlgorithm(EMC230X_FAN fan, bool enable);

  void     runControlAlgorithm(EMC230X_FAN fan, uint16_t targetRPM); // RPM, not tach counts. 0 = off.
  uint16_t rpmToTachCount(uint16_t rpm);

  // Data Accessors
  uint16_t          getFanTachReading(EMC230X_FAN fan);
  EMC230X_FANPOLES  getFanPoles(EMC230X_FAN fan);
  EMC230X_STATUS    setFanPoles(EMC230X_FAN fan, EMC230X_FANPOLES poles);
  // Read-only view of everything the last get*() calls cached. The getters populate this
  // struct and previously nothing could read most of it back -- getSoftWareLock(), for
  // one, had no observable effect at all. Values are only as fresh as the matching
  // get*() call; nothing here triggers bus traffic.
  const sEMC230X_State &getCachedState() const { return emc230xState_; }

  // Mask of fan channels this part implements (see fanControllers_). The bulk getters
  // identify the part on first use, so this is populated after any of them has run;
  // call getChipInfo() explicitly if you need it before that.
  uint8_t           getAvailableFans() { return fanControllers_; }
  uint8_t           getProductFeatures() { return emc230xState_.ChipInfo.ProductFeatures; } // see datasheet page 48
  EMC230X_PRODUCTID getChipID()   { return (EMC230X_PRODUCTID) emc230xState_.ChipInfo.ProductID; } // see datasheet page 48
  uint8_t           getChipRev()  { return emc230xState_.ChipInfo.Revision; } // see datasheet page 48
  EMC230X_MANUFACTURERID getManufacturerID() { return (EMC230X_MANUFACTURERID) emc230xState_.ChipInfo.ManufacturerID; } // see datasheet page 48

  EMC230X(uint8_t addr_, smbus_write_register write_func, smbus_read_register read_func);
  EMC230X();
  ~EMC230X();
};

} // end namespace

#endif /* EMC230X_HPP_ */
