/*
Arduino Library for Texas Instruments HDC1080 Digital Humidity and Temperature Sensor
Written by AA for ClosedCube
---
The MIT License (MIT)
Copyright (c) 2016-2017 ClosedCube Limited
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <driver/i2c.h>
#include <esp_log.h>
#include "freertos/task.h"

#include "hdc1080.h"

static char tag[] = "hdc1080";

uint16_t HDC1080_readData(uint8_t reg);

void HDC1080_setResolution(hdc1080_resolution_t humidity, hdc1080_resolution_t temperature) {
	hdc1080_register_t reg;
	reg.HumidityMeasurementResolution = 0;
	reg.TemperatureMeasurementResolution = 0;

	if (temperature == HDC1080_RESOLUTION_11BIT)
		reg.TemperatureMeasurementResolution = 0x01;

	switch (humidity)
	{
		case HDC1080_RESOLUTION_8BIT:
			reg.HumidityMeasurementResolution = 0x02;
			break;
		case HDC1080_RESOLUTION_11BIT:
			reg.HumidityMeasurementResolution = 0x01;
			break;
		default:
			break;
	}

	HDC1080_writeRegister(reg);
}

hdc1080_serial_number_t HDC1080_readSerialNumber() {
	hdc1080_serial_number_t sernum;
	sernum.serialFirst = HDC1080_readData(HDC1080_SERIAL_ID_FIRST);
	sernum.serialMid = HDC1080_readData(HDC1080_SERIAL_ID_MID);
	sernum.serialLast = HDC1080_readData(HDC1080_SERIAL_ID_LAST);
	return sernum;
}

hdc1080_register_t HDC1080_readRegister() {
	hdc1080_register_t reg;
	reg.rawData = (HDC1080_readData(HDC1080_CONFIGURATION) >> 8);
	return reg;
}

void HDC1080_writeRegister(hdc1080_register_t reg) {
	ESP_LOGI(tag, "Writing register");

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (HDC1080_ADDRESS << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
	i2c_master_write_byte(cmd, HDC1080_CONFIGURATION, 1);
	i2c_master_write_byte(cmd, reg.rawData, 1);
	i2c_master_write_byte(cmd, 0x0, 1);
	i2c_master_stop(cmd);
	int res = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
}

void HDC1080_heatUp(uint8_t seconds) {
	hdc1080_register_t reg = HDC1080_readRegister();
	reg.Heater = 1;
	reg.ModeOfAcquisition = 1;
	HDC1080_writeRegister(reg);

	// uint8_t buf[4];
	// for (int i = 1; i < (seconds*66); i++) {
	// 	Wire.beginTransmission(_address);
	// 	Wire.write(0x00);
	// 	Wire.endTransmission();
	// 	delay(20);
	// 	Wire.requestFrom(_address, (uint8_t)4);
	// 	Wire.readBytes(buf, (size_t)4);
	// }
	reg.Heater = 0;
	reg.ModeOfAcquisition = 0;
	HDC1080_writeRegister(reg);
}



double HDC1080_readTemperature() {
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (HDC1080_ADDRESS << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
	i2c_master_write_byte(cmd, HDC1080_TEMPERATURE, 1);
	i2c_master_stop(cmd);
	int res = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	vTaskDelay(15/portTICK_PERIOD_MS);

	uint16_t rawT;
	uint8_t msb;
	uint8_t lsb;

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (HDC1080_ADDRESS << 1) | I2C_MASTER_READ, 1 /* expect ack */);
	i2c_master_read_byte(cmd, &msb, 0);
	i2c_master_read_byte(cmd, &lsb, 1);
	i2c_master_stop(cmd);
	res = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
	if (res != ESP_OK) {
	   ESP_LOGW(tag, "READ Error 0x%x", res);
	}

	ESP_LOGD(tag, "msb 0x%x lsb 0x%x", msb, lsb);

	rawT = (uint16_t)((msb << 8) | lsb);
	return (rawT / 65536.0) * 165.0 - 40.0;
}

