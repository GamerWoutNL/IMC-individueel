#include "twist.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char* TAG = "TWIST";

static const size_t I2C_MASTER_TX_BUF_DISABLE = 0;
static const size_t I2C_MASTER_RX_BUF_DISABLE = 0;
static const int INTR_FLAGS = 0;

twist_err_t twist_init(twist_t *re)
{
    twist_err_t ret;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = re->sda_pin,
        .scl_io_num = re->scl_pin,
        .sda_pullup_en = re->sda_pullup_en,
        .scl_pullup_en = re->scl_pullup_en,
        .master.clk_speed = 20000
    };
    ret = i2c_param_config(re->port, &conf);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "twist config failed");
        return TWIST_ERR_CONFIG;
    }
    ESP_LOGV(TAG, "twist config complete");

    ret = i2c_driver_install(re->port, I2C_MODE_MASTER, I2C_MASTER_TX_BUF_DISABLE, I2C_MASTER_RX_BUF_DISABLE, INTR_FLAGS);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "twist driver install failed");
        return TWIST_ERR_INSTALL;
    }
    ESP_LOGV(TAG, "twist driver installed");
    
    return TWIST_ERR_OK;
}

twist_err_t twist_check_connection(twist_t *re)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (re->i2c_addr << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(re->port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL)
    {
        ESP_LOGE(TAG, "twist unable to write to address");
        return TWIST_ERR_FAIL;
    }

    cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (re->i2c_addr << 1) | READ_BIT, ACK_CHECK_EN);
	ret =i2c_master_cmd_begin(re->port, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if( ret == ESP_FAIL ) {
	   ESP_LOGE(TAG,"twist unable to read from address");
	   return TWIST_ERR_FAIL;
	}

    return TWIST_ERR_OK;
}

twist_err_t twist_connect_color(twist_t *re, int16_t red, int16_t green, int16_t blue)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);

    i2c_master_write_byte(cmd, re->i2c_addr << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, TWIST_CONNECT_RED, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, red >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, red &0xFF, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, green >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, green & 0xFF, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, blue >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, blue & 0xFF, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(re->port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL)
    {
        ESP_LOGE(TAG, "twist unable to write to register");
        return false;
    }
    return true;
}

twist_err_t twist_read_register(twist_t *re, encoderRegisters reg, uint8_t *data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (re->i2c_addr << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, 1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(re->port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL)
    {
        ESP_LOGE(TAG, "twist unable to write to address");
        return TWIST_ERR_FAIL;
    }

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (re->i2c_addr << 1) | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data, 1);
    ret = i2c_master_cmd_begin(re->port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL)
    {
        ESP_LOGE(TAG, "twist unable to read from address");
        return TWIST_ERR_FAIL;
    }
    return TWIST_ERR_OK;
}

twist_err_t twist_read_register16(twist_t *re, encoderRegisters reg, uint16_t *data)
{
    uint8_t LSB = 0x00;
    uint8_t MSB = 0x00;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (re->i2c_addr << 1), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg + 1, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (re->i2c_addr << 1) | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &MSB, 1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(re->port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL)
    {
        return TWIST_ERR_FAIL;
    }
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (re->i2c_addr << 1), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (re->i2c_addr << 1) | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &LSB, 1);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(re->port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL)
    {
        return TWIST_ERR_FAIL;
    }
    uint16_t temp = ((uint16_t)MSB << 8 | LSB);

    *data = temp;

    return TWIST_ERR_OK;
}

bool twist_write_register(twist_t *re, encoderRegisters reg, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
   	i2c_master_start(cmd);
   	i2c_master_write_byte(cmd, re->i2c_addr << 1 | WRITE_BIT, ACK_CHECK_EN);
   	i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
   	i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
   	i2c_master_stop(cmd);
  	 esp_err_t ret = i2c_master_cmd_begin(re->port, cmd, 1000 / portTICK_RATE_MS);
   	i2c_cmd_link_delete(cmd);
   	if (ret == ESP_FAIL) {
      ESP_LOGE(TAG,"twist unable to write to register");
      return false;
   	}
   	return true;
}

bool twist_write_register16(twist_t *re, encoderRegisters reg, uint16_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, re->i2c_addr << 1 | WRITE_BIT, ACK_CHECK_EN);
   	i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
   	i2c_master_write_byte(cmd, data & 0xFF, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, data >> 8, ACK_CHECK_EN);
   	i2c_master_stop(cmd);
   	esp_err_t ret = i2c_master_cmd_begin(re->port, cmd, 1000 / portTICK_RATE_MS);
   	i2c_cmd_link_delete(cmd);
   	if (ret == ESP_FAIL) {
      ESP_LOGE(TAG,"twist unable to write to register");
      return false;
   	}
   	return true;
}

bool twist_write_register32(twist_t *re, encoderRegisters reg, uint32_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, re->i2c_addr << 1 | WRITE_BIT, ACK_CHECK_EN);
   	i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
   	i2c_master_write_byte(cmd, data >> 16, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, data >> 8, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, data & 0xFF, ACK_CHECK_EN);

   	i2c_master_stop(cmd);
   	esp_err_t ret = i2c_master_cmd_begin(re->port, cmd, 1000 / portTICK_RATE_MS);
   	i2c_cmd_link_delete(cmd);
   	if (ret == ESP_FAIL) {
      ESP_LOGE(TAG,"twist unable to write to register");
      return false;
   	}
   	return true;
}