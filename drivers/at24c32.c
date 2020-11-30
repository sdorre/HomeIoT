/* Driver for DS3231 high precision RTC module
 *
 * Part of esp-open-rtos
 * Copyright (C) 2015 Richard A Burton <richardaburton@gmail.com>
 * Copyright (C) 2016 Bhuvanchandra DV <bhuvanchandra.dv@gmail.com>
 * MIT Licensed as described in the file LICENSE
*/

#include "at24c32.h"
#include <driver/i2c.h>
#include <esp_log.h>

static char tag[] = "at24c32";

bool at24c32_read_byte(uint16_t address, uint8_t data)
{
    if (address > 4095) {
        ESP_LOGE(tag, "Reading off limit !!");
        return false;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (AT24C32_ADDR << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
	i2c_master_write_byte(cmd, address >> 8, 1);
	i2c_master_write_byte(cmd, address & 0xFFu, 1);

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (AT24C32_ADDR << 1) | I2C_MASTER_READ, 1 /* expect ack */);
	i2c_master_read_byte(cmd, &data, 1);
	i2c_master_stop(cmd);
	int res = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
	if (res != ESP_OK) {
	   ESP_LOGI(tag, "READ Error 0x%x", res);
    }
    return res;
}

bool at24c32_read(uint16_t address, uint8_t *data, uint16_t len)
{
    ESP_LOGI(tag, "Read at 0x%x len %d", address, len);
    if (address > 4095) {
        ESP_LOGE(tag, "Reading off limit !!");
        return false;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (AT24C32_ADDR << 1) | I2C_MASTER_WRITE, true /* expect ack */);
	i2c_master_write_byte(cmd, address >> 8, 1);
	i2c_master_write_byte(cmd, address & 0xFFu, 1);

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (AT24C32_ADDR << 1) | I2C_MASTER_READ, true /* expect ack */);
	i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
	i2c_master_stop(cmd);
	int res = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
	if (res != ESP_OK) {
	   ESP_LOGI(tag, "READ Page Error 0x%x", res);
    }
    return res;
}

bool at24c32_write_byte(uint16_t address, uint8_t data)
{
    if (address > 4095) {
        ESP_LOGE(tag, "Writing off limit !!");
        return false;
    }

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (AT24C32_ADDR << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
	i2c_master_write_byte(cmd, address >> 8, 1);
	i2c_master_write_byte(cmd, address & 0xFFu, 1);
	i2c_master_write_byte(cmd, data, 1);
	i2c_master_stop(cmd);
	int res = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
    if (res != ESP_OK) {
	   ESP_LOGI(tag, "Write byte Error 0x%x", res);
    }
    return res;
}

bool at24c32_write_page(uint16_t address, uint8_t *data, uint8_t len)
{
    ESP_LOGI(tag, "Write Page at 0x%x len %d", address, len);
    if (address > 4095) {
        ESP_LOGE(tag, "Writing off limit !!");
        return false;
    }

    if (len > 32) {
        ESP_LOGW(tag, "len(%d) is bigger than a page, data will be overwritten", len);
    }

    // TODO Should we check page alignment ?
    // address % 32 ?
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (AT24C32_ADDR << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
	i2c_master_write_byte(cmd, address >> 8, 1);
	i2c_master_write_byte(cmd, address & 0xFFu, 1);
	i2c_master_write(cmd, data, len, 1);
	i2c_master_stop(cmd);
	int res = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
    if (res != ESP_OK) {
	   ESP_LOGI(tag, "Write Error 0x%x", res);
    }
    return res;
}