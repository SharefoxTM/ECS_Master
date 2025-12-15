/**
 * @file    tcp_typedefs.h
 * @brief   This file contains the TCP server type definitions.
 ******************************************************************************
 * @attention
 * This software is provided AS-IS.
 */
#ifndef TCP_TYPEDEFS_H
#define TCP_TYPEDEFS_H

typedef enum tcp_server_states {
  TCP_SERVER_STATE_NONE = 0,
  TCP_SERVER_STATE_ACCEPTED,
  TCP_SERVER_STATE_RECEIVED,
  TCP_SERVER_STATE_SENT,
  TCP_SERVER_STATE_CLOSING,
} tcp_server_states_t;

typedef enum http_status_code {
  HTTP_STATUS_OK = 200,
  HTTP_STATUS_CREATED = 201,
  HTTP_STATUS_ACCEPTED = 202,
  HTTP_STATUS_NO_CONTENT = 204,

  HTTP_STATUS_BAD_REQUEST = 400,
  HTTP_STATUS_FORBIDDEN = 403,
  HTTP_STATUS_NOT_FOUND = 404,
  HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
  HTTP_STATUS_REQUEST_TIMEOUT = 408,
  HTTP_STATUS_CONFLICT = 409,
  HTTP_STATUS_PRECONDITION_FAILED = 412,
  HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE = 413,
  HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE = 415,

  HTTP_STATUS_SERVER_ERROR = 500,
  HTTP_STATUS_NOT_IMPLEMENTED = 501,
  HTTP_STATUS_SERVICE_UNAVAILABLE = 503,
  HTTP_STATUS_INTERNAL_SERVER_ERROR = 505,
} http_status_code_t;

typedef enum tcp_server_error {
  TCP_SERVER_ERR_OK,
  TCP_SERVER_ERR_MEM,
  TCP_SERVER_ERR_BUF,
  TCP_SERVER_ERR_TIMEOUT,
  TCP_SERVER_ERR_RTE,
  TCP_SERVER_ERR_INPROGRESS,
  TCP_SERVER_ERR_VAL,
  TCP_SERVER_ERR_WOULDBLOCK,
  TCP_SERVER_ERR_USE,
  TCP_SERVER_ERR_ALREADY,
  TCP_SERVER_ERR_ISCONN,
  TCP_SERVER_ERR_CONN,
  TCP_SERVER_ERR_IF,
  TCP_SERVER_ERR_ABRT,
  TCP_SERVER_ERR_RST,
} tcp_server_error_t;

typedef struct tcp_server_struct {
  tcp_server_states_t state;
  uint16_t len;
  uint16_t retries;
  struct tcp_pcb *pcb;
  struct pbuf *p;
} tcp_server_struct_t;

#endif // TCP_TYPEDEFS_H