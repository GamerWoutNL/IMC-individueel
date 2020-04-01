#include "stdbool.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/task.h"
#include "twist.h"
#include "rotary.h"

const int8_t statusButtonClickedBit = 2;
const int8_t statusButtonPressedBit = 1;
const int8_t statusEncoderMovedBit = 0;

const int8_t enableInterruptButtonBit = 1;
const int8_t enableInterruptEncoderBit = 0;

int16_t lastCount = 0x00;
twist_t* re;

void rotary_init(twist_t *rotary)
{
    re = rotary;
}

bool rotary_isConnected()
{
    return twist_check_connection(re) == TWIST_ERR_OK ? true : false;
}

int16_t rotary_getCount()
{
    uint16_t data = 0x00;
    twist_read_register16(re, TWIST_COUNT, &data);    
    return data;
}

bool rotary_setCount(int16_t amount)
{
    return twist_write_register16(re, TWIST_COUNT, amount) == TWIST_ERR_OK ? true : false;
}

//Checks if encoder has been turned, if so, what direction
twist_encoder_state rotary_getEncoderState()
{
    int16_t currCount = rotary_getCount();
    twist_encoder_state state = TWIST_EQUAL;

    if (currCount == lastCount)
    {
        state = TWIST_EQUAL;
    }
    else if (currCount > lastCount)
    {
        state = TWIST_GREATER;
    }
    else
    {
        state = TWIST_LESS;
    }

    lastCount = currCount;
    return state;    
}

int16_t rotary_getDiff(bool clearValue)
{
    uint16_t data = 0x00;
    twist_read_register16(re, TWIST_DIFFERENCE, &data);
    if (clearValue == true)
    {
        twist_write_register16(re, TWIST_DIFFERENCE, 0);
    }
    return data;
}

bool rotary_isMoved()
{
    uint8_t data = 0x00;
    twist_read_register(re, TWIST_STATUS, &data);
    bool moved = data & (1 << statusEncoderMovedBit);
    twist_write_register(re, TWIST_STATUS, data & ~(1 << statusEncoderMovedBit));
    return (moved);
}

bool rotary_isPressed()
{
    uint8_t data = 0x00;
    twist_read_register(re, TWIST_STATUS, &data);  
    bool pressed = data & (1 << statusButtonPressedBit);
    twist_write_register(re, TWIST_STATUS, data & ~(1 << statusButtonPressedBit));
    return (pressed);
}

bool rotary_isClicked()
{
    uint8_t data = 0x00;
    twist_read_register(re, TWIST_STATUS, &data);  
    bool clicked = data & (1 << statusButtonClickedBit);
    twist_write_register(re, TWIST_STATUS, data & ~(1 << statusButtonClickedBit));
    return (clicked);
}

uint16_t rotary_timeSinceLastMovement(bool clearValue)
{
    uint16_t data = 0x00;
    twist_read_register16(re, TWIST_LAST_ENCODER_EVENT, &data);    
    if (clearValue == true) 
    {
        twist_write_register16(re, TWIST_LAST_ENCODER_EVENT, 0);
    }
    return data;
}

uint16_t rotary_timeSinceLastPress(bool clearValue)
{
    uint16_t data = 0x00;
    twist_read_register16(re, TWIST_LAST_BUTTON_EVENT, &data);    
    if (clearValue == true)
    {
        twist_write_register16(re, TWIST_LAST_BUTTON_EVENT, 0);
    }
    return data;
}

void rotary_clearInterrupts()
{
    twist_write_register(re, TWIST_STATUS, 0);
}

bool rotary_setColor(uint8_t red, uint8_t green, uint8_t blue){
    return (twist_write_register32(re, TWIST_RED, (uint32_t)red << 16 | (uint32_t)green << 8 | blue));  
}

bool rotary_setRed(uint8_t val){
    return (twist_write_register(re, TWIST_RED, val)); 
}

bool rotary_setGreen(uint8_t val){
    return (twist_write_register(re, TWIST_GREEN, val)); 
}

bool rotary_setBlue(uint8_t val){
    return (twist_write_register(re, TWIST_BLUE, val));  
}

uint8_t rotary_getRed(){
    uint8_t data = 0x00;
    twist_read_register(re, TWIST_RED, &data);    
    return data;
}

uint8_t rotary_getGreen(){
    uint8_t data = 0x00;
    twist_read_register(re, TWIST_GREEN, &data);    
    return data;
}

uint8_t rotary_getBlue(){
    uint8_t data = 0x00;
    twist_read_register(re, TWIST_BLUE, &data);    
    return data;
}

bool rotary_connectColor(int16_t red, int16_t green, int16_t blue)
{
    return twist_connect_color(re, red, green, blue) == TWIST_ERR_OK ? true : false;
}

bool rotary_connectRed(int16_t val){
    return twist_write_register16(re, TWIST_CONNECT_RED, val) == TWIST_ERR_OK ? true : false;
}

bool rotary_connectGreen(int16_t val)
{
    return twist_write_register16(re, TWIST_CONNECT_GREEN, val) == TWIST_ERR_OK ? true : false;
}

bool rotary_connectBlue(int16_t val)
{
    return twist_write_register16(re, TWIST_CONNECT_BLUE, val) == TWIST_ERR_OK ? true : false;
}

int16_t rotary_getRedConnect()
{
    uint16_t data = 0x00;
    twist_read_register16(re, TWIST_CONNECT_RED, &data);    
    return data;
}

int16_t rotary_getGreenConnect()
{
    uint16_t data = 0x00;
    twist_read_register16(re, TWIST_CONNECT_GREEN, &data);    
    return data;
}

int16_t rotary_getBlueConnect()
{
    uint16_t data = 0x00;
    twist_read_register16(re, TWIST_CONNECT_BLUE, &data);    
    return data;
}

bool rotary_setLimit(uint16_t val)
{
    return twist_write_register16(re, TWIST_LIMIT, val) == TWIST_ERR_OK ? true : false;
}

uint16_t rotary_getLimit()
{
    uint16_t data = 0x00;
    twist_read_register16(re, TWIST_LIMIT, &data);    
    return data;
}

uint16_t rotary_getIntTimeout()
{
    uint16_t data = 0x00;
    twist_read_register16(re, TWIST_TURN_INT_TIMEOUT, &data);    
    return data;
}

bool rotary_setIntTimeout(uint16_t timeout)
{
    return twist_write_register16(re, TWIST_TURN_INT_TIMEOUT, timeout) == TWIST_ERR_OK ? true : false;
}

uint16_t rotary_getVersion(){
    uint16_t data = 0x00;
    twist_read_register16(re, TWIST_VERSION, &data);    
    return data;
}

void rotary_changeAddress(uint8_t newAddress)
{
    twist_write_register(re, TWIST_CHANGE_ADDRESS, newAddress);

    re->i2c_addr = newAddress;
}