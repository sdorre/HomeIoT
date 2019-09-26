# Docker Server

Small Server at home gathering all data and showing everything on a Web page

## Setup

#### install a new folder under /srv/iothome

#### copy the conf files in this folder

#### run the following commands

- docker run -d -p 1883:1883 --restart always eclipse-mosquitto
- docker run -d -p 8086:8086 --restart always -v /srv/iothome/influxdb:/var/lib/influxdb influxdb
- docker run -d -p 3000:3000 --restart always --user $(id -u) -v /srv/iothome/grafana:/var/lib/grafana grafana/grafana
- docker run -d --restart always -v /srv/iothome/telegraf.conf:/etc/telegraf/telegraf.conf:ro telegraf


## TODO
- [x] Upload first draft
- [ ] Dockerfile
