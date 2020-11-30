# ESP32 Garden Node

Based on FreeRTOS Using the Espressif Development toolkit
The Board is a ESP32 EVM from Olimex.

This snippet provides a lib to read some i2c sensors:
- HDC1080
- SHT20
- ADS1115

a Hall current sensor (ACS712) will be plugged on the ADC to measure current consumption of the garden.

## Notes

- FreeRTOS delay under 10ms does not work by default. This is due to the default TickRATE which is 100Hz. This must be modified with menuconfig.
- What should be the power scenario of this project ?
	- powered on battery ? (is it possible ?)
	- 5v coming from the cabin (or even Solar Panel ?)
- Presence detection with interrupt ? 
	- When Door opening ?
	- When current is flowing ? We could plug the AC-DC 5V on the line, when board is powered up => something is using the grid ?
	  But how to deal with a always on power then ?

## TODO
- [ ] Optimize build
- [ ] Optimize Power consumption
- [ ] Read ADC values over i2c
- [ ] Read current sensor value and compute consumption