double HDC1080_readHumidity() {
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (HDC1080_ADDRESS << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
	i2c_master_write_byte(cmd, HDC1080_HUMIDITY, 1);
	i2c_master_stop(cmd);
	int res = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	vTaskDelay(pdMS_TO_TICKS(10));

	uint8_t msb;
	uint8_t lsb;
	uint16_t rawH;

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (HDC1080_ADDRESS << 1) | I2C_MASTER_READ, 1 /* expect ack */);
	i2c_master_read_byte(cmd, &msb, 0);
	i2c_master_read_byte(cmd, &lsb, 1);
	i2c_master_stop(cmd);
	res = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
	if (res != ESP_OK) {
	   ESP_LOGW(tag, "READ Error 0x%x", res);
	}

	ESP_LOGD(tag, "msb 0x%x lsb 0x%x", msb, lsb);

	rawH = (uint16_t)((msb << 8) | lsb);
	return (rawH / 65536.0) * 100.0;
}

uint16_t HDC1080_readManufacturerId() {
	return HDC1080_readData(HDC1080_MANUFACTURER_ID);
}

uint16_t HDC1080_readDeviceId() {
	return HDC1080_readData(HDC1080_DEVICE_ID);
}

uint16_t HDC1080_readData(uint8_t reg) {
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (HDC1080_ADDRESS << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
	i2c_master_write_byte(cmd, reg, 1);
	// i2c_master_stop(cmd);
	// int res = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	// i2c_cmd_link_delete(cmd);
	// if (res != ESP_OK) {
	//    // ESP_LOGI(tag, "CMD Error 0x%x", res);
   // }

	uint8_t msb;
	uint8_t lsb;
	// cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (HDC1080_ADDRESS << 1) | I2C_MASTER_READ, 1 /* expect ack */);
	i2c_master_read_byte(cmd, &msb, 0);
	i2c_master_read_byte(cmd, &lsb, 1);
	i2c_master_stop(cmd);
	int res = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
	if (res != ESP_OK) {
	   // ESP_LOGI(tag, "READ Error 0x%x", res);
    }

	ESP_LOGD(tag, "msb 0x%x lsb 0x%x", msb, lsb);

	short ret = (short)((msb << 8) | lsb);
	return ret;
}

void task_hdc1080(void *ignore) {
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = 21;
	conf.scl_io_num = 22;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = 400000;
	i2c_param_config(I2C_NUM_0, &conf);

	ESP_LOGI(tag, "configured");
	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
	ESP_LOGI(tag, "installed");

	uint16_t manufacturerID = HDC1080_readManufacturerId();
	uint16_t deviceID = HDC1080_readDeviceId();

	ESP_LOGI(tag, "Manufacturer 0x%x, Device 0x%x", manufacturerID,  deviceID);

	hdc1080_register_t reg = HDC1080_readRegister();

	ESP_LOGI(tag, "Register Before 0x%x mode of acquisition %d tres %d", reg.rawData, reg.ModeOfAcquisition, reg.TemperatureMeasurementResolution);
	reg.ModeOfAcquisition = 0;
	reg.TemperatureMeasurementResolution = HDC1080_RESOLUTION_14BIT;
	HDC1080_writeRegister(reg);

	ESP_LOGI(tag, "Register After 0x%x", reg.rawData);

	reg = HDC1080_readRegister();
	ESP_LOGI(tag, "Register After read 0x%x", reg.rawData);



	double data = 0;

	while(1) {
		data = HDC1080_readTemperature();
		ESP_LOGI(tag, "Temperature %f", data);
		vTaskDelay(250/portTICK_PERIOD_MS);
		data = HDC1080_readHumidity();
		ESP_LOGI(tag, "Himidity %f", data);
		vTaskDelay(5000/portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}
