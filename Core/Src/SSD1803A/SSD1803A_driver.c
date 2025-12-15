#include "../Inc/SSD1803A/SSD1803A_driver.h"
#include "i2c.h"

uint8_t entrymode;
uint8_t displaycontrol;
uint8_t ddramStart;
uint8_t lines;

display_t disp;

void sendCommand(uint8_t cmd);
void sendData(uint8_t data);
void sendBuffer(uint8_t *buffer);
void finishCommand(void);

void SSD1803A_init(void) {
  disp.rows = 4;
  disp.columns = 16;
  SSD1803A_reset();

  sendCommand(COMMAND_8BIT_4LINES_RE1_IS0);
  sendCommand(COMMAND_4LINES);
  sendCommand(COMMAND_BOTTOM_VIEW);
  sendCommand(COMMAND_BS1_1);
  sendCommand(COMMAND_8BIT_4LINES_RE0_IS1);
  sendCommand(COMMAND_BS0_1);
  sendCommand(COMMAND_FOLLOWER_CONTROL);
  sendCommand(COMMAND_POWER_CONTROL);
  sendCommand(COMMAND_CONTRAST_DEFAULT);
  sendCommand(COMMAND_8BIT_4LINES_RE0_IS0);
  sendCommand(COMMAND_DISPLAY | COMMAND_DISPLAY_ON | COMMAND_CURSOR_OFF | COMMAND_BLINK_OFF);

  sendCommand(COMMAND_8BIT_4LINES_RE1_IS0);
  sendCommand(COMMAND_ROM_SELECT);
  sendData(COMMAND_ROM_A);
  sendCommand(COMMAND_8BIT_4LINES_RE0_IS0);

  SSD1803A_clr_screen();
  // Screens_show(SCREEN_OFF);
}

void SSD1803A_reset(void) {
  SCREEN_RST_GPIO_Port->BSRR = SCREEN_RST_Pin;
  HAL_Delay(10);
  SCREEN_RST_GPIO_Port->BSRR = SCREEN_RST_Pin << 16;
  HAL_Delay(5);
  SCREEN_RST_GPIO_Port->BSRR = SCREEN_RST_Pin;
  HAL_Delay(10);
}

void SSD1803A_clr_screen(void) { sendCommand(COMMAND_CLEAR_DISPLAY); }

void SSD1803A_home(void) {
  sendCommand(COMMAND_8BIT_4LINES_RE0_IS0);
  sendCommand(COMMAND_RETURN_HOME);
}

void SSD1803A_setCursor(uint8_t row, uint8_t column) { sendCommand((ADDRESS_DDRAM | (row * 0x20 + column))); }

