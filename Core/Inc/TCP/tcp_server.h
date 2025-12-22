/**
 * @file tcp_server.h
 * @brief TCP server declarations for the componentStorage ECS_Master project.
 *
 * This software is provided AS-IS.
 */

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "cJSON.h"
#include "lwip.h"
#include "main.h"
// #include "modbus.h"
#include "tcp.h"
#include "tcp_request_parser.h"
#include "tcp_typedefs.h"

extern struct tcp_pcb *tcp_server_pcb;

tcp_server_error_t tcp_server_init(struct tcp_pcb *tcp_server_pcb);
tcp_server_error_t tcp_server_close(struct tcp_pcb *tpcb,
                                    tcp_server_struct_t *es);
tcp_server_error_t tcp_server_error(struct tcp_pcb *tpcb,
                                    tcp_server_struct_t *es,
                                    tcp_server_error_t err);
tcp_server_error_t tcp_server_change_address(struct tcp_pcb *tpcb,
                                             tcp_server_struct_t *es,
                                             uint8_t *ipaddr, uint8_t *netmask,
                                             uint8_t *gateway);
tcp_server_error_t tcp_server_send_response(struct tcp_pcb *tpcb,
                                            const cJSON *resp);

#endif // TCP_SERVER_H