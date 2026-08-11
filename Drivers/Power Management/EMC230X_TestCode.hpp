/*
 * EMC230X_TestCode.hpp
 *
 *  Bring-up / API conformance test for the EMC230X driver.
 *
 *  TEMPORARY CODE. This file and EMC230X_TestCode.cpp exist to prove and document the
 *  driver API against real silicon. Delete both together when no longer needed, and
 *  remove the EMC230X_Test() call from App::Init().
 */

#pragma once

#ifndef EMC230X_TESTCODE_HPP_
#define EMC230X_TESTCODE_HPP_

#include <EMC230X.hpp>

/*
 * Exercises the whole EMC230X public API against whatever part is on the bus and prints
 * a report via printf (the CDC port). Detects the device first and only touches the fan
 * channels that part actually implements.
 *
 * The caller must have called setI2CDriver() first; nothing else is assumed.
 *
 * This WRITES to the chip: every setter is round-tripped (write, read back, compare,
 * restore). It leaves every fan commanded off, so run it before whatever configures the
 * fans for real.
 *
 * Returns the number of failed checks -- 0 means the whole API behaved as documented.
 */
uint16_t EMC230X_Test(EMC230X::EMC230X &fan);

#endif /* EMC230X_TESTCODE_HPP_ */
