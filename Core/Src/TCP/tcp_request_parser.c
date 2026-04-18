#include "TCP/tcp_request_parser.h"
#include "TCP/cJSON.h"
#include "TCP/tcp_server.h"
#include "TCP/tcp_typedefs.h"
#include "modbus/modbus.h"
#include "modbus/modbus_conf.h"
#include "stm32f1xx_hal_def.h"
#include <stdint.h>
#include <string.h>

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
  if (cJSON_IsArray(data)) {
    for (uint8_t i = 0; i < cJSON_GetArraySize(data); i++) {
      cJSON *item = cJSON_GetArrayItem(data, i);
      if (!cJSON_IsObject(item)) {
        LOG_ERROR("Invalid item format in init action\r");
        return err;
      }
      if (!cJSON_HasObjectItem(item, "row") || !cJSON_HasObjectItem(item, "data")) {
        LOG_ERROR("Missing row or data field in init action\r");
        return err;
      }
      if (!cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(item, "row")) || !cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(item, "data"))) {
        LOG_ERROR("Invalid data format for row or data in init action\r");
        return err;
      }
      uint8_t row = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(item, "row"));
      uint64_t value = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(item, "data"));

      if(Modbus_InitSlave(row, (uint8_t *)(&value), sizeof(uint64_t)) != HAL_OK) {
        LOG_ERROR("Failed to initialize slave at row %d with value %llu\r", row, value);
        return err;
      }
    }

    cJSON *resp = cJSON_CreateObject();

    cJSON_AddItemToObject(resp, "mode", cJSON_CreateString("init"));
    cJSON_AddItemToObject(resp, "status", cJSON_CreateNumber(HTTP_STATUS_OK));
    cJSON_AddItemToObject(resp, "data", data);
    err = tcp_server_send_response(newpcb, resp);
    

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
  LEDMode_t led;
  if(!cJSON_IsString(data) || (data->valuestring == NULL)) {
    return err;
  }
  if(strcmp(cJSON_GetStringValue(data), "OFF") == 0) {
    led = LEDMODE_OFF;
  } else if(strcmp(cJSON_GetStringValue(data), "KR") == 0) {
    led = LEDMODE_KR;
  } else if(strcmp(cJSON_GetStringValue(data), "VEGAS") == 0) {
    led = LEDMODE_VEGAS;
  } else {
    led = LEDMODE_ON;
  }
  
  Modbus_ChangeLedMode(MODBUS_SLAVE_BROADCAST, led);
  err = TCP_SERVER_ERR_OK;
  
  cJSON *resp = cJSON_CreateObject();

  cJSON_AddItemToObject(resp, "mode", cJSON_CreateString("led"));
  cJSON_AddItemToObject(resp, "status", cJSON_CreateNumber(HTTP_STATUS_OK));
  cJSON_AddItemToObject(resp, "data", data);
  err = tcp_server_send_response(newpcb, resp);

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
  uint8_t row, slot, width;
  cJSON *resp = cJSON_CreateObject();

  if(!cJSON_IsObject(data)) {
    LOG_ERROR("Invalid data format for take action\r");
    return err;
  }
  if(!cJSON_HasObjectItem(data, "row")) {
    LOG_ERROR("Missing row field in take action\r");
    return err;
  }
  if(!cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(data, "row"))) {
    LOG_ERROR("Invalid data format for row in take action\r");
    return err;
  }
  if(!cJSON_HasObjectItem(data, "slot")) {
    LOG_ERROR("Missing slot field in take action\r");
    return err;
  }
  if(!cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(data, "slot"))) {
    LOG_ERROR("Invalid data format for slot in take action\r");
    return err;
  }
  if(!cJSON_HasObjectItem(data, "width")) {
    LOG_ERROR("Missing width field in take action\r");
    return err;
  }
  if(!cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(data, "width"))) {
    LOG_ERROR("Invalid data format for width in take action\r");
    return err;
  }

  row = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(data, "row"));
  slot = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(data, "slot"));
  width = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(data, "width"));

  err = Modbus_RetrieveReel(row, slot, width);

  if (err == TCP_SERVER_ERR_OK) {
    cJSON_AddItemToObject(resp, "mode", cJSON_CreateString("init"));
    cJSON_AddItemToObject(resp, "status", cJSON_CreateNumber(HTTP_STATUS_OK));
    cJSON_AddItemToObject(resp, "data", data);
    err = tcp_server_send_response(newpcb, resp);
  }

  cJSON_Delete(resp);
  return err;
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
  uint8_t width;
  if(!cJSON_IsObject(data)) {
    LOG_ERROR("Invalid data format for put action\r");
    return err;
  }
  if(!cJSON_HasObjectItem(data, "width")) {
    LOG_ERROR("Missing width field in put action\r");
    return err;
  }
  if(!cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(data, "width"))) {
    LOG_ERROR("Invalid data format for width in put action\r");
    return err;
  }
  width = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(data, "width"));
  err = Modbus_StoreReel(width);

  if (err == TCP_SERVER_ERR_OK) {
    cJSON_AddItemToObject(resp, "mode", cJSON_CreateString("init"));
    cJSON_AddItemToObject(resp, "status", cJSON_CreateNumber(HTTP_STATUS_OK));
    cJSON_AddItemToObject(resp, "data", data);
    err = tcp_server_send_response(newpcb, resp);
  }

  cJSON_Delete(resp);
  return err;
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
  uint8_t row;
  uint64_t status;
  if(!cJSON_IsObject(data)) {
    LOG_ERROR("Invalid data format for status action\r");
    return err;
  }
  if(!cJSON_HasObjectItem(data, "row")) {
    LOG_ERROR("Missing row field in status action\r");
    return err;
  }
  if(!cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(data, "row"))) {
    LOG_ERROR("Invalid data format for row in status action\r");
    return err;
  }
  row = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(data, "row"));

  err = Modbus_GetStatus(row, &status);

  cJSON *statusItem = cJSON_CreateNumber(status);
  cJSON_AddItemToObject(data, "statusItem", statusItem);

  if (err == TCP_SERVER_ERR_OK) {
    cJSON_AddItemToObject(resp, "mode", cJSON_CreateString("init"));
    cJSON_AddItemToObject(resp, "status", cJSON_CreateNumber(HTTP_STATUS_OK));
    cJSON_AddItemToObject(resp, "data", data);
    err = tcp_server_send_response(newpcb, resp);
  }

  cJSON_Delete(resp);

  return TCP_SERVER_ERR_OK;
}
