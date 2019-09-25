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

#include "sht20.h"

static char tag[] = "sht20";

void SHT20_setResolution(sht20_resolution_t humidity, sht20_resolution_t temperature)
{

}

sht20_register_t SHT20_readRegister()
{
	sht20_register_t reg = { 0 };
	int res = i2c_master_read_slave_reg(I2C_NUM_0, SHT20_ADDRESS, SHT20_REGISTER_READ, &reg, sizeof(reg));
	if (res != ESP_OK) {
	   ESP_LOGW(tag, "READ Error 0x%x", res);
    }
	return reg;
}

void SHT20_writeRegister(sht20_register_t reg)
{
	i2c_master_write_slave_reg(I2C_NUM_0, SHT20_ADDRESS, SHT20_REGISTER_WRITE, &reg, sizeof(reg));
}

void SHT20_softReset(uint8_t seconds)
{
	i2c_master_write_slave_reg(I2C_NUM_0, SHT20_ADDRESS, SHT20_SOFT_RESET, NULL, 0);
}

double SHT20_readTemperature()
{
	i2c_master_write_slave_reg(I2C_NUM_0, SHT20_ADDRESS, SHT20_TEMPERATURE_NOHOLD, NULL, 0);

	vTaskDelay(90/portTICK_PERIOD_MS);

	uint16_t rawT;
	uint8_t data[2];
	uint8_t csum;

	int res = i2c_master_read_slave(I2C_NUM_0, SHT20_ADDRESS, data, sizeof(data));

	ESP_LOGD(tag, "msb 0x%x lsb 0x%x", data[0], data[1]);

	rawT = (uint16_t)((data[0] << 8) | (data[1] & 0xFC)); // set the 2 last bits of lsb to 0
	return (rawT / 65536.0) * 175.72 - 46.85;
}

double SHT20_readHumidity()
{
	i2c_master_write_slave_reg(I2C_NUM_0, SHT20_ADDRESS, SHT20_HUMIDITY_NOHOLD, NULL, 0);

	vTaskDelay(90/portTICK_PERIOD_MS);

	uint16_t rawT;
	uint8_t data[2];
	uint8_t csum;

	int res = i2c_master_read_slave(I2C_NUM_0, SHT20_ADDRESS, data, sizeof(data));

	ESP_LOGD(tag, "msb 0x%x lsb 0x%x", data[0], data[1]);

	rawT = (uint16_t)((data[0] << 8) | (data[1] & 0xFC)); // set the 2 last bits of lsb to 0
	return (rawT / 65536.0) * 125.0 - 6;
}

void task_sht20(void *ignore)
{
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = 21;		//could use 16
	conf.scl_io_num = 22;		// could use 17
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = 400000;
	i2c_param_config(I2C_NUM_0, &conf);

	ESP_LOGI(tag, "configured");
	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
	ESP_LOGI(tag, "installed");

	sht20_register_t reg = SHT20_readRegister();

	ESP_LOGI(tag, "Register Before 0x%x", reg.rawData);

	SHT20_writeRegister(reg);

	// reg = HDC1080_readRegister();
	// ESP_LOGI(tag, "Register After read 0x%x", reg.rawData);

	double data = 0;

	while(1) {
		data = SHT20_readTemperature();
		ESP_LOGI(tag, "Temperature %f", data);
		vTaskDelay(250/portTICK_PERIOD_MS);
		data = SHT20_readHumidity();
		ESP_LOGI(tag, "Himidity %f", data);
		vTaskDelay(1000/portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}
