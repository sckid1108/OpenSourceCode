/*
 * bme280.h
 *
 *  Created on: Dec 30, 2025
 *      Author: Michael Margolese
 */
#pragma once

#ifndef INC_BME280_H_
#define INC_BME280_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define __FPU_PRESENT 1

#define BME280_RESET_SIGNAL 0xB6 // write this to BME280_REGISTER_SOFTRESET(0xE0)
#define BME280_I2C_ADDR 0x76 // If SDO is pull low, otherwise 0x77

#define BME280_OVER_0x		0x00 //skipped, output set to 0x80000
#define BME280_OVER_1x		0x01
#define BME280_OVER_2x		0x02
#define BME280_OVER_4x		0x03
#define BME280_OVER_8x		0x04
#define BME280_OVER_16x	    0x05

#define BME280_FORCED_MODE 0x01
#define BME280_NORMAL_MODE 0x03
#define BME280_SLEEP_MODE 0x00

#define BME280_STANDBY_500us	0x00
#define BME280_STANDBY_62500us	0x01
#define BME280_STANDBY_125ms	0x02
#define BME280_STANDBY_250ms	0x03
#define BME280_STANDBY_500ms	0x04
#define BME280_STANDBY_1000ms	0x05
#define BME280_STANDBY_10ms		0x06
#define BME280_STANDBY_20ms		0x07

#define BME280_IIR_OFF	0x00
#define BME280_IIR_2x	0x01
#define BME280_IIR_4x	0x02
#define BME280_IIR_8x	0x03
#define BME280_IIR_16x	0x04

#define BME280_SPI_OFF	0x00
#define BME280_SPI_ON	0x01

// The standard atmosphere (symbol: atm) is a unit of pressure defined as 101,325 Pa (1,013.25 hPa), which is equivalent to 1,013.25 millibars, 760 mm Hg, 29.9212 inches Hg, or 14.696 psi.
// 1 atm
#define SEALEVELPRESSURE_HPA (1013.25f)

typedef int32_t  BME280_S32_t;
typedef uint32_t BME280_U32_t;
typedef long long int BME280_S64_t;

typedef uint32_t (*write_i2c_t)(uint8_t addr, uint8_t reg, uint8_t buffer[], uint16_t cnt, uint32_t timeout);
typedef uint32_t (*read_i2c_t)(uint8_t addr, uint8_t reg, uint8_t buffer[], uint16_t cnt, uint32_t timeout);

typedef enum {
    BME280_REGISTER_DIG_T1            = 0x88,
    BME280_REGISTER_DIG_T2            = 0x8A,
    BME280_REGISTER_DIG_T3            = 0x8C,

    BME280_REGISTER_DIG_P1            = 0x8E,
    BME280_REGISTER_DIG_P2            = 0x90,
    BME280_REGISTER_DIG_P3            = 0x92,
    BME280_REGISTER_DIG_P4            = 0x94,
    BME280_REGISTER_DIG_P5            = 0x96,
    BME280_REGISTER_DIG_P6            = 0x98,
    BME280_REGISTER_DIG_P7            = 0x9A,
    BME280_REGISTER_DIG_P8            = 0x9C,
    BME280_REGISTER_DIG_P9            = 0x9E,

    BME280_REGISTER_DIG_H1            = 0xA1,

    BME280_REGISTER_CHIPID            = 0xD0,

    BME280_REGISTER_SOFTRESET         = 0xE0,  // Writing 0xB6 does a POR procedure

    BME280_REGISTER_CAL26             = 0xE1,  // R calibration stored in 0xE1-0xF0
	BME280_REGISTER_CAL41             = 0xF0,

    BME280_REGISTER_DIG_H2            = 0xE1,
    BME280_REGISTER_DIG_H3            = 0xE3,
    BME280_REGISTER_DIG_H4            = 0xE4,
    BME280_REGISTER_DIG_H5            = 0xE5,
    BME280_REGISTER_DIG_H6            = 0xE7,

    BME280_REGISTER_STATUS            = 0xF3,
	BME280_REGISTER_CONTROLHUMID      = 0xF2, // Changes to this register only take effect after a write to BME280_REGISTER_CONTROLMEAS
    BME280_REGISTER_CONTROLMEAS       = 0xF4,
    BME280_REGISTER_CONFIG            = 0xF5,

    BME280_REGISTER_PRESSUREDATA_MSB  = 0xF7, // Pressure[19:12]
	BME280_REGISTER_PRESSUREDATA_LSB  = 0xF8, // Pressure[11:4]
	BME280_REGISTER_PRESSUREDATA_XLSB = 0xF9, // Pressure[3:0], only use bits 7,6,5,4

    BME280_REGISTER_TEMPDATA_MSB      = 0xFA, // Temp[19:12]
	BME280_REGISTER_TEMPDATA_LSB      = 0xFB, // Temp[11:4]
	BME280_REGISTER_TEMPDATA_XLSB     = 0xFC, // Temp[3:0], only use bits 7,6,5,4

    BME280_REGISTER_HUMIDDATA_MSB     = 0xFD, // Humidity[15:8]
	BME280_REGISTER_HUMIDDATA_LSB     = 0xFE  // Humidity[7:0]
} BME280_Register;

