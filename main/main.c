/**
 * Copyright (c) 2020 Waylon Lodder
*/
//General
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "sodium.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
//ESP
#include "esp_system.h"
#include "esp_log.h"
//Timer
#include "sntp_sync.h"
//GPIO expander
#include "smbus.h"
//LCD screen
#include "i2c-lcd1602.h"
//OLED screen
// #include "ssd1306.h"
// #include "ssd1306_draw.h"
// #include "ssd1306_font.h"
// #include "ssd1306_default_if.h"
//Rotary encoder
#include "rotary.h"
#include "twist.h"
//Goertzel
#include "goertzel.h"
#include <math.h>
#include "nvs_flash.h"
//Radio
#include "radio.h"
//Wifi
#include "wifi.h"
//Talking clock
#include "talking_clock.h"
// Menu
#include "menu.h"

//I2C
#define I2C_MASTER_NUM                   I2C_NUM_0
#define I2C_MASTER_TX_BUF_LEN    0                     // disabled
#define I2C_MASTER_RX_BUF_LEN    0                     // disabled
#define I2C_MASTER_FREQ_HZ          20000 //100000
#define I2C_MASTER_SDA_IO              18
#define I2C_MASTER_SCL_IO               23
//LED management
#include "leds.h"
#include <pca9685.h>
//General
#define GENERALTAG "Proftaak_2.3"
#define LCDTAG "LCD Display"
#define OLEDTAG "OLED Display"
#define ENCODERTAG "Rotary Encoder"
#define RADIOTAG "Radio"
#define TALKINGCLOCKTAG "Talking Clock"
#define CLOCKTAG "Clock"
#define GPIOTAG "GPIO"
//LCD screen
#define LCD_Address                             0x27
#define LCD_NUM_ROWS                     4
#define LCD_NUM_COLUMNS              40
#define LCD_NUM_VIS_COLUMNS      20
//SSD1306 OLED
#define SSD1306DisplayAddress           0x3C
#define SSD1306DisplayWidth               128
#define SSD1306DisplayHeight              32
#define SSD1306ResetPin                      -1
//Time variable
char currentTime[16];
//Draw methods for SSD1306
// void SetupDemo(struct SSD1306_Device *DisplayHandle, const struct SSD1306_FontDef *Font);
// void SayHello(struct SSD1306_Device *DisplayHandle, const char *HelloText);
//LCD screen
i2c_lcd1602_info_t *lcd_info;
//Rotary Encoder
twist_t twist;
SemaphoreHandle_t xMutex;
//Clock
TimerHandle_t timer_1_sec;
struct tm timeinfo;
//SSD1306
// struct SSD1306_Device SSD1306Display;

static void i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    //LCD I2C
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE; // GY-2561 provides 10kΩ pullups
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE; // GY-2561 provides 10kΩ pullups
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    //Twist I2C
    twist.i2c_addr = QWIIC_TWIST_ADDR;
    twist.sda_pin = I2C_MASTER_SDA_IO;
    twist.scl_pin = I2C_MASTER_SCL_IO;
    twist.sda_pullup_en = GPIO_PULLUP_ENABLE;
    twist.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_LEN, I2C_MASTER_TX_BUF_LEN, 0);
}

// WARNING: ESP32 does not support blocking input from stdin yet, so this polls
// the UART and effectively hangs up the SDK.
static uint8_t _wait_for_user(void)
{
    uint8_t c = 0;
#ifdef USE_STDIN
    while (!c)
    {
        STATUS s = uart_rx_one_char(&c);
        if (s == OK)
        {
            printf("%c", c);
        }
    }
#else
    vTaskDelay(1000 / portTICK_RATE_MS);
#endif
    return c;
}

//SSD1306 draw methods
// bool DefaultBusInit(void)
// {
//     assert(SSD1306_I2CMasterInitDefault() == true);
//     assert(SSD1306_I2CMasterAttachDisplayDefault(&SSD1306Display, SSD1306DisplayWidth, SSD1306DisplayHeight, SSD1306DisplayAddress, SSD1306ResetPin) == true);
//     return true;
// }

// void SSD1306Setup(struct SSD1306_Device *DisplayHandle, const struct SSD1306_FontDef *Font)
// {
//     SSD1306_Clear(DisplayHandle, SSD_COLOR_BLACK);
//     SSD1306_SetFont(DisplayHandle, Font);
// }

