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
#ifndef IPC_HEADER_H
#define IPC_HEADER_H

#define IPC_DATA_LEN_MAX	100

enum ipc_msg {
	msg_command_req = 0,
	msg_command_resp,
	msg_event,
	msg_info,
	msg_command_loopback,
	msg_unknown,
};

uint8_t ipc_send(enum ipc_msg type, void *data, uint8_t data_len);

uint8_t ipc_send_event(void *data, uint8_t data_len);
uint8_t ipc_send_cmd(void *data, uint8_t data_len);
uint8_t ipc_send_rsp(void *data, uint8_t data_len);
uint8_t ipc_send_info(void *data, uint8_t data_len);

uint8_t ipc_init(void);

#endif /* IPC_HEADER_H */