void SSD1803A_setMode(displayMode_t mode) {
  switch (mode) {
  case VIEW_TOP:
    ddramStart = ADDRESS_DDRAM + ADDRESS_DDRAM_TOP_OFFSET;
    sendCommand(COMMAND_8BIT_4LINES_RE1_IS0);
    sendCommand(COMMAND_TOP_VIEW);
    finishCommand();
    break;
  case VIEW_BOTTOM:
    ddramStart = ADDRESS_DDRAM;
    sendCommand(COMMAND_8BIT_4LINES_RE1_IS0);
    sendCommand(COMMAND_BOTTOM_VIEW);
    finishCommand();
    break;
  case DISPLAY_ON:
    displaycontrol |= COMMAND_DISPLAY_ON;
    sendCommand(COMMAND_8BIT_4LINES_RE0_IS0);
    sendCommand((COMMAND_DISPLAY | displaycontrol));
    finishCommand();
    break;
  case DISPLAY_OFF:
    displaycontrol &= ~COMMAND_DISPLAY_ON;
    sendCommand(COMMAND_8BIT_4LINES_RE0_IS0);
    sendCommand((COMMAND_DISPLAY | displaycontrol));
    finishCommand();
    break;
  case CURSOR_ON:
    displaycontrol |= COMMAND_CURSOR_ON;
    sendCommand(COMMAND_8BIT_4LINES_RE0_IS1);
    sendCommand((COMMAND_DISPLAY | displaycontrol));
    finishCommand();
    break;
  case CURSOR_OFF:
    displaycontrol &= ~COMMAND_CURSOR_ON;
    sendCommand(COMMAND_8BIT_4LINES_RE0_IS1);
    sendCommand((COMMAND_DISPLAY | displaycontrol));
    finishCommand();
    break;
  case BLINK_ON:
    displaycontrol |= COMMAND_BLINK_ON;
    sendCommand(COMMAND_8BIT_4LINES_RE0_IS1);
    sendCommand((COMMAND_DISPLAY | displaycontrol));
    finishCommand();
    break;
  case BLINK_OFF:
    displaycontrol &= ~COMMAND_BLINK_ON;
    sendCommand(COMMAND_8BIT_4LINES_RE0_IS1);
    sendCommand((COMMAND_DISPLAY | displaycontrol));
    finishCommand();
    break;
  case DISPLAY_SHIFT_LEFT:
    sendCommand((COMMAND_SHIFT | COMMAND_DISPLAY_SHIFT_LEFT));
    break;
  case DISPLAY_SHIFT_RIGHT:
    sendCommand((COMMAND_SHIFT | COMMAND_DISPLAY_SHIFT_RIGHT));
    break;
  case CURSOR_SHIFT_LEFT:
    sendCommand((COMMAND_SHIFT | COMMAND_CURSOR_SHIFT_LEFT));
    break;
  case CURSOR_SHIFT_RIGHT:
    sendCommand((COMMAND_SHIFT | COMMAND_CURSOR_SHIFT_RIGHT));
    break;
  case LEFT_TO_RIGHT:
    entrymode |= ENTRY_MODE_LEFT_TO_RIGHT;
    sendCommand((COMMAND_ENTRY_MODE_SET | entrymode));
    break;
  case RIGHT_TO_LEFT:
    entrymode &= ~ENTRY_MODE_LEFT_TO_RIGHT;
    sendCommand((COMMAND_ENTRY_MODE_SET | entrymode));
    break;
  case AUTOSCROLL_ON:
    entrymode |= ENTRY_MODE_SHIFT_INCREMENT;
    sendCommand((COMMAND_ENTRY_MODE_SET | entrymode));
    break;
  case AUTOSCROLL_OFF:
    entrymode &= ~ENTRY_MODE_SHIFT_INCREMENT;
    sendCommand((COMMAND_ENTRY_MODE_SET | entrymode));
    break;
  case CONTRAST:
    sendCommand(COMMAND_8BIT_4LINES_RE0_IS1);
    sendCommand(COMMAND_POWER_CONTROL);
    sendCommand(COMMAND_CONTRAST_DEFAULT);
    finishCommand();
    break;
  case LINES_4:
    sendCommand(COMMAND_8BIT_4LINES_RE0_IS0);
    lines = 4;
    break;
  case LINES_3_1:
    sendCommand(COMMAND_8BIT_4LINES_RE1_IS0);
    sendCommand(COMMAND_3LINES_TOP);
    sendCommand(COMMAND_8BIT_4LINES_RE0_IS0_DH1);
    lines = 3;
    break;
  case LINES_3_2:
    sendCommand(COMMAND_8BIT_4LINES_RE1_IS0);
    sendCommand(COMMAND_3LINES_MIDDLE);
    sendCommand(COMMAND_8BIT_4LINES_RE0_IS0_DH1);
    lines = 3;
    break;
  case LINES_3_3:
    sendCommand(COMMAND_8BIT_4LINES_RE1_IS0);
    sendCommand(COMMAND_3LINES_BOTTOM);
    sendCommand(COMMAND_8BIT_4LINES_RE0_IS0_DH1);
    lines = 3;
    break;
  case LINES_2:
    sendCommand(COMMAND_8BIT_4LINES_RE1_IS0);
    sendCommand(COMMAND_2LINES);
    sendCommand(COMMAND_8BIT_4LINES_RE0_IS0_DH1);
    lines = 2;
    break;
  case SET_ROM_A:
    sendCommand(COMMAND_8BIT_4LINES_RE1_IS0);
    sendCommand(COMMAND_ROM_SELECT);
    sendData(COMMAND_ROM_A);
    finishCommand();
    break;
  case SET_ROM_B:
    sendCommand(COMMAND_8BIT_4LINES_RE1_IS0);
    sendCommand(COMMAND_ROM_SELECT);
    sendData(COMMAND_ROM_B);
    finishCommand();
    break;
  case SET_ROM_C:
    sendCommand(COMMAND_8BIT_4LINES_RE1_IS0);
    sendCommand(COMMAND_ROM_SELECT);
    sendData(COMMAND_ROM_C);
    finishCommand();
    break;
  }
}

void SSD1803A_setContrast(uint8_t value) {
  sendCommand(COMMAND_8BIT_4LINES_RE0_IS1);
  sendCommand(0x70 | (value & 0x0F));
  sendCommand(COMMAND_POWER_ICON_CONTRAST | ((value >> 4) & 0x03));
  finishCommand();
}

void SSD1803A_createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x07;
  sendCommand(ADDRESS_CGRAM | (location << 3));
  sendBuffer(charmap);
}

void SSD1803A_write(char *string) {
  for (uint8_t i = 0; i < strlen(string); i++) {
    uint8_t data = string[i];
    uint8_t i2c_data[2];
    i2c_data[0] = MODE_DATA;
    i2c_data[1] = data;

    HAL_I2C_Master_Transmit(&hi2c1, displayAddress, i2c_data, 2, HAL_MAX_DELAY);
  }
}

void SSD1803A_writeCharacter(char character) {
  uint8_t i2c_data[2];
  i2c_data[0] = MODE_DATA;
  i2c_data[1] = character;

  HAL_I2C_Master_Transmit(&hi2c1, displayAddress, i2c_data, 2, HAL_MAX_DELAY);
}

void SSD1803A_writeNumber(uint8_t num) {
  char printer[3];
  sprintf(printer, "%d", num);
  SSD1803A_write(printer);
}

void SSD1803A_writeByte(uint8_t byte) {
  char printer[4];
  sprintf(printer, "%03d", byte);
  SSD1803A_write(printer);
}

void finishCommand(void) {
  if (lines == 4) {
    sendCommand(COMMAND_8BIT_4LINES_RE0_IS0);
  } else {
    sendCommand(COMMAND_8BIT_4LINES_RE0_IS0_DH1);
  }
}

void sendCommand(uint8_t cmd) {
  uint8_t i2c_data[2];
  i2c_data[0] = MODE_COMMAND;
  i2c_data[1] = cmd;
  HAL_I2C_Master_Transmit(&hi2c1, displayAddress, i2c_data, 2, HAL_MAX_DELAY);
}

void sendData(uint8_t data) {
  uint8_t i2c_data[2];
  i2c_data[0] = MODE_DATA;
  i2c_data[1] = data;
  HAL_I2C_Master_Transmit(&hi2c1, displayAddress, i2c_data, 2, HAL_MAX_DELAY);
}

void sendBuffer(uint8_t *buffer) {
  for (uint8_t i = 0; i < strlen((char *)buffer); i++) {
    sendData(buffer[i]);
  }
}