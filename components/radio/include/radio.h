#ifndef RADIO_H
#define RADIO_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct {
    char name[10];
    char url[100];
} radio_channel_t;

extern radio_channel_t station_3fm;
extern radio_channel_t station_qmusic;
extern radio_channel_t station_slam;
extern radio_channel_t station_538;

void radioTask(void *pvParameters);
void radioConnect(radio_channel_t channel);

#endif 