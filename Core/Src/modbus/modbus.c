#include "modbus/modbus.h"
#include "modbus/circularBuffer.h"
#include "usart.h"

uint8_t generatePackage(ModbusFrame_t *frame, uint8_t *package);
cbuf_handle_t hcbuf;

ModbusError_t Modbus_init(void) {
  // Initialization code for Modbus
  uint8_t pdata[256] = {0};
  hcbuf = circular_buf_init(pdata, 256);
  if (hcbuf == NULL) {
    return MODBUS_ERROR_INIT;
  }
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
  HAL_UART_Transmit_DMA(&huart1, package, packageSize);

  return MODBUS_OK;
}

ModbusError_t Modbus_parsePackage(Modbus_CircularBuffer_t cb) {
  uint8_t package[8], i = 0;
  if (cb.newDataFlag) {
    while (cb.head != cb.tail && i != 8) {
      package[i++] = cb.buffer[cb.head++];
      cb.head %= 256;
    }
  }

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