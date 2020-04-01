#ifndef WIFI_H
#define WIFI_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_peripherals.h"
#include "periph_wifi.h"

extern esp_periph_set_handle_t set;

void wifi_init(char* SSID, char* password, char* identity);

#endif