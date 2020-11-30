/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_sleep.h>

#include <hdc1080.h>
#include <sht20.h>
#include <ads1115.h>
#include <ds3231.h>
#include <at24c32.h>

#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"

static char tag[] = "GardenNode";

static float int_temp_data = 0;
static float int_humid_data = 0;
static float ext_temp_data = 0;
static float ext_humid_data = 0;

static uint8_t restart_counter = 0;

static uint8_t buffer[4096];

static void sd_card_init_and_write()
{
    ESP_LOGI(tag, "Initializing SD card");

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:
    host.flags = SDMMC_HOST_FLAG_1BIT;
    host.max_freq_khz = SDMMC_FREQ_PROBING;

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and formatted
    // in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.
    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(tag, "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(tag, "Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", ret);
        }
        return;
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(tag, "Opening file");
    FILE* f = fopen("/sdcard/garden.csv", "a");
    if (f == NULL) {
        ESP_LOGE(tag, "Failed to open file for writing");
        return;
    }

    at24c32_read(0, buffer, 4096);

    for (int i=0; i<128; i+=2) {
        fprintf(f, "%s%s\n", (char*)&buffer[i * 32],(char*)&buffer[i * 32 + 1]);
    }

    fclose(f);
    ESP_LOGI(tag, "File written");

    // All done, unmount partition and disable SDMMC host peripheral
    esp_vfs_fat_sdmmc_unmount();
    ESP_LOGI(tag, "Card unmounted");    
}

void app_main()
{
    esp_log_level_set("*", ESP_LOG_INFO);

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = 21; // 13;		// could use 16
    conf.scl_io_num = 22; // 16;		// could use 17
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000;
    i2c_param_config(I2C_NUM_0, &conf);

    ESP_LOGI(tag, "configured");
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    ESP_LOGI(tag, "installed");

    nvs_handle_t nvs_handle;
    ret = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(tag, "Error (%s) opening NVS handle!", esp_err_to_name(ret));
    } else {
        ESP_LOGD(tag, "Done");

        // Read
        ESP_LOGI(tag, "Reading restart counter from NVS ... ");
        ret = nvs_get_u8(nvs_handle, "restart_counter", &restart_counter);
        switch (ret) {
            case ESP_OK:
                ESP_LOGI(tag, "Done\n");
                ESP_LOGI(tag, "Restart counter = %d\n", restart_counter);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGI(tag, "The value is not initialized yet!\n");
                break;
            default :
                ESP_LOGI(tag, "Error (%s) reading!\n", esp_err_to_name(ret));
        }

        // Write
        ESP_LOGI(tag, "Updating restart counter in NVS ... ");
        if (restart_counter >= 128) {
            restart_counter = 0;
            sd_card_init_and_write();
        }

        ret = nvs_set_u8(nvs_handle, "restart_counter", restart_counter + 2);

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        ESP_LOGI(tag, "Committing updates in NVS ... ");
        ret = nvs_commit(nvs_handle);
        // ESP_LOGI(tag, (err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(nvs_handle);
    }

    // ADS1115_readRegister();

    // ads1115_register_t reg = {0x8385};
    // ESP_LOGI(tag, "GET ADS Config 0x%x", reg.rawData);
    // ESP_LOGI(tag, "ComparatorQueueAndDisable 0x%x", reg.ComparatorQueueAndDisable);
    // ESP_LOGI(tag, "InputMultiplexerConfig 0x%x", reg.InputMultiplexerConfig);
    // ESP_LOGI(tag, "DataRate 0x%x", reg.DataRate);

    /* Output human-readable time */
    struct tm tmp;
    static char data_buf[32];

    /* If 'now' is not valid */
    tmp.tm_year = 120;
    tmp.tm_mon = 11;
    tmp.tm_mday = 1;
    tmp.tm_hour = 19;
    tmp.tm_min = 56;

    // ds3231_setTime(&tmp);

    ds3231_getTime(&tmp);
    time_t t_now = mktime(&tmp);

    float rtc_temp;
    ds3231_getTempFloat(&rtc_temp);
    ESP_LOGI(tag, "RTC Temperature %f", rtc_temp);

    int_temp_data = HDC1080_readTemperature();
    ESP_LOGI(tag, "Temperature %f", int_temp_data);

    int_humid_data = HDC1080_readHumidity();
    ESP_LOGI(tag, "Humidity %f", int_humid_data);

    ext_temp_data = SHT20_readTemperature();
    ESP_LOGI(tag, "Ext Temperature %f", ext_temp_data);

    ext_humid_data = SHT20_readHumidity();
    ESP_LOGI(tag, "Ext Himidity %f", ext_humid_data);

    uint16_t write_address = restart_counter * 32;
    snprintf(data_buf,sizeof(data_buf),"%ld;%.2f;%.2f;%.2f;", t_now, int_temp_data, int_humid_data, ext_temp_data);
    ESP_LOGI(tag, "data1: %s", data_buf);
    at24c32_write_page(write_address, (uint8_t*)data_buf, sizeof(data_buf));

    write_address += 32;
    memset(data_buf, 0xff, sizeof(data_buf));
    snprintf(data_buf,sizeof(data_buf),"%.2f", ext_humid_data);
    ESP_LOGI(tag, "data2: %s", data_buf);
    at24c32_write_page(write_address, (uint8_t*)data_buf, sizeof(data_buf));

    ESP_LOGI(tag, "Done %d times", restart_counter);

    esp_sleep_enable_timer_wakeup(15 * 60 * 1000000); // 15min
    ESP_LOGI(tag, "Let's go to sleep");
    esp_deep_sleep_start();
}