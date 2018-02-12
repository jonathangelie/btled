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
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include <sys/epoll.h>
#include "src/shared/mainloop.h"

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

static struct {
	int8_t desc[client + 1];
	struct btsocket_param param;
	socket_rx_notifier rx_cb;
} socket_mgmt = {
	.desc = {SOCKET_INVALID, SOCKET_INVALID},
	.param = {0, NULL},
};

uint8_t btsocket_send(uint8_t *data, uint8_t data_len)
{
	if (socket_mgmt.desc[client] == SOCKET_INVALID ||
		socket_mgmt.desc[server] == SOCKET_INVALID) {
		return BTLE_ERROR_INVALID_STATE;
	}

	if (!data || !data_len) {
		return BTLE_ERROR_INVALID_ARG;
	} else {
		int8_t ret;
		ret = send(socket_mgmt.desc[client], data, data_len, 0);
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

static void btsocket_read(int fd, uint32_t events, void *user_data)
{

	if (events & (EPOLLERR | EPOLLHUP)) {
		mainloop_remove_fd(socket_mgmt.desc[client]);
		return;
	}

	if (socket_mgmt.desc[client] == SOCKET_INVALID ||
		socket_mgmt.desc[server] == SOCKET_INVALID) {
		return ;
	} else {

		int rx_len;
		uint8_t data[socket_mgmt.param.mtu];

		rx_len = recv(socket_mgmt.desc[client], data, sizeof(data), MSG_PEEK);
		if (rx_len != sizeof(data)) {
			return;
		}

		rx_len = recv(socket_mgmt.desc[client], data, sizeof(data), 0);
		if (rx_len != sizeof(data)) {
			return;
		}
		/* notify */
		socket_mgmt.param.rx_cb(data, rx_len);
	}
}

static void btsocket_accept_conn(int fd, uint32_t events, void *user_data)
{

	if (events & (EPOLLERR | EPOLLHUP)) {
		mainloop_remove_fd(socket_mgmt.desc[server]);
		return;
	} else {
		socklen_t addrlen;
		struct sockaddr_un client_sockaddr;
		addrlen = sizeof(client_sockaddr);

		socket_mgmt.desc[client] = accept(socket_mgmt.desc[server],
										 (struct sockaddr *) &client_sockaddr,
										 &addrlen);
		if (socket_mgmt.desc[client] == SOCKET_INVALID) {
			ERR("Failed to accept: %d\n", errno);
			return;
		}
		INFO("connection established: %d\n", socket_mgmt.desc[client]);
		btsocket_mtu_negociation();

		if (mainloop_add_fd(socket_mgmt.desc[client], EPOLLIN, btsocket_read,
							NULL, btsocket_close) < 0) {
			btsocket_close(NULL);
		}

	}

	INFO("connection established: %d\n", socket_mgmt.desc[client]);
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
	if (bind(socket_mgmt.desc[server], (struct sockaddr *) &server_socket,
			 sizeof(server_socket)) == -1) {
		ERR("Failed to bind server socket err: %d\n", errno);
		btsocket_close(NULL);
		return BTLE_ERROR_INTERNAL;
	}
	/* ready to accept incoming connections */
	if (listen(socket_mgmt.desc[server], SOCKET_CONN_MAX) == -1) {
		ERR("Failed to listen server socket err: %d\n", errno);
		btsocket_close(NULL);
		return BTLE_ERROR_INTERNAL;
	}

	if (mainloop_add_fd(socket_mgmt.desc[server], EPOLLIN, btsocket_accept_conn,
						NULL, btsocket_close) < 0) {
		btsocket_close(NULL);
		return BTLE_ERROR_INTERNAL;
	}

	return BTLE_SUCCESS;
}

uint8_t btsocket_init(struct btsocket_param *param)
{
	if (socket_mgmt.desc[server] != SOCKET_INVALID)
		return BTLE_ERROR_ALREADY;

	if (!param)
		return BTLE_ERROR_NULL_ARG;

	if (!param->rx_cb || !param->mtu || param->mtu > SOCKET_MTU) {
		return BTLE_ERROR_INVALID_ARG;
	}

	socket_mgmt.param = *param;

	/* unix stream socket */
	socket_mgmt.desc[server] = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_mgmt.desc[server] == SOCKET_INVALID) {
		return BTLE_ERROR_INTERNAL;
	}

	INFO("creating socket %d\n", socket_mgmt.desc[server]);
	if (setsockopt(socket_mgmt.desc[server], SOL_SOCKET,  SO_REUSEADDR,
				   &(int){ 1 }, sizeof(int)) < 0 ) {
		ERR("allow reuse of local addresse failed\n");
		return BTLE_ERROR_INTERNAL;
	} else {
		uint8_t ret;

		ret = btsocket_server_setup();
		if (ret) {
			ERR("socket init err(%d)\n", ret);
			return ret;
		}
	}
	INFO("socket init [ok]\n");

	return BTLE_SUCCESS;
}

void btsocket_close()
{
	uint8_t idx;

	for (idx = 0; idx < sizeof(socket_mgmt.desc); idx++) {
		if (socket_mgmt.desc[idx] != SOCKET_INVALID) {
			close(socket_mgmt.desc[idx]);
			socket_mgmt.desc[idx] = SOCKET_INVALID;
		}
	}
}
