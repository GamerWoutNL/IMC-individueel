#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "menu.h"

/*Create menus*/
MENU_STRUCT menus[9] = {
    {
        MENU_MAIN_RADIO_ID,
        { /*menpush, volpush, menright, menleft, volleft, vollright*/
            MENU_RADIO_ID,
            -1,
            MENU_MAIN_WEATHER_ID,
            MENU_MAIN_AGENDA_ID,
            -1,
            -1
        },
        {
            "HOOFDMENU",
            "selectie",
            "<Radio>AgendaWeer",
            " "
        },
        {
            NULL, NULL, NULL, NULL, NULL, NULL
        },
        NULL,
        NULL
    },
    {
        MENU_MAIN_AGENDA_ID,
        {
            MENU_AGENDA_ID,
            -1,
            MENU_MAIN_RADIO_ID,
            MENU_MAIN_WEATHER_ID,
            -1,
            -1
        },
        {
            "HOOFDMENU",
            "selectie",
            "Radio<Agenda>Weer",
            " "
        },
        {
            NULL, NULL, NULL, NULL, NULL, NULL
        },
        NULL,
        NULL
    },
    {
        MENU_MAIN_WEATHER_ID,
        {
            MENU_WEATHER_ID,
            -1,
            MENU_MAIN_AGENDA_ID,
            MENU_MAIN_RADIO_ID,
            -1,
            -1
        },
        {
            "HOOFDMENU",
            "selectie",
            "RadioAgenda<Weer>",
            " "
        },
        {
            NULL, NULL, NULL, NULL, NULL, NULL
        },
        NULL,
        NULL
    },
    {
        MENU_AGENDA_ID,
        {
            -1,
            MENU_MAIN_AGENDA_ID,
            -1,
            -1,
            -1,
            -1
        },
        {
            "AGENDA",
            " ",
            " ",
            " "
        },
        {
            NULL, NULL, agendaChange, agendaChange, NULL, NULL
        },
        NULL,
        NULL
    },
    {
        MENU_WEATHER_ID,
        {
            -1,
            MENU_MAIN_WEATHER_ID,
            -1,
            -1,
            -1,
            -1
        },
        {
            "WEER",
            " ",
            " ",
            " "
        },
        {
            NULL, NULL, weatherChange, weatherChange, NULL, NULL
        },
        NULL,
        NULL
    },
    {
        MENU_RADIO_ID,
        {
            -1,
            MENU_MAIN_RADIO_ID,
            MENU_RADIO_SUB_3,
            MENU_RADIO_SUB_1,
            -1,
            -1
        },
        {
            "RADIO",
            "zender:",
            "<Zender1>Zender2Zender3Zender4",
            " "
        },
        {
            radioChangeOne, NULL, NULL, NULL, NULL, NULL
        },
        NULL,
        NULL
    },
    {
        MENU_RADIO_SUB_1,
        {
            -1,
            MENU_MAIN_RADIO_ID,
            MENU_RADIO_ID,
            MENU_RADIO_SUB_2,
            -1,
            -1
        },
        {
            "RADIO",
            "zender:",
            "Zender1<Zender2>Zender3Zender4",
            " "
        },
        {
            radioChangeTwo, NULL, NULL, NULL, NULL, NULL
        },
        NULL,
        NULL
    },
    {
        MENU_RADIO_SUB_2,
        {
            -1,
            MENU_MAIN_RADIO_ID,
            MENU_RADIO_SUB_1,
            MENU_RADIO_SUB_3,
            -1,
            -1
        },
        {
            "RADIO",
            "zender:",
            "Zender1Zender2<Zender3>Zender4",
            " "
        },
        {
            radioChangeThree, NULL, NULL, NULL, NULL, NULL
        },
        NULL,
        NULL
    },
    {
        MENU_RADIO_SUB_3,
        {
            -1,
            MENU_MAIN_RADIO_ID,
            MENU_RADIO_SUB_2,
            MENU_RADIO_ID,
            -1,
            -1
        },
        {
            "RADIO",
            "zender:",
            "Zender1Zender2Zender3<Zender4>",
            " "
        },
        {
            radioChangeFour, NULL, NULL, NULL, NULL, NULL
        },
        NULL,
        NULL
    }
};

static unsigned int lastKeyPress = MENU_NO_KEY;
static unsigned int currentMenuIndex = 0;
static unsigned int currentMenuId;
i2c_lcd1602_info_t* lcd_info;

void menuInit(i2c_lcd1602_info_t *info)
{
    lcd_info = info;
    currentMenuId = menus[currentMenuIndex].id;
	printMenuItem(menus[currentMenuIndex].text);
}

