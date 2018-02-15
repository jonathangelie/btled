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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <stdbool.h>

#include <unistd.h>

#include "lib/bluetooth.h"
#include "lib/sdp.h"
#include "lib/l2cap.h"
#include "lib/uuid.h"
#include "lib/hci.h"
#include "lib/hci_lib.h"

#include "lib/mgmt.h"
#include "src/shared/mgmt.h"
#include "src/shared/util.h"
#include "src/shared/att.h"
#include "src/shared/queue.h"
#include "src/shared/gatt-db.h"
#include "src/shared/gatt-client.h"

#define MODULE "gattc"
#include "btprint.h"

#include "btle_error.h"
#include "gattc.h"
#include "cmd.h"

#define ATT_CID 4
#define ATT_DEFAULT_LE_MTU 23

#define GATT_INVALID      0x00
#define GATT_NOTIFICATION 0x01
#define GATT_INDICATION   0x02

static void gattc_write_complete(bool success, uint8_t att_ecode,
				 void *user_data)
{
	struct cmd_adaper *adapter = user_data;
	uint8_t status = (success ? 0 : 1);

	cmd_send_status(adapter->devId, CMD_GATTC_WRITE_REQ, status);
}

static uint8_t gattc_write(uint8_t cmd, uint8_t devId,
			   uint8_t *data, uint8_t data_len)
{
	uint8_t ret = BTLE_SUCCESS;
	struct cmd_adaper *adapter;

	adapter = cmd_get_adapter_by_id(devId);
	if (!adapter)
		ret = BTLE_ERROR_INVALID_ARG;
	else if (!adapter->cli)
		ret = BTLE_ERROR_INVALID_STATE;

	if (!ret) {
		uint16_t handle;
		uint8_t len;
		struct bt_gatt_client *gatt = adapter->cli->gatt;

		cmd_strtou16(&data[0], &handle);
		len = data_len - 2;

		if (CMD_GATTC_WRITE_CMD == cmd) {
			bool signed_write = false;

			if (!bt_gatt_client_write_without_response(gatt, handle,
								   signed_write,
								   &data[2],
								   len)) {
				ret = BTLE_ERROR_INTERNAL;
			}
		} else if (cmd == CMD_GATTC_SUBSCRIBE_REQ) {
			if (!bt_gatt_client_write_value(gatt, handle,
							&data[2], len,
							gattc_write_complete,
							adapter, NULL)) {
				ret = BTLE_ERROR_INTERNAL;
			}
		}
	}

	if (ret || CMD_GATTC_WRITE_CMD == cmd)
		cmd_send_status(devId, cmd, ret);

	return ret;
}

uint8_t gattc_write_req(uint8_t devId, uint8_t *data, uint8_t data_len)
{
	return gattc_write(CMD_GATTC_WRITE_REQ, devId, data, data_len);
}

uint8_t gattc_write_cmd(uint8_t devId, uint8_t *data, uint8_t data_len)
{
	return gattc_write(CMD_GATTC_WRITE_CMD, devId, data, data_len);
}

static void gattc_subscribe_complete(uint16_t att_ecode, void *user_data)
{
	struct cmd_adaper *adapter = user_data;

	cmd_send_status_msg(adapter->devId, CMD_GATTC_SUBSCRIBE_REQ, att_ecode,
			    &adapter->cli->cccd_id, 1);
}

void on_gattc_notification(uint16_t value_handle, const uint8_t *value,
			   uint16_t length, void *user_data)
{
	struct cmd_adaper *adapter = user_data;

	struct {
		uint16_t value_handle;
		uint16_t data_len;
		uint8_t cccd_id;
		uint8_t data[length];
	} msg;

	msg.cccd_id = adapter->cli->cccd_id;
	msg.value_handle = value_handle;
	msg.data_len = length;
	memcpy(&msg.data[0], value, sizeof(msg.data));

	cmd_send_event_msg(adapter->devId, EVENT_GATTC_NOTIFICATION,
			   &msg, sizeof(msg));
}

uint8_t gattc_subscribe_req(uint8_t devId, uint8_t *data, uint8_t data_len)
{
	uint8_t ret = BTLE_ERROR_INTERNAL;
	struct cmd_adaper *adapter;

	adapter = cmd_get_adapter_by_id(devId);
	if (!adapter) {
		ret = BTLE_ERROR_INVALID_ARG;
	} else if (!adapter->cli) {
		ret = BTLE_ERROR_INVALID_STATE;
	} else {
		if (data[0] == GATT_NOTIFICATION) {
			uint16_t chrc_value_handle;
			uint8_t cccd_id;
			struct bt_gatt_client *gatt;

			gatt = adapter->cli->gatt;
			cmd_strtou16(&data[1], &chrc_value_handle);

			cccd_id = bt_gatt_client_register_notify(
				gatt,
				chrc_value_handle,
				gattc_subscribe_complete,
				on_gattc_notification,
				adapter, NULL);
			if (cccd_id) {
				adapter->cli->cccd_id = cccd_id;
				ret = BTLE_SUCCESS;
			}
		}
	}

	if (ret)
		cmd_send_status(devId, CMD_GATTC_SUBSCRIBE_REQ, ret);

	return ret;
}

