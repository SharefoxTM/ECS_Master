#ifndef SCREENS_DATA_H
#define SCREENS_DATA_H

#include "SSD1803A/screens.h"

// Screen definitions

Screen_t scrBoot = {
	.name = "Booting...",
	.function = &mainRender,
	.handleInput = NULL,
	.renderOptions = OPTIONS_PRINT_HEADER,
};

// Main menu
Screen_t scrMain = {
	.name = "Main menu",
	.allowedButtons = BTN_DOWN | BTN_UP | BTN_ENTER,
	.handleInput = &mainInput,
	.function = &mainRender,
	.renderOptions = OPTIONS_PRINT_HEADER | OPTIONS_PRINT_VERTICAL_SELECTOR | OPTIONS_PRINT_ENTER_ICON,
};

// Submenus
Screen_t scrLights = {
	.name = "Lights",
	.allowedButtons = BTN_DOWN | BTN_UP | BTN_ENTER,
	.handleInput = &mainInput,
	.function = &mainRender,
	.renderOptions = OPTIONS_PRINT_VERTICAL_SELECTOR | OPTIONS_PRINT_ENTER_ICON,
};

Screen_t scrSettings = {
	.name = "Settings",
	.allowedButtons = BTN_DOWN | BTN_UP | BTN_ENTER,
	.handleInput = &mainInput,
	.function = &mainRender,
	.renderOptions = OPTIONS_PRINT_HEADER | OPTIONS_PRINT_VERTICAL_SELECTOR | OPTIONS_PRINT_ENTER_ICON,
};

Screen_t scrRows = {
	.name = "Rows",
	.allowedButtons = BTN_DOWN | BTN_UP | BTN_ENTER,
	.handleInput = &mainInput,
	.function = &mainRender,
	.renderOptions = OPTIONS_PRINT_HEADER | OPTIONS_PRINT_VERTICAL_SELECTOR | OPTIONS_PRINT_ENTER_ICON,
};

// Submenu of rows
Screen_t scrSlots = {
	.name = "Slots",
	.allowedButtons = BTN_DOWN | BTN_UP | BTN_ENTER,
	.handleInput = &rowStatusInput,
	.function = &rowStatusRender,
	.renderOptions =
		OPTIONS_PRINT_HEADER | OPTIONS_PRINT_ENTER_ICON | OPTIONS_PRINT_HORIZONTAL_SELECTOR | OPTIONS_PRINT_INLINE,
};

// Submenu of lights
Screen_t light_on = {
	.name = "ON",
	.function = &setLights,
};

Screen_t light_vegas = {
	.name = "VEGAS",
	.function = &setLights,
};

Screen_t light_KR = {
	.name = "KNIGHT RIDER",
	.function = &setLights,
};

Screen_t light_off = {
	.name = "OFF",
	.function = &setLights,
};

// Submenu of settings
Screen_t scrNetwork = {
	.name = "Network",
	.allowedButtons = BTN_DOWN | BTN_UP | BTN_ENTER,
	.handleInput = &mainInput,
	.function = &mainRender,
	.renderOptions = OPTIONS_PRINT_VERTICAL_SELECTOR | OPTIONS_PRINT_ENTER_ICON,
};

// Submenu of network
Screen_t scrIP = {
	.name = "IP Address",
	.allowedButtons = BTN_DOWN | BTN_UP | BTN_RIGHT | BTN_LEFT | BTN_ENTER,
	.handleInput = &networkInput,
	.function = &networkRender,
	.renderOptions = OPTIONS_PRINT_ENTER_ICON | OPTIONS_PRINT_HORIZONTAL_SELECTOR | OPTIONS_NETWORK,
};

Screen_t scrSubnet = {
	.name = "Subnet Mask",
	.allowedButtons = BTN_DOWN | BTN_UP | BTN_RIGHT | BTN_LEFT | BTN_ENTER,
	.handleInput = &networkInput,
	.function = &networkRender,
	.renderOptions = OPTIONS_PRINT_ENTER_ICON | OPTIONS_PRINT_HORIZONTAL_SELECTOR | OPTIONS_NETWORK,
};

Screen_t scrGateway = {
	.name = "Gateway",
	.allowedButtons = BTN_DOWN | BTN_UP | BTN_RIGHT | BTN_LEFT | BTN_ENTER,
	.handleInput = &networkInput,
	.function = &networkRender,
	.renderOptions = OPTIONS_PRINT_ENTER_ICON | OPTIONS_PRINT_HORIZONTAL_SELECTOR | OPTIONS_NETWORK,
};

Screen_t scrNetworkSubmit = {
	.name = "Submit",
	.allowedButtons = BTN_ENTER,
	.handleInput = &networkInput,
	.function = &networkSubmit,
};

// Next page screen
// TODO: Implement next page render and input functions, and link to appropriate screen
Screen_t nextPage = {
	.name = "NEXT PAGE",
	.allowedButtons = BTN_DOWN | BTN_UP | BTN_ENTER,
	.handleInput = nextPageInput,
	.function = nextPageRender,
	.renderOptions = OPTIONS_PRINT_HEADER | OPTIONS_PRINT_VERTICAL_SELECTOR | OPTIONS_PRINT_ENTER_ICON,
};

#endif /* SCREENS_DATA_H */