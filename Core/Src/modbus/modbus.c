#include "modbus/modbus.h"
#include "Utilities/log.h"
#include "main.h"
#include "modbus/circularBuffer.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_uart.h"
#include "usart.h"
#include <stdint.h>
#include <string.h>

uint8_t totalSlaves;
cbuf_handle_t hcbuf_modbus;
ModbusSlave_t Slaves[MODBUS_MAX_SLAVES];
uint8_t modbusRxDMABuffer[256];
uint8_t modbusTxDMABuffer[256];

uint8_t findSlaves(void);
uint8_t findIndexForAddress(uint8_t address);
uint8_t pauseRxDMA(void);
uint8_t readScanResponse(uint8_t expectedAddress, uint8_t *payloadOut, uint16_t payloadOutSize);

HAL_StatusTypeDef transmitBlocking(const uint8_t *data, uint16_t len, uint32_t timeout);
HAL_StatusTypeDef transmitDMA(const uint8_t *data, uint16_t len);
HAL_StatusTypeDef waitForTxComplete(uint32_t timeoutMs);
HAL_StatusTypeDef readResponsePayload(uint8_t expectedAddress, uint8_t expectedFunction, uint8_t *payloadOut,
																			uint16_t payloadOutSize, uint8_t *payloadLenOut, uint32_t timeout);

void initRegisters(void);
void findAvailableSlot(uint8_t *address, uint16_t *slot, uint8_t width);
void holdSlot(uint8_t address, uint16_t slot, uint8_t width);
void releaseSlot(uint8_t address, uint16_t slot, uint8_t width);
void resumeRxDMA(uint8_t wasPaused);
void drainRx(uint32_t timeoutMs);
void appendCrc16(uint8_t *data, uint16_t offset);

ModbusError_t Modbus_init(void) {
	LOG_INFO("Initializing Modbus\r");
	HAL_Delay(1000); // Give slaves some time to startup when poweron
	static uint8_t pdata[256] = { 0 };
	hcbuf_modbus = cbuf_init(pdata, 256);
	if (hcbuf_modbus == NULL) {
		return MODBUS_ERROR_INIT;
	}

	// Get input registers from all possible slave addresses
	totalSlaves = findSlaves();
	if (totalSlaves == 0) {
		LOG_ERROR("No Modbus slaves detected!");
		return MODBUS_ERROR_INIT;
	}

	// Get holding registers and discrete inputs from all detected slaves
	initRegisters();

	// Start Receiver DMA Interrupts
	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, modbusRxDMABuffer, 256);
	__HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
	return MODBUS_OK;
}

HAL_StatusTypeDef Modbus_ChangeLedMode(uint8_t address, LEDMode_t mode) {
	uint8_t package[] = { address,
												MODBUS_FUNC_WRITE_SINGLE_REGISTER,
												(uint8_t)(MODBUS_MODULE_LEDMODE_ADDRESS >> 8),
												(uint8_t)(MODBUS_MODULE_LEDMODE_ADDRESS & 0xFF),
												0,
												mode,
												0,
												0 };

	LOG_DEBUG("Sending package to change LED mode: %02x %02x %02x %02x %02x %02x\r", package[0], package[1], package[2],
						package[3], package[4], package[5]);

	uint16_t crc = crc16(package, 6);
	package[6] = crc & 0xFF; // CRC low byte
	package[7] = crc >> 8;	 // CRC high byte

	return transmitDMA(package, 8);
}

HAL_StatusTypeDef Modbus_StoreReel(uint8_t width) {
	uint8_t address;
	uint16_t slot;

	findAvailableSlot(&address, &slot, width);

	uint8_t package[] = { address,
												MODBUS_FUNC_WRITE_MULTIPLE_COILS,
												(uint8_t)(slot >> 8),
												(uint8_t)(slot & 0xFF),
												0x00,
												width,
												0x00,
												width,
												0,
												0 };

	for (uint8_t i = 0; i < width; i++)
		package[6] |= (1 << i);

	LOG_DEBUG("Sending package to store reel: %02x %02x %02x %02x %02x %02x %02x "
						"%02x \r",
						package[0], package[1], package[2], package[3], package[4], package[5], package[6], package[7]);

	uint16_t crc = crc16(package, 8);
	package[8] = crc & 0xFF; // CRC low byte
	package[9] = crc >> 8;	 // CRC high byte

	return transmitDMA(package, 10);
}