uint8_t gattc_unsubscribe_req(uint8_t devId, uint8_t *data, uint8_t data_len)
{
	uint8_t ret = BTLE_ERROR_INTERNAL;
	struct cmd_adaper *adapter;

	adapter = cmd_get_adapter_by_id(devId);
	if (!adapter) {
		ret = BTLE_ERROR_INVALID_ARG;
	} else if (!adapter->cli) {
		ret = BTLE_ERROR_INVALID_STATE;
	} else {
		uint8_t cccd_id;

		cccd_id = data[0];

		if (bt_gatt_client_unregister_notify(adapter->cli->gatt,
						     cccd_id)) {
			ret = BTLE_SUCCESS;
		}
	}

	cmd_send_status(devId, CMD_GATTC_UNSUBSCRIBE_REQ, ret);

	return ret;
}

static void gattc_read_complete(bool success, uint8_t att_ecode,
				const uint8_t *value, uint16_t length,
				void *user_data)
{
	struct cmd_adaper *adapter = user_data;
	uint8_t status = (success ? 0 : 1);

	cmd_send_status_msg(adapter->devId, CMD_GATTC_READ_REQ, status,
			    (void *)value, length);
}

uint8_t gattc_read_req(uint8_t devId, uint8_t *data, uint8_t data_len)
{
	uint8_t ret = BTLE_SUCCESS;
	struct cmd_adaper *adapter;

	adapter = cmd_get_adapter_by_id(devId);
	if (!adapter)
		ret = BTLE_ERROR_INVALID_ARG;
	else if (!adapter->cli)
		ret = BTLE_ERROR_INVALID_STATE;

	if (!ret) {
		uint16_t handle;

		cmd_strtou16(&data[0], &handle);

		if (!bt_gatt_client_read_value(adapter->cli->gatt, handle,
					       gattc_read_complete, adapter,
					       NULL)) {
			ret = BTLE_ERROR_INTERNAL;
		}
	}

	if (ret)
		cmd_send_status(devId, CMD_GATTC_READ_REQ, ret);

	return ret;
}

static void gattc_disconnect_cb(int err, void *user_data)
{
	struct cmd_adaper *adapter = user_data;

	INFO("[%d] Device disconnected: %s\n", adapter->devId, strerror(err));
	cmd_send_event_msg(adapter->devId, EVENT_DISCONNECTED,
			   strerror(err), strlen(strerror(err)));

	memset(adapter, 0, sizeof(*adapter));
}

static void service_added_cb(struct gatt_db_attribute *attr, void *user_data)
{
	struct cmd_adaper *adapter = user_data;
	bt_uuid_t uuid;

	return;
	struct {
		/* must be naturally packed */
		uint16_t start;
		uint16_t end;
		char uuid_str[MAX_LEN_UUID_STR + 1];
	} msg;

	gatt_db_attribute_get_service_uuid(attr, &uuid);
	bt_uuid_to_string(&uuid, msg.uuid_str, sizeof(msg.uuid_str));
	gatt_db_attribute_get_service_handles(attr, &msg.start, &msg.end);

	cmd_send_event_msg(adapter->devId, EVENT_GATTC_DISC_PRIMARY,
			   &msg, sizeof(msg));
}

static void service_removed_cb(struct gatt_db_attribute *attr, void *user_data)
{
}

static void att_debug_cb(const char *str, void *user_data)
{
	return;
	struct cmd_adaper *adapter = user_data;

	DBG("[%d] att:%s\n", adapter->devId, str);
}

static void gatt_debug_cb(const char *str, void *user_data)
{
	return;
	struct cmd_adaper *adapter = user_data;

	DBG("[%d] gatt:%s\n", adapter->devId, str);
}

static void gattc_disc_desc_complete(struct gatt_db_attribute * attr,
				     void *user_data)
{
	struct cmd_adaper *adapter = user_data;
	const bt_uuid_t *uuid;

	struct {
		/* must be naturally packed */
		uint16_t handle;
		uint16_t uuid16;
		uint8_t uuid_str[MAX_LEN_UUID_STR + 1];
	} msg;

	msg.handle = gatt_db_attribute_get_handle(attr);
	uuid = gatt_db_attribute_get_type(attr);
	msg.uuid16 = uuid->value.u16;
	bt_uuid_to_string(uuid, (char *)msg.uuid_str, sizeof(msg.uuid_str));

	cmd_send_event_msg(adapter->devId, EVENT_GATTC_DISC_DESC,
			   &msg, sizeof(msg));
}

