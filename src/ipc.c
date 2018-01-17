/*
 *  Copyright (C) 2018  Jonathan Gelie <contact@jonathangelie.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdint.h>
#include <string.h>

#include "btle_error.h"
#include "btsocket.h"
#include "ipc.h"
#include "cmd.h"

#define MODULE "ipc"
#include "btprint.h"

#define IPC_MTU 	(2 + IPC_DATA_LEN_MAX)

/* has to be naturally packed */
struct ipc_pkt {
	uint8_t type;
	uint8_t data_len;
	uint8_t data[0];
};

static msg_cmd_cb func;

uint8_t ipc_send(enum ipc_msg type, void *data, uint8_t data_len)
{
	if (!data) {
		return BTLE_ERROR_NULL_ARG;
}
	if (!data_len || data_len > IPC_DATA_LEN_MAX || type >= msg_unknown) {
		return BTLE_ERROR_INVALID_ARG;
	} else {
		uint8_t ret;
		uint8_t buf[IPC_MTU];
		struct ipc_pkt *pkt;

		pkt = (void*)&buf[0];
		pkt->type = type;
		pkt->data_len = data_len;
		memcpy(pkt->data, data, data_len);

		ret = btsocket_send(buf, sizeof(buf));
		return ret;
	}
}

uint8_t ipc_send_event(void *data, uint8_t data_len)
{
	return ipc_send(msg_event, data, data_len);
}

uint8_t ipc_send_cmd(void *data, uint8_t data_len)
{
	return ipc_send(msg_command_req, data, data_len);
}

uint8_t ipc_send_rsp(void *data, uint8_t data_len)
{
	return ipc_send(msg_command_resp, data, data_len);
}

uint8_t ipc_send_info(void *data, uint8_t data_len)
{
	return ipc_send(msg_info, data, data_len);
}

static void ipc_dispatch(uint8_t type, uint8_t *data, uint8_t data_len)
{
	if (type == msg_command_req && func) {
		/* command handler */
		func(data, data_len);
	} else if (type == msg_command_loopback) {
		/* loopback test */
		ipc_send(type, data, data_len);
	}
}

static void ipc_rx_cb(uint8_t *payload, uint8_t payload_len)
{
	struct ipc_pkt *pkt;

	if (payload_len != IPC_MTU) {
		/* packet could not be bigger than IPC_MTU */
		ERR("invalid payload length (%d)\n", payload_len);
		return;
	}
	pkt = (struct ipc_pkt*)&payload[0];

	INFO("IPC RCV:type(%d) | len(%d) | %s\n",
		 pkt->type, pkt->data_len, pkt->data);

	ipc_dispatch(pkt->type, pkt->data, pkt->data_len);
}

uint8_t ipc_init(msg_cmd_cb req_cb)
{
	uint8_t ret = BTLE_ERROR_NULL_ARG;

	if (req_cb) {

		struct btsocket_param param = {
			.mtu = IPC_MTU,
			.rx_cb = ipc_rx_cb,
		};
		func = req_cb;

		ret = btsocket_init(&param);
	}
	INFO("btsocket_init(%d)\n", ret);
	return ret;
}

void ipc_close(void)
{
	btsocket_close();
	func = NULL;
}
