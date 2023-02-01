/*
C Library written to control an ADS1115 ADC I2C sensor
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

#include "ads1115.h"

static char tag[] = "ads1115";

void i2c_master_write_slave_reg(i2c_port_t i2c_num, uint8_t adress, uint8_t reg, uint8_t *data, uint8_t data_length)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (adress << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
	i2c_master_write_byte(cmd, reg, 1);

	if ((data != NULL) || (data_length > 0)) {
		while(data_length > 0) {
			i2c_master_write_byte(cmd, *data, 1);
			data++;
			data_length--;
		}
	}

	i2c_master_stop(cmd);
	int res = i2c_master_cmd_begin(i2c_num, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
}

int i2c_master_read_slave(i2c_port_t i2c_num, uint8_t adress, uint8_t *data, uint8_t data_length)
{
	int res;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (adress << 1) | I2C_MASTER_READ, 1 /* expect ack */);
	i2c_master_read_byte(cmd, &data[0], 0);
	i2c_master_read_byte(cmd, &data[1], 1);
	i2c_master_stop(cmd);
	res = i2c_master_cmd_begin(i2c_num, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	return res;
}

ads1115_register_t ADS1115_readRegister()
{
	ads1115_register_t reg = { 0 };
	i2c_master_write_slave_reg(I2C_NUM_0, ADS1115_ADDRESS_ADDR_GND, ADS1115_RA_CONFIG, NULL, 0);
	int res = i2c_master_read_slave(I2C_NUM_0, ADS1115_ADDRESS_ADDR_GND, &reg, sizeof(reg));
	if (res != ESP_OK) {
	   ESP_LOGW(tag, "READ Error 0x%x", res);
    }
	ESP_LOGW(tag, "Reading Registers 0x%x", reg.rawData);

	return reg;
}

void ADS1115_writeRegister(ads1115_register_t reg)
{
	i2c_master_write_slave_reg(I2C_NUM_0, ADS1115_ADDRESS_ADDR_GND, ADS1115_RA_CONFIG, &reg, sizeof(reg));
}

int16_t ADS1115_readConversion()
{
	i2c_master_write_slave_reg(I2C_NUM_0, ADS1115_ADDRESS_ADDR_GND, ADS1115_RA_CONVERSION, NULL, 0);

	vTaskDelay(90/portTICK_PERIOD_MS);

	uint16_t rawT;
	uint8_t data[2];
	uint8_t csum;

	int res = i2c_master_read_slave(I2C_NUM_0, ADS1115_ADDRESS_ADDR_GND, data, sizeof(data));

	ESP_LOGD(tag, "msb 0x%x lsb 0x%x", data[0], data[1]);

	rawT = (uint16_t)((data[0] << 8) | (data[1] & 0xFC)); // set the 2 last bits of lsb to 0
	return (rawT / 65536.0) * 175.72 - 46.85;
}

void task_ads1115(void *ignore)
{
	// i2c_config_t conf;
	// conf.mode = I2C_MODE_MASTER;
	// conf.sda_io_num = 21;		//could use 16
	// conf.scl_io_num = 22;		// could use 17
	// conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	// conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	// conf.master.clk_speed = 400000;
	// i2c_param_config(I2C_NUM_0, &conf);

	// ESP_LOGI(tag, "configured");
	// i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
	// ESP_LOGI(tag, "installed");

	// sht20_register_t reg = SHT20_readRegister();

	// ESP_LOGI(tag, "Register Before 0x%x", reg.rawData);

	// SHT20_writeRegister(reg);

	// // reg = HDC1080_readRegister();
	// // ESP_LOGI(tag, "Register After read 0x%x", reg.rawData);

	// double data = 0;

	// while(1) {
	// 	data = SHT20_readTemperature();
	// 	ESP_LOGI(tag, "Temperature %f", data);
	// 	vTaskDelay(250/portTICK_PERIOD_MS);
	// 	data = SHT20_readHumidity();
	// 	ESP_LOGI(tag, "Himidity %f", data);
	// 	vTaskDelay(1000/portTICK_PERIOD_MS);
	// }
	// vTaskDelete(NULL);
}
