/*
 * bme280.c
 *
 *  Created on: Dec 30, 2025
 *      Author: Michael Margolese
 */

#include <stddef.h>
#include <string.h>
#include <math.h>
//#include <stm32l4xx_hal.h>
#include <stm32c0xx_hal.h>
#include "bme280.h"

void bme280_Init(sBME280 *bmeData, uint8_t i2c_addr, write_i2c_t func_write_i2c, read_i2c_t func_read_i2c)
{
	assert_param(bmeData != NULL);
	assert_param(func_write_i2c != NULL);
	assert_param(func_read_i2c != NULL);
	memset(bmeData, 0, sizeof(sBME280));

	bmeData->i2c_addr  = i2c_addr;
	bmeData->read_i2c  = func_read_i2c;
	bmeData->write_i2c = func_write_i2c;
}

//uint32_t write_i2c(uint8_t addr, uint8_t reg, uint8_t buffer[], uint16_t cnt, uint32_t timeout);
//uint32_t read_i2c(uint8_t addr, uint8_t reg, uint8_t buffer[], uint16_t cnt, uint32_t timeout);

int32_t bme280_StartUp(sBME280 *bmeData)
{
	assert_param(bmeData != NULL);
	assert_param(bmeData->write_i2c != NULL);
	assert_param(bmeData->read_i2c != NULL);

	uint8_t buffer[3] = {0,0,0};
	int32_t error = 0;

	// Soft Reset
	buffer[0] = 0xB6;
	error = bmeData->write_i2c(bmeData->i2c_addr, BME280_REGISTER_SOFTRESET, buffer, 1, 100);
	if (error != 0) return -1*error;
	HAL_Delay(3); // wait for chip to POR

	// Read Chip ID
	buffer[0] = 0;
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_CHIPID, buffer, 1, 100);
	if (error != 0) return -1*error;
	if (buffer[0] != 0x60) return -100; // Invalid Chip ID
	bmeData->chipid = buffer[0];

	// Read Status
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_STATUS, buffer, 1, 100);
	if (error != 0) return -1*error;
	bmeData->status = (BME280_status_t) buffer[0];

	// Read Configuration
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_CONFIG, buffer, 1, 100);
	if (error != 0) return -1*error;
	bmeData->config = (BME280_config_t) buffer[0];

	// Read Humidity Configuration
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_CONTROLHUMID, buffer, 1, 100);
	if (error != 0) return -1*error;
	bmeData->humidity_cntrl = (BME280_control_humidity_t) buffer[0];

	// Read Temp/Pressure Configuration
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_CONTROLMEAS, buffer, 1, 100);
	if (error != 0) return -1*error;
	bmeData->pressure_temp_cntrl = (BME280_control_meas_t) buffer[0];

	// Read in 0x8000 or 0x800000 into readings
	// Read Humidity
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_HUMIDDATA_MSB, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->lastHumidity = (BME280_U32_t) ((buffer[0] << 8) | buffer[1]);

	// Read Temperature
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_TEMPDATA_MSB, buffer, 3, 100);
	if (error != 0) return -1*error;
	bmeData->lastTemp = (BME280_S32_t) ((buffer[0] << 16) | (buffer[1] << 8) | buffer[2]);

	// Read Pressure
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_PRESSUREDATA_MSB, buffer, 3, 100);
	if (error != 0) return -1*error;
	bmeData->lastPressure = (BME280_U32_t) ((buffer[0] << 16) | (buffer[1] << 8) | buffer[2]);

	// Read Calibration Data

	// Read T1, T2, T3
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_T1, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_T1 = (uint16_t) ((buffer[1] << 8) | buffer[0]);

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_T2, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_T2 = (int16_t) ((buffer[1] << 8) | buffer[0]);

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_T3, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_T3 = (int16_t) ((buffer[1] << 8) | buffer[0]);

	// Read P1 - P9
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_P1, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_P1 = (uint16_t) ((buffer[1] << 8) | buffer[0]);

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_P2, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_P2 = (int16_t) ((buffer[1] << 8) | buffer[0]);

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_P3, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_P3 = (int16_t) ((buffer[1] << 8) | buffer[0]);

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_P4, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_P4 = (int16_t) ((buffer[1] << 8) | buffer[0]);

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_P5, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_P5 = (int16_t) ((buffer[1] << 8) | buffer[0]);

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_P6, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_P6 = (int16_t) ((buffer[1] << 8) | buffer[0]);

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_P7, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_P7 = (int16_t) ((buffer[1] << 8) | buffer[0]);

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_P8, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_P8 = (int16_t) ((buffer[1] << 8) | buffer[0]);

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_P9, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_P9 = (int16_t) ((buffer[1] << 8) | buffer[0]);

	// Read H1, H2, H3
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_H1, buffer, 1, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_H1 = (uint8_t) buffer[0];

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_H2, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_H2 = (int16_t) ((buffer[1] << 8) | buffer[0]);

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_H3, buffer, 1, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_H3 = (uint8_t) buffer[0];

	// Read H4, H5, H6
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_H4, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_H4 = (int16_t) ((buffer[0] << 4) | (buffer[1] & 0xF));

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_H5, buffer, 2, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_H5 = (int16_t) ((buffer[1] << 4) | (buffer[0] & 0xF));

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_DIG_H6, buffer, 1, 100);
	if (error != 0) return -1*error;
	bmeData->cal_data.dig_H6 = (uint8_t) buffer[0];

	bmeData->t_fine = 0;

	return error;
}

