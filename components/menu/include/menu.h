#ifndef MENU_H
#define MENU_H

#include "i2c-lcd1602.h"

void menuInit(i2c_lcd1602_info_t* info);
int getMenuIndex();
int getMenuId();
void callOnKeyFunction(unsigned int currentItem, int keyIndex);
void callOnExit(unsigned int currentItem);
void callOnEntry(unsigned int currentItem);

int handleMenuTransition(unsigned int currentItem, int keyIndex, unsigned int* pCurrentId);
unsigned int findNewMenuIndex(unsigned int menuId);
void handleMenu(unsigned int* pIndex, int key, unsigned int* pId);
void printMenuItem(char* lines[]);

void radioChangeOne(void); 
void radioChangeTwo(void);
void radioChangeThree(void);
void radioChangeFour(void);
void weatherChange(void);
void agendaChange(void);
void taskMenuLoop(void* pvParameters);
void setLastKeyPress(int newKeyPress);  


#define MAX_MENU_KEY 6
#define LCD_MAX_LINES 4

/*Define key IDs*/
#define MENU_NO_KEY -1
#define MENU_LEFT_PRESS 0
#define MENU_RIGHT_PRESS 1
#define MENU_TURN_LEFT_LEFT 2
#define MENU_TURN_LEFT_RIGHT 3
#define MENU_TURN_RIGHT_LEFT 4
#define MENU_TURN_RIGHT_RIGHT 5

/*Define IDs for menus*/
#define MENU_MAIN_RADIO_ID 0
#define MENU_MAIN_AGENDA_ID 1
#define MENU_MAIN_WEATHER_ID 2
#define MENU_AGENDA_ID 3
#define MENU_RADIO_ID 4
#define MENU_WEATHER_ID 5
#define MENU_RADIO_SUB_1 6
#define MENU_RADIO_SUB_2 7
#define MENU_RADIO_SUB_3 8

/*Create struct*/
typedef struct {
    unsigned int id;
    unsigned int newId[MAX_MENU_KEY];
    char* text[LCD_MAX_LINES];
    void(*fpOnKey[MAX_MENU_KEY])(void);
    void(*fpOnEntry)(void);
    void(*fpOnExit)(void);

} MENU_STRUCT;

MENU_STRUCT menus[];

#endif
