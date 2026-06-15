#include "SSD1803A/screens.h"
#include "SSD1803A/SSD1803A_driver.h"
#include "SSD1803A/screens_data.h"
#include "TCP/tcp_server.h"
#include "Utilities/log.h"
#include "modbus/modbus.h"
#include "modbus/modbus_conf.h"
#include <stdint.h>
#include <string.h>


void addChild(Screen_t *parent, Screen_t *child);
void render_title(Screen_t *screen);
void render_children(Screen_t *scr, uint8_t offset);
void render_networkSettings(Screen_t *scr);
void render_verticalSelector(uint8_t row);
void render_horizontalSelector(uint8_t col);
void render_inlineHorizontalSelector(uint8_t row, uint8_t col);
void clr_cursors(void);

uint8_t countChildren(Screen_t *scr);

extern struct netif gnetif;
Screen_t *currentScreen;

uint8_t vSelectorPos = 99;
uint8_t hSelectorPos = 99;

uint8_t rowCounter = 1;

ip4_addr_t screen_ip;
ip4_addr_t screen_subnet;
ip4_addr_t screen_gateway;

void screen_init(void) {
	LOG_INFO("Initializing screen\r");
	SSD1803A_init();

	LOG_VERBOSE("Coupling children\r");
	LOG_VERBOSE("Coupling lights to main\r");
	addChild(&scrMain, &scrLights);
	LOG_VERBOSE("Coupling rows to main\r");
	addChild(&scrMain, &scrRows);
	LOG_VERBOSE("Coupling settings to main\r");
	addChild(&scrMain, &scrSettings);

	LOG_VERBOSE("Coupling light_on to lights\r");
	addChild(&scrLights, &light_on);
	LOG_VERBOSE("Coupling light_vegas to lights\r");
	addChild(&scrLights, &light_vegas);
	LOG_VERBOSE("Coupling light_kr to lights\r");
	addChild(&scrLights, &light_KR);
	LOG_VERBOSE("Coupling light_off to lights\r");
	addChild(&scrLights, &light_off);

	LOG_VERBOSE("Coupling slots to rows\r");
	addChild(&scrRows, &scrSlots);

	LOG_VERBOSE("Coupling network to settings\r");
	addChild(&scrSettings, &scrNetwork);

	LOG_VERBOSE("Coupling IP to network\r");
	addChild(&scrNetwork, &scrIP);
	LOG_VERBOSE("Coupling subnet to network\r");
	addChild(&scrNetwork, &scrSubnet);
	LOG_VERBOSE("Coupling gateway to network\r");
	addChild(&scrNetwork, &scrGateway);
	LOG_VERBOSE("Coupling submit to network\r");
	addChild(&scrNetwork, &scrNetworkSubmit);
	LOG_VERBOSE("Finished coupling children\r");
	screen_ip = gnetif.ip_addr;
	screen_subnet = gnetif.netmask;
	screen_gateway = gnetif.gw;
	currentScreen = &scrBoot;
	currentScreen->function();
}

void mainRender(void) {
	SSD1803A_clr_screen();
	LOG_VERBOSE("Rendering screen %s using main render\r", currentScreen->name);
	if (currentScreen->renderOptions & OPTIONS_PRINT_HEADER) {
		LOG_DEBUG("Printing title\r");
		render_title(currentScreen);

		LOG_VERBOSE("Rendering children of screen %s\r", currentScreen->name);
		render_children(currentScreen, 1);
	} else {
		LOG_VERBOSE("Rendering children of screen %s\r", currentScreen->name);
		render_children(currentScreen, 0);
	}
	if (currentScreen->renderOptions & OPTIONS_PRINT_HORIZONTAL_SELECTOR) {
		if (currentScreen->allowedButtons & BTN_RIGHT) {
			LOG_VERBOSE("Setting horizontal selector\r");
			render_horizontalSelector(0);
		}
	}

	if (currentScreen->renderOptions & OPTIONS_PRINT_VERTICAL_SELECTOR) {
		if (currentScreen->allowedButtons & BTN_DOWN) {
			LOG_VERBOSE("Setting vertical selector\r");
			if (currentScreen->renderOptions & OPTIONS_PRINT_HEADER) {
				LOG_DEBUG("Header selected, setting selector on row 1\r");
				render_verticalSelector(1);
			} else {
				LOG_DEBUG("No header selected, setting selector on row 0\r");
				render_verticalSelector(0);
			};
		}
	}

	if (currentScreen->renderOptions & OPTIONS_PRINT_ENTER_ICON) {
		SSD1803A_setCursor(3, 15);
		SSD1803A_writeCharacter(0x1C);
	}
}

