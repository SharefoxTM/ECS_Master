#include "modbus/modbus.h"

uint8_t generatePackage(ModbusFrame_t *frame, uint8_t *package);

ModbusError_t Modbus_init(void) {
  // Initialization code for Modbus
  return MODBUS_OK;
}

ModbusError_t Modbus_ChangeLedMode(uint8_t address, LEDMode_t mode) {
  // Set led mode
  ModbusFrame_t frame = {};
  uint8_t package[6], packageSize;

  frame.address = address;
  frame.data = (uint16_t)mode;
  frame.startAddress = MODBUS_MODULE_LEDMODE_ADDRESS;
  frame.functionCode = MODBUS_FUNC_WRITE_SINGLE_REGISTER;

  packageSize = generatePackage(&frame, package);
  LOG_DEBUG("Sending package: %02x %02x %02x %02x %02x %02x\r", package[0],
            package[1], package[2], package[3], package[4], package[5]);
  // HAL_UART_Transmit_DMA(&huart1, package, packageSize);

  return MODBUS_OK;
}

uint8_t generatePackage(ModbusFrame_t *frame, uint8_t *package) {
  uint8_t packageSize = 0;
  package[packageSize++] = frame->address;
  package[packageSize++] = frame->functionCode;
  package[packageSize++] = frame->data >> 8;
  package[packageSize++] = frame->data & 0xFF;
  frame->crc = crc16(package, packageSize);
  package[packageSize++] = frame->crc & 0xFF;
  package[packageSize++] = frame->crc >> 8;
  return packageSize;
}