/**
 * @file    screens.h
 * @brief   This file contains different screen definitions.
 ******************************************************************************
 * @attention
 * This software is provided AS-IS.
 */

#ifndef SCREENS_H
#define SCREENS_H

#include "SSD1803A_driver.h"
// #include "storage.h"

typedef enum screen_selection {
	SCREEN_OFF,
	SCREEN_HOME,
	SCREEN_SET_LIGHTS,
	SCREEN_SHOW_STORAGE_ROWS,
	SCREEN_SHOW_STORAGE_SLOTS,
	SCREEN_CHANGE_IP,
	SCREEN_SET_IP,
	SCREEN_SET_NETMASK,
	SCREEN_SET_GW,
	SCREEN_BOOT = 99,
} screen_selection_t;

typedef enum screen_option {
	VERTICAL_SELECTOR,
	HORIZONTAL_SELECTOR,
} screen_option_t;

typedef struct Screen Screen_t;

typedef enum {
	BTN_NONE = 0,
	BTN_UP = 1 << 0,
	BTN_DOWN = 1 << 1,
	BTN_LEFT = 1 << 2,
	BTN_RIGHT = 1 << 3,
	BTN_ENTER = 1 << 4
} ButtonMask;

typedef enum {
	OPTIONS_NONE = 0,
	OPTIONS_PRINT_HEADER = 1 << 0,
	OPTIONS_PRINT_VERTICAL_SELECTOR = 1 << 1,
	OPTIONS_PRINT_HORIZONTAL_SELECTOR = 1 << 2,
	OPTIONS_PRINT_INLINE = 1 << 3,
	OPTIONS_PRINT_ENTER_ICON = 1 << 4,
} OptionMask;

typedef void (*ScreenFn)(void);
typedef void (*ScreenInputFn)(Screen_t *scr, ButtonMask btn, void *arg);

struct Screen {
	char name[12];

	Screen_t *parent;
	Screen_t *child;
	Screen_t *next; // sibling

	ScreenFn function;
	ScreenInputFn handleInput;

	ButtonMask allowedButtons;
	OptionMask renderOptions;
};

// screen functions
void screen_init(void);
void mainRender(void);
void rowStatusRender(void);
void nextPageRender(void);
void setLights(void);

void mainInput(Screen_t *scr, ButtonMask btn, void *arg);
void rowStatusInput(Screen_t *scr, ButtonMask btn, void *arg);
void nextPageInput(Screen_t *scr, ButtonMask btn, void *arg);

screen_selection_t get_currentScreen(void);
void set_currentScreen(screen_selection_t screen);

uint8_t screen_getVerticalSelectorLocation(void);
void screen_setVerticalSelectorLocation(uint8_t location);

uint8_t screen_getHorizontalSelectorLocation(void);
void screen_setHorizontalSelectorLocation(uint8_t location);

void screen_updateRowStatusNumber(uint8_t row);
void screen_updateRowStatusData(uint64_t data);

void screen_show(screen_selection_t screen);

Screen_t *screen_getChildAtIndex(Screen_t *scr, uint8_t location);

extern Screen_t *currentScreen;

extern Screen_t scrMain;
extern Screen_t scrSettings;
extern Screen_t scrRows;
extern Screen_t scrBoot;
extern uint8_t rowCounter;

#endif // SCREENS_H