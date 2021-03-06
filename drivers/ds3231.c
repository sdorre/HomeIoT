/* Driver for DS3231 high precision RTC module
 *
 * Part of esp-open-rtos
 * Copyright (C) 2015 Richard A Burton <richardaburton@gmail.com>
 * Copyright (C) 2016 Bhuvanchandra DV <bhuvanchandra.dv@gmail.com>
 * MIT Licensed as described in the file LICENSE
*/

#include "ds3231.h"
#include <driver/i2c.h>
#include <esp_log.h>

static char tag[] = "ds3231";

/* Convert normal decimal to binary coded decimal */
static inline uint8_t  decToBcd(uint8_t dec)
{
    return (dec / 10) * 16 + dec % 10;
}

/* Convert binary coded decimal to normal decimal */
static inline uint8_t  bcdToDec(uint8_t bcd)
{
    return (bcd / 16) * 10 + bcd % 16;
}

/* Send a number of bytes to the rtc over i2c
 * returns true to indicate success
 */
static inline int ds3231_send(uint8_t reg, uint8_t *data, uint8_t len)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (DS3231_ADDR << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
	i2c_master_write_byte(cmd, reg, 1);
	i2c_master_write(cmd, data, len, 1);
	i2c_master_stop(cmd);
	int res = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
    return res;
}

/* Read a number of bytes from the rtc over i2c
 * returns true to indicate success
 */
static inline int ds3231_recv(uint8_t reg, uint8_t *data, uint8_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (DS3231_ADDR << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
	i2c_master_write_byte(cmd, reg, 1);

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (DS3231_ADDR << 1) | I2C_MASTER_READ, 1 /* expect ack */);
	i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
	i2c_master_stop(cmd);
	int res = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
	if (res != ESP_OK) {
	   ESP_LOGI(tag, "READ Error 0x%x", res);
    }
    return res;
}

int ds3231_setTime(struct tm *time)
{
    uint8_t data[7];

    /* time/date data */
    data[0] = decToBcd(time->tm_sec);
    data[1] = decToBcd(time->tm_min);
    data[2] = decToBcd(time->tm_hour);
    /* The week data must be in the range 1 to 7, and to keep the start on the
     * same day as for tm_wday have it start at 1 on Sunday. */
    data[3] = decToBcd(time->tm_wday + 1);
    data[4] = decToBcd(time->tm_mday);
    data[5] = decToBcd(time->tm_mon + 1);
    data[6] = decToBcd(time->tm_year - 100);

    return ds3231_send(DS3231_ADDR_TIME, data, 7);
}

int ds3231_setAlarm(uint8_t alarms, struct tm *time1, uint8_t option1, struct tm *time2, uint8_t option2)
{
    int i = 0;
    uint8_t data[7];

    /* alarm 1 data */
    if (alarms != DS3231_ALARM_2)
    {
        data[i++] = (option1 >= DS3231_ALARM1_MATCH_SEC ? decToBcd(time1->tm_sec) : DS3231_ALARM_NOTSET);
        data[i++] = (option1 >= DS3231_ALARM1_MATCH_SECMIN ? decToBcd(time1->tm_min) : DS3231_ALARM_NOTSET);
        data[i++] = (option1 >= DS3231_ALARM1_MATCH_SECMINHOUR ? decToBcd(time1->tm_hour) : DS3231_ALARM_NOTSET);
        data[i++] = (option1 == DS3231_ALARM1_MATCH_SECMINHOURDAY ? (decToBcd(time1->tm_wday + 1) & DS3231_ALARM_WDAY) :
            (option1 == DS3231_ALARM1_MATCH_SECMINHOURDATE ? decToBcd(time1->tm_mday) : DS3231_ALARM_NOTSET));
    }

    /* alarm 2 data */
    if (alarms != DS3231_ALARM_1)
    {
        data[i++] = (option2 >= DS3231_ALARM2_MATCH_MIN ? decToBcd(time2->tm_min) : DS3231_ALARM_NOTSET);
        data[i++] = (option2 >= DS3231_ALARM2_MATCH_MINHOUR ? decToBcd(time2->tm_hour) : DS3231_ALARM_NOTSET);
        data[i++] = (option2 == DS3231_ALARM2_MATCH_MINHOURDAY ? (decToBcd(time2->tm_wday + 1) & DS3231_ALARM_WDAY) :
            (option2 == DS3231_ALARM2_MATCH_MINHOURDATE ? decToBcd(time2->tm_mday) : DS3231_ALARM_NOTSET));
    }

    return ds3231_send((alarms == DS3231_ALARM_2 ? DS3231_ADDR_ALARM2 : DS3231_ADDR_ALARM1), data, i);
}

/* Get a byte containing just the requested bits
 * pass the register address to read, a mask to apply to the register and
 * an uint* for the output
 * you can test this value directly as true/false for specific bit mask
 * of use a mask of 0xff to just return the whole register byte
 * returns true to indicate success
 */
bool ds3231_getFlag(uint8_t addr, uint8_t mask, uint8_t *flag)
{
    uint8_t data;

    /* get register */
    if (!ds3231_recv(addr, &data, 1))
    {
        /* return only requested flag */
        *flag = (data & mask);
        return true;
    }

    return false;
}

/* Set/clear bits in a byte register, or replace the byte altogether
 * pass the register address to modify, a byte to replace the existing
 * value with or containing the bits to set/clear and one of
 * DS3231_SET/DS3231_CLEAR/DS3231_REPLACE
 * returns true to indicate success
 */
