#ifndef HDC1080_H
#define HDC1080_H

#define HDC1080_ADDRESS 0x40

typedef enum
{
    HDC1080_RESOLUTION_14BIT,
    HDC1080_RESOLUTION_11BIT,
    HDC1080_RESOLUTION_8BIT,
} hdc1080_resolution_t;

typedef enum {
	HDC1080_TEMPERATURE		= 0x00,
	HDC1080_HUMIDITY		= 0x01,
	HDC1080_CONFIGURATION	= 0x02,
	HDC1080_MANUFACTURER_ID = 0xFE,
	HDC1080_DEVICE_ID		= 0xFF,
	HDC1080_SERIAL_ID_FIRST	= 0xFB,
	HDC1080_SERIAL_ID_MID	= 0xFC,
	HDC1080_SERIAL_ID_LAST	= 0xFD,
} hdc1080_pointer_t;

typedef union {
	uint8_t rawData[6];
	struct {
		uint16_t serialFirst;
		uint16_t serialMid;
		uint16_t serialLast;
	};
} hdc1080_serial_number_t;

typedef union {
	uint8_t rawData;
	struct {
		uint8_t HumidityMeasurementResolution : 2;
		uint8_t TemperatureMeasurementResolution : 1;
		uint8_t BatteryStatus : 1;
		uint8_t ModeOfAcquisition : 1;
		uint8_t Heater : 1;
		uint8_t ReservedAgain : 1;
		uint8_t SoftwareReset : 1;
	};
} hdc1080_register_t;

void HDC1080_setResolution(hdc1080_resolution_t humidity, hdc1080_resolution_t temperature);

hdc1080_serial_number_t HDC1080_readSerialNumber();
hdc1080_register_t HDC1080_readRegister();
void HDC1080_writeRegister(hdc1080_register_t reg);
void HDC1080_heatUp(uint8_t seconds);

double HDC1080_readTemperature();
double HDC1080_readHumidity();

uint16_t HDC1080_readManufacturerId();
uint16_t HDC1080_readDeviceId();

void task_hdc1080(void *ignore);

#endif //HDC1080_H