static void gattc_disc_char_complete(struct gatt_db_attribute * attr,
				     void *			user_data)
{
	struct cmd_adaper *adapter = user_data;
	bt_uuid_t uuid;

	struct {
		/* must be naturally packed */
		uint16_t handle;
		uint16_t value_handle;
		uint16_t ext_prop;
		uint8_t properties;
		uint8_t uuid_str[MAX_LEN_UUID_STR + 1];
	} msg;

	if (!gatt_db_attribute_get_char_data(attr, &msg.handle,
					     &msg.value_handle,
					     &msg.properties,
					     &msg.ext_prop, &uuid)) {
		return;
	}
	bt_uuid_to_string((const bt_uuid_t *)&uuid, (char *)msg.uuid_str,
			  sizeof(msg.uuid_str));

	cmd_send_event_msg(adapter->devId, EVENT_GATTC_DISC_CHAR,
			   &msg, sizeof(msg));

	gatt_db_service_foreach_char(attr, gattc_disc_desc_complete, adapter);
}

static void gattc_disc_prim_complete(struct gatt_db_attribute * attr,
				     void *			user_data)
{
	struct cmd_adaper *adapter = user_data;
	bool is_primary;
	bt_uuid_t uuid;

	struct {
		/* must be naturally packed */
		uint16_t start;
		uint16_t end;
		char uuid_str[MAX_LEN_UUID_STR + 1];
	} msg;

	if (!gatt_db_attribute_get_service_data(attr, &msg.start, &msg.end,
						&is_primary, &uuid)) {
		return;
	}
	bt_uuid_to_string(&uuid, msg.uuid_str, sizeof(msg.uuid_str));

	if (is_primary) {
		cmd_send_event_msg(adapter->devId, EVENT_GATTC_DISC_PRIMARY,
				   &msg, sizeof(msg));
	}
	/* gatt_db_service_foreach_incl(attr, gattc_disc_inclu_complete, adapter); */
	gatt_db_service_foreach_char(attr, gattc_disc_char_complete, adapter);
}

static void dicovery_done(bool success, uint8_t att_ecode, void *user_data)
{
	struct cmd_adaper *adapter = user_data;

	if (!success) {
		cmd_send_event_msg(adapter->devId, EVENT_GATTC_DISC_PRIMARY,
				   NULL,
				   0);
		return;
	}

	gatt_db_foreach_service(adapter->cli->db, NULL,
				gattc_disc_prim_complete, adapter);

	cmd_send_event_msg(adapter->devId, EVENT_GATTC_DISC_PRIMARY, NULL, 0);
}

static void gattc_service_changed_cb(uint16_t start_handle, uint16_t end_handle,
				     void *user_data)
{
	struct cmd_adaper *adapter = user_data;

	DBG("\nService Changed handled - start: 0x%04x end: 0x%04x\n",
	    start_handle, end_handle);

	gatt_db_foreach_service_in_range(adapter->cli->db, NULL,
					 gattc_disc_prim_complete, adapter,
					 start_handle, end_handle);
}

static struct client *client_create(struct cmd_adaper *adapter, int fd,
				    uint16_t mtu)
{
	struct client *cli;

	cli = new0(struct client, 1);
	if (!cli) {
		ERR("Failed to allocate memory for client\n");
		return NULL;
	}

	cli->att = bt_att_new(fd, false);
	if (!cli->att) {
		ERR("Failed to initialze ATT transport layer\n");
		bt_att_unref(cli->att);
		free(cli);
		return NULL;
	}

	if (!bt_att_set_close_on_unref(cli->att, true)) {
		ERR("Failed to set up ATT transport layer\n");
		bt_att_unref(cli->att);
		free(cli);
		return NULL;
	}

	if (!bt_att_register_disconnect(cli->att, gattc_disconnect_cb,
					adapter, NULL)) {
		ERR("Failed to set ATT disconnect handler\n");
		bt_att_unref(cli->att);
		free(cli);
		return NULL;
	}

	cli->fd = fd;
	cli->db = gatt_db_new();
	if (!cli->db) {
		ERR("Failed to create GATT database\n");
		bt_att_unref(cli->att);
		free(cli);
		return NULL;
	}

	cli->gatt = bt_gatt_client_new(cli->db, cli->att, mtu);
	if (!cli->gatt) {
		ERR("Failed to create GATT client\n");
		gatt_db_unref(cli->db);
		bt_att_unref(cli->att);
		free(cli);
		return NULL;
	}

	adapter->cli = cli;

	gatt_db_register(cli->db, service_added_cb, service_removed_cb,
			 adapter, NULL);

	bt_att_set_debug(cli->att, att_debug_cb, adapter, NULL);
	bt_gatt_client_set_debug(cli->gatt, gatt_debug_cb, adapter, NULL);

	bt_gatt_client_ready_register(cli->gatt, dicovery_done, adapter, NULL);
	bt_gatt_client_set_service_changed(cli->gatt, gattc_service_changed_cb,
					   adapter, NULL);

	/* bt_gatt_client already holds a reference */
	gatt_db_unref(cli->db);

	return cli;
}

