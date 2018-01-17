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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include <glib.h>

#define MODULE "socket"
#include "btprint.h"

#include "btle_error.h"
#include "btsocket.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define SOCKET_INVALID  (-1)
#define SOCKET_ADDRESS	"/var/run/btled"

#define SOCKET_CONN_MAX 5

enum btsocket_type {
	server = 0,
	client,
};

struct gio_mgmt{
	int desc;
	guint evt_src_id;
};
static struct {
	struct gio_mgmt gios[client +1];
	struct btsocket_param param;
	socket_rx_notifier rx_cb;
} socket_mgmt = {
	.gios = {
		[server] = {SOCKET_INVALID, 0},
		[client] = {SOCKET_INVALID, 0},
	},
	.param = {0, NULL},
};


uint8_t btsocket_send(uint8_t *data, uint8_t data_len)
{
	if (socket_mgmt.gios[client].desc == SOCKET_INVALID ||
		socket_mgmt.gios[server].desc == SOCKET_INVALID) {
		return BTLE_ERROR_INVALID_STATE;
	}

	if (!data || !data_len) {
		return BTLE_ERROR_INVALID_ARG;
	} else {
		int8_t ret;
		ret = send(socket_mgmt.gios[client].desc, data, data_len, 0);
		if (ret == -1) {
			ERR("tx: %d\n", errno);
			return BTLE_ERROR_INTERNAL;
		}
	}

	return BTLE_SUCCESS;
}

static uint8_t btsocket_mtu_negociation(void)
{
	uint8_t ret;
	uint8_t data[2];

	data[0] = 0xff & (socket_mgmt.param.mtu >> 8);
	data[1] = 0xff & socket_mgmt.param.mtu;

	ret = btsocket_send(data, sizeof(data));

	return ret;
}

static gboolean btsocket_read(GIOChannel *chan, GIOCondition cond, gpointer data)
{

	if (cond & G_IO_NVAL) {
		return FALSE;
	} else {
		int rx_len;
		uint8_t data[socket_mgmt.param.mtu];

		rx_len = recv(socket_mgmt.gios[client].desc, data, sizeof(data), MSG_PEEK);
		if (rx_len != sizeof(data)) {
			return FALSE;
		}

		rx_len = recv(socket_mgmt.gios[client].desc, data, sizeof(data), 0);
		if (rx_len != sizeof(data)) {
			return FALSE;
		}
		/* notify */
		socket_mgmt.param.rx_cb(data, rx_len);
	}
	return TRUE;
}

void btsocket_set_event(struct gio_mgmt *gio,  GIOFunc func)
{
	GIOChannel *io;

	io = g_io_channel_unix_new(gio->desc);
	g_io_channel_set_close_on_unref(io, TRUE);

	gio->evt_src_id = g_io_add_watch(io, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
			func, NULL);
	g_io_channel_unref(io);
}

static gboolean btsocket_accept_conn(GIOChannel *chan, GIOCondition cond, gpointer data)
{

	if (cond & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
		return FALSE;
	} else {
		socklen_t addrlen;
		struct sockaddr_un client_sockaddr;
		addrlen = sizeof(client_sockaddr);

		socket_mgmt.gios[client].desc = accept(socket_mgmt.gios[server].desc,
										 (struct sockaddr *) &client_sockaddr,
										 &addrlen);
		if (socket_mgmt.gios[client].desc == SOCKET_INVALID) {
			ERR("Failed to accept: %d\n", errno);
			return FALSE;
		}
		INFO("connection established: %d\n", socket_mgmt.gios[client].desc);

		btsocket_mtu_negociation();

		btsocket_set_event(&socket_mgmt.gios[client], btsocket_read);
	}

	INFO("connection established: %d\n", socket_mgmt.gios[client].desc);

	return TRUE;
}

uint8_t btsocket_server_setup(void)
{
	struct sockaddr_un server_socket = {
		.sun_family=  AF_UNIX,
	};
	memcpy(&server_socket.sun_path[0], SOCKET_ADDRESS,
		   sizeof(server_socket.sun_path));

	unlink(SOCKET_ADDRESS);
	/* gets a unique name for the socket*/
	if (bind(socket_mgmt.gios[server].desc, (struct sockaddr *) &server_socket,
			 sizeof(server_socket)) == -1) {
		ERR("Failed to bind server socket err: %d\n", errno);
		btsocket_close();
		return BTLE_ERROR_INTERNAL;
	}
	/* ready to accept incoming connections */
	if (listen(socket_mgmt.gios[server].desc, SOCKET_CONN_MAX) == -1) {
		ERR("Failed to listen server socket err: %d\n", errno);
		btsocket_close();
		return BTLE_ERROR_INTERNAL;
	}

	chmod(SOCKET_ADDRESS, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

	btsocket_set_event(&socket_mgmt.gios[server],
					   btsocket_accept_conn);

	return BTLE_SUCCESS;

}

uint8_t btsocket_init(struct btsocket_param *param)
{
	if (socket_mgmt.gios[server].desc != SOCKET_INVALID)
		return BTLE_ERROR_ALREADY;

	if (!param)
		return BTLE_ERROR_NULL_ARG;

	if (!param->rx_cb)
		return BTLE_ERROR_NULL_ARG;

	if (!param->mtu || param->mtu > SOCKET_MTU) {
		return BTLE_ERROR_INVALID_ARG;
}
	socket_mgmt.param = *param;

	/* unix stream socket */
	socket_mgmt.gios[server].desc = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_mgmt.gios[server].desc == SOCKET_INVALID) {
		return BTLE_ERROR_INTERNAL;
	}

	INFO("creating socket %d\n", socket_mgmt.gios[server].desc);
	if (setsockopt(socket_mgmt.gios[server].desc, SOL_SOCKET,  SO_REUSEADDR,
				   &(int){ 1 }, sizeof(int)) < 0 ) {
		ERR("allow reuse of local addresse failed\n");
		return BTLE_ERROR_INTERNAL;
	}

	return btsocket_server_setup();
}

void btsocket_close()
{
	uint8_t idx;

	for (idx = 0; idx < ARRAY_SIZE(socket_mgmt.gios); idx++) {
		if (socket_mgmt.gios[idx].desc != SOCKET_INVALID) {
			close(socket_mgmt.gios[idx].desc);
			socket_mgmt.gios[idx].desc = SOCKET_INVALID;
		}
		if (socket_mgmt.gios[idx].evt_src_id > 0) {
			g_source_remove(socket_mgmt.gios[idx].evt_src_id);
			socket_mgmt.gios[idx].evt_src_id = 0;
		}
	}
}