bool ds3231_setFlag(uint8_t addr, uint8_t bits, uint8_t mode)
{
    uint8_t data;

    /* get status register */
    if (!ds3231_recv(addr, &data, 1))
    {
        /* clear the flag */
        if (mode == DS3231_REPLACE)
            data = bits;
        else if (mode == DS3231_SET)
            data |= bits;
        else
            data &= ~bits;

        if (!ds3231_send(addr, &data, 1))
            return true;
    }

    return false;
}

bool ds3231_getOscillatorStopFlag(bool *flag)
{
    uint8_t f;

    if (ds3231_getFlag(DS3231_ADDR_STATUS, DS3231_STAT_OSCILLATOR, &f))
    {
        *flag = (f ? true : false);
        return true;
    }

    return false;
}

inline bool ds3231_clearOscillatorStopFlag()
{
    return ds3231_setFlag(DS3231_ADDR_STATUS, DS3231_STAT_OSCILLATOR, DS3231_CLEAR);
}

inline bool ds3231_getAlarmFlags(uint8_t *alarms)
{
    return ds3231_getFlag(DS3231_ADDR_STATUS, DS3231_ALARM_BOTH, alarms);
}

inline bool ds3231_clearAlarmFlags(uint8_t alarms)
{
    return ds3231_setFlag(DS3231_ADDR_STATUS, alarms, DS3231_CLEAR);
}

inline bool ds3231_enableAlarmInts(uint8_t alarms)
{
    return ds3231_setFlag(DS3231_ADDR_CONTROL, DS3231_CTRL_ALARM_INTS | alarms, DS3231_SET);
}

inline bool ds3231_disableAlarmInts(uint8_t alarms)
{
    /* Just disable specific alarm(s) requested
     * does not disable alarm interrupts generally (which would enable the squarewave)
     */
    return ds3231_setFlag(DS3231_ADDR_CONTROL, alarms, DS3231_CLEAR);
}

inline bool ds3231_enable32khz()
{
    return ds3231_setFlag(DS3231_ADDR_STATUS, DS3231_STAT_32KHZ, DS3231_SET);
}

inline bool ds3231_disable32khz()
{
    return ds3231_setFlag(DS3231_ADDR_STATUS, DS3231_STAT_32KHZ, DS3231_CLEAR);
}

inline bool ds3231_enableSquarewave()
{
    return ds3231_setFlag(DS3231_ADDR_CONTROL, DS3231_CTRL_ALARM_INTS, DS3231_CLEAR);
}

inline bool ds3231_disableSquarewave()
{
    return ds3231_setFlag(DS3231_ADDR_CONTROL, DS3231_CTRL_ALARM_INTS, DS3231_SET);
}

bool ds3231_setSquarewaveFreq(uint8_t freq)
{
    uint8_t flag = 0;

    if (ds3231_getFlag(DS3231_ADDR_CONTROL, 0xff, &flag))
    {
        /* clear current rate */
        flag &= ~DS3231_CTRL_SQWAVE_8192HZ;
        /* set new rate */
        flag |= freq;

        return ds3231_setFlag(DS3231_ADDR_CONTROL, flag, DS3231_REPLACE);
    }
    return false;
}

bool ds3231_getRawTemp(int16_t *temp)
{
    uint8_t data[2];

    data[0] = DS3231_ADDR_TEMP;
    if (!ds3231_recv(DS3231_ADDR_TEMP,data, 2))
    {
        *temp = (int16_t)(int8_t)data[0] << 2 | data[1] >> 6;
        return true;
    }

    return false;
}

bool ds3231_getTempInteger(int8_t *temp)
{
    int16_t tInt;

    if (ds3231_getRawTemp(&tInt)) {
        *temp = tInt >> 2;
        return true;
    }

    return false;
}

bool ds3231_getTempFloat(float *temp)
{
    int16_t tInt;

    if (ds3231_getRawTemp(&tInt)) {
        *temp = tInt * 0.25;
        return true;
    }

    return false;
}

bool ds3231_getTime(struct tm *time)
{
    uint8_t data[7];

    /* read time */
    if (ds3231_recv(DS3231_ADDR_TIME, data, 7))
    {
        return false;
    }

    ESP_LOGI(tag, "READ Time 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
                        data[0], data[1], data[2], data[3], data[4], data[5], data[6]);

    /* convert to unix time structure */
    time->tm_sec = bcdToDec(data[0]);
    time->tm_min = bcdToDec(data[1]);
    if (data[2] & DS3231_12HOUR_FLAG) {
        /* 12H */
        time->tm_hour = bcdToDec(data[2] & DS3231_12HOUR_MASK) - 1;
        /* AM/PM? */
        if (data[2] & DS3231_PM_FLAG) time->tm_hour += 12;
    } else {
        /* 24H */
        time->tm_hour = bcdToDec(data[2]);
    }
    time->tm_wday = bcdToDec(data[3]) - 1;
    time->tm_mday = bcdToDec(data[4]);
    time->tm_mon  = bcdToDec(data[5] & DS3231_MONTH_MASK) - 1;
    time->tm_year = bcdToDec(data[6]) + 100;
    time->tm_isdst = 0;

    // apply a time zone (if you are not using localtime on the rtc or you want to check/apply DST)
    //applyTZ(time);

    return true;

}
