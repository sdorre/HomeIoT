#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sdkconfig.h"


typedef enum
{
  BMP085_MODE_ULTRALOWPOWER          = 0,
  BMP085_MODE_STANDARD               = 1,
  BMP085_MODE_HIGHRES                = 2,
  BMP085_MODE_ULTRAHIGHRES           = 3
} bmp085_mode_t;

uint32_t readUncompensatedTemp();
uint32_t readUncompensatedPressure(uint32_t mode);

double centigrade_to_fahrenheit(double centigrade);
double pascals_to_inHg(double pressure);

void x();



void task_bmp180(void *ignore);
