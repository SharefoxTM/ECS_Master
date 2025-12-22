#include "TCP/tcp_request_parser.h"
#include "TCP/cJSON.h"
#include "TCP/tcp_server.h"
#include "TCP/tcp_typedefs.h"

tcp_server_error_t parse_init(struct tcp_pcb *newpcb, cJSON *data);
tcp_server_error_t parse_led(struct tcp_pcb *newpcb, cJSON *data);
tcp_server_error_t parse_take(struct tcp_pcb *newpcb, cJSON *data);
tcp_server_error_t parse_put(struct tcp_pcb *newpcb, cJSON *data);
tcp_server_error_t parse_status(struct tcp_pcb *newpcb, cJSON *data);

/**
 * @brief  Parse incoming TCP requests
 * @param  newpcb: Pointer to the TCP protocol control block
 * @param  p: Pointer to the pbuf containing the incoming data
 * Should contain a JSON object with "action" and "data" fields.
 * @retval tcp_server_error_t: Error code indicating success or type of failure
 */
tcp_server_error_t parse_request(struct tcp_pcb *newpcb, struct pbuf *p) {
  // Store everything in the correct holder
  cJSON *json = cJSON_Parse((char *)p->payload);
  if (json == NULL) {
    return TCP_SERVER_ERR_VAL;
  }

  cJSON *action = cJSON_GetObjectItemCaseSensitive(json, "action");
  if (!cJSON_IsString(action) || (action->valuestring == NULL)) {
    cJSON_Delete(json);
    return TCP_SERVER_ERR_VAL;
  }
  cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "data");
  if (!cJSON_IsObject(data) &&
      (!cJSON_IsString(data) || (data->valuestring == NULL))) {
    cJSON_Delete(json);
    return TCP_SERVER_ERR_VAL;
  }

  // Start parsing based on action
  tcp_server_error_t result = TCP_SERVER_ERR_OK;

  if (strcmp(action->valuestring, "led") == 0) {
    result = parse_led(newpcb, data);
  } else if (strcmp(action->valuestring, "take") == 0) {
    result = parse_take(newpcb, data);
  } else if (strcmp(action->valuestring, "put") == 0) {
    result = parse_put(newpcb, data);
  } else if (strcmp(action->valuestring, "status") == 0) {
    result = parse_status(newpcb, data);
  } else if (strcmp(action->valuestring, "init") == 0) {
    result = parse_init(newpcb, data);
  } else {
    result = TCP_SERVER_ERR_VAL;
  }

  cJSON_Delete(json);
  return result;
}

/**
 * @fn tcp_server_error_t parse_init(struct tcp_pcb*, cJSON* data)
 * @brief Inits rows to a certain binary value.
 * @param newpcb: the TCP protocol control block
 * @param data : the JSON data
 * @return tcp_server_error_t: Error code indicating success or type of failure
 */
tcp_server_error_t parse_init(struct tcp_pcb *newpcb, cJSON *data) {
  tcp_server_error_t err = TCP_SERVER_ERR_VAL;
  if (cJSON_IsObject(data)) {
    cJSON *resp = cJSON_CreateObject();
    // TODO: implement init logic here
    if (err == TCP_SERVER_ERR_OK) {
      cJSON_AddItemToObject(resp, "mode", cJSON_CreateString("init"));
      cJSON_AddItemToObject(resp, "status", cJSON_CreateNumber(HTTP_STATUS_OK));
      cJSON_AddItemToObject(resp, "data", data);
      err = tcp_server_send_response(newpcb, resp);
    }

    cJSON_Delete(resp);
  }
  return err;
}

/**
 * @fn tcp_server_error_t parse_led(struct tcp_pcb*, cJSON* data)
 * @brief Sends an led command to the modbus.
 * @param newpcb: the TCP protocol control block
 * @param data : the JSON data
 * @return tcp_server_error_t: Error code indicating success or type of failure
 */
tcp_server_error_t parse_led(struct tcp_pcb *newpcb, cJSON *data) {
  tcp_server_error_t err = TCP_SERVER_ERR_VAL;
  // TODO: implement LED control logic here
  cJSON *resp = cJSON_CreateObject();
  if (err == TCP_SERVER_ERR_OK) {
    cJSON_AddItemToObject(resp, "mode", cJSON_CreateString("led"));
    cJSON_AddItemToObject(resp, "status", cJSON_CreateNumber(HTTP_STATUS_OK));
    cJSON_AddItemToObject(resp, "data", data);
    err = tcp_server_send_response(newpcb, resp);
  }

  cJSON_Delete(resp);
  return TCP_SERVER_ERR_OK;
}

/**
 * @fn tcp_server_error_t parse_take(struct tcp_pcb*, cJSON* data)
 * @brief Takes a reel from a slave row.
 * @param newpcb: the TCP protocol control block
 * @param data : the JSON data
 * @return tcp_server_error_t: Error code indicating success or type of failure
 */
tcp_server_error_t parse_take(struct tcp_pcb *newpcb, cJSON *data) {
  tcp_server_error_t err = TCP_SERVER_ERR_VAL;
  cJSON *resp = cJSON_CreateObject();
  // TODO: implement retrieve logic here
  if (err == TCP_SERVER_ERR_OK) {
    cJSON_AddItemToObject(resp, "mode", cJSON_CreateString("init"));
    cJSON_AddItemToObject(resp, "status", cJSON_CreateNumber(HTTP_STATUS_OK));
    cJSON_AddItemToObject(resp, "data", data);
    err = tcp_server_send_response(newpcb, resp);
  }

  cJSON_Delete(resp);
  return TCP_SERVER_ERR_OK;
}

/**
 * @fn tcp_server_error_t parse_put(struct tcp_pcb*, cJSON* data)
 * @brief Stores a reel in a slave row.
 * @param newpcb: the TCP protocol control block
 * @param data : the JSON data
 * @return tcp_server_error_t: Error code indicating success or type of failure
 */
tcp_server_error_t parse_put(struct tcp_pcb *newpcb, cJSON *data) {
  tcp_server_error_t err = TCP_SERVER_ERR_VAL;
  cJSON *resp = cJSON_CreateObject();
  // TODO: implement put logic here
  if (err == TCP_SERVER_ERR_OK) {
    cJSON_AddItemToObject(resp, "mode", cJSON_CreateString("init"));
    cJSON_AddItemToObject(resp, "status", cJSON_CreateNumber(HTTP_STATUS_OK));
    cJSON_AddItemToObject(resp, "data", data);
    err = tcp_server_send_response(newpcb, resp);
  }

  cJSON_Delete(resp);
  return TCP_SERVER_ERR_OK;
}

/**
 * @fn tcp_server_error_t parse_status(struct tcp_pcb*, cJSON* data)
 * @brief Retrieves the binary value from a slave row.
 * @param newpcb: the TCP protocol control block
 * @param data : the JSON data
 * @return tcp_server_error_t: Error code indicating success or type of failure
 */
tcp_server_error_t parse_status(struct tcp_pcb *newpcb, cJSON *data) {
  tcp_server_error_t err = TCP_SERVER_ERR_VAL;
  cJSON *resp = cJSON_CreateObject();
  // TODO: implement status logic here
  if (err == TCP_SERVER_ERR_OK) {
    cJSON_AddItemToObject(resp, "mode", cJSON_CreateString("init"));
    cJSON_AddItemToObject(resp, "status", cJSON_CreateNumber(HTTP_STATUS_OK));
    cJSON_AddItemToObject(resp, "data", data);
    err = tcp_server_send_response(newpcb, resp);
  }

  cJSON_Delete(resp);

  return TCP_SERVER_ERR_OK;
}