int getMenuIndex()
{
    return currentMenuIndex;
}

int getMenuId()
{
    return currentMenuId;
}
void setLastKeyPress(int newKeyPress)
{
    lastKeyPress = newKeyPress;
}

void radioChangeOne(void) {
    printf("Radio zender verandert naar 1\n");
}

void radioChangeTwo(void) {
    printf("Radio zender verandert naar 2\n");
}

void radioChangeThree(void) {
    printf("Radio zender verandert naar 3\n");
}

void radioChangeFour(void) {
    printf("Radio zender verandert naar 4\n");
}

void weatherChange(void) {
    printf("Weer verandert\n");
}

void agendaChange(void) {
    printf("Agenda verandert\n");
}

/* Simulates LCD display */
void printMenuItem(char* lines[]) {
    unsigned int lineNr = 0;
    i2c_lcd1602_clear(lcd_info);
    // printf("=DISPLAY============\n");
    // for (lineNr = 0; lineNr < LCD_MAX_LINES; lineNr += 1) {
    //     if (lines[lineNr] != NULL) {
    //         printf("%s\n", lines[lineNr]);
    //     }
    // }
    // printf("====================\n\n");
    for (lineNr = 0; lineNr < LCD_MAX_LINES; lineNr += 1)
    {
        if (lines[lineNr] != NULL)
        {
            i2c_lcd1602_move_cursor(lcd_info, 0, lineNr);
            vTaskDelay(pdMS_TO_TICKS(1000));
            i2c_lcd1602_write_string(lcd_info, lines[lineNr]);
        }
    }
}

void callOnKeyFunction(unsigned int currentItem, int keyIndex) {
    if (NULL != menus[currentItem].fpOnKey[keyIndex]) {
        (*menus[currentItem].fpOnKey[keyIndex])();
    }
}

void callOnExit(unsigned int currentItem) {
    if (NULL != menus[currentItem].fpOnExit) {
        (*menus[currentItem].fpOnExit)();
    }
}

void callOnEntry(unsigned int currentItem) {
    if (NULL != menus[currentItem].fpOnEntry) {
        (*menus[currentItem].fpOnEntry)();
    }
}

int handleMenuTransition(unsigned int currentItem, int keyIndex, unsigned int* pCurrentId) {
    callOnKeyFunction(currentItem, keyIndex);
    /* Check for valid transition */
    if (menus[currentItem].newId[keyIndex] != -1) {
        callOnExit(currentItem);
        *pCurrentId = menus[currentItem].newId[keyIndex];
        return 1;
    }
    return 0;
}

unsigned int findNewMenuIndex(unsigned int menuId) {
    int currentItem = 0;
    while (menus[currentItem].id != menuId) {
        currentItem += 1;
    }
    return currentItem;
}

void handleMenu(unsigned int* pIndex, int key, unsigned int* pId) {
    int transition = 0;
    // printf("handling menu after keypress");
    switch (key) {
        /* Simulate left rotary */
    case MENU_LEFT_PRESS:
        transition = handleMenuTransition(*pIndex, MENU_LEFT_PRESS, pId);
        break;
    case MENU_TURN_LEFT_LEFT:
        transition = handleMenuTransition(*pIndex, MENU_TURN_LEFT_LEFT, pId);
        break;
    case MENU_TURN_LEFT_RIGHT:
        transition = handleMenuTransition(*pIndex, MENU_TURN_LEFT_RIGHT, pId);
        break;
    case MENU_RIGHT_PRESS:
        transition = handleMenuTransition(*pIndex, MENU_RIGHT_PRESS, pId);
        break;
        /* Simulate right rotary */
    case MENU_TURN_RIGHT_RIGHT:
        transition = handleMenuTransition(*pIndex, MENU_TURN_RIGHT_RIGHT, pId);
        break;
    case MENU_TURN_RIGHT_LEFT:
        transition = handleMenuTransition(*pIndex, MENU_TURN_RIGHT_LEFT, pId);
        break;
    default:
        break;
    }
    if (transition) {
        *pIndex = findNewMenuIndex(*pId);
        printMenuItem(menus[*pIndex].text);
        callOnEntry(*pIndex);
    }
    lastKeyPress = MENU_NO_KEY;
}

void taskMenuLoop(void* pvParameters)
{
    unsigned int currentMenuIndex = 0;
    unsigned int currentMenuId = menus[currentMenuIndex].id;
    printMenuItem(menus[currentMenuIndex].text);
    while (1) {
        handleMenu(&currentMenuIndex, lastKeyPress, &currentMenuId);
        vTaskDelay(pdMS_TO_TICKS(10)); // delay 10 ms
    }
}