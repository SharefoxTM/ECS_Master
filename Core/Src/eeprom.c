/*
 * eeprom.c
 *
 *  Created on: Feb 19, 2024
 *      Author: Glenn
 */
#include "eeprom.h"

ip4_addr_t local_ip;
ip4_addr_t local_subnet;
ip4_addr_t local_gateway;

__attribute__((__section__(".config_data"))) uint32_t userConfig[4];

void eeprom_init(void) {
	LOG_INFO("Initializing EEPROM\r");
	uint32_t data[4];

	if (userConfig[0] != MAGIC_BYTES_CHECKER) {
		if (__USE_VPN__) {
			ip4addr_aton("172.17.6.6", &local_ip);
			ip4addr_aton("255.255.248.0", &local_subnet);
			ip4addr_aton("172.17.0.1", &local_gateway);
		} else {
			ip4addr_aton("192.168.69.207", &local_ip);
			ip4addr_aton("255.255.255.0", &local_subnet);
			ip4addr_aton("192.168.69.1", &local_gateway);
		}

		data[0] = MAGIC_BYTES_CHECKER;
		data[1] = local_ip.addr;
		data[2] = local_subnet.addr;
		data[3] = local_gateway.addr;
		LOG_DEBUG("Writing default network configuration to EEPROM\r");
		eeprom_write(data, 4);
	} else {
		local_ip.addr = userConfig[1];
		local_subnet.addr = userConfig[2];
		local_gateway.addr = userConfig[3];
	}
}

void eeprom_write(uint32_t *data, uint8_t size) {
	FLASH_EraseInitTypeDef FlashErase;
	uint32_t PAGEError, *addr = &userConfig[0];

	if (HAL_FLASH_Unlock() != HAL_OK) {
		LOG_ERROR("Failed to unlock FLASH\r");
		return;
	}
	LOG_DEBUG("FLASH unlocked\r");

	FlashErase.Banks = FLASH_BANK_1;
	FlashErase.NbPages = 1;
	FlashErase.TypeErase = FLASH_TYPEERASE_PAGES;
	FlashErase.PageAddress = (uint32_t)addr;

	if (HAL_FLASHEx_Erase(&FlashErase, &PAGEError) != HAL_OK) {
		LOG_ERROR("Failed to erase FLASH\r");
		HAL_FLASH_Lock();
		return;
	}
	LOG_DEBUG("FLASH erased\r");
	for (int i = 0; i < size; i++) {
		addr = &userConfig[i];
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)addr, (uint32_t)data[i]) != HAL_OK) {
			LOG_ERROR("Failed to program FLASH\r");
			HAL_FLASH_Lock();
			return;
		}
	}
	if (HAL_FLASH_Lock() != HAL_OK) {
		LOG_ERROR("Failed to lock FLASH\r");
	}
	LOG_DEBUG("FLASH locked\r");
}