HAL_StatusTypeDef Modbus_RetrieveReel(uint8_t address, uint8_t slot, uint8_t width) {
	uint8_t package[] = {
		address, MODBUS_FUNC_WRITE_MULTIPLE_COILS, (uint8_t)(slot >> 8), (uint8_t)(slot & 0xFF), 0x00, width, 0x00, 0x00, 0,
		0
	};
	LOG_DEBUG("Sending package to retrieve reel: %02x %02x %02x %02x %02x %02x %02x "
						"%02x \r",
						package[0], package[1], package[2], package[3], package[4], package[5], package[6], package[7]);

	uint16_t crc = crc16(package, 8);
	package[8] = crc & 0xFF; // CRC low byte
	package[9] = crc >> 8;	 // CRC high byte

	USART1_DE_GPIO_Port->BSRR = USART1_DE_Pin; // Set DE pin high
	releaseSlot(address, slot, width);

	return transmitDMA(package, 10);
}

HAL_StatusTypeDef Modbus_InitSlave(uint8_t address, uint8_t *registerValues, uint8_t numRegisters) {
	uint8_t package[256] = { 0 };
	package[0] = address;
	package[1] = MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS;
	package[2] = 0x00;											// Starting register high byte
	package[3] = 0x00;											// Starting register low byte
	package[4] = (numRegisters * 2) >> 8;		// Byte count high byte
	package[5] = (numRegisters * 2) & 0xFF; // Byte count low byte

	for (uint8_t i = 0; i < numRegisters; i++) {
		package[6 + (i * 2)] = registerValues[i * 2];				// Register value high byte
		package[7 + (i * 2)] = registerValues[(i * 2) + 1]; // Register value low byte
	}

	uint16_t crc = crc16(package, 6 + (numRegisters * 2));
	package[6 + (numRegisters * 2)] = crc & 0xFF; // CRC low byte
	package[7 + (numRegisters * 2)] = crc >> 8;		// CRC high byte

	return transmitDMA(package, 8 + (numRegisters * 2));
}

HAL_StatusTypeDef Modbus_GetStatus(uint8_t address, uint64_t *status) {
	uint8_t package[] = { address, MODBUS_FUNC_READ_INPUT_REGISTERS, 0x00, 0x02, 0x00, 0x01, 0, 0 };

	uint16_t crc = crc16(package, 6);
	package[6] = crc & 0xFF; // CRC low byte
	package[7] = crc >> 8;	 // CRC high byte

	if (transmitDMA(package, 8) != HAL_OK) {
		return HAL_ERROR;
	}

	uint8_t payload[8] = { 0 };
	uint8_t payloadLen = 0;
	if (readResponsePayload(address, MODBUS_FUNC_READ_INPUT_REGISTERS, payload, sizeof(payload), &payloadLen, 1000) !=
			HAL_OK) {
		return HAL_ERROR;
	}

	if (payloadLen != 2) {
		return HAL_ERROR;
	}

	*status = 0;
	for (uint16_t i = 0; i < payloadLen; i++) {
		*status = (*status << 8) | payload[i];
	}

	return HAL_OK;
}

