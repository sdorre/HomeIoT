/* Driver for DS3231 high precision RTC module
 *
 * Part of esp-open-rtos
 * Copyright (C) 2015 Richard A Burton <richardaburton@gmail.com>
 * Copyright (C) 2016 Bhuvanchandra DV <bhuvanchandra.dv@gmail.com>
 * MIT Licensed as described in the file LICENSE
*/

#ifndef __AT24C32_H__
#define __AT24C32_H__

#include <stdint.h>
#include <stdbool.h>


#ifdef	__cplusplus
extern "C" {
#endif

#define AT24C32_ADDR            0x57

bool at24c32_read_byte(uint16_t address, uint8_t data);
bool at24c32_read(uint16_t address, uint8_t *data, uint16_t len);

bool at24c32_write_byte(uint16_t address, uint8_t data);
bool at24c32_write_page(uint16_t address, uint8_t *data, uint8_t len);

#ifdef	__cplusplus
}
#endif

#endif  /* __AT24C32_H__ */
