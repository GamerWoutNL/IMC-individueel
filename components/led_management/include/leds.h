#ifndef LEDS_H
#define LEDS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "pca9685.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    READY,
    LOADING
} state_t;

typedef struct {
    int r;
    int g;
    int b;
} color_t;

typedef struct {
    int red;
    int green;
    int blue;
} rgb_led_channels_t;

extern color_t white;
extern color_t red;
extern color_t orange;
extern color_t yellow;
extern color_t green;
extern color_t blue;
extern color_t indigo;
extern color_t violet;

extern rgb_led_channels_t ledChannels;

void leds_task(void *param);
void leds_changeState(state_t state);
void leds_init(void);
void leds_setFrequency(uint16_t freq);
void leds_setColor(rgb_led_channels_t ledChannels, color_t color);
void leds_rainbow(void);

#ifdef __cplusplus
}
#endif

#endif