int32_t bme280_GetStatusConfig(sBME280 *bmeData)
{
	assert_param(bmeData != NULL);
	assert_param(bmeData->write_i2c != NULL);
	assert_param(bmeData->read_i2c != NULL);

	uint8_t buffer[1] = {0};
	int32_t error = 0;

	// Read Status
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_STATUS, buffer, 1, 100);
	if (error != 0) return -1*error;
	bmeData->status = (BME280_status_t) buffer[0];

	// Read Configuration
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_CONFIG, buffer, 1, 100);
	if (error != 0) return -1*error;
	bmeData->config = (BME280_config_t) buffer[0];

	// Read Humidity Configuration
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_CONTROLHUMID, buffer, 1, 100);
	if (error != 0) return -1*error;
	bmeData->humidity_cntrl = (BME280_control_humidity_t) buffer[0];

	// Read Temp/Pressure Configuration
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_CONTROLMEAS, buffer, 1, 100);
	if (error != 0) return -1*error;
	bmeData->pressure_temp_cntrl = (BME280_control_meas_t) buffer[0];

	return error;
}

int32_t bme280_SetDeviceConfig(sBME280 *bmeData, BME280_config_t config)
{
	assert_param(bmeData != NULL);
	assert_param(bmeData->write_i2c != NULL);
	assert_param(bmeData->read_i2c != NULL);

	uint8_t buffer[1] = {0};
	int32_t error = 0;

	// Write Configuration
	buffer[0] = config.reg;
	error = bmeData->write_i2c(bmeData->i2c_addr, BME280_REGISTER_CONFIG, buffer, 1, 100);
	if (error != 0) return -1*error;

	// Read Configuration
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_CONFIG, buffer, 1, 100);
	if (error != 0) return -1*error;
	bmeData->config = (BME280_config_t) buffer[0];

	return error;
}

