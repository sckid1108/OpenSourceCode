/*
 * EMC230X_TestCode.cpp
 *
 *  Bring-up / API conformance test for the EMC230X driver. See the header.
 *  TEMPORARY CODE -- delete this file and its header together when done.
 */

#include <EMC230X_TestCode.hpp>
#include <stdio.h>

using namespace EMC230X;

namespace
{

uint16_t g_pass = 0;
uint16_t g_fail = 0;

/* Every assertion in this file funnels through here so the summary count is honest. */
void check(bool ok, const char *what)
{
	if (ok) { g_pass++; }
	else    { g_fail++; }

	printf("  [%s] %s\r\n", ok ? "PASS" : "FAIL", what);
}

/* Same, but prints the two values when they disagree -- otherwise a failing round-trip
   tells you nothing about which side is wrong. */
void checkEq(uint32_t got, uint32_t want, const char *what)
{
	bool ok = (got == want);

	if (ok) { g_pass++; }
	else    { g_fail++; }

	if (ok) printf("  [PASS] %s\r\n", what);
	else    printf("  [FAIL] %s : got %lu, expected %lu\r\n",
	               what, (unsigned long) got, (unsigned long) want);
}

void checkStatus(EMC230X_STATUS st, const char *what)
{
	check(st == EMC230X_STATUS::EMC230X_STATUS_OK, what);

	if (st != EMC230X_STATUS::EMC230X_STATUS_OK)
	{
		printf("         status = 0x%02X\r\n", (unsigned) st);
	}
}

const char *partName(EMC230X_PRODUCTID id)
{
	switch (id)
	{
		case EMC230X_PRODUCTID::EMC2301_ID: return "EMC2301 (1 fan)";
		case EMC230X_PRODUCTID::EMC2302_ID: return "EMC2302 (2 fans)";
		case EMC230X_PRODUCTID::EMC2303_ID: return "EMC2303 (3 fans)";
		case EMC230X_PRODUCTID::EMC2305_ID: return "EMC2305 (5 fans)";
		default:                            return "UNKNOWN";
	}
}

/* Fan mask each part is expected to report, so getAvailableFans() is checked against the
   product ID rather than just printed. */
uint8_t expectedFanMask(EMC230X_PRODUCTID id)
{
	switch (id)
	{
		case EMC230X_PRODUCTID::EMC2301_ID: return FANCNTRL_1;
		case EMC230X_PRODUCTID::EMC2302_ID: return FANCNTRL_1 | FANCNTRL_2;
		case EMC230X_PRODUCTID::EMC2303_ID: return FANCNTRL_1 | FANCNTRL_2 | FANCNTRL_3;
		case EMC230X_PRODUCTID::EMC2305_ID: return FANCNTRL_1 | FANCNTRL_2 | FANCNTRL_3 |
		                                           FANCNTRL_4 | FANCNTRL_5;
		default:                            return FANCNTRL_1;
	}
}

const EMC230X_FAN kFans[5] =
{
	FANCNTRL_1, FANCNTRL_2, FANCNTRL_3, FANCNTRL_4, FANCNTRL_5
};

/* sEMC230X_State is __attribute__((packed)), so the address of a uint16_t member may be
   unaligned -- and an unaligned 16-bit load faults on Cortex-M0+. Read these by value;
   the compiler then emits safe byte-wise access. The uint8_t members are fine either way. */
uint16_t driveFailBandOf(const sEMC230X_State &st, uint8_t i)
{
	switch (i)
	{
		case 0:  return st.DriveFailBand1;
		case 1:  return st.DriveFailBand2;
		case 2:  return st.DriveFailBand3;
		case 3:  return st.DriveFailBand4;
		default: return st.DriveFailBand5;
	}
}

uint16_t tachTargetOf(const sEMC230X_State &st, uint8_t i)
{
	switch (i)
	{
		case 0:  return st.TachTarget1;
		case 1:  return st.TachTarget2;
		case 2:  return st.TachTarget3;
		case 3:  return st.TachTarget4;
		default: return st.TachTarget5;
	}
}

} // anonymous namespace


