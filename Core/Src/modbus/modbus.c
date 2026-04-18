#include "modbus/modbus.h"
#include "modbus/circularBuffer.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_uart.h"
#include "usart.h"
#include <stdint.h>

uint8_t totalSlaves;
cbuf_handle_t hcbuf_modbus;
ModbusSlave_t Slaves[16];
uint8_t MODBUS_DMA_RXData[256];

uint8_t findSlaves(void);
uint8_t findIndexForAddress(uint8_t address);

void initRegisters(void);
void findAvailableSlot(uint8_t *address, uint16_t *slot, uint8_t width);
void holdSlot(uint8_t address, uint16_t slot, uint8_t width);
void releaseSlot(uint8_t address, uint16_t slot, uint8_t width);

ModbusError_t Modbus_init(void) {
  uint8_t pdata[256] = {0};
  hcbuf_modbus = cbuf_init(pdata, 256);
  if (hcbuf_modbus == NULL) {
    return MODBUS_ERROR_INIT;
  }

  // Get input registers from all possible slave addresses
  totalSlaves = findSlaves();
  if (totalSlaves == 0) {
    return MODBUS_ERROR_INIT;
  }

  // Get holding registers and discrete inputs from all detected slaves
  initRegisters();

  // Start Receiver DMA Interrupts
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, MODBUS_DMA_RXData, 256);
  return MODBUS_OK;
}

HAL_StatusTypeDef Modbus_ChangeLedMode(uint8_t address, LEDMode_t mode) {
  uint8_t package[] = {address,
                       MODBUS_FUNC_WRITE_SINGLE_REGISTER,
                       (uint8_t)(MODBUS_MODULE_LEDMODE_ADDRESS >> 8),
                       (uint8_t)(MODBUS_MODULE_LEDMODE_ADDRESS & 0xFF),
                       (uint8_t)((uint16_t)mode >> 8),
                       (uint8_t)((uint16_t)mode & 0xFF),
                       0,
                       0};

  LOG_DEBUG(
      "Sending package to change LED mode: %02x %02x %02x %02x %02x %02x\r",
      package[0], package[1], package[2], package[3], package[4], package[5]);

  uint16_t crc = crc16(package, 6);
  package[6] = crc & 0xFF; // CRC low byte
  package[7] = crc >> 8;   // CRC high byte

  return HAL_UART_Transmit_DMA(&huart1, package, 8);
}

HAL_StatusTypeDef Modbus_StoreReel(uint8_t width) {
  uint8_t address;
  uint16_t slot;

  findAvailableSlot(&address, &slot, width);

  uint8_t package[] = {address,
                       MODBUS_FUNC_WRITE_MULTIPLE_COILS,
                       (uint8_t)(slot >> 8),
                       (uint8_t)(slot & 0xFF),
                       0x00,
                       width,
                       0x00,
                       width,
                       0,
                       0};

  for (uint8_t i = 0; i < width; i++)
    package[6] |= (1 << i);

  LOG_DEBUG("Sending package to store reel: %02x %02x %02x %02x %02x %02x %02x "
            "%02x \r",
            package[0], package[1], package[2], package[3], package[4],
            package[5], package[6], package[7]);

  uint16_t crc = crc16(package, 8);
  package[8] = crc & 0xFF; // CRC low byte
  package[9] = crc >> 8;   // CRC high byte

  return HAL_UART_Transmit_DMA(&huart1, package, 10);
}

HAL_StatusTypeDef Modbus_RetrieveReel(uint8_t address, uint8_t slot,
                                      uint8_t width) {
  uint8_t package[] = {address,
                       MODBUS_FUNC_WRITE_MULTIPLE_COILS,
                       (uint8_t)(slot >> 8),
                       (uint8_t)(slot & 0xFF),
                       0x00,
                       width,
                       0x00,
                       0x00,
                       0,
                       0};
  LOG_DEBUG(
      "Sending package to retrieve reel: %02x %02x %02x %02x %02x %02x %02x "
      "%02x \r",
      package[0], package[1], package[2], package[3], package[4], package[5],
      package[6], package[7]);

  uint16_t crc = crc16(package, 8);
  package[8] = crc & 0xFF; // CRC low byte
  package[9] = crc >> 8;   // CRC high byte

  releaseSlot(address, slot, width);

  return HAL_UART_Transmit_DMA(&huart1, package, 8);
}

