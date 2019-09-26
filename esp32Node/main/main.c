/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_sleep.h>

// #include "bmp180.h"
#include "hdc1080.h"
// #include "sht20.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "mqtt_client.h"


static char tag[] = "IoTHome";

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

static float int_temp_data = 0;
static float int_humid_data = 0;
static float ext_temp_data = 0;
static float ext_humid_data = 0;

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void wifi_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(tag, "start the WIFI SSID:[%s] password:[%s]", "CONFIG_WIFI_SSID", "******");
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(tag, "Waiting for wifi");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    char msg[128];
    switch (event->event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            break;
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(tag, "MQTT_EVENT_CONNECTED");
            snprintf(msg, sizeof(msg), "{\"int_temperature\":%f,\"int_humidity\":%f,\"ext_temperature\":%f,\"ext_humidity\":%f}",
                    int_temp_data, int_humid_data, ext_temp_data, ext_humid_data);
            int msg_id = esp_mqtt_client_publish(client, CONFIG_EMITTER_CHANNEL_KEY, msg, 0, 0, 0);
            esp_sleep_enable_timer_wakeup(15 * 60 * 1000000); // 15min
            esp_deep_sleep_start();
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(tag, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(tag, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(tag, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(tag, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(tag, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(tag, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}

static void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        // .uri = "mqtts://api.emitter.io:443",    // for mqtt over ssl
        .uri = CONFIG_MQTT_SERVER_URI,
        //.uri = "mqtt://x.x.x.x:1883", //for mqtt over tcp
        // .uri = "ws://api.emitter.io:8080", //for mqtt over websocket
        // .uri = "wss://api.emitter.io:443", //for mqtt over websocket secure
        .event_handle = mqtt_event_handler,
    };

    ESP_LOGI(tag, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void app_main()
{
    ESP_LOGW(tag, "Main");

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    nvs_flash_init();
    wifi_init();

    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = 21;		//could use 16
    conf.scl_io_num = 22;		// could use 17
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000;
    i2c_param_config(I2C_NUM_0, &conf);

    ESP_LOGI(tag, "configured");
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    ESP_LOGI(tag, "installed");

    int_temp_data = HDC1080_readTemperature();
    // float data = SHT20_readTemperature();
    ESP_LOGI(tag, "Temperature %f", int_temp_data);

    int_humid_data = HDC1080_readHumidity();
    // data = SHT20_readHumidity();
    ESP_LOGI(tag, "Himidity %f", int_humid_data);

    // ext_temp_data = SHT20_readTemperature();
    // // float data = SHT20_readTemperature();
    // ESP_LOGI(tag, "Ext Temperature %f", ext_temp_data);
    // ext_humid_data = SHT20_readHumidity();
    // // data = SHT20_readHumidity();
    // ESP_LOGI(tag, "Ext Himidity %f", ext_humid_data);

    mqtt_app_start();

    // esp_sleep_enable_timer_wakeup(10 * 1000000); // 10sec
    // esp_bt_controller_disable();
    // esp_deep_sleep_start();
    // xTaskCreate(&task_bmp180, "blink_task", 2048, NULL, 5, NULL);
    // xTaskCreate(&task_sht20, "blink_task", 2048, NULL, 5, NULL);
}

// TODO Check if DHCP can be disabled
// TODO assign static IP from the router
// TODO optimize sleep sequence (could we disable WIFI ?)
// TODO fetch data without sending them. fetch data 9 times without activating Wifi and MQTT. the 10th time only start sending it.
// TODO
