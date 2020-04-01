#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_peripherals.h"
#include "nvs_flash.h"
#include "periph_wifi.h"
#include <string.h>
#include "wifi.h"

static const char* CONFIG_WIFI_SSID = "TomTostisWifi";
static const char *CONFIG_WIFI_PASSWORD = "tostiszijnlekker";
static const char *CONFIG_WIFI_IDENTITY = ""; // Leave empty if not connected to eduroam
esp_periph_set_handle_t set;

void wifi_init(char *_ssid, char *_password, char *_identity)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    tcpip_adapter_init();
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    set = esp_periph_set_init(&periph_cfg);

    if ((_ssid[0] == '\0') || (_password[0] == '\0'))
    {
        periph_wifi_cfg_t wifi_cfg = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        };
        if (strlen(CONFIG_WIFI_IDENTITY) > 0)
        {
            wifi_cfg.identity = strdup(CONFIG_WIFI_IDENTITY);
        }

        esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);
        esp_periph_start(set, wifi_handle);
        periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);
    }
    else //Otherwise, use given SSID, password and identity
    {
        periph_wifi_cfg_t wifi_cfg = {
            .ssid = _ssid,
            .password = _password,
        };
        if (strlen(_identity) > 0)
        {
            wifi_cfg.identity = strdup(_identity);
        }

        esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);
        esp_periph_start(set, wifi_handle);
        periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);
    }
}