ModbusError_t Modbus_parsePackage(cbuf_handle_t cb) {
	if (cbuf_empty(cb))
		return MODBUS_OK;
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

	LOG_VERBOSE("Gotten following from buffer: 0x%x 0x%x 0x%x", slaveAddress, functionCode,
							coilAddress[0] << 8 | coilAddress[1]);

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

	LOG_VERBOSE("Gotten following from buffer: 0x%x 0x%x 0x%x", slaveAddress, functionCode, data[0] << 8 | data[1]);

	uint8_t package[] = { slaveAddress, functionCode, data[0], data[1] };
	uint16_t crc = crcholder[0] | (crcholder[1] << 8);
	cbuf_reset(cb);
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

ModbusError_t Modbus_getSlaveCoilCount(uint8_t address, uint8_t *count) {
	if (totalSlaves == 0) {
		return MODBUS_ERROR_INIT;
	}

	*count = Slaves[findIndexForAddress(address)].InputRegisters[MODBUS_TOTAL_SLOTS_OFFSET];
	return MODBUS_OK;
}

ModbusError_t Modbus_handleReadHoldingRegisterResponse(uint8_t *rxFrame) {
	// TODO: Implement read holding register response handling
	return MODBUS_OK;
}

ModbusError_t Modbus_handleWriteSingleRegisterResponse(uint8_t *rxFrame) {
	// TODO: Implement write single register response handling
	return MODBUS_OK;
}
HAL_StatusTypeDef Modbus_GetRowCoilsStatus(uint8_t address, uint32_t timeout, uint8_t *coilStatus) {
	uint16_t rowIndex = findIndexForAddress(address); // Assuming address corresponds to row index for simplicity
	if (rowIndex == 0xFF) {
		LOG_ERROR("Address %d not found among detected slaves\r", address);
		return HAL_ERROR;
	}
	uint16_t len = Slaves[rowIndex].InputRegisters[MODBUS_TOTAL_SLOTS_OFFSET];
	uint8_t package[] = {
		address, MODBUS_FUNC_READ_DISCRETE_INPUTS, 0x00, 0x00, (uint8_t)(len >> 8), (uint8_t)(len & 0xFF), 0, 0
	};
	uint16_t crc = crc16(package, 6);

	package[6] = crc & 0xFF; // CRC low byte
	package[7] = crc >> 8;	 // CRC high byte
	LOG_DEBUG("Requesting discrete input status for row %d (address %d)\r", address, address);
#ifdef LOG_LEVEL_VERBOSE
	LOG_VERBOSE("Requesting slot status for row %d (address %d): %02x %02x %02x %02x %02x %02x %02x %02x\r", address,
							address, package[0], package[1], package[2], package[3], package[4], package[5], package[6], package[7]);
	LOG_VERBOSE("Waiting for response... Timeout: %lu ms\r", timeout);
#endif

	if (transmitDMA(package, 8) != HAL_OK) {
		LOG_ERROR("Failed to send request for row %d\r", address);
		return HAL_ERROR;
	}
	LOG_DEBUG("Request sent for row %d, waiting for response...\r", address);

	uint8_t payloadLen = 0;
	uint8_t payload[256] = { 0 };

	if (readResponsePayload(address, MODBUS_FUNC_READ_DISCRETE_INPUTS, payload, sizeof(payload), &payloadLen, timeout) !=
			HAL_OK) {
		LOG_ERROR("Failed to read response for row %d\r", address);
		return HAL_ERROR;
	}

	for (uint8_t i = 0; i < payloadLen; i++) {
		coilStatus[i] = payload[i];
	}
	return HAL_OK;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1) {
		USART1_DE_GPIO_Port->BRR = USART1_DE_Pin;
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1) {
		USART1_DE_GPIO_Port->BRR = USART1_DE_Pin;
	}
}

uint8_t findSlaves(void) {
	LOG_DEBUG("Scanning for Modbus slaves...\r");
	uint8_t slaveCount = 0;
	for (uint8_t i = 1; i <= MODBUS_MAX_SLAVES; i++) {
		uint8_t Tx[] = { i, MODBUS_FUNC_READ_INPUT_REGISTERS, 0x00, 0x00, 0x00, 0x03, 0, 0 };
		uint8_t payload[6] = { 0 };
		uint16_t crc = crc16(Tx, 6);
		Tx[6] = crc & 0xFF; // CRC low byte
		Tx[7] = crc >> 8;		// CRC high byte

		drainRx(2);

		if (transmitBlocking(Tx, 8, MODBUS_RX_TIMEOUT_MS) != HAL_OK) {
			LOG_DEBUG("TX failed for address %d\r", i);
		} else {
			uint8_t isValidSlaveResponse = readScanResponse(i, payload, sizeof(payload));
			if (isValidSlaveResponse) {
				LOG_DEBUG("Found slave at address %d\r", i);
				LOG_DEBUG("  Total slots: %d\r", (payload[0] << 8) | payload[1]);
				LOG_DEBUG("  Free slots: %d\r", (payload[2] << 8) | payload[3]);
				LOG_DEBUG("  Status: 0x%02x\r", (payload[4] << 8) | payload[5]);
#ifdef LOG_LEVEL_VERBOSE
				for (int j = 0; j < 6; j++) {
					LOG_VERBOSE("    Register %f: 0x%02x\r", (float)j / 2, payload[j]);
				}
#endif
				Slaves[slaveCount].address = i;
				Slaves[slaveCount].InputRegisters[0] = (uint16_t)payload[0] << 8 | payload[1];	 // Total slots
				Slaves[slaveCount].InputRegisters[1] = (uint16_t)payload[2] << 8 | payload[3];	 // Free slots
				Slaves[slaveCount++].InputRegisters[2] = (uint16_t)payload[4] << 8 | payload[5]; // Status
				HAL_Delay(2);
			}
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

uint8_t pauseRxDMA(void) {
	if (huart1.RxState == HAL_UART_STATE_READY) {
		return 0;
	}

	if (HAL_UART_DMAStop(&huart1) == HAL_OK) {
		return 1;
	}

	return 0;
}

uint8_t readScanResponse(uint8_t expectedAddress, uint8_t *payloadOut, uint16_t payloadOutSize) {
	uint8_t header[3] = { 0 };
	uint8_t payload[64] = { 0 };
	uint8_t isValidSlaveResponse = 0;
	uint8_t wasRxDmaPaused = pauseRxDMA();

	HAL_StatusTypeDef headerStatus = HAL_UART_Receive(&huart1, header, sizeof(header), 2000);
	if (headerStatus != HAL_OK) {
		if (headerStatus == HAL_BUSY) {
			LOG_DEBUG("UART busy while reading response header from address %d\r", expectedAddress);
		} else {
			LOG_DEBUG("No response from address %d\r", expectedAddress);
		}
	} else if (header[0] != expectedAddress) {
		LOG_DEBUG("Unexpected address in response. expected=%d got=%d\r", expectedAddress, header[0]);
	} else if (header[1] == (MODBUS_FUNC_READ_INPUT_REGISTERS | MODBUS_ERROR_RESPONSE_MASK)) {
		if (HAL_UART_Receive(&huart1, payload, 2, 1000) == HAL_OK) {
			LOG_DEBUG("Slave %d exception code: %02x\r", expectedAddress, header[2]);
		}
	} else if (header[1] != MODBUS_FUNC_READ_INPUT_REGISTERS) {
		LOG_DEBUG("Unexpected function for address %d: %02x\r", expectedAddress, header[1]);
	} else if (header[2] > 6) {
		LOG_DEBUG("Unexpected byte count for address %d: %d\r", expectedAddress, header[2]);
	} else {
		uint16_t bytesToRead = (uint16_t)header[2] + 2; // Data + CRC
		if (bytesToRead > sizeof(payload)) {
			LOG_DEBUG("Response too long from address %d: %d\r", expectedAddress, bytesToRead);
		} else if (HAL_UART_Receive(&huart1, payload, bytesToRead, 1000) != HAL_OK) {
			LOG_DEBUG("Incomplete response from address %d\r", expectedAddress);
		} else {
			uint8_t rxFrame[70] = { 0 };
			rxFrame[0] = header[0];
			rxFrame[1] = header[1];
			rxFrame[2] = header[2];
			memcpy(&rxFrame[3], payload, bytesToRead);

			uint16_t rxCrc = (uint16_t)payload[header[2]] | ((uint16_t)payload[header[2] + 1] << 8);
			uint16_t calcCrc = crc16(rxFrame, (uint16_t)(3 + header[2]));
			if (rxCrc != calcCrc) {
				LOG_DEBUG("CRC mismatch from address %d\r", expectedAddress);
			} else if (header[2] < 6) {
				LOG_DEBUG("Not enough data bytes from address %d\r", expectedAddress);
			} else if (payloadOutSize < 6) {
				LOG_DEBUG("Output payload buffer too small for address %d\r", expectedAddress);
			} else {
				memcpy(payloadOut, payload, 6);
				isValidSlaveResponse = 1;
			}
		}
	}

	resumeRxDMA(wasRxDmaPaused);

	return isValidSlaveResponse;
}

HAL_StatusTypeDef transmitBlocking(const uint8_t *data, uint16_t len, uint32_t timeout) {
	USART1_DE_GPIO_Port->BSRR = USART1_DE_Pin; // Set DE pin high
	HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, (uint8_t *)data, len, timeout);
	USART1_DE_GPIO_Port->BSRR = USART1_DE_Pin << 16; // Set DE pin low
	return status;
}

HAL_StatusTypeDef transmitDMA(const uint8_t *data, uint16_t len) {
	if (len > sizeof(modbusTxDMABuffer)) {
		return HAL_ERROR;
	}

	if (huart1.gState != HAL_UART_STATE_READY) {
		return HAL_BUSY;
	}

	memcpy(modbusTxDMABuffer, data, len);
	USART1_DE_GPIO_Port->BSRR = USART1_DE_Pin; // Set DE pin high

	HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(&huart1, modbusTxDMABuffer, len);
	if (status != HAL_OK) {
		USART1_DE_GPIO_Port->BSRR = USART1_DE_Pin << 16; // Set DE pin low
	}

	return status;
}

HAL_StatusTypeDef waitForTxComplete(uint32_t timeoutMs) {
	uint32_t tickStart = HAL_GetTick();

	while (huart1.gState != HAL_UART_STATE_READY) {
		if ((HAL_GetTick() - tickStart) >= timeoutMs) {
			return HAL_TIMEOUT;
		}
	}

	return HAL_OK;
}

HAL_StatusTypeDef readResponsePayload(uint8_t expectedAddress, uint8_t expectedFunction, uint8_t *payloadOut,
																			uint16_t payloadOutSize, uint8_t *payloadLenOut, uint32_t timeout) {
	uint8_t header[3] = { 0 };
	uint8_t frameNoCrc[259] = { 0 };
	uint16_t bytesToRead;
	HAL_StatusTypeDef status = HAL_ERROR;
	uint8_t wasRxDmaPaused;

	if (payloadOut == NULL || payloadLenOut == NULL) {
		return HAL_ERROR;
	}

	// HAL_UART_DMAStop used by Modbus_PauseRxDMA also stops TX DMA, so wait for TX completion first.
	if (waitForTxComplete(timeout) != HAL_OK) {
		LOG_DEBUG("Timed out waiting for TX completion before reading response from address %d\r", expectedAddress);
		return HAL_BUSY;
	}

	wasRxDmaPaused = pauseRxDMA();

	HAL_StatusTypeDef headerStatus = HAL_UART_Receive(&huart1, header, sizeof(header), timeout);
	if (headerStatus != HAL_OK) {
		if (headerStatus == HAL_BUSY) {
			LOG_DEBUG("UART busy while reading response header from address %d\r", expectedAddress);
		}
		LOG_DEBUG("No response from address %d\r", expectedAddress);
		resumeRxDMA(wasRxDmaPaused);
		return headerStatus;
	}

	if (header[0] != expectedAddress) {
		LOG_ERROR("Unexpected address in response. expected=%d got=%d\r", expectedAddress, header[0]);
		resumeRxDMA(wasRxDmaPaused);
		return HAL_ERROR;
	}

	if (header[1] == (expectedFunction | MODBUS_ERROR_RESPONSE_MASK)) {
		LOG_DEBUG("Received exception response from address %d, code: %02x\r", expectedAddress, header[2]);
		uint8_t exceptionTail[2] = { 0 };
		HAL_UART_Receive(&huart1, exceptionTail, sizeof(exceptionTail), timeout);
		resumeRxDMA(wasRxDmaPaused);
		return HAL_ERROR;
	}

	if (header[1] != expectedFunction) {
		LOG_DEBUG("Unexpected function for address %d: %02x\r", expectedAddress, header[1]);
		resumeRxDMA(wasRxDmaPaused);
		return HAL_ERROR;
	}

	if (header[2] > payloadOutSize) {
		LOG_DEBUG("Response too long for address %d: %d\r", expectedAddress, header[2]);
		resumeRxDMA(wasRxDmaPaused);
		return HAL_ERROR;
	}

	bytesToRead = (uint16_t)header[2] + 2; // Data + CRC
	if (bytesToRead > 256) {
		resumeRxDMA(wasRxDmaPaused);
		return HAL_ERROR;
	}

	uint8_t rxBuffer[256] = { 0 };
	HAL_StatusTypeDef payloadStatus = HAL_UART_Receive(&huart1, rxBuffer, bytesToRead, timeout);
	if (payloadStatus != HAL_OK) {
		if (payloadStatus == HAL_BUSY) {
			LOG_DEBUG("UART busy while reading payload from address %d\r", expectedAddress);
		}
		resumeRxDMA(wasRxDmaPaused);
		return HAL_ERROR;
	}

	frameNoCrc[0] = header[0];
	frameNoCrc[1] = header[1];
	frameNoCrc[2] = header[2];
	memcpy(&frameNoCrc[3], rxBuffer, header[2]);

	uint16_t rxCrc = (uint16_t)rxBuffer[header[2]] | ((uint16_t)rxBuffer[header[2] + 1] << 8);
	if (crc16(frameNoCrc, (uint16_t)(3 + header[2])) != rxCrc) {
		LOG_DEBUG("CRC mismatch from address %d\r", expectedAddress);
		resumeRxDMA(wasRxDmaPaused);
		return HAL_ERROR;
	}
	LOG_DEBUG("Received valid response from address %d\r", expectedAddress);
#ifdef LOG_LEVEL_VERBOSE
	LOG_VERBOSE("Response data:");
	for (uint8_t i = 0; i < header[2]; i++) {
		LOG_VERBOSE(" %02x", rxBuffer[i]);
	}
#endif
	memcpy(payloadOut, rxBuffer, header[2]);
	*payloadLenOut = header[2];
	status = HAL_OK;

	resumeRxDMA(wasRxDmaPaused);
	return status;
}

void initRegisters(void) {
	uint8_t wasRxDmaPaused = pauseRxDMA();

	for (uint8_t j = 0; j < totalSlaves; j++) {
		// Start copy holding registers
		uint8_t Tx[] = { Slaves[j].address,
										 MODBUS_FUNC_READ_HOLDING_REGISTERS,
										 0x00,
										 0x00,
										 0x00,
										 (Slaves[j].InputRegisters[0] + 1), // Max 41
										 0,
										 0 },
						RxRegistersSize = (Slaves[j].InputRegisters[0] * 2) + 4, RxSize = (Slaves[j].InputRegisters[0]) + 5;

		uint8_t RxHR[RxRegistersSize], Rx[RxSize];
		appendCrc16(Tx, 6);
		if (transmitBlocking(Tx, 8, MODBUS_RX_TIMEOUT_MS) == HAL_OK &&
				HAL_UART_Receive(&huart1, RxHR, RxRegistersSize, MODBUS_RX_TIMEOUT_MS) == HAL_OK) {
			for (uint8_t i = 0; i < RxHR[2]; i += 2) {
				Slaves[j].HoldingRegisters[i / 2] = RxHR[3 + i] << 8 | RxHR[4 + i];
			}
			// End copy holding registers

			// Start copy coils
			Tx[1] = MODBUS_FUNC_READ_COILS;
			Tx[5] = 0x28;
			appendCrc16(Tx, 6);
			if (transmitBlocking(Tx, 8, MODBUS_RX_TIMEOUT_MS) == HAL_OK &&
					HAL_UART_Receive(&huart1, Rx, RxSize, MODBUS_RX_TIMEOUT_MS) == HAL_OK) {
				for (uint8_t i = 0; i < Rx[2]; i++) {
					Slaves[j].Coils[i] = Rx[3 + i];
				}
			} else {
				LOG_DEBUG("Failed to read coils from address %d\r", Slaves[j].address);
			}
		} else {
			LOG_DEBUG("Failed to request holding registers from address %d\r", Slaves[j].address);
		}
	}
	resumeRxDMA(wasRxDmaPaused);
}

void findAvailableSlot(uint8_t *address, uint16_t *slot, uint8_t width) {
	uint8_t consecutiveZeros = 0;

	for (uint8_t i = 0; i < totalSlaves; i++) {
		// Check each bit position in the coil register
		for (uint16_t bitIndex = 0; bitIndex < (Slaves[i].InputRegisters[MODBUS_TOTAL_SLOTS_OFFSET]); bitIndex++) {
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

void resumeRxDMA(uint8_t wasPaused) {
	if (!wasPaused) {
		return;
	}

	if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, modbusRxDMABuffer, sizeof(modbusRxDMABuffer)) == HAL_OK) {
		__HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
	}
}

void drainRx(uint32_t timeoutMs) {
	uint8_t dummy;
	while (HAL_UART_Receive(&huart1, &dummy, 1, timeoutMs) == HAL_OK) {
	}
}

void appendCrc16(uint8_t *data, uint16_t offset) {
	uint16_t crc = crc16(data, offset);
	data[offset] = crc & 0xFF;	 // CRC low byte
	data[offset + 1] = crc >> 8; // CRC high byte
}