void rowStatusRender(void) {
	uint64_t tempNum;
	SSD1803A_clr_screen();
	LOG_VERBOSE("Rendering screen %s using row status render\r", currentScreen->name);
	if (currentScreen->renderOptions & OPTIONS_PRINT_HEADER) {
		LOG_DEBUG("Printing title\r");
		render_title(currentScreen);
	}
	LOG_DEBUG("Rendering row status for row %d\r", rowCounter);
	screen_updateRowStatusNumber(rowCounter);
	render_horizontalSelector(0);
	if (Modbus_GetRowCoilsStatus(rowCounter, MODBUS_RX_TIMEOUT_MS, (uint8_t *)&tempNum) != HAL_OK) {
		LOG_ERROR("Failed to get status for row %d\r", rowCounter);
		SSD1803A_setCursor(2, 0);
		SSD1803A_write("Row unavailable");
		return;
	}
	screen_updateRowStatusData(tempNum);
}

void nextPageRender(void) {
	render_title(currentScreen);
	render_children(currentScreen, 1);
	render_verticalSelector(0);
}

void networkRender(void) {
	render_title(currentScreen);
	render_networkSettings(currentScreen);
	render_horizontalSelector(0);
}

uint8_t screen_getVerticalSelectorLocation(void) {
	return vSelectorPos;
}

void screen_setVerticalSelectorLocation(uint8_t location) {
	render_verticalSelector(location);
}

uint8_t screen_getHorizontalSelectorLocation(void) {
	return hSelectorPos;
}

void screen_setHorizontalSelectorLocation(uint8_t location) {
	render_horizontalSelector(location);
}

void screen_updateRowStatusNumber(uint8_t row) {
	char temp[16];
	sprintf(temp, "Row %02d: Back", row);
	SSD1803A_setCursor(1, 1);
	SSD1803A_write(temp);
}

void screen_updateRowStatusData(uint64_t data) {
	SSD1803A_setCursor(2, 0);
	uint8_t count;
	if ((HAL_StatusTypeDef)Modbus_getSlaveCoilCount(rowCounter, &count) == HAL_OK) {
		LOG_DEBUG("Row %d has %d coils\r", rowCounter, count);
		for (int i = 0; i < count; i++) {
			if (data & (1 << i)) {
				SSD1803A_writeNumber(1);
			} else {
				SSD1803A_writeNumber(0);
			}
		}
	} else {
		LOG_ERROR("Failed to get coil count for row %d\r", rowCounter);
		count = 0;
	}
}

Screen_t *screen_getChildAtIndex(Screen_t *scr, uint8_t location) {
	Screen_t *child = scr->child;
	uint8_t index = 0;
	if (currentScreen->renderOptions & OPTIONS_PRINT_HEADER)
		index = 1;

	while (child != NULL) {
		if (index == location) {
			return child;
		}
		child = child->next;
		index++;
	}
	return NULL;
}

void setLights(void) {
	LOG_DEBUG("SETTING LIGHTS: %s\r", currentScreen->name);
	Modbus_ChangeLedMode(MODBUS_SLAVE_BROADCAST, screen_getVerticalSelectorLocation());
	currentScreen = &scrMain;
	currentScreen->function();
}

void networkSubmit(void) {
	tcp_server_change_address((uint8_t *)ip4_addr_get_u32(&screen_ip), (uint8_t *)ip4_addr_get_u32(&screen_subnet),
														(uint8_t *)ip4_addr_get_u32(&screen_gateway));
}

