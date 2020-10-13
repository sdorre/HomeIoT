# ESP32 Node

Based on FreeRTOS Using the Espressif Development toolkit

This snippet provides a lib to read some i2c sensors:
- HDC1080
- SHT20
- BME180

## Notes

- FreeRTOS delay under 10ms does not work by default. This is due to the default TickRATE which is 100Hz. This must be modified with menuconfig.

## TODO
- [ ] Optimize build
- [ ] Optimize Power consumption
