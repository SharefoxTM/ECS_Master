#include "../Inc/SSD1803A/screens.h"
#include "SSD1803A/SSD1803A_driver.h"
#include "Utilities/log.h"
#include "modbus/modbus.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <string.h>

void addChild(Screen_t *parent, Screen_t *child);
void render_title(Screen_t *screen);
void render_children(Screen_t *scr, uint8_t offset);
void render_verticalSelector(uint8_t row);
void render_horizontalSelector(uint8_t col);

uint8_t countChildren(Screen_t *scr);

extern struct netif gnetif;
Screen_t *currentScreen;

uint8_t vSelectorPos = 99;
uint8_t hSelectorPos = 99;

Screen_t scrMain = {
    .name = "Main menu",
    .allowedButtons = BTN_DOWN | BTN_UP | BTN_ENTER,
    .handleInput = &mainInput,
    .function = &mainRender,
    .renderOptions = OPTIONS_PRINT_HEADER | OPTIONS_PRINT_VERTICAL_SELECTOR |
                     OPTIONS_PRINT_ENTER_ICON,
};

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
    .renderOptions = OPTIONS_PRINT_VERTICAL_SELECTOR | OPTIONS_PRINT_ENTER_ICON,
};

Screen_t scrRows = {
    .name = "Rows",
    .allowedButtons = BTN_DOWN | BTN_UP | BTN_ENTER,
    .handleInput = &mainInput,
    .function = &mainRender,
    .renderOptions = OPTIONS_PRINT_HEADER | OPTIONS_PRINT_VERTICAL_SELECTOR |
                     OPTIONS_PRINT_ENTER_ICON,
};

Screen_t scrSlots = {
    .name = "Slots",
    .allowedButtons = BTN_DOWN | BTN_UP | BTN_ENTER,
    .handleInput = &rowStatusInput,
    .function = &rowStatusRender,
    .renderOptions = OPTIONS_PRINT_HEADER | OPTIONS_PRINT_ENTER_ICON,
};

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

Screen_t nextPage = {
    .name = "NEXT PAGE",
    .allowedButtons = BTN_DOWN | BTN_UP | BTN_ENTER,
    .handleInput = nextPageInput,
    .function = nextPageRender,
    .renderOptions = OPTIONS_PRINT_HEADER | OPTIONS_PRINT_VERTICAL_SELECTOR |
                     OPTIONS_PRINT_ENTER_ICON,
};

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

  currentScreen = &scrMain;
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
  static uint8_t rowNumber = 1;
  uint64_t tempNum;
  SSD1803A_clr_screen();
  LOG_VERBOSE("Rendering screen %s using row status render\r", currentScreen->name);
  if (currentScreen->renderOptions & OPTIONS_PRINT_HEADER) {
    LOG_DEBUG("Printing title\r");
    render_title(currentScreen);
  }
  char temp[16];
  sprintf(temp, "Row %d: ", rowNumber);
  LOG_DEBUG("Rendering row status for row %d\r", rowNumber);
  SSD1803A_setCursor(1, 0);
  SSD1803A_write(temp);
  SSD1803A_setCursor(2, 0);

  if(Modbus_GetStatus(rowNumber, &tempNum) != HAL_OK) {
    LOG_ERROR("Failed to get status for row %d\r", rowNumber);
    sprintf(temp, "Row unavailable");
    SSD1803A_write(temp);
    return;
  }

  for (int i = 0; i < 40; i++) {
    if (tempNum & (1 << i)) {
      SSD1803A_writeNumber(1);
    } else {
      SSD1803A_writeNumber(0);
    }
  }

}

void nextPageRender(void) {
  render_title(currentScreen);
  render_children(currentScreen, 1);
  render_verticalSelector(0);
}

uint8_t screen_getVerticalSelectorLocation(void) { return vSelectorPos; }

void screen_setVerticalSelectorLocation(uint8_t location) {
  render_verticalSelector(location);
}

uint8_t screen_getHorizontalSelectorLocation(void) { return hSelectorPos; }

void screen_setHorizontalSelectorLocation(uint8_t location) {
  render_horizontalSelector(location);
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
  // TODO: Implement the change lights functionality
  Modbus_ChangeLedMode(MODBUS_SLAVE_BROADCAST,
                       screen_getVerticalSelectorLocation());
  currentScreen = &scrMain;
  currentScreen->function();
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
    SSD1803A_writeCharacter(0x12);
  }
  hSelectorPos = col;
}

void clr_cursors(void) {
  render_horizontalSelector(99);
  render_verticalSelector(99);
}
