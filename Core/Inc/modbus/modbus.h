/**
 * @file modbus.h
 * @brief Modbus declarations for the componentStorage ECS_Master project.
 *
 * This software is provided AS-IS.
 */

#ifndef MODBUS_H
#define MODBUS_H

#include "main.h"
#include "modbus_crc.h"
#include "usart.h"

#define MODBUS_SLAVE_BROADCAST 0xF0

#define MODBUS_LEDMODE_OFF 0x0000
#define MODBUS_LEDMODE_ON 0x0001
#define MODBUS_LEDMODE_VEGAS 0x0002
#define MODBUS_LEDMODE_KR 0x0004

#define MODBUS_MODULE_CSR_ADDRESS 0x0000
#define MODBUS_MODULE_SLOT_ADDRESS 0x0001
#define MODBUS_MODULE_LEDMODE_ADDRESS 0x0029

#define MODBUS_CSR_SYSTEM_ENABLED_OFFSET 0x0000
#define MODBUS_CSR_SYSTEM_ENABLED_MASK 0x0001

#define MODBUS_CSR_QUEUE_EMPTY_OFFSET 0x0001
#define MODBUS_CSR_QUEUE_EMPTY_MASK 0x0001
#define MODBUS_CSR_QUEUE_FULL_OFFSET 0x0002
#define MODBUS_CSR_QUEUE_FULL_MASK 0x0001

#define MODBUS_CSR_SYSTEM_READY_OFFSET 0x0003
#define MODBUS_CSR_SYSTEM_READY_MASK 0x0001
#define MODBUS_CSR_SYSTEM_BUSY_OFFSET 0x0004
#define MODBUS_CSR_SYSTEM_BUSY_MASK 0x0001

#define MODBUS_CSR_CMD_ERROR_OFFSET 0x0005
#define MODBUS_CSR_CMD_ERROR_MASK 0x0001
#define MODBUS_CSR_CRC_ERROR_OFFSET 0x0006
#define MODBUS_CSR_CRC_ERROR_MASK 0x0001

#define MODBUS_CSR_SLOTCOUNT_OFFSET 0x000A
#define MODBUS_CSR_SLOTCOUNT_MASK 0x003F

/**
 * @struct ModbusFrame_t
 * @brief Represents a simplified Modbus RTU frame broken into discrete fields.
 *
 * Field descriptions:
 * @param address: 1-byte slave/server address (typically 0x01–0xF7).
 * @param functionCode: Modbus operation code (e.g. 0x03 Read Holding Registers,
 * 0x06 Write Single Register).
 * @param byteCount: Number of subsequent data bytes in the payload for
 * responses (or expected in certain requests).
 * @param startAddress: 16-bit starting register/address (big-endian in the raw
 * frame).
 * @param data: Single 16-bit register value (for single-register transactions).
 * @param crc: 16-bit Modbus RTU CRC.
 *
 * Frame order (RTU wire format):
 *   [Address][Function Code][[Byte Count]/[Start Addr Hi][Start Addr Lo]]
 *   [Data Hi][Data Lo][CRC Lo][CRC Hi]
 *
 * Notes:
 * - This structure models only single-register style frames; multi-register
 * functions require additional buffering.
 * - Always verify byteCount against functionCode expectations before trusting
 * data.
 * - Compute crc over all bytes from address through last data byte; compare
 * with received crc for integrity.
 */
typedef struct {
  uint8_t address;
  uint8_t functionCode;
  uint8_t byteCount;
  uint16_t startAddress;
  uint16_t data;
  uint16_t crc;
} ModbusFrame_t;

typedef enum Commands {
  MODBUS_INIT,
  MODBUS_GET_STATUS,
  MODBUS_RESET_SLAVE,
} Commands_t;

typedef enum LEDMode {
  LEDMODE_OFF,
  LEDMODE_ON,
  LEDMODE_VEGAS,
  LEDMODE_KR,
} LEDMode_t;

typedef enum {
  MODBUS_OK = 0,
  MODBUS_ERR_INVALID_ADDRESS,
  MODBUS_ERR_INVALID_FUNCTION,
  MODBUS_ERR_CRC_MISMATCH,
  MODBUS_ERR_TIMEOUT,
} ModbusError_t;

typedef enum {
  MODBUS_FUNC_READ_COIL = 0x01,
  MODBUS_FUNC_READ_DISCRETE_INPUT,
  MODBUS_FUNC_READ_HOLDING_REGISTERS,
  MODBUS_FUNC_READ_INPUT_REGISTERS,
  MODBUS_FUNC_WRITE_SINGLE_COIL,
  MODBUS_FUNC_WRITE_SINGLE_REGISTER,
  MODBUS_FUNC_WRITE_MULTIPLE_COILS = 0x0F,
  MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS,
} ModbusFunctionCode_t;

ModbusError_t Modbus_init(void);
ModbusError_t Modbus_ChangeLedMode(uint8_t address, LEDMode_t mode);

#endif // MODBUS_H