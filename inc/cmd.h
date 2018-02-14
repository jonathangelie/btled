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
#ifndef CMD_HEADER_H
#define CMD_HEADER_H

#define CMD_MAX_ADAPTER  16

enum cmds {
	CMD_MGMT_GET_DEVICE_INFO = 0,	/* [devid] */
	CMD_MGMT_RESET,					/* [devid] */
	CMD_MGMT_POWER,					/* [devid] (mode(u8)=0:off, 1:on) */
	CMD_MGMT_SET_LOCAL_NAME,		/* [devid | len(u8) < 31bytes | data] */
	CMD_MGMT_SET_CONNECTION_PARAM, 	/* [devid | addr | addrtype | min(u8) | max(u8)| interval(u8) | timeout(u16)] */
	CMD_MGMT_SCAN,					/* [devid | (mode(u8)=0:stop, 1:start) | timeout_ms(u16)] */
	CMD_MGMT_READ_CONTROLLER_INFO,	/* [devid] */
	CMD_MGMT_CONNECT, 				/* [devid] | addr | addr_type */

	CMD_GATTC_WRITE_CMD,			/* [devid | handle(u16) | data ] */
	CMD_GATTC_WRITE_REQ,			/* [devid | handle(u16) | data ] */
	CMD_GATTC_READ_REQ,				/* [devid | handle(u16)] */
	CMD_GATTC_SUBSCRIBE_REQ,		/* [devid | handle(u16)] | value*/
	CMD_GATTC_UNSUBSCRIBE_REQ,		/* [devid | handle(u16)] | cccd_id*/

	CMD_GATTS_ADD_SVC,
	CMD_GATTS_ADD_CHARACTERISTIC,
	CMD_GATTS_ADD_DESCRIPTOR,
	CMD_MAX, /* must be last element */
};

enum event_type {
	EVENT_CONNECTED = 0,
	EVENT_DISCONNECTED,
	EVENT_SCAN_STATUS,
	EVENT_SCAN_RESULT,
	EVENT_NEW_CONN_PARAM,
	EVENT_GATTC_NOTIFICATION,
	EVENT_GATTC_INDICATION,
	EVENT_GATTC_DISC_PRIMARY,
	EVENT_GATTC_DISC_CHAR,
	EVENT_GATTC_DISC_DESC,
};

enum state {
	STATE_DISCONNECTED,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_SCANNING,
};

struct client {
	int fd;
	struct bt_att *att;
	struct gatt_db *db;
	struct bt_gatt_client *gatt;

	uint8_t cccd_id;
};

struct cmd_adaper {
	enum state st;
	uint8_t devId;
	struct client * cli;
};

struct cmd_adaper * cmd_get_adapter_by_id(uint8_t devId);

uint8_t cmd_strtou16(uint8_t *in, uint16_t *out);

void cmd_send_status(uint8_t devId, uint8_t cmd, uint8_t ret);

void cmd_send_status_msg(uint8_t devId, uint8_t cmd, uint8_t ret,
						void *data, uint8_t data_len);
void cmd_send_event(uint8_t devId, uint8_t evt_type);
void cmd_send_event_msg(uint8_t devId, uint8_t evt_type,
						void *data, uint8_t data_len);

uint8_t cmd_server_handler(uint8_t *data, uint8_t data_len);
uint8_t cmd_server_init(void);
void cmd_server_close(void);
#endif /* CMD_HEADER_H */