HAL_StatusTypeDef Modbus_InitSlave(uint8_t address, uint8_t *registerValues, uint8_t numRegisters) {
  uint8_t package[256] = {0};
  package[0] = address;
  package[1] = MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS;
  package[2] = 0x00; // Starting register high byte
  package[3] = 0x00; // Starting register low byte
  package[4] = (numRegisters * 2) >> 8; // Byte count high byte
  package[5] = (numRegisters * 2) & 0xFF; // Byte count low byte

  for (uint8_t i = 0; i < numRegisters; i++) {
    package[6 + (i * 2)] = registerValues[i * 2];     // Register value high byte
    package[7 + (i * 2)] = registerValues[(i * 2) + 1]; // Register value low byte
  }

  uint16_t crc = crc16(package, 6 + (numRegisters * 2));
  package[6 + (numRegisters * 2)] = crc & 0xFF; // CRC low byte
  package[7 + (numRegisters * 2)] = crc >> 8;   // CRC high byte

  return HAL_UART_Transmit_DMA(&huart1, package, 8 + (numRegisters * 2));
}

HAL_StatusTypeDef Modbus_GetStatus(uint8_t address, uint64_t *status) {
  uint8_t package[] = {address,
                       MODBUS_FUNC_READ_INPUT_REGISTERS,
                       0x00,
                       0x00,
                       0x00,
                       0x01,
                       0,
                       0};

  uint16_t crc = crc16(package, 6);
  package[6] = crc & 0xFF; // CRC low byte
  package[7] = crc >> 8;   // CRC high byte

  if (HAL_UART_Transmit_DMA(&huart1, package, 8) != HAL_OK) {
    return HAL_ERROR;
  }

  uint8_t rxBuffer[5];
  if (HAL_UART_Receive(&huart1, rxBuffer, sizeof(rxBuffer), 1000) != HAL_OK) {
    return HAL_ERROR;
  }

  if (rxBuffer[1] != MODBUS_FUNC_READ_INPUT_REGISTERS) {
    return HAL_ERROR;
  }
  
  for (uint16_t i = 0; i < rxBuffer[2]; i++) {
    *status = (*status << 8) | rxBuffer[3 + i];
  }
  return HAL_OK;
}

ModbusError_t Modbus_parsePackage(cbuf_handle_t cb) {
  uint8_t slaveAddress, functionCode, coilAddress[2];

  // Read address byte
  if (cbuf_get(cb, &slaveAddress, 1) != CB_OK) {
    return MODBUS_ERR_INVALID_ADDRESS;
  }

  // Read function code byte
  if (cbuf_get(cb, &functionCode, 1) != CB_OK) {
    return MODBUS_ERR_INVALID_FUNCTION;
  }
  if (cbuf_get(cb, coilAddress, 2) != CB_OK) {
    return MODBUS_ERR_INVALID_FUNCTION;
  }

  uint8_t data[256];
  if (functionCode & MODBUS_ERROR_RESPONSE_MASK) {
    // TODO: Handle error response
  }

  switch (functionCode) {
  case MODBUS_FUNC_WRITE_SINGLE_COIL:
  case MODBUS_FUNC_WRITE_SINGLE_REGISTER:
    break;
  case MODBUS_FUNC_READ_COILS:
  case MODBUS_FUNC_READ_DISCRETE_INPUTS:
    break;
  case MODBUS_FUNC_WRITE_MULTIPLE_COILS:
  case MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS:
    break;
  default:
    return MODBUS_ERR_INVALID_FUNCTION;
  }
  // Read data bytes
  if (cbuf_get(cb, (uint8_t *)&data, 2) != CB_OK) {
    return MODBUS_ERR_INVALID_FUNCTION;
  }

  // Read CRC bytes
  uint8_t crcholder[2];
  if (cbuf_get(cb, crcholder, 2) != CB_OK) {
    return MODBUS_ERR_INVALID_FUNCTION;
  }

  uint8_t package[] = {slaveAddress, functionCode, data[0], data[1]};
  uint16_t crc = crcholder[0] | (crcholder[1] << 8);
  if (crc16(package, sizeof(package)) != crc) {
    return MODBUS_ERR_CRC_MISMATCH;
  }

  // Process based on function code
  switch (functionCode) {
  case MODBUS_FUNC_READ_HOLDING_REGISTERS:
    // TODO: Handle read holding register response

  case MODBUS_FUNC_WRITE_SINGLE_REGISTER:
    // TODO: Handle write single register response

  default:
    return MODBUS_ERR_INVALID_FUNCTION;
  }
}

ModbusError_t Modbus_handleReadHoldingRegisterResponse(uint8_t *rxFrame) {
  // TODO: Implement read holding register response handling
  return MODBUS_OK;
}

ModbusError_t Modbus_handleWriteSingleRegisterResponse(uint8_t *rxFrame) {
  // TODO: Implement write single register response handling
  return MODBUS_OK;
}

