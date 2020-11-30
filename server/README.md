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

## Setup with docker compose 

- `sudo USER_UID=$(id -u) docker-compose up`

### Installation note

- I need to create `/srv/iothome` and the subfolders `grafana` and `influxdb` with the user permissions. 
so : `sudo chown linaro /srv/iothome` and `sudo chown USER /srv/iothome/grafana`

### Usage note

- grafana web interface accessible via : http://<ip-address>:3000
- grafana default user/passwd : admin/admin

## TODO
- [x] Upload first draft
- [x] Dockerfile
- 
