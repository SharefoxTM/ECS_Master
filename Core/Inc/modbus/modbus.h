/**
 * @file modbus.h
 * @brief Modbus declarations for the componentStorage ECS_Master project.
 *
 * This software is provided AS-IS.
 */

#ifndef MODBUS_H
#define MODBUS_H

#include "main.h"
#include "modbus/circularBuffer.h"
#include "modbus_conf.h"
#include "modbus_crc.h"
#include "usart.h"

extern cbuf_handle_t hcbuf_modbus;

ModbusError_t Modbus_init(void);
ModbusError_t Modbus_parsePackage(cbuf_handle_t cb);
ModbusError_t Modbus_getSlaveCoilCount(uint8_t address, uint8_t *count);
HAL_StatusTypeDef Modbus_ChangeLedMode(uint8_t address, LEDMode_t mode);
HAL_StatusTypeDef Modbus_StoreReel(uint8_t width);
HAL_StatusTypeDef Modbus_RetrieveReel(uint8_t address, uint8_t position, uint8_t width);
HAL_StatusTypeDef Modbus_GetRowCoilsStatus(uint8_t address, uint32_t timeout, uint8_t *coilStatus);
HAL_StatusTypeDef Modbus_GetStatus(uint8_t address, uint64_t *status);
HAL_StatusTypeDef Modbus_InitSlave(uint8_t address, uint8_t *registerValues, uint8_t numRegisters);

#endif // MODBUS_H