uint16_t EMC230X_Test(EMC230X::EMC230X &fanCtl)
{
	g_pass = 0;
	g_fail = 0;

	printf("\r\n=========== EMC230X API TEST ===========\r\n");

	/* --------------------------------------------------------------------------
	 * 1. Identity. Nothing below is meaningful if we are not talking to the part,
	 *    so this section gates the rest.
	 * ------------------------------------------------------------------------*/
	printf("\r\n-- 1. Identity --\r\n");

	checkStatus(fanCtl.getChipInfo(), "getChipInfo()");

	EMC230X_MANUFACTURERID mfg = fanCtl.getManufacturerID();
	checkEq(mfg, MICROCHIP_MANUFACTURER, "getManufacturerID() == 0x5D");

	if (mfg != MICROCHIP_MANUFACTURER)
	{
		printf("\r\nNo EMC230X responding -- aborting. Check wiring and the SMBus address.\r\n");
		printf("=========== %u passed, %u failed ===========\r\n", g_pass, g_fail);
		return g_fail;
	}

	EMC230X_PRODUCTID pid = fanCtl.getChipID();
	printf("  device    : %s (product ID 0x%02X)\r\n", partName(pid), (unsigned) pid);
	printf("  revision  : 0x%02X\r\n", (unsigned) fanCtl.getChipRev());
	printf("  features  : 0x%02X\r\n", (unsigned) fanCtl.getProductFeatures());

	uint8_t fans = fanCtl.getAvailableFans();
	printf("  fan mask  : 0x%02X\r\n", (unsigned) fans);
	checkEq(fans, expectedFanMask(pid), "getAvailableFans() matches the product ID");

	/* --------------------------------------------------------------------------
	 * 2. Conversion math. Pure computation, no bus traffic -- these must hold on
	 *    any part. Values are datasheet Equation 4-3, RPM = 3932160 / count, and
	 *    match Tests/tach_test.c.
	 * ------------------------------------------------------------------------*/
	printf("\r\n-- 2. RPM to tach count (no I2C) --\r\n");

	checkEq(fanCtl.rpmToTachCount(5000),  786, "rpmToTachCount(5000) == 786");
	checkEq(fanCtl.rpmToTachCount(4000),  983, "rpmToTachCount(4000) == 983");
	checkEq(fanCtl.rpmToTachCount(3000), 1310, "rpmToTachCount(3000) == 1310");
	checkEq(fanCtl.rpmToTachCount(2500), 1572, "rpmToTachCount(2500) == 1572");
	checkEq(fanCtl.rpmToTachCount(2000), 1966, "rpmToTachCount(2000) == 1966");
	checkEq(fanCtl.rpmToTachCount(1500), 2621, "rpmToTachCount(1500) == 2621");
	checkEq(fanCtl.rpmToTachCount(1000), 3932, "rpmToTachCount(1000) == 3932");

	checkEq(fanCtl.rpmToTachCount(0), 0x1FFF, "rpmToTachCount(0) == TACHO_OFF");
	checkEq(fanCtl.rpmToTachCount(100), fanCtl.rpmToTachCount(480),
	        "below 480 RPM clamps to the slowest measurable count");

	/* --------------------------------------------------------------------------
	 * 3. Software lock. If set, every write below would be silently ignored, so
	 *    skip the round-trips rather than report a wall of false failures.
	 * ------------------------------------------------------------------------*/
	printf("\r\n-- 3. Software lock --\r\n");

	checkStatus(fanCtl.getSoftWareLock(), "getSoftWareLock()");

	bool locked = (fanCtl.getCachedState().SoftwareLock != 0);
	printf("  software lock: %s\r\n", locked ? "SET (writes ignored)" : "clear");

	/* --------------------------------------------------------------------------
	 * 4. Bulk getters. Each refreshes part of the cached state.
	 * ------------------------------------------------------------------------*/
	printf("\r\n-- 4. Bulk getters --\r\n");

	checkStatus(fanCtl.getChipConfig(),         "getChipConfig()");
	checkStatus(fanCtl.getFanStatus(),          "getFanStatus()");
	checkStatus(fanCtl.getFanStallStatus(),     "getFanStallStatus()");
	checkStatus(fanCtl.getFanSpinStatus(),      "getFanSpinStatus()");
	checkStatus(fanCtl.getDriveFailStatus(),    "getDriveFailStatus()");
	checkStatus(fanCtl.getFanInterruptEnable(), "getFanInterruptEnable()");
	checkStatus(fanCtl.getPWMPolarity(),        "getPWMPolarity()");
	checkStatus(fanCtl.getPWMOutput(),          "getPWMOutput()");
	checkStatus(fanCtl.getPWMBaseFreq(),        "getPWMBaseFreq()");
	checkStatus(fanCtl.getPWMDivider(),         "getPWMDivider()");
	checkStatus(fanCtl.getFanDriveSettings(),   "getFanDriveSettings()");
	checkStatus(fanCtl.getFanConfig1(),         "getFanConfig1()");
	checkStatus(fanCtl.getFanConfig2(),         "getFanConfig2()");
	checkStatus(fanCtl.getPIDGain(),            "getPIDGain()");
	checkStatus(fanCtl.getFanSpinUpConfig(),    "getFanSpinUpConfig()");
	checkStatus(fanCtl.getFanMaxStep(),         "getFanMaxStep()");
	checkStatus(fanCtl.getFanMinDrive(),        "getFanMinDrive()");
	checkStatus(fanCtl.getFanValidTachCount(),  "getFanValidTachCount()");
	checkStatus(fanCtl.getDriveFailBand(),      "getDriveFailBand()");
	checkStatus(fanCtl.getFanTachTarget(),      "getFanTachTarget()");
	checkStatus(fanCtl.getFanTachReading(),     "getFanTachReading()");
	checkStatus(fanCtl.getChipState(),          "getChipState() (all of the above)");

	/* --------------------------------------------------------------------------
	 * 5. Per-fan setter round-trips. Only for fans this part implements. Each one
	 *    writes a distinctive value, reads it back through the driver's own getter
	 *    and compares -- which is what actually proves the register addressing.
	 * ------------------------------------------------------------------------*/
	printf("\r\n-- 5. Setter round-trips --\r\n");

	if (locked)
	{
		printf("  skipped: device is software locked\r\n");
	}
	else for (uint8_t i = 0; i < 5; i++)
	{
		EMC230X_FAN f = kFans[i];

		if ((fans & (uint8_t) f) == 0)
		{
			continue; // not present on this part
		}

		printf("  --- fan %u ---\r\n", (unsigned)(i + 1));

		const sEMC230X_State &st = fanCtl.getCachedState();

		// Direct setting mode: the Fan Drive Setting register is read-only while the
		// RPM control algorithm is enabled (datasheet section 6.12).
		checkStatus(fanCtl.toggleControlAlgorithm(f, false), "toggleControlAlgorithm(off)");

		// Every setter below writes a distinctive value and then reads it back through
		// the driver's own getter. Comparing the value is the part that actually proves
		// the register addressing -- a status of OK alone only proves the bus ACKed.
		const uint8_t  *drive[5] = { &st.FanDriveSetting.PWM1, &st.FanDriveSetting.PWM2,
		                             &st.FanDriveSetting.PWM3, &st.FanDriveSetting.PWM4,
		                             &st.FanDriveSetting.PWM5 };
		const uint8_t  *maxst[5] = { &st.FanDrvMaxStepSize1, &st.FanDrvMaxStepSize2,
		                             &st.FanDrvMaxStepSize3, &st.FanDrvMaxStepSize4,
		                             &st.FanDrvMaxStepSize5 };
		const uint8_t  *mindr[5] = { &st.FanDrvMinStepSize1, &st.FanDrvMinStepSize2,
		                             &st.FanDrvMinStepSize3, &st.FanDrvMinStepSize4,
		                             &st.FanDrvMinStepSize5 };
		const uint8_t  *vtach[5] = { &st.ValidTachCount1, &st.ValidTachCount2,
		                             &st.ValidTachCount3, &st.ValidTachCount4,
		                             &st.ValidTachCount5 };
		const EMC230X_FanConfig1 *cf1[5] = { &st.Fan1Config1, &st.Fan2Config1, &st.Fan3Config1,
		                                     &st.Fan4Config1, &st.Fan5Config1 };
		const EMC230X_FanConfig2 *cf2[5] = { &st.Fan1Config2, &st.Fan2Config2, &st.Fan3Config2,
		                                     &st.Fan4Config2, &st.Fan5Config2 };
		const EMC230X_PIDGain    *pid[5] = { &st.Fan1PIDGain, &st.Fan2PIDGain, &st.Fan3PIDGain,
		                                     &st.Fan4PIDGain, &st.Fan5PIDGain };
		const EMC230X_FanSpinUpConfig *spn[5] = { &st.FanSpinUp1, &st.FanSpinUp2, &st.FanSpinUp3,
		                                          &st.FanSpinUp4, &st.FanSpinUp5 };

		checkStatus(fanCtl.setFanDriveSettings(f, 0x40), "setFanDriveSettings(0x40)");
		checkStatus(fanCtl.getFanDriveSettings(),        "  getFanDriveSettings()");
		checkEq(*drive[i], 0x40,                         "  drive setting reads back 0x40");

		checkStatus(fanCtl.setFanMaxStep(f, 0x0A), "setFanMaxStep(0x0A)");
		checkStatus(fanCtl.getFanMaxStep(),        "  getFanMaxStep()");
		checkEq(*maxst[i], 0x0A,                   "  max step reads back 0x0A");

		checkStatus(fanCtl.setFanMinDrive(f, 0x20), "setFanMinDrive(0x20)");
		checkStatus(fanCtl.getFanMinDrive(),        "  getFanMinDrive()");
		checkEq(*mindr[i], 0x20,                    "  min drive reads back 0x20");

		checkStatus(fanCtl.setFanValidTachCount(f, 0xF5), "setFanValidTachCount(0xF5)");
		checkStatus(fanCtl.getFanValidTachCount(),        "  getFanValidTachCount()");
		checkEq(*vtach[i], 0xF5,                          "  valid tach count reads back 0xF5");

		checkStatus(fanCtl.setDriveFailBand(f, 0x0080), "setDriveFailBand(0x0080)");
		checkStatus(fanCtl.getDriveFailBand(),          "  getDriveFailBand()");
		checkEq(driveFailBandOf(st, i), 0x0080,         "  drive fail band reads back 0x0080");

		// This one matters most: the tach target is the path through the <<3 write /
		// >>3 read register justification, so a scaling regression surfaces here as a
		// mismatch instead of silently skewing every RPM by a factor of 8.
		uint16_t want = fanCtl.rpmToTachCount(2000);
		checkStatus(fanCtl.setFanTachTarget(f, want), "setFanTachTarget(2000 RPM)");
		checkStatus(fanCtl.getFanTachTarget(),        "  getFanTachTarget()");
		checkEq(tachTargetOf(st, i), want,            "  tach target round-trips (<<3 / >>3)");

		checkStatus(fanCtl.setPWMPolarity(f, 0), "setPWMPolarity(normal)");
		checkStatus(fanCtl.setPWMOutput(f, 0),   "setPWMOutput(open drain)");
		checkStatus(fanCtl.setPWMBaseFreq(f, EMC230X_PWMFREQ::FREQ_26KHZ),
		            "setPWMBaseFreq(26 kHz)");
		checkStatus(fanCtl.setPWMDivider(f, 1), "setPWMDivider(1)");

		EMC230X_FanConfig1 cfg1 = {0};
		cfg1.UDT = 3; cfg1.EDG = 1; cfg1.FRNG = 0; cfg1.ENAG = 0;
		checkStatus(fanCtl.setFanConfig1(f, cfg1),      "setFanConfig1(0x0B)");
		checkEq(cf1[i]->fanConfig1, 0x0B,               "  fan config 1 reads back 0x0B");

		EMC230X_FanConfig2 cfg2 = {0};
		cfg2.ERG = 1; cfg2.DPT = 1; cfg2.GHEN = 1; cfg2.ENRC = 0;
		checkStatus(fanCtl.setFanConfig2(f, cfg2),      "setFanConfig2(0x2A)");
		checkEq(cf2[i]->fanConfig2, 0x2A,               "  fan config 2 reads back 0x2A");

		EMC230X_PIDGain gain = {0};
		gain.GPR = 2; gain.GIN = 2; gain.GDE = 2;
		checkStatus(fanCtl.setPIDGain(f, gain), "setPIDGain(4x/4x/4x)");
		checkEq(pid[i]->pidGain, 0x2A,          "  PID gain reads back 0x2A");

		EMC230X_FanSpinUpConfig spin = {0};
		spin.DFC = 3; spin.NKCK = 1; spin.SPLV = 7; spin.SPT = 1;
		checkStatus(fanCtl.setFanSpinUpConfig(f, spin), "setFanSpinUpConfig()");
		checkEq(spn[i]->fanSpinUpConfig, 0xFD,          "  spin-up config reads back 0xFD");

		checkStatus(fanCtl.setFanInterrupt(f, false), "setFanInterrupt(false)");

		checkStatus(fanCtl.setFanPoles(f, EMC230X_FANPOLES::FANPOLE_2), "setFanPoles(2)");
		checkEq(fanCtl.getFanPoles(f), EMC230X_FANPOLES::FANPOLE_2, "  getFanPoles() agrees");
		checkEq(cf1[i]->EDG, EMC230X_FANPOLES::FANPOLE_2, "  EDG field matches the pole setting");
	}

	/* --------------------------------------------------------------------------
	 * 6. Tach and RPM. The float and integer paths are computed completely
	 *    differently, so agreeing to within a rounding step is real evidence that
	 *    both implement Equation 4-2 correctly.
	 * ------------------------------------------------------------------------*/
	printf("\r\n-- 6. Tach / RPM --\r\n");

	checkStatus(fanCtl.getFanTachReading(), "getFanTachReading() refresh");

	for (uint8_t i = 0; i < 5; i++)
	{
		EMC230X_FAN f = kFans[i];

		if ((fans & (uint8_t) f) == 0)
		{
			continue;
		}

		uint16_t count = fanCtl.getFanTachReading(f);

		uint16_t rpmFloat = 0xFFFF;
		uint16_t rpmInt   = 0xFFFF;

		EMC230X_STATUS sf = fanCtl.getFanRPM(f, &rpmFloat);
		EMC230X_STATUS si = fanCtl.getFanRPMReading(f, &rpmInt);

		printf("  fan %u: count %u, rpm(float) %u, rpm(int) %u\r\n",
		       (unsigned)(i + 1), count, rpmFloat, rpmInt);

		checkStatus(sf, "  getFanRPM()");
		checkStatus(si, "  getFanRPMReading()");

		if (count >= 0x1FFF)
		{
			// Full scale: no tach edges. Stopped or disconnected, reported as 0 RPM
			// with an OK status rather than as an error.
			checkEq(rpmInt, 0, "  stopped fan reports 0 RPM");
		}
		else
		{
			uint16_t diff = (rpmFloat > rpmInt) ? (rpmFloat - rpmInt) : (rpmInt - rpmFloat);
			check(diff <= 1, "  float and integer RPM agree within 1");
		}
	}

	/* --------------------------------------------------------------------------
	 * 7. Negative tests. A published driver has to reject bad input rather than
	 *    write to register 0x00 or dereference a null driver pointer.
	 * ------------------------------------------------------------------------*/
	printf("\r\n-- 7. Error handling --\r\n");

	EMC230X_FAN bogus = (EMC230X_FAN) 0x00;
	checkEq(fanCtl.setFanDriveSettings(bogus, 0x10), EMC230X_STATUS_INVALIDARG,
	        "setFanDriveSettings(invalid fan) rejected");
	checkEq(fanCtl.setFanTachTarget(bogus, 1000), EMC230X_STATUS_INVALIDARG,
	        "setFanTachTarget(invalid fan) rejected");

	EMC230X_FAN multi = (EMC230X_FAN) 0xFF; // a mask, not a single fan
	checkEq(fanCtl.setFanMaxStep(multi, 0x10), EMC230X_STATUS_INVALIDARG,
	        "setFanMaxStep(fan mask) rejected");

	uint16_t dummy = 0;
	checkEq(fanCtl.getFanRPM(bogus, &dummy), EMC230X_STATUS_INVALIDARG,
	        "getFanRPM(invalid fan) rejected");
	checkEq(fanCtl.getFanRPMReading(FANCNTRL_1, nullptr), EMC230X_STATUS_INVALIDARG,
	        "getFanRPMReading(null pointer) rejected");

	// A second instance with no SMBus driver installed must report that rather than
	// calling through a null function pointer.
	EMC230X::EMC230X orphan;
	checkEq(orphan.getChipInfo(), EMC230X_SMBUS_DRIVER_NULL,
	        "getChipInfo() without setI2CDriver() reports SMBUS_DRIVER_NULL");
	checkEq(orphan.getFanTachReading(), EMC230X_SMBUS_DRIVER_NULL,
	        "getFanTachReading() without setI2CDriver() reports SMBUS_DRIVER_NULL");

	/* --------------------------------------------------------------------------
	 * 8. Leave the hardware safe. Section 5 wrote drive settings; do not exit with
	 *    a fan spinning.
	 * ------------------------------------------------------------------------*/
	printf("\r\n-- 8. Returning fans to a safe state --\r\n");

	for (uint8_t i = 0; i < 5; i++)
	{
		EMC230X_FAN f = kFans[i];

		if ((fans & (uint8_t) f) == 0)
		{
			continue;
		}

		fanCtl.toggleControlAlgorithm(f, false);
		fanCtl.setFanDriveSettings(f, 0);
	}
	printf("  all present fans commanded off\r\n");

	printf("\r\n=========== %u passed, %u failed ===========\r\n\r\n", g_pass, g_fail);

	return g_fail;
}