uint8_t findSlaves(void) {
  uint8_t slaveCount = 0;
  for (uint8_t i = 1; i <= 16; i++) {
    uint8_t Tx[] = {
        i, MODBUS_FUNC_READ_INPUT_REGISTERS, 0x00, 0x02, 0x00, 0x03, 0, 0};
    uint8_t Rx[11];
    uint16_t crc = crc16(Tx, 6);
    Tx[6] = crc & 0xFF; // CRC low byte
    Tx[7] = crc >> 8;   // CRC high byte
    HAL_UART_Transmit(&huart1, Tx, 8, 1000);
    if (HAL_UART_Receive(&huart1, Rx, 11, 1000) != HAL_TIMEOUT) {
      Slaves[slaveCount].address = i;
      Slaves[slaveCount].InputRegisters[0] = Rx[3] << 8 | Rx[4]; // Total slots
      Slaves[slaveCount].InputRegisters[1] = Rx[5] << 8 | Rx[6]; // Free slots
      Slaves[slaveCount++].InputRegisters[2] = Rx[7] << 8 | Rx[8]; // Status
    }
  }
  return slaveCount;
}

uint8_t findIndexForAddress(uint8_t address) {
  for (uint8_t i = 0; i < totalSlaves; i++) {
    if (Slaves[i].address == address) {
      return i;
    }
  }
  return 0xFF; // Address not found
}

void initRegisters(void) {
  for (uint8_t j = 0; j < totalSlaves; j++) {
    // Start copy holding registers
    uint8_t Tx[] = {Slaves[j].address,
                    MODBUS_FUNC_READ_HOLDING_REGISTERS,
                    0x00,
                    0x00,
                    0x00,
                    (Slaves[j].InputRegisters[0] + 1), // Max 41
                    0,
                    0},
            RxRegistersSize = (Slaves[j].InputRegisters[0] * 2) + 4,
            RxSize = (Slaves[j].InputRegisters[0]) + 5;

    uint8_t RxHR[RxRegistersSize], Rx[RxSize];
    uint16_t crc = crc16(Tx, 6);
    Tx[6] = crc & 0xFF; // CRC low byte
    Tx[7] = crc >> 8;   // CRC high byte
    HAL_UART_Transmit(&huart1, Tx, 8, 1000);
    HAL_UART_Receive(&huart1, RxHR, RxRegistersSize, 1000);
    for (uint8_t i = 0; i < RxHR[2]; i += 2) {
      Slaves[j].HoldingRegisters[i / 2] = RxHR[3 + i] << 8 | RxHR[4 + i];
    }
    // End copy holding registers

    // Start copy coils
    Tx[1] = MODBUS_FUNC_READ_COILS;
    Tx[5] = 0x28;
    crc = crc16(Tx, 6);
    Tx[6] = crc & 0xFF; // CRC low byte
    Tx[7] = crc >> 8;   // CRC high byte
    HAL_UART_Transmit(&huart1, Tx, 8, 1000);
    HAL_UART_Receive(&huart1, Rx, RxSize, 1000);
    for (uint8_t i = 0; i < Rx[2]; i++) {
      Slaves[j].Coils[i] = Rx[3 + i];
    }
  }
}

void findAvailableSlot(uint8_t *address, uint16_t *slot, uint8_t width) {
  uint8_t consecutiveZeros = 0;

  for (uint8_t i = 0; i < totalSlaves; i++) {
    // Check each bit position in the coil register
    for (uint16_t bitIndex = 0;
         bitIndex < (Slaves[i].InputRegisters[MODBUS_TOTAL_SLOTS_OFFSET]);
         bitIndex++) {
      uint8_t coilByteIndex = bitIndex / 8;
      uint8_t bitOffset = bitIndex % 8;

      // Check if this bit is 0 (space is free)
      if ((Slaves[i].Coils[coilByteIndex] & (1 << bitOffset)) == 0) {
        consecutiveZeros++;
        if (consecutiveZeros == width) {
          // Found enough consecutive zeros

          *address = Slaves[i].address;
          *slot = bitIndex - width + 1;
          holdSlot(i, *slot, width);
          return;
        }
      } else {
        consecutiveZeros = 0;
      }
    }
  }
}

void holdSlot(uint8_t address, uint16_t slot, uint8_t width) {
  for (uint8_t i = 0; i < width; i++) {
    Slaves[address].Coils[(slot + i) / 8] |= (1 << ((slot + i) % 8));
  }
}

void releaseSlot(uint8_t address, uint16_t slot, uint8_t width) {
  for (uint8_t i = 0; i < width; i++) {
    Slaves[address].Coils[(slot + i) / 8] &= ~(1 << ((slot + i) % 8));
  }
}