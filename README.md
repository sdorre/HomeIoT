# HomeIoT

This project show the whole setup of a small IoT Project at Home

It includes:
- a Small server running on Linux and a bunch of dockerized app
- two (at the moment) esp32 nodes that reports data to this server

## Server

[link](server)


## ESP32 node

[link](esp32Node)

## ESP32 Garden node

[link](gardenNode)

As the garden is far from home. This platform will record data using simple memory.
- over EEPROM during the day for each new measurement
- export in a CSV file once the EEPROM is full (around once a day).

Based on the Olimex ESP32 EVB. This board has battery, charger sd card capabilities.


## TODO
- [x] README to all subfolder with link each other
- [x] Grouping all docker container into one dockerfile
- [ ] ESP32 Firmware improvement
- [ ] Garden Firmware improvement
- [ ] Wireless Capabilities to Garden node? Maybe LoraWAN?
- [ ] Or connect to Mobile phone as hotpost and upload when I am around (Wifi detection? press button?)
- [ ] Reduce Garden board power consumption (around 50ma while deep sleep!)
