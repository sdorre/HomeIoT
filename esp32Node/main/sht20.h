#ifndef SHT20_H
#define SHT20_H

/**
 * Lets not the pinout of the SHT20 once and for all !!
 * BLUE   = GND
 * YELLOW = SCL
 * GREEN  = SDA
 * RED    = 3.3V
 */

#define SHT20_ADDRESS 0x40

typedef enum
{
    SHT20_RESOLUTION_T14BIT_RH12BIT,
    SHT20_RESOLUTION_T13BIT_RH8BIT,
    SHT20_RESOLUTION_T12BIT_RH10BIT,
    SHT20_RESOLUTION_T11BIT_RH11BIT,
} sht20_resolution_t;

typedef enum {
	SHT20_TEMPERATURE_HOLD	 = 0xE3,
	SHT20_HUMIDITY_HOLD		 = 0xE5,
    SHT20_TEMPERATURE_NOHOLD = 0xF3,
    SHT20_HUMIDITY_NOHOLD	 = 0xF5,
    SHT20_REGISTER_READ	     = 0xE6,
	SHT20_REGISTER_WRITE	 = 0xE7,
	SHT20_SOFT_RESET		 = 0xFE,
} sht20_pointer_t;

typedef union {
	uint8_t rawData;
	struct {
		uint8_t MeasurementResolutionMsb : 1;
		uint8_t EndOfBattery : 1;
		uint8_t Reserved : 3;
		uint8_t EnableHeater : 1;
		uint8_t DisableOTPReload : 1;
		uint8_t MeasurementResolutionLsb : 1;
	};
} sht20_register_t;

void SHT20_setResolution(sht20_resolution_t humidity, sht20_resolution_t temperature);

sht20_register_t SHT20_readRegister();
void SHT20_writeRegister(sht20_register_t reg);

void SHT20_heatUp(uint8_t seconds);
void SHT20_softReset(uint8_t seconds);

double SHT20_readTemperature();
double SHT20_readHumidity();

void task_sht20(void *ignore);

#endif //I2C_H