void addChild(Screen_t *parent, Screen_t *child) {
	child->parent = parent;

	if (parent->child == NULL) {
		parent->child = child;
		return;
	}

	Screen_t *c = parent->child;
	while (c->next != NULL) {
		c = c->next;
	}
	c->next = child; // Append as sibling of other children
}

void render_title(Screen_t *screen) {
	char temp[16];
	uint8_t col = 0;

	SSD1803A_clr_screen();

	if (strlen(currentScreen->name) + 6 < 16) {
		col = (16 - (strlen(currentScreen->name) + 6)) / 2;
		sprintf(temp, "== %s ==", currentScreen->name);
	} else {
		col = (16 - strlen(currentScreen->name)) / 2;
		sprintf(temp, "%s", currentScreen->name);
	}

	LOG_DEBUG("Title: %s\r", temp);

	SSD1803A_setCursor(0, col);
	SSD1803A_write(temp);
}

void render_children(Screen_t *scr, uint8_t offset) {
	Screen_t *child = scr->child;
	uint8_t index = 1;
	while (child && offset < 4) {
		SSD1803A_setCursor(offset++, 1);
		SSD1803A_writeNumber(index++);
		SSD1803A_write(". ");
		SSD1803A_write(child->name);

		child = child->next;
	}
}

void render_networkSettings(Screen_t *scr) {
	SSD1803A_setCursor(2, 0);
	if (scr->name == scrIP.name) {
		SSD1803A_writeIP(&screen_ip);
		return;
	}
	if (scr->name == scrSubnet.name) {
		SSD1803A_writeIP(&screen_subnet);
		return;
	}
	if (scr->name == scrGateway.name) {
		SSD1803A_writeIP(&screen_gateway);
		return;
	}
}

void render_verticalSelector(uint8_t row) {
	if (vSelectorPos != 99) {
		SSD1803A_setCursor(vSelectorPos, 0);
		SSD1803A_writeCharacter(0x20);
	}
	if (row != 99) {
		SSD1803A_setCursor(row, 0);
		SSD1803A_writeCharacter(0x10);
	}
	vSelectorPos = row;
}

void render_horizontalSelector(uint8_t col) {
	if (currentScreen->renderOptions & OPTIONS_PRINT_INLINE) {
		render_inlineHorizontalSelector(screen_getVerticalSelectorLocation(), col);
		return;
	}
	if (hSelectorPos != 99) {
		SSD1803A_setCursor(1, hSelectorPos);
		SSD1803A_writeCharacter(0x20);
		SSD1803A_setCursor(3, hSelectorPos);
		SSD1803A_writeCharacter(0x20);
	}
	if (col != 99) {
		SSD1803A_setCursor(1, col);
		SSD1803A_writeCharacter(0x12);
		SSD1803A_setCursor(3, col);
		SSD1803A_writeCharacter(0x13);
	}
	hSelectorPos = col;
}

void render_inlineHorizontalSelector(uint8_t row, uint8_t col) {
	if (hSelectorPos != 99) {
		SSD1803A_setCursor(row, hSelectorPos);
		SSD1803A_writeCharacter(0x20);
	}
	if (col != 99) {
		SSD1803A_setCursor(row, col);
		SSD1803A_writeCharacter(0x10);
	}
	hSelectorPos = col;
}

void clr_cursors(void) {
	render_horizontalSelector(99);
	render_verticalSelector(99);
}

void screen_show(screen_selection_t screen) {
	switch (screen) {
	case SCREEN_HOME:
		currentScreen = &scrMain;
		break;
	case SCREEN_SET_LIGHTS:
		currentScreen = &scrLights;
		break;
	case SCREEN_SHOW_STORAGE_ROWS:
		currentScreen = &scrRows;
		break;
	case SCREEN_CHANGE_IP:
		currentScreen = &scrSettings;
		break;
	case SCREEN_BOOT:
		currentScreen = &scrBoot;
		break;
	default:
		return;
	}
	currentScreen->function();
}