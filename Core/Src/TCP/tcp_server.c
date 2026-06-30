#include "TCP/tcp_server.h"
#include "TCP/tcp_typedefs.h"
#include "eeprom.h"

void error(void *arg, err_t err);

err_t accept(void *arg, struct tcp_pcb *tpcb, err_t err);
err_t recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
err_t poll(void *arg, struct tcp_pcb *tpcb);
err_t sent(void *arg, struct tcp_pcb *tpcb, uint16_t len);

err_t send_response(struct tcp_pcb *newpcb, cJSON *resp);

extern struct netif gnetif;

/**
 * @brief  Initialize the TCP server
 * @param  tcp_server_pcb: Pointer to the TCP protocol control block pointer
 * @retval tcp_server_error_t: Error code indicating success or type of failure
 */
tcp_server_error_t tcp_server_init(struct tcp_pcb **tcp_server_pcb) {
	LOG_INFO("Initializing TCP server\r");
	if (!tcp_server_pcb) {
		return TCP_SERVER_ERR_VAL;
	}

	*tcp_server_pcb = tcp_new();
	if (!*tcp_server_pcb) {
		return TCP_SERVER_ERR_MEM;
	}

	if (tcp_bind(*tcp_server_pcb, IP_ADDR_ANY, 5050) == ERR_OK) {
		*tcp_server_pcb = tcp_listen(*tcp_server_pcb);
		tcp_accept(*tcp_server_pcb, accept);
		LOG_INFO("TCP server listening on port 5050\r");
		return TCP_SERVER_ERR_OK;
	}

	return TCP_SERVER_ERR_CONN;
}

/**
 * @brief  Close the TCP server connection
 * @param  tpcb: Pointer to the TCP protocol control block
 * @param  es: Pointer to the TCP server structure
 * @retval tcp_server_error_t: Error code indicating success or type of failure
 */
tcp_server_error_t tcp_server_close(struct tcp_pcb *tpcb, tcp_server_struct_t *es) {
	tcp_arg(tpcb, NULL);
	tcp_sent(tpcb, NULL);
	tcp_recv(tpcb, NULL);
	tcp_err(tpcb, NULL);
	tcp_poll(tpcb, NULL, 0);
	if (es != NULL) {
		mem_free(es);
	}
	tcp_close(tpcb);
	return TCP_SERVER_ERR_OK;
}

/**
 *   @brief  Handle TCP server errors
 *   @param  tpcb: Pointer to the TCP protocol control block
 *   @param  es: Pointer to the TCP server structure
 *   @param  err: Error code indicating the type of error
 *   @retval tcp_server_error_t: Error code indicating success or type of
 * failure
 */
tcp_server_error_t tcp_server_error(struct tcp_pcb *tpcb, tcp_server_struct_t *es, tcp_server_error_t err) {
	LWIP_UNUSED_ARG(es);
	cJSON *resp = cJSON_CreateObject();
	cJSON_AddItemToObject(resp, "mode", cJSON_CreateString("error"));
	cJSON_AddItemToObject(resp, "status", cJSON_CreateNumber(HTTP_STATUS_INTERNAL_SERVER_ERROR));
	cJSON_AddItemToObject(resp, "data", cJSON_CreateNumber(err));
	tcp_server_send_response(tpcb, resp);
	cJSON_Delete(resp);

	return TCP_SERVER_ERR_OK;
}

/**
 *  @brief  Send an HTTP response over the TCP server
 *  @param  tpcb: Pointer to the TCP protocol control block
 *  @param  es: Pointer to the TCP server structure
 *  @param  resp: cJSON object containing the response data
 *  @retval tcp_server_error_t: Error code indicating success or type of
 *  failure
 */
tcp_server_error_t tcp_server_send_response(struct tcp_pcb *tpcb, const cJSON *resp) {
	if (!tpcb || !resp) {
		return TCP_SERVER_ERR_VAL;
	}

	char *resp_out = cJSON_Print(resp);
	if (!resp_out) {
		return TCP_SERVER_ERR_MEM;
	}

	err_t err = tcp_write(tpcb, (uint8_t *)resp_out, strlen(resp_out), TCP_WRITE_FLAG_COPY);
	free(resp_out);

	if (err != ERR_OK) {
		return TCP_SERVER_ERR_CONN;
	}

	tcp_sent(tpcb, sent);
	return TCP_SERVER_ERR_OK;
}

