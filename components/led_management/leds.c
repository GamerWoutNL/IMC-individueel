#include "leds.h"

#define ADDR PCA9685_ADDR_BASE
#define SDA_GPIO 18
#define SCL_GPIO 23

state_t currentState = READY;
i2c_dev_t dev;

color_t white   =   {0xFF, 0xFF, 0xFF};
color_t red     =   {0xFF, 0x00, 0x00};
color_t orange  =   {0xFF, 0x7F, 0x00};
color_t yellow  =   {0xFF, 0xFF, 0x00};
color_t green   =   {0x00, 0xFF, 0x00};
color_t blue    =   {0x00, 0x00, 0xFF};
color_t indigo  =   {0x2E, 0x2B, 0x5F};
color_t violet  =   {0x8B, 0x00, 0xFF};

rgb_led_channels_t ledChannels = {0, 1, 2};

/* Task to handle the color change
Input:      NULL
Returns:    nothing */
void leds_task(void *param)
{
    while (1)
    {
        switch (currentState)
        {
        case READY:
            leds_setColor(ledChannels, green);
            break;

        case LOADING:
            leds_setColor(ledChannels, red);
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* Function to change the color given the state of the Smarkt Speaker. 
Input:      the state of the Smart Speaker. The leds_task() will handle the color change
Returns:    nothing */
void leds_changeState(state_t state)
{
    currentState = state;
}

void leds_init(void)
{
    memset(&dev, 0, sizeof(i2c_dev_t));

    for (int i = 0; i < 3; i++)
    {
        if (pca9685_init_desc(&dev, ADDR, 0, SDA_GPIO, SCL_GPIO) == ESP_OK)
        {
            break;
        }
    }

    for (int i = 0; i < 3; i++)
    {
        if (pca9685_init(&dev) == ESP_OK)
        {
            break;
        }
    }

    for (int i = 0; i < 3; i++)
    {
        if (pca9685_restart(&dev) == ESP_OK)
        {
            break;
        }
    }
}

void leds_setFrequency(uint16_t freq)
{
    for (int i = 0; i < 3; i++)
    {
        if (pca9685_set_pwm_frequency(&dev, freq) == ESP_OK)
        {
            break;
        }
    }
}

void leds_setPWM(int channel, int value)
{
    uint16_t val = 4095 - value;

    if (val > 4095)
    {
        val = 4095;
    }

    for (int i = 0; i < 3; i++)
    {
        if (pca9685_set_pwm_value(&dev, channel, val) == ESP_OK)
        {
            break;
        }
    }
}

/* Wrapper function to set the RGB LED to a given color. 
Input:      the rgb channels of the RGB LED connected to the PWM driver
Input:      a color
Returns:    nothing */
void leds_setColor(rgb_led_channels_t ledChannels, color_t color)
{
    leds_setPWM(ledChannels.red, (int)(4095.0 / 255.0 * color.r));
    leds_setPWM(ledChannels.green, (int)(4095.0 / 255.0 * color.g));
    leds_setPWM(ledChannels.blue, (int)(4095.0 / 255.0 * color.b));
}

/* Rainbow function, no explanation needed
Input:      nothing
Returns:    nothing */
void leds_rainbow(void)
{
    /* Only 10 times and not a while(true) so the thread doesn't block */
    for (int i = 0; i < 10; i++)
    {
        leds_setColor(ledChannels, red);
        vTaskDelay(pdMS_TO_TICKS(500));

        leds_setColor(ledChannels, orange);
        vTaskDelay(pdMS_TO_TICKS(500));

        leds_setColor(ledChannels, yellow);
        vTaskDelay(pdMS_TO_TICKS(500));

        leds_setColor(ledChannels, green);
        vTaskDelay(pdMS_TO_TICKS(500));

        leds_setColor(ledChannels, blue);
        vTaskDelay(pdMS_TO_TICKS(500));

        leds_setColor(ledChannels, indigo);
        vTaskDelay(pdMS_TO_TICKS(500));

        leds_setColor(ledChannels, violet);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}