// void SSD1306Draw(struct SSD1306_Device *DisplayHandle, const char *HelloText)
// {
//     SSD1306_FontDrawAnchoredString(DisplayHandle, TextAnchor_Center, HelloText, SSD_COLOR_WHITE);
//     SSD1306_Update(DisplayHandle);
// }
//end SSD1306 draw methods

//Task for OLED display
// void ssd1306_task(void *pvPrameter)
// {
//     if (DefaultBusInit() == true)
//     {
//         SSD1306Setup(&SSD1306Display, &Font_droid_sans_fallback_24x28);
//         SSD1306Draw(&SSD1306Display, "Test");
//     }
//     vTaskDelete(NULL);
// }

//Task for LCD display
void lcd1602_task(void *pvParameter)
{
    xSemaphoreTake(xMutex, portMAX_DELAY);
    //Set up the SMBus
    smbus_info_t *smbus_info = smbus_malloc();
    smbus_init(smbus_info, I2C_MASTER_NUM, LCD_Address);
    smbus_set_timeout(smbus_info, 1000 / portTICK_RATE_MS);

    //Set up the LCD1602 device with backlight off
    lcd_info = i2c_lcd1602_malloc();
    i2c_lcd1602_init(lcd_info, smbus_info, true, LCD_NUM_ROWS, LCD_NUM_COLUMNS, LCD_NUM_VIS_COLUMNS);
    _wait_for_user();
    i2c_lcd1602_set_display(lcd_info, true);
    //Clear the LCD (to make sure no text is displayed)
    i2c_lcd1602_clear(lcd_info);
    //Set the cursor to the first line on the LCD
    i2c_lcd1602_move_cursor(lcd_info, 0, 0);
    i2c_lcd1602_clear(lcd_info);

    //Draw menu
    // i2c_lcd1602_move_cursor(lcd_info, 0, 0);
    // i2c_lcd1602_write_string(lcd_info, "HOOFDMENU");
    // i2c_lcd1602_move_cursor(lcd_info, 0, 1);
    // i2c_lcd1602_write_string(lcd_info, "selectie");
    // i2c_lcd1602_move_cursor(lcd_info, 0, 2);
    // i2c_lcd1602_write_string(lcd_info, "<Radio>AgendaWeer");
    // i2c_lcd1602_move_cursor(lcd_info, 0, 3);
    // i2c_lcd1602_write_string(lcd_info, "");
    menuInit(lcd_info);
    xSemaphoreGive(xMutex);
    vTaskDelete(NULL);
}

//Task for Rotary Encoder
void rotary_task(void *pvParameters)
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 10;
    while (1)
    {
        xLastWakeTime = xTaskGetTickCount();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        xSemaphoreTake(xMutex, portMAX_DELAY);

        bool pressed = rotary_isPressed();
        printf("Pressed: %s\n", (pressed == true) ? "true" : "false");

        twist_encoder_state state = rotary_getEncoderState();

        printf("State: %d\n", state);

        if (pressed == true)
        {
            if (state == 1)
            {
                setLastKeyPress(MENU_RIGHT_PRESS);
                rotary_setColor(0x30, 0x00, 0x30);
            } else
            {
                setLastKeyPress(MENU_LEFT_PRESS);
                rotary_setColor(0x10, 0x00, 0x00);
            }
        }
        else if (state == 1)
        {
            setLastKeyPress(MENU_TURN_LEFT_LEFT);
            rotary_setColor(0x00, 0x10, 0x00);
        }
        else if (state == 2)
        {
            setLastKeyPress(MENU_TURN_LEFT_RIGHT);
            rotary_setColor(0x00, 0x00, 0x10);
        }
        else
        {
            rotary_setColor(0x10, 0x10, 0x10);
        }
        xSemaphoreGive(xMutex);
    }
    vTaskDelete(NULL);
}

void taskMenuTestLoop(void *pvParameters)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));         // delay 1 second
        setLastKeyPress(randombytes_uniform(6)); //generate random key press between 0 and 5. Not every key has an associated menu step!
    }
}

void app_main()
{
    /* LED init */
    i2cdev_init();
    leds_init();
    leds_setFrequency(500);

    /* Test code for the leds_rainbow() function */
    //leds_rainbow();

    /* Test code for the leds_changeState() function */
    // xTaskCreate(&leds_task, "leds_task", 2 * 1024, NULL, 5, NULL);
    // leds_changeState(READY);
    // vTaskDelay(pdMS_TO_TICKS(5000));
    // leds_changeState(LOADING);
}