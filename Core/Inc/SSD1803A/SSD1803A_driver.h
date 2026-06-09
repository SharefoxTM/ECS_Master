/**
 * @file    SSD1803A_driver.h
 * @brief   This file contains the driver functions for the SSD1803A display
 *controller.
 ******************************************************************************
 * @attention
 * This software is provided AS-IS.
 */

#ifndef SSD1803A_DRIVER_H
#define SSD1803A_DRIVER_H

#include "lwip.h"
#include "main.h"
#include <string.h>

#define displayAddress 0x78

#define MODE_COMMAND 0x00
#define MODE_DATA		 0x40

#define COMMAND_CLEAR_DISPLAY				0x01
#define COMMAND_RETURN_HOME					0x02
#define COMMAND_ENTRY_MODE_SET			0x04
#define ENTRY_MODE_LEFT_TO_RIGHT		0x02
#define ENTRY_MODE_SHIFT_INCREMENT	0x01
#define COMMAND_SHIFT								0x10
#define COMMAND_DISPLAY_SHIFT_LEFT	0x08
#define COMMAND_DISPLAY_SHIFT_RIGHT 0x0C
#define COMMAND_CURSOR_SHIFT_LEFT		0x00
#define COMMAND_CURSOR_SHIFT_RIGHT	0x04

#define ADDRESS_CGRAM						 0x40
#define ADDRESS_DDRAM						 0x80
#define ADDRESS_DDRAM_TOP_OFFSET 0x04

#define COMMAND_8BIT_4LINES_RE0_IS0			0x38
#define COMMAND_8BIT_4LINES_RE0_IS1			0x39
#define COMMAND_8BIT_4LINES_RE1_IS0			0x3A
#define COMMAND_8BIT_4LINES_RE0_IS0_DH1 0x3C
#define COMMAND_8BIT_4LINES_RE0_IS1_DH1 0x3D

// Command from extended set (RE = 1, IS = 0)
#define COMMAND_BS1_1							 0x1E
#define COMMAND_POWER_DOWN_DISABLE 0x02
#define COMMAND_TOP_VIEW					 0x05
#define COMMAND_BOTTOM_VIEW				 0x06
#define COMMAND_4LINES						 0x09
#define COMMAND_3LINES_TOP				 0x1F
#define COMMAND_3LINES_MIDDLE			 0x17
#define COMMAND_3LINES_BOTTOM			 0x13
#define COMMAND_2LINES						 0x1B

// Command from extended set (RE = 0, IS = 1)
#define COMMAND_DISPLAY			0x08
#define COMMAND_DISPLAY_ON	0x04
#define COMMAND_DISPLAY_OFF 0x00
#define COMMAND_CURSOR_ON		0x02
#define COMMAND_CURSOR_OFF	0x00
#define COMMAND_BLINK_ON		0x01
#define COMMAND_BLINK_OFF		0x00

// Command from extended set (RE = 1, IS = 1)
#define COMMAND_SHIFT_SCROLL_ENABLE		 0x10
#define COMMAND_SHIFT_SCROLL_ALL_LINES 0x0F
#define COMMAND_SHIFT_SCROLL_LINE_1		 0x01
#define COMMAND_SHIFT_SCROLL_LINE_2		 0x02
#define COMMAND_SHIFT_SCROLL_LINE_3		 0x04
#define COMMAND_SHIFT_SCROLL_LINE_4		 0x08

#define COMMAND_BS0_1								0x1B
#define COMMAND_INTERNAL_DIVIDER		0x13
#define COMMAND_CONTRAST_DEFAULT		0x70
#define COMMAND_POWER_CONTROL				0x54
#define COMMAND_POWER_ICON_CONTRAST 0x5C
#define COMMAND_FOLLOWER_CONTROL		0x6C
#define COMMAND_ROM_SELECT					0x72
#define COMMAND_ROM_A								0x00
#define COMMAND_ROM_B								0x04
#define COMMAND_ROM_C								0x08

typedef enum {
	VIEW_TOP,
	VIEW_BOTTOM,
	DISPLAY_ON,
	DISPLAY_OFF,
	CURSOR_ON,
	CURSOR_OFF,
	BLINK_ON,
	BLINK_OFF,
	DISPLAY_SHIFT_LEFT,
	DISPLAY_SHIFT_RIGHT,
	CURSOR_SHIFT_LEFT,
	CURSOR_SHIFT_RIGHT,
	LEFT_TO_RIGHT,
	RIGHT_TO_LEFT,
	AUTOSCROLL_ON,
	AUTOSCROLL_OFF,
	CONTRAST,
	LINES_4,
	LINES_3_1,
	LINES_3_2,
	LINES_3_3,
	LINES_2,
	SET_ROM_A,
	SET_ROM_B,
	SET_ROM_C
} displayMode_t;

typedef struct {
	uint8_t rows;
	uint8_t columns;
} display_t;

void SSD1803A_init(void);
void SSD1803A_reset(void);
void SSD1803A_clr_screen(void);
void SSD1803A_home(void);
void SSD1803A_setCursor(uint8_t row, uint8_t column);
void SSD1803A_setMode(displayMode_t mode);
void SSD1803A_setContrast(uint8_t value);
void SSD1803A_createChar(uint8_t location, uint8_t charmap[]);
void SSD1803A_write(char *buffer);
void SSD1803A_writeCharacter(char character);
void SSD1803A_writeNumber(uint8_t num);
void SSD1803A_writeByte(uint8_t byte);
void SSD1803A_writeIP(uint32_t ip);

#endif // SSD1803A_DRIVER_H