int32_t bme280_SetMeasurementConfig(sBME280 *bmeData, BME280_control_humidity_t cfghumidity, BME280_control_meas_t cfgpresstemp)
{
	assert_param(bmeData != NULL);
	assert_param(bmeData->write_i2c != NULL);
	assert_param(bmeData->read_i2c != NULL);

	uint8_t buffer[1] = {0};
	int32_t error = 0;

	// Write Humidity
	buffer[0] = cfghumidity.reg;
	error = bmeData->write_i2c(bmeData->i2c_addr, BME280_REGISTER_CONTROLHUMID, buffer, 1, 100);
	if (error != 0) return -1*error;

	// Write Pressure Temperature
	buffer[0] = cfgpresstemp.reg;
	error = bmeData->write_i2c(bmeData->i2c_addr, BME280_REGISTER_CONTROLMEAS, buffer, 1, 100);
	if (error != 0) return -1*error;

	// Read Humidity Configuration
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_CONTROLHUMID, buffer, 1, 100);
	if (error != 0) return -1*error;
	bmeData->humidity_cntrl = (BME280_control_humidity_t) buffer[0];

	// Read Temp/Pressure Configuration
	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_CONTROLMEAS, buffer, 1, 100);
	if (error != 0) return -1*error;
	bmeData->pressure_temp_cntrl = (BME280_control_meas_t) buffer[0];

	return error;
}

int32_t bme280_MeasureTemperature(sBME280 *bmeData, int32_t *temperature)
{
	assert_param(bmeData != NULL);
	assert_param(bmeData->write_i2c != NULL);
	assert_param(bmeData->read_i2c != NULL);
	assert_param(temperature != NULL);

	uint8_t buffer[3] = {0,0,0};
	int32_t error = 0;

    int32_t temperature_min = -4000; // -40C min
    int32_t temperature_max = 8500;  //  85C max

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_TEMPDATA_MSB, buffer, 3, 100);
	if (error != 0) return -1*error;

	uint32_t adc_T = (uint32_t)((buffer[0] << 16) | (buffer[1] << 8) | buffer[2]);

	if (adc_T == 0x800000) return -200; // value in case temperature measurement was disabled

    BME280_S32_t var1, var2;

    adc_T >>= 4;

    var1  = ((((adc_T>>3) - ((int32_t)bmeData->cal_data.dig_T1 << 1))) * ((int32_t)bmeData->cal_data.dig_T2)) >> 11;

    var2  = (((((adc_T>>4) - ((int32_t)bmeData->cal_data.dig_T1)) * ((adc_T>>4) - ((int32_t)bmeData->cal_data.dig_T1))) >> 12) * ((int32_t)bmeData->cal_data.dig_T3)) >> 14;

    bmeData->t_fine = var1 + var2;

    *temperature = (bmeData->t_fine * 5 + 128) >> 8;

    if (*temperature < temperature_min)
    {
        *temperature = temperature_min;
    }
    else if (*temperature > temperature_max)
    {
        *temperature = temperature_max;
    }

	bmeData->lastTemp = *temperature;

    return error;
}

int32_t bme280_MeasureHumidity(sBME280 *bmeData, uint32_t *humidity)
{
	assert_param(bmeData != NULL);
	assert_param(bmeData->write_i2c != NULL);
	assert_param(bmeData->read_i2c != NULL);
	assert_param(humidity != NULL);

	uint8_t buffer[2] = {0,0};
	int32_t error = 0;

	int32_t temp;
	bme280_MeasureTemperature(bmeData, &temp); // must read this to get latest fine temperature

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_HUMIDDATA_MSB, buffer, 2, 100);
	if (error != 0) return -1*error;

	uint32_t adc_H = (uint32_t)((buffer[0] << 8) | buffer[1]);

	if (adc_H == 0x8000) return -300; // value in case humidity measurement was disabled

    int32_t v_x1_u32r = (bmeData->t_fine - ((int32_t)76800));

    v_x1_u32r = (((((adc_H << 14) - (((int32_t)bmeData->cal_data.dig_H4) << 20) -
                (((int32_t)bmeData->cal_data.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
                (((((((v_x1_u32r * ((int32_t)bmeData->cal_data.dig_H6)) >> 10) *
                (((v_x1_u32r * ((int32_t)bmeData->cal_data.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                ((int32_t)2097152)) * ((int32_t)bmeData->cal_data.dig_H2) + 8192) >> 14));

    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)bmeData->cal_data.dig_H1)) >> 4));

    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;

    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;

    *humidity = v_x1_u32r >> 12;
	bmeData->lastHumidity = *humidity;

	return error;
}

int32_t bme280_MeasurePressure(sBME280 *bmeData, uint32_t *pressure)
{
	assert_param(bmeData != NULL);
	assert_param(bmeData->write_i2c != NULL);
	assert_param(bmeData->read_i2c != NULL);
	assert_param(pressure != NULL);

	uint8_t buffer[3] = {0,0,0};
	int32_t error = 0;

	int32_t temp;
	bme280_MeasureTemperature(bmeData, &temp); // must read this to get latest fine temperature

	error = bmeData->read_i2c(bmeData->i2c_addr, BME280_REGISTER_PRESSUREDATA_MSB, buffer, 3, 100);
	if (error != 0) return -1*error;

	uint32_t adc_P = (uint32_t)((buffer[0] << 16) | (buffer[1] << 8) | buffer[2]);

	if (adc_P == 0x800000) return -400; // value in case pressure measurement was disabled

    adc_P >>= 4;

    int64_t var1, var2, p;

    var1 = ((int64_t)bmeData->t_fine) - 128000ul;
    var2 = var1 * var1 * (int64_t)bmeData->cal_data.dig_P6;
    var2 = var2 + ((var1*(int64_t)bmeData->cal_data.dig_P5)<<17);
    var2 = var2 + (((int64_t)bmeData->cal_data.dig_P4)<<35);
    var1 = ((var1 * var1 * (int64_t)bmeData->cal_data.dig_P3)>>8) +
    ((var1 * (int64_t)bmeData->cal_data.dig_P2)<<12);
    var1 = (((((int64_t)1)<<47)+var1))*((int64_t)bmeData->cal_data.dig_P1)>>33;

    if (var1 == 0) return -500; // avoid exception caused by division by zero

    p = 1048576ul - adc_P;
    p = (((p<<31) - var2)*3125ul) / var1;
    var1 = (((int64_t)bmeData->cal_data.dig_P9) * (p>>13) * (p>>13)) >> 25;
    var2 = (((int64_t)bmeData->cal_data.dig_P8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (((int64_t)bmeData->cal_data.dig_P7)<<4);

    *pressure = p;
    bmeData->lastPressure = *pressure;

	return error;
}

#if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1U)
//altitude = 145366.45f*(1.0f - pow( pressure [millibars] / SEALEVELPRESSURE_HPA , 0.19029495718363463368220742150333f)); // in feet https://en.wikipedia.org/wiki/Pressure_altitude
//altitude = 0.3048f * altitude; // feet to meters
int32_t bme280_MeasureAltitudeInFt(sBME280 *bmeData, float *altFeet)
{
	assert_param(bmeData != NULL);
	assert_param(bmeData->write_i2c != NULL);
	assert_param(bmeData->read_i2c != NULL);
	assert_param(altFeet != NULL);

	uint32_t pressure;
	int32_t error = bme280_MeasurePressure(bmeData, &pressure);
	if (error != 0) return error;

	float hPa = ((float)(1.0f*pressure) / 256.0f) / 100.0f;
	*altFeet = 145366.45f*(1.0f - pow( hPa / SEALEVELPRESSURE_HPA , 0.19029495718363463368220742150333f));

	return error;
}

//altitude = 44330.0f * (1.0f - pow(atmospheric / SEALEVELPRESSURE_HPA, 0.19029495718363463368220742150333f));
int32_t bme280_MeasureAltitudeInMeter(sBME280 *bmeData, float *altMeter)
{
	assert_param(bmeData != NULL);
	assert_param(bmeData->write_i2c != NULL);
	assert_param(bmeData->read_i2c != NULL);
	assert_param(altMeter != NULL);

	uint32_t pressure;
	int32_t error = bme280_MeasurePressure(bmeData, &pressure);
	if (error != 0) return error;

	float hPa = ((float)(1.0f*pressure) / 256.0f) / 100.0f;
	*altMeter = 44330.0f*(1.0f - pow( hPa / SEALEVELPRESSURE_HPA , 0.19029495718363463368220742150333f));

	return error;
}
#endif