static int l2cap_le_att_connect(bdaddr_t *src, bdaddr_t *dst,
				uint8_t dst_type, int sec)
{
	int sock;
	struct sockaddr_l2 srcaddr, dstaddr;
	struct bt_security btsec;

	sock = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if (sock < 0) {
		ERR("Failed to create L2CAP socket");
		return -1;
	}

	/* Set up source address */
	memset(&srcaddr, 0, sizeof(srcaddr));
	srcaddr.l2_family = AF_BLUETOOTH;
	srcaddr.l2_cid = htobs(ATT_CID);
	srcaddr.l2_bdaddr_type = 0;
	bacpy(&srcaddr.l2_bdaddr, src);

	if (bind(sock, (struct sockaddr *)&srcaddr, sizeof(srcaddr)) < 0) {
		ERR("Failed to bind L2CAP socket");
		close(sock);
		return -1;
	}

	/* Set the security level */
	memset(&btsec, 0, sizeof(btsec));
	btsec.level = sec;
	if (setsockopt(sock, SOL_BLUETOOTH, BT_SECURITY, &btsec,
		       sizeof(btsec)) != 0) {
		ERR("Failed to set L2CAP security level\n");
		close(sock);
		return -1;
	}

	/* Set up destination address */
	memset(&dstaddr, 0, sizeof(dstaddr));
	dstaddr.l2_family = AF_BLUETOOTH;
	dstaddr.l2_cid = htobs(ATT_CID);
	dstaddr.l2_bdaddr_type = dst_type;
	bacpy(&dstaddr.l2_bdaddr, dst);

	INFO("Connecting to device...type(%d)\n", dst_type);

	if (connect(sock, (struct sockaddr *)&dstaddr, sizeof(dstaddr)) < 0) {
		ERR("Failed to connect %s\n", strerror(errno));
		close(sock);
		return -1;
	}

	INFO(" Done\n");

	return sock;
}

uint8_t gattc_connect(uint8_t devId, uint8_t *data, uint8_t data_len)
{
	uint8_t ret = BTLE_SUCCESS;
	struct cmd_adaper *adapter;

	adapter = cmd_get_adapter_by_id(devId);
	if (!adapter) {
		ret = BTLE_ERROR_INVALID_ARG;
	} else if (adapter->cli) {
		ret = BTLE_ERROR_ALREADY;
	}

	if (ret) {
		cmd_send_status(devId, CMD_MGMT_CONNECT, ret);
		return ret;
	}

	if (data_len < 18) {
		ret = BTLE_ERROR_INVALID_ARG;
		cmd_send_status(devId, CMD_MGMT_CONNECT, ret);
	} else {
		bdaddr_t src_addr, dst_addr;
		uint8_t *dst_addr_type;
		uint8_t mtu = ATT_DEFAULT_LE_MTU;
		uint8_t sec_level = 0;
		int fd;
		uint8_t idx;
		uint8_t *p_data = &data[0];
		char str[20];
		for (idx = 0; idx < 6; idx++, p_data += 3) {
			dst_addr.b[idx] = strtol((char *)p_data, NULL, 16);
		}
		ba2str((const bdaddr_t *)&dst_addr, str);
		DBG("dest addr %s\n", str);

		dst_addr_type = (void *)&data[17];

		if (data_len == 19)
			sec_level = data[18];

		if (hci_devba(devId, &src_addr) >= 0) {
			fd = l2cap_le_att_connect(&src_addr, &dst_addr,
						  *dst_addr_type, sec_level);
			if (fd > 0) {
				adapter->devId = devId;
				adapter->cli = client_create(adapter, fd, mtu);
			}

			if (!adapter->cli) {
				cmd_send_status(devId, CMD_MGMT_CONNECT,
						BTLE_ERROR_INTERNAL);
				memset(adapter, 0, sizeof(*adapter));
			}
		}
		cmd_send_status(devId, CMD_MGMT_CONNECT, BTLE_SUCCESS);
	}
	return ret;
}