/*
Register Address    Register content           Data type
0x88 / 0x89        dig_T1 [7:0] / [15:8]      unsigned short
0x8A / 0x8B        dig_T2 [7:0] / [15:8]      signed short
0x8C / 0x8D        dig_T3 [7:0] / [15:8]      signed short
0x8E / 0x8F        dig_P1 [7:0] / [15:8]      unsigned short
0x90 / 0x91        dig_P2 [7:0] / [15:8]      signed short
0x92 / 0x93        dig_P3 [7:0] / [15:8]      signed short
0x94 / 0x95        dig_P4 [7:0] / [15:8]      signed short
0x96 / 0x97        dig_P5 [7:0] / [15:8]      signed short
0x98 / 0x99        dig_P6 [7:0] / [15:8]      signed short
0x9A / 0x9B        dig_P7 [7:0] / [15:8]      signed short
0x9C / 0x9D        dig_P8 [7:0] / [15:8]      signed short
0x9E / 0x9F        dig_P9 [7:0] / [15:8]      signed short
0xA1               dig_H1 [7:0]               unsigned char
0xE1 / 0xE2        dig_H2 [7:0] / [15:8]      signed short
0xE3               dig_H3 [7:0]               unsigned char
0xE4 / 0xE5[3:0]   dig_H4 [11:4] / [3:0]      signed short
0xE5[7:4] / 0xE6   dig_H5 [3:0] / [11:4]      signed short
0xE7               dig_H6                     signed char
*/
typedef struct
{
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;

    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;

} sBME280_calib_data;

// Pressure Temperature and Humidity Oversampling settings
// 000 (0) - Skipped (output set to 0x8000)
// 001 (1) - oversampling ×1
// 010 (2) - oversampling ×2
// 011 (3) - oversampling ×4
// 100 (4) - oversampling ×8
// 101+ (5+) - oversampling ×16

typedef union {
    struct {
        uint8_t osrs_h   : 3; // Bits [2:0] - Humidity oversampling
        uint8_t reserved : 5; // Bits [7:3] - Reserved
    } bits;
    uint8_t reg;
} BME280_control_humidity_t;

// Pressure and Temperature DAQ options - needs to be written after changing CONTROL_HUMIDITY register
typedef union {
    struct {
        uint8_t mode   : 2; // Bits [1:0] - Sensor mode, 00 - sleep, 01/10 - forced, 11 - normal
        uint8_t osrs_p : 3; // Bits [4:2] - Oversampling of pressure data
        uint8_t osrs_t : 3; // Bits [7:5] - Oversampling of temperature data
    } bits;
    uint8_t reg;
} BME280_control_meas_t;

