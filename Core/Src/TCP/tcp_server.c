#include "TCP/tcp_server.h"

void error(void *arg, err_t err);

err_t accept(void *arg, struct tcp_pcb *tpcb, err_t err);
err_t recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
err_t poll(void *arg, struct tcp_pcb *tpcb);
err_t sent(void *arg, struct tcp_pcb *tpcb, uint16_t len);

err_t send_response(struct tcp_pcb *newpcb, cJSON *resp);

extern struct netif gnetif;

/**
 * @brief  Initialize the TCP server
 * @param  None
 * @retval tcp_server_error_t: Error code indicating success or type of failure
 */
tcp_server_error_t tcp_server_init(struct tcp_pcb *tcp_server_pcb) {
	LOG_INFO("Initializing TCP server\r");
	tcp_server_pcb = tcp_new();
	if (!tcp_server_pcb) {
		return TCP_SERVER_ERR_MEM;
	}
	if (tcp_bind(tcp_server_pcb, IP_ADDR_ANY, 5050) == TCP_SERVER_ERR_OK) {
		tcp_server_pcb = tcp_listen(tcp_server_pcb);
		tcp_accept(tcp_server_pcb, accept);
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
	LWIP_UNUSED_ARG(err);
	LWIP_UNUSED_ARG(tpcb);
	LWIP_UNUSED_ARG(es);
	// TODO: implement error handling
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
	char *resp_out = cJSON_Print(resp);
	tcp_write(tpcb, resp_out, strlen(resp_out), 0);
	tcp_arg(tpcb, resp_out);
	tcp_sent(tpcb, sent);
	free(resp_out);
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
	LOG_DEBUG("Bringing network interface down\r");
	netif_set_down(&gnetif);
	LOG_DEBUG("Setting new network interface address\r");
	netif_set_addr(&gnetif, &ipaddr, &netmask, &gateway);
	ethernetif_update_config(&gnetif);
	LOG_DEBUG("Bringing network interface up\r");
	netif_set_up(&gnetif);
	return TCP_SERVER_ERR_OK;
}

void error(void *arg, err_t err) {
	// TODO: implement error handling
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
	char *resp_out = cJSON_Print(resp);
	tcp_write(newpcb, resp_out, strlen(resp_out), 0);
	tcp_arg(newpcb, resp_out);
	tcp_sent(newpcb, sent);

	free(resp_out);
	return TCP_SERVER_ERR_OK;
}

void ethernetif_notify_conn_changed(struct netif *netif) {
	if (netif_is_up(netif)) {
		LOG_INFO("Network interface is up\r");
	} else {
		LOG_INFO("Network interface is down\r");
	}
}
