#ifndef RE_LIB_H
#define RE_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/task.h"
#include "twist.h"
#include "stdbool.h"

/*
prototypes
*/

void rotary_init(twist_t *rotary);
bool rotary_isConnected();
int16_t rotary_getCount();
bool rotary_setCount(int16_t amount);
twist_encoder_state rotary_getEncoderState();
int16_t rotary_getDiff(bool clearValue);
bool rotary_isMoved();
bool rotary_isPressed();
bool rotary_isClicked();
uint16_t rotary_timeSinceLastMovement(bool clearValue);
uint16_t rotary_timeSinceLastPress(bool clearValue);
void rotary_clearInterrupts();

bool rotary_setColor(uint8_t red, uint8_t green, uint8_t blue);
bool rotary_setRed(uint8_t val);
bool rotary_setGreen(uint8_t val);
bool rotary_setBlue(uint8_t val);
uint8_t rotary_getRed();
uint8_t rotary_getGreen();
uint8_t rotary_getBlue();

bool rotary_connectColor(int16_t red, int16_t green, int16_t blue);
bool rotary_connectRed(int16_t val);
bool rotary_connectGreen(int16_t val);
bool rotary_connectBlue(int16_t val);
int16_t rotary_getRedConnect();
int16_t rotary_getGreenConnect();
int16_t rotary_getBlueConnect();

bool rotary_setLimit(uint16_t val);
uint16_t rotary_getlimit();

uint16_t rotary_getIntTimeout();
bool rotary_setIntTimeout(uint16_t timeout);

uint16_t rotary_getVersion();
void rotary_changeAddress(uint8_t newAddress);

#ifdef __cplusplus
}
#endif

#endif