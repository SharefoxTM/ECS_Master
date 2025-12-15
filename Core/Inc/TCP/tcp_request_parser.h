/**
 * @file    request_parser.h
 * @brief   This file contains the request parser function declarations.
 ******************************************************************************
 * @attention
 * This software is provided AS-IS.
 */

#ifndef REQUEST_PARSER_H
#define REQUEST_PARSER_H

#include "cJSON.h"
#include "lwip.h"
#include "main.h"
#include "string.h"
#include "tcp.h"
#include "tcp_typedefs.h"

tcp_server_error_t parse_request(struct tcp_pcb *newpcb, struct pbuf *p);

#endif /* REQUEST_PARSER_H */