/**
 *   @brief  Change the TCP server's IP address, netmask, and gateway
 *   @param  ipaddr: New IP address
 *   @param  netmask: New netmask
 *   @param  gateway: New gateway
 *   @retval tcp_server_error_t: Error code indicating success or type of
 *   failure
 */
tcp_server_error_t tcp_server_change_address(ip4_addr_t ipaddr, ip4_addr_t netmask, ip4_addr_t gateway) {
	LOG_DEBUG("Setting new network interface address\r");
	netif_set_addr(&gnetif, &ipaddr, &netmask, &gateway);
	ethernetif_update_config(&gnetif);
	eeprom_write((uint32_t[]){ MAGIC_BYTES_CHECKER, ipaddr.addr, netmask.addr, gateway.addr }, 4);
	return TCP_SERVER_ERR_OK;
}

void error(void *arg, err_t err) {
	tcp_server_struct_t *es = (tcp_server_struct_t *)arg;
	LWIP_UNUSED_ARG(err);
	if (es != NULL) {
		mem_free(es);
	}
}

err_t accept(void *arg, struct tcp_pcb *tpcb, err_t err) {
	tcp_server_struct_t *es;
	LWIP_UNUSED_ARG(arg);
	LWIP_UNUSED_ARG(err);

	es = (tcp_server_struct_t *)mem_malloc(sizeof(tcp_server_struct_t));
	if (es == NULL) {
		return ERR_MEM;
	}
	es->state = TCP_SERVER_STATE_ACCEPTED;
	es->pcb = tpcb;
	es->len = 0;
	es->retries = 0;
	es->p = NULL;

	tcp_arg(tpcb, es);
	tcp_recv(tpcb, recv);
	tcp_err(tpcb, error);
	tcp_poll(tpcb, poll, 4);

	return ERR_OK;
}

err_t recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
	LWIP_UNUSED_ARG(arg);
	if (err == TCP_SERVER_ERR_OK && tpcb != NULL) {
		if (tpcb->callback_arg != NULL) {
			if (tpcb->state == ESTABLISHED) {
				if (p != NULL) {
					tcp_server_error_t error = parse_request(tpcb, p);
					pbuf_free(p);
					if (error != TCP_SERVER_ERR_OK) {
						return tcp_server_error(tpcb, tpcb->callback_arg, err);
					}
					return TCP_SERVER_ERR_OK;
				}
			}
		}
	}
	return TCP_SERVER_ERR_ABRT;
}

err_t poll(void *arg, struct tcp_pcb *tpcb) {
	tcp_server_struct_t *es = (tcp_server_struct_t *)arg;
	if (es != NULL) {
		if (es->retries++ < 4) {
			return TCP_SERVER_ERR_OK;
		}
	}
	return TCP_SERVER_ERR_ABRT;
}
err_t sent(void *arg, struct tcp_pcb *tpcb, uint16_t len) {
	tcp_server_struct_t *es = (tcp_server_struct_t *)arg;
	LWIP_UNUSED_ARG(len);
	if (es != NULL) {
		es->state = TCP_SERVER_STATE_SENT;
		tcp_server_close(tpcb, es);
		return TCP_SERVER_ERR_OK;
	}
	return TCP_SERVER_ERR_ABRT;
}

err_t send_response(struct tcp_pcb *newpcb, cJSON *resp) {
	if (!newpcb || !resp) {
		return ERR_ARG;
	}

	char *resp_out = cJSON_Print(resp);
	if (!resp_out) {
		return ERR_MEM;
	}

	err_t err = tcp_write(newpcb, (uint8_t *)resp_out, strlen(resp_out), TCP_WRITE_FLAG_COPY);
	free(resp_out);

	if (err != ERR_OK) {
		return err;
	}

	tcp_sent(newpcb, sent);
	return ERR_OK;
}

void ethernetif_notify_conn_changed(struct netif *netif) {
	if (netif_is_up(netif)) {
		LOG_INFO("Network interface is up\r");
	} else {
		LOG_INFO("Network interface is down\r");
	}
}