typedef union {
    struct {
        uint8_t im_update : 1; // Bits [0] - Set to 1 when NVM data is being copied to image regs, 0 - when done (happens every POR and before conversion)
        uint8_t reserved0 : 2; // Bits [2:1] - Reserved
        uint8_t measuring : 1; // Bits [3] - Set to 1 when conversion is running, 0 when results are ready
        uint8_t reserved1 : 4;
    } bits;
    uint8_t reg;
} BME280_status_t;

// IIR Time Constant
// 000 - Filter Off
// 001 - 2
// 010 - 4
// 011 - 8
// 100+ - 16

// Standby Time t_sb - period between measurement readings
// 000 -    0.5ms
// 001 -   62.5ms
// 010 -  125ms
// 011 -  250ms
// 100 -  500ms
// 101 - 1000ms
// 110 -   10ms
// 111 -   20ms

typedef union {
    struct {
        uint8_t spi3w_en  : 1; // Bits [0] - Set 1 for SPI Mode
        uint8_t reserved  : 1;
        uint8_t iirfilter : 3; // Bits [4:2] - IIR Time Constant
        uint8_t t_sb      : 3; // Bits [7:5] - Inactive duration in normal mode
    } bits;
    uint8_t reg;
} BME280_config_t;

typedef struct
{
	uint8_t i2c_addr;
	sBME280_calib_data cal_data;
	uint8_t chipid; // 0x60
	BME280_status_t status;
	BME280_control_humidity_t humidity_cntrl;
	BME280_control_meas_t pressure_temp_cntrl;
	BME280_config_t config;
	BME280_S32_t t_fine;     // fine temperature reading (corrects humidity)

	BME280_S32_t lastTemp;     // last temperature reading
	BME280_U32_t lastHumidity; // last humidity reading
	BME280_U32_t lastPressure; // last pressure reading

	write_i2c_t write_i2c;
	read_i2c_t read_i2c;

} sBME280;


void bme280_Init(sBME280 *bmeData, uint8_t i2c_addr, write_i2c_t func_write_i2c, read_i2c_t func_read_i2c);

int32_t bme280_StartUp(sBME280 *bmeData);

// Reads Status, Chip Config, Humidity, Temperature and Pressure Settings
int32_t bme280_GetStatusConfig(sBME280 *bmeData);

// Set device configuration - reads configuration back into registers
int32_t bme280_SetDeviceConfig(sBME280 *bmeData, BME280_config_t config);

// Set pressure, humidity, temperature measurement settings - reads configuration back into registers
int32_t bme280_SetMeasurementConfig(sBME280 *bmeData, BME280_control_humidity_t cfghumidity, BME280_control_meas_t cfgpresstemp);

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of "5123" equals 51.23 DegC.
int32_t bme280_MeasureTemperature(sBME280 *bmeData, int32_t *temperature);

// Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format (22 integer and 10 fractional bits).
// Output value of "47445" represents 47445/1024 = 46.333 %RH
int32_t bme280_MeasureHumidity(sBME280 *bmeData, uint32_t *humidity);

// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of "24674867" represents 24674867/256 = 96386.2 Pa = 963.862 hPa
// 1 millibar = 1 hPa [hectoPascals]
int32_t bme280_MeasurePressure(sBME280 *bmeData, uint32_t *pressure);

// Tested this with BME680 - works well if you have a floating point
//altitude = 145366.45f*(1.0f - pow( pressure [millibars] / SEALEVELPRESSURE_HPA , 0.190284f)); // in feet https://en.wikipedia.org/wiki/Pressure_altitude
//altitude = 0.3048f * altitude; // feet to meters
#if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1U)
int32_t bme280_MeasureAltitudeInFt(sBME280 *bmeData, float *altFeet);
int32_t bme280_MeasureAltitudeInMeter(sBME280 *bmeData, float *altMeter);
#endif

#ifdef __cplusplus
}
#endif

#endif /* INC_BME280_H_ */
