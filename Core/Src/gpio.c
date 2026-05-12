/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    gpio.c
 * @brief   This file provides code for the configuration
 *          of all used GPIO pins.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */
#include "Utilities/log.h"
#include "modbus/modbus.h"

void handleVerticalInput(Screen_t *scr, ButtonMask btn);
void handleHorizontalInput(Screen_t *scr, ButtonMask btn);
void handleVerticalSelector(Screen_t *scr, ButtonMask btn);
ButtonMask prevBtn;
extern uint8_t rowCounter;

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
 * Analog
 * Input
 * Output
 * EVENT_OUT
 * EXTI
 */
void MX_GPIO_Init(void) {

	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(USART1_DE_GPIO_Port, USART1_DE_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(SCREEN_RST_GPIO_Port, SCREEN_RST_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : USART1_DE_Pin */
	GPIO_InitStruct.Pin = USART1_DE_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(USART1_DE_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : BUTTON_ENTER_Pin */
	GPIO_InitStruct.Pin = BUTTON_ENTER_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BUTTON_ENTER_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : BUTTON_LEFT_Pin */
	GPIO_InitStruct.Pin = BUTTON_LEFT_Pin | BUTTON_RIGHT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BUTTON_LEFT_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : BUTTON_DOWN_Pin */
	GPIO_InitStruct.Pin = BUTTON_DOWN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BUTTON_DOWN_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : BUTTON_UP_Pin */
	GPIO_InitStruct.Pin = BUTTON_UP_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BUTTON_UP_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : SCREEN_RST_Pin */
	GPIO_InitStruct.Pin = SCREEN_RST_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(SCREEN_RST_GPIO_Port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 2 */
void gpio_getUserInput(void) {
	ButtonMask btn = BTN_NONE;

	if (HAL_GPIO_ReadPin(BUTTON_DOWN_GPIO_Port, BUTTON_DOWN_Pin) == GPIO_PIN_RESET) {
		btn |= BTN_DOWN;
	}
	if (HAL_GPIO_ReadPin(BUTTON_UP_GPIO_Port, BUTTON_UP_Pin) == GPIO_PIN_RESET) {
		btn |= BTN_UP;
	}
	if (HAL_GPIO_ReadPin(BUTTON_LEFT_GPIO_Port, BUTTON_LEFT_Pin) == GPIO_PIN_RESET) {
		btn |= BTN_LEFT;
	}
	if (HAL_GPIO_ReadPin(BUTTON_RIGHT_GPIO_Port, BUTTON_RIGHT_Pin) == GPIO_PIN_RESET) {
		btn |= BTN_RIGHT;
	}
	if (HAL_GPIO_ReadPin(BUTTON_ENTER_GPIO_Port, BUTTON_ENTER_Pin) == GPIO_PIN_RESET) {
		btn |= BTN_ENTER;
	}
	if (btn == (BTN_ENTER | BTN_DOWN | BTN_UP)) {
		LOG_WARN("Resetting system due to button press\r");
		NVIC_SystemReset();
	}

	if (btn != BTN_NONE && currentScreen->handleInput != NULL && prevBtn != btn) {
#if LOG_LEVEL <= 2
		for (uint8_t i = 0; i < 5; i++)
			if (btn & 0x01 << i)
				LOG_DEBUG("Button %s pressed\r", (i == 0	 ? "BTN_UP"
																					: i == 1 ? "BTN_DOWN"
																					: i == 2 ? "BTN_LEFT"
																					: i == 3 ? "BTN_RIGHT"
																									 : "BTN_ENTER"));
#endif
		if (strcmp(currentScreen->name, "Slots") != 0) {
			currentScreen->handleInput(currentScreen, btn, NULL);
		} else {
			currentScreen->handleInput(currentScreen, btn, &rowCounter);
		}
	}
	prevBtn = btn;
}

void mainInput(Screen_t *scr, ButtonMask btn, void *arg) {
	if (btn & (BTN_DOWN | BTN_UP) && scr->child != NULL) {
		// Handle vertical if allowed
		handleVerticalInput(scr, btn);
	}

	if ((scr->allowedButtons & (BTN_RIGHT | BTN_LEFT)) && (btn & (BTN_RIGHT | BTN_LEFT))) {
		// Handle horizontal if allowed
		handleHorizontalInput(scr, btn);
	}

	if (btn & BTN_ENTER && scr->child != NULL) {
		uint8_t location;
		location = screen_getVerticalSelectorLocation();
		Screen_t *child = screen_getChildAtIndex(scr, location);
		if (child != NULL) {
			currentScreen = child;
			currentScreen->function();
		}
	}
}

void rowStatusInput(Screen_t *scr, ButtonMask btn, void *arg) {
	if (btn & (BTN_DOWN | BTN_UP)) {
		// Increment or decrement row number
		uint8_t *pRowCounter = (uint8_t *)arg;
		if (btn & BTN_DOWN) {
			(*pRowCounter)++;
			if (*pRowCounter == 17) {
				*pRowCounter = 1;
			}
		} else if (btn & BTN_UP) {
			(*pRowCounter)--;
			if (*pRowCounter == 0) {
				*pRowCounter = 16;
			}
		}
		screen_updateRowStatusNumber(*pRowCounter);
	}
	if (btn & BTN_ENTER) {
		if (screen_getHorizontalSelectorLocation() == 0) {
			uint64_t tempNum;
			if (Modbus_GetRowCoilsStatus(rowCounter, MODBUS_RX_TIMEOUT_MS, (uint8_t *)&tempNum) != HAL_OK) {
				LOG_ERROR("Failed to get status for row %d\r", rowCounter);
				SSD1803A_setCursor(2, 0);
				SSD1803A_write("Row unavailable");
				return;
			}
			screen_updateRowStatusData(tempNum);
		} else {
			currentScreen = scr->parent->parent; // Go back to main menu
			currentScreen->function();
		}
	}
}

void nextPageInput(Screen_t *scr, ButtonMask btn, void *arg) {
}

void handleVerticalInput(Screen_t *scr, ButtonMask btn) {
	uint8_t location, borderTop = 0, borderBottom = 3;
	location = screen_getVerticalSelectorLocation();
	if (currentScreen->renderOptions & OPTIONS_PRINT_HEADER) {
		borderTop = 1;
	};

	if (btn & BTN_DOWN) {
		location++;
		if (location > borderBottom || screen_getChildAtIndex(scr, location) == NULL) {
			location = borderTop;
		}
	} else if (btn & BTN_UP) {
		if (location == borderTop) {
			// Wrap around to last child
			uint8_t lastIndex = borderTop;
			while (screen_getChildAtIndex(scr, lastIndex) != NULL) {
				lastIndex++;
			}
			location = lastIndex - 1;
		} else {
			location--;
		}
	}
	screen_setVerticalSelectorLocation(location);
}

void handleHorizontalInput(Screen_t *scr, ButtonMask btn) {
	uint8_t location;
	location = screen_getHorizontalSelectorLocation();

	if (btn & BTN_RIGHT) {
		location++;
		if (location > 15) {
			location = 0;
		}
		screen_setHorizontalSelectorLocation(location);
	} else if (btn & BTN_LEFT) {
		if (location == 0) {
			location = 15;
		} else {
			location--;
		}
		screen_setHorizontalSelectorLocation(location);
	}
}

void handleVerticalSelector(Screen_t *scr, ButtonMask btn) {
	if (btn & BTN_DOWN) {
	} else if (btn & BTN_UP) {
	}
}

/* USER CODE END 2 */
