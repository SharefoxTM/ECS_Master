/**
 * @file eeprom.h
 * @brief EEPROM read/write functions for the componentStorage ECS_Master project.
 *
 * This software is provided AS-IS.
 */
#ifndef EEPROM_H
#define EEPROM_H

#include "lwip.h"
#include "main.h"
#include <string.h>

#define __USE_VPN__ 0

#define MAGIC_BYTES_CHECKER 0x5a5a5a5a

extern ip4_addr_t local_ip;
extern ip4_addr_t local_subnet;
extern ip4_addr_t local_gateway;

void eeprom_init(void);
void eeprom_write(uint32_t *data, uint8_t size);
void eeprom_read(uint8_t *buffer, uint16_t size);

#endif // EEPROM_H