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


#include "lib/bluetooth.h"

#include "lib/mgmt.h"
#include "src/shared/mgmt.h"
#include "src/shared/util.h"

#define MODULE "cmd"
#include "btprint.h"

#include "btle_error.h"
#include "btsocket.h"
#include "ipc.h"
#include "gattc.h"
#include "cmd.h"

#ifndef BUILD_BUG_ON_ZERO
/*  Force a compilation error if condition is true */
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int : -!!(e); }))
#endif

#ifndef BIT
#define BIT(n) (1UL << (n))
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#define LE_ADV_DATA_LEN         31
#define LE_NAME                         31

#define CMD_ADAPTER_MAX 16



enum scan_mode {
	SCAN_STOP = 0,
	SCAN_START,
};

enum power_mode {
	POWER_OFF = 0,
	POWER_ON,
};

struct msg {
	struct {
		uint8_t devId;
		uint8_t msg_type;  /* cf @event_type for event or @cmds for response */
		uint8_t status;
	} header;
	uint8_t data_len;
	uint8_t data[0];
};

static uint8_t cmd_get_device_info(uint8_t devId, uint8_t *data,
				   uint8_t data_len);
static uint8_t cmd_reset(uint8_t devId, uint8_t *data, uint8_t data_len);
static uint8_t cmd_power(uint8_t devId, uint8_t *data, uint8_t data_len);
static uint8_t cmd_set_local_name(uint8_t devId, uint8_t *data,
				  uint8_t data_len);
static uint8_t cmd_set_conn_param(uint8_t devId, uint8_t *data,
				  uint8_t data_len);
static uint8_t cmd_scan(uint8_t devId, uint8_t *data, uint8_t data_len);
static uint8_t cmd_read_controller_info(uint8_t devId, uint8_t *data,
					uint8_t data_len);

static const struct {
	uint8_t (*cmd_fct)(uint8_t devId, uint8_t *data, uint8_t data_len);
} cmd_table[] = {
	[CMD_MGMT_GET_DEVICE_INFO] = { cmd_get_device_info },
	[CMD_MGMT_RESET] = { cmd_reset },
	[CMD_MGMT_POWER] = { cmd_power },
	[CMD_MGMT_SET_LOCAL_NAME] = { cmd_set_local_name },
	[CMD_MGMT_SET_CONNECTION_PARAM] = { cmd_set_conn_param },
	[CMD_MGMT_SCAN] = { cmd_scan },
	[CMD_MGMT_READ_CONTROLLER_INFO] = { cmd_read_controller_info },
	[CMD_MGMT_CONNECT] = { gattc_connect },

	[CMD_GATTC_WRITE_CMD] = { gattc_write_cmd },
	[CMD_GATTC_WRITE_REQ] = { gattc_write_req },
	[CMD_GATTC_READ_REQ] = { gattc_read_req },
	[CMD_GATTC_SUBSCRIBE_REQ] = { gattc_subscribe_req },
	[CMD_GATTC_UNSUBSCRIBE_REQ] = { gattc_unsubscribe_req },

	[CMD_MAX] = { NULL },
};

struct cmd_param {
	uint8_t devId;
	enum cmds cmd;
};

struct cmd_param param;

static struct {
	struct mgmt *desc;
	uint16_t reg_flag; /* up to 16 adapters */
	struct {
		uint8_t devId;
	} cmd_param;

	struct cmd_adaper adapter[CMD_MAX_ADAPTER];
} btmgmt = {
	.desc = NULL,
	.reg_flag = 0,
};

#define is_flag_set(idx)        (btmgmt.reg_flag & BIT(idx))
#define set_flag(idx)           (btmgmt.reg_flag |= BIT(idx))
#define unset_flag(idx)         (btmgmt.reg_flag &= ~BIT(idx))

void cmd_send_status(uint8_t devId, uint8_t cmd, uint8_t ret)
{
	struct msg resp = {
		.header = {
			.devId = devId,
			.msg_type = cmd,
			.status = ret,
		},
	};

	ipc_send_rsp(&resp, sizeof(resp));
}

void cmd_send_status_msg(uint8_t devId, uint8_t cmd, uint8_t ret,
			 void *data, uint8_t data_len)
{
	struct msg *resp;
	uint8_t container[sizeof(*resp) + data_len];

	resp = (void *)&container[0];

	resp->header.devId = devId;
	resp->header.msg_type = cmd;
	resp->header.status = ret;
	resp->data_len = data_len;
	if (resp->data_len)
		memcpy(resp->data, data, resp->data_len);

	ipc_send_rsp(&container[0], sizeof(container));
}

void cmd_send_event(uint8_t devId, uint8_t evt_type)
{
	struct msg resp = {
		.header = {
			.devId = devId,
			.msg_type = evt_type,
		},
	};

	ipc_send_event(&resp, sizeof(resp));
}

void cmd_send_event_msg(uint8_t devId, uint8_t evt_type,
			void *data, uint8_t data_len)
{
	struct msg *resp;
	uint8_t container[sizeof(*resp) + data_len];

	resp = (void *)&container[0];

	resp->header.devId = devId;
	resp->header.msg_type = evt_type;
	resp->data_len = data_len;
	memcpy(resp->data, data, resp->data_len);

	ipc_send_event(&container[0], sizeof(container));
}


static void cmd_event_connected(uint16_t index, uint16_t length,
				const void *param, void *user_data)
{
	INFO("%s\n", __FUNCTION__);
	cmd_send_event(index, EVENT_CONNECTED);
}

static void cmd_event_disconnected(uint16_t index, uint16_t length,
				   const void *param, void *user_data)
{
	INFO("%s\n", __FUNCTION__);
	cmd_send_event(index, EVENT_DISCONNECTED);
}

static void cmd_event_dev_found(uint16_t index, uint16_t length,
				const void *param, void *user_data)
{
	const struct mgmt_ev_device_found *ev_device_found = param;
	uint8_t le_adv_len = MIN(ev_device_found->eir_len, LE_ADV_DATA_LEN);

	struct {
		/* must be naturally packed */
		uint32_t flags;
		uint8_t addr[6];
		uint8_t addr_type;
		int8_t rssi;
		uint8_t le_adv_data_len;
		uint8_t le_adv_data[le_adv_len];
	} device;

	memcpy(&device.addr[0], &ev_device_found->addr.bdaddr,
	       sizeof(device.addr));
	device.addr_type = ev_device_found->addr.type;
	device.flags = ev_device_found->flags;
	device.rssi = ev_device_found->rssi;
	device.le_adv_data_len = le_adv_len;

	memcpy(&device.le_adv_data[0], &ev_device_found->eir[0],
	       device.le_adv_data_len);

	INFO("dev[%d] found %02X:%02X:%02X:%02X:%02X:%02X flag(%d) rssi(%d)\n",
	     index,
	     device.addr[0], device.addr[1],
	     device.addr[2], device.addr[3],
	     device.addr[4], device.addr[5],
	     ev_device_found->flags, ev_device_found->rssi);

	cmd_send_event_msg(index, EVENT_SCAN_RESULT, &device, sizeof(device));
}

static void cmd_event_new_conn_param(uint16_t index, uint16_t length,
				     const void *param, void *user_data)
{
	const struct mgmt_ev_new_conn_param *evt_conn_param = param;

	struct {
		uint16_t min_interval;
		uint16_t max_interval;
		uint16_t latency;
		uint16_t timeout;
	} conn_params = {
		.min_interval = evt_conn_param->min_interval,
		.max_interval = evt_conn_param->max_interval,
		.latency = evt_conn_param->latency,
		.timeout = evt_conn_param->timeout,
	};

	INFO("dev[%d] new conn params: min: %d max: %d latency: %d timeout: %d\n",
		index,
		conn_params.min_interval,
		conn_params.max_interval,
		conn_params.latency,
		conn_params.timeout);

	cmd_send_event_msg(index, EVENT_NEW_CONN_PARAM,
			   &conn_params, sizeof(conn_params));
}

static void cmd_event_scan(uint16_t index, uint16_t length,
			   const void *param, void *user_data)
{
	const struct mgmt_ev_discovering *evt_disc = param;
	uint8_t msg;

	INFO("dev[%d] scanning (0x%x): %s", index, evt_disc->type,
	     evt_disc->discovering ? "on going" : "stopped");

	msg = evt_disc->discovering ? SCAN_START : SCAN_STOP;

	cmd_send_event_msg(index, EVENT_SCAN_STATUS, &msg, 1);
}

static uint8_t cmd_mgmt_event_registration(uint8_t devId)
{
	if (!is_flag_set(devId)) {
		set_flag(devId);

		if (!mgmt_register(btmgmt.desc, MGMT_EV_DEVICE_CONNECTED, devId,
				   cmd_event_connected, NULL, NULL)) {
			ERR("registering MGMT_EV_DEVICE_CONNECTED event");
			return BTLE_ERROR_INTERNAL;
		}

		if (!mgmt_register(btmgmt.desc, MGMT_EV_DEVICE_DISCONNECTED,
				   devId,
				   cmd_event_disconnected, NULL, NULL)) {
			ERR("registering MGMT_EV_DEVICE_DISCONNECTED event");
			return BTLE_ERROR_INTERNAL;
		}

		if (!mgmt_register(btmgmt.desc, MGMT_EV_DEVICE_FOUND, devId,
				   cmd_event_dev_found, NULL, NULL)) {
			ERR("registering MGMT_EV_DEVICE_FOUND event");
			return BTLE_ERROR_INTERNAL;
		}

		if (!mgmt_register(btmgmt.desc, MGMT_EV_NEW_CONN_PARAM, devId,
				   cmd_event_new_conn_param, NULL, NULL)) {
			ERR("registering MGMT_EV_NEW_CONN_PARAM event");
			return BTLE_ERROR_INTERNAL;
		}

		if (!mgmt_register(btmgmt.desc, MGMT_EV_DISCOVERING, devId,
				   cmd_event_scan, NULL, NULL)) {
			ERR("registering MGMT_EV_DISCOVERING event");
			return BTLE_ERROR_INTERNAL;
		}
	}

	return BTLE_SUCCESS;
}

static void cmd_mgmt_dbg(const char *str, void *user_data)
{
	/* DBG("mgmt: %s\n", str); */
}

static uint8_t cmd_mgmt_init(uint8_t devId)
{
	uint8_t ret;

	if (!btmgmt.desc) {
		btmgmt.desc = mgmt_new_default();
		if (!btmgmt.desc) {
			ERR("new mgmt\n");
			return BTLE_ERROR_INTERNAL;
		}

		ret = mgmt_set_debug(btmgmt.desc, cmd_mgmt_dbg, NULL, NULL);
	}
	ret = cmd_mgmt_event_registration(devId);

	return ret;
}

static uint8_t cmd_get_device_info(uint8_t devId, uint8_t *data,
				   uint8_t data_len)
{
	cmd_send_status(devId, CMD_MGMT_GET_DEVICE_INFO,
			BTLE_ERROR_NOT_IMPLEMENTED);
	return BTLE_ERROR_NOT_IMPLEMENTED;
}

static uint8_t cmd_reset(uint8_t devId, uint8_t *data, uint8_t data_len)
{
	cmd_send_status(devId, CMD_MGMT_RESET, BTLE_ERROR_NOT_IMPLEMENTED);
	return BTLE_ERROR_NOT_IMPLEMENTED;
}

static void cmd_power_complete(uint8_t status, uint16_t length,
			       const void *param, void *user_data)
{
	uint8_t *devId = user_data;

	INFO("[%d] cmd power complete (%d)\n", *devId, status);
	cmd_send_status(*devId, CMD_MGMT_POWER, status);
}

static uint8_t cmd_power(uint8_t devId, uint8_t *data, uint8_t data_len)
{
	uint8_t val;

	INFO("%s mode(%s)\n", __FUNCTION__,
	     (data[0] == POWER_ON) ? "on" : "off");

	if (data[0] == POWER_ON) {
		val = 0x01;
	} else if (data[0] == POWER_OFF) {
		val = 0x00;
	} else {
		cmd_send_status(devId, CMD_MGMT_POWER, BTLE_ERROR_INVALID_ARG);
		return BTLE_ERROR_INVALID_ARG;
	}

	btmgmt.cmd_param.devId = devId;
	mgmt_send(btmgmt.desc, MGMT_OP_SET_POWERED, devId, 1, &val,
		  cmd_power_complete, &btmgmt.cmd_param.devId, NULL);

	return BTLE_SUCCESS;
}

static uint8_t cmd_set_local_name(uint8_t devId, uint8_t *data,
				  uint8_t data_len)
{
	struct {
		uint8_t name_len;
		uint8_t name[data[0]];
	} param;

	param.name_len = data[0];
	memcpy(param.name, &data[1], param.name_len);

	INFO("set local name: len(%d) name(%s)\n",
	     param.name_len, param.name);

	return BTLE_SUCCESS;
}

uint8_t cmd_strtou16(uint8_t *in, uint16_t *out)
{
	if (!in || !out)
		return BTLE_ERROR_NULL_ARG;

	*out = (in[0] & 0x00FF) << 8;
	*out += in[1];

	return BTLE_SUCCESS;
}

static uint8_t cmd_set_conn_param(uint8_t devId, uint8_t *data,
				  uint8_t data_len)
{
	struct mgmt_conn_param conn_param;
	uint8_t *p_data = &data[0];
	uint8_t idx;

	for (idx = 0; idx < 6; idx++, p_data += 3) {
		conn_param.addr.bdaddr.b[idx] = strtol((char *)p_data, NULL, 16);
	}
	p_data = &data[17];

	cmd_strtou16(&p_data[0], &conn_param.min_interval);
	cmd_strtou16(&p_data[2], &conn_param.max_interval);
	cmd_strtou16(&p_data[4], &conn_param.latency);
	cmd_strtou16(&p_data[6], &conn_param.timeout);

	INFO("%02X:%02X:%02X:%02X:%02X:%02X\n",
	     conn_param.addr.bdaddr.b[0], conn_param.addr.bdaddr.b[1],
	     conn_param.addr.bdaddr.b[2], conn_param.addr.bdaddr.b[3],
	     conn_param.addr.bdaddr.b[4], conn_param.addr.bdaddr.b[5]);

	INFO("[%d] conn param min:%d max:%d latency:%d timeout:%d\n",
	     devId, conn_param.min_interval, conn_param.max_interval,
	     conn_param.latency, conn_param.timeout);

	return BTLE_SUCCESS;
}

static void cmd_scan_complete(uint8_t status, uint16_t length,
			      const void *param, void *user_data)
{
	uint8_t *devId = user_data;

	INFO("[%d] scan complete (%d)\n", *devId, status);
	cmd_send_status(*devId, CMD_MGMT_SCAN, status);
}

static uint8_t cmd_scan(uint8_t devId, uint8_t *data, uint8_t data_len)
{
	struct mgmt_cp_start_discovery cp;
	uint16_t opcode = MGMT_OP_START_DISCOVERY;
	uint16_t ret;

	struct {
		uint8_t mode;
	} scan_param;

	cp.type = (1 << BDADDR_LE_PUBLIC) | (1 << BDADDR_LE_RANDOM);
	scan_param.mode = data[0];

	if (scan_param.mode == SCAN_START) {
		INFO("[%d] starting scan\n", devId);
		opcode = MGMT_OP_START_DISCOVERY;
	} else if (scan_param.mode == SCAN_STOP) {
		INFO("[%d] stopping scan\n", devId);
		opcode = MGMT_OP_STOP_DISCOVERY;
	}

	btmgmt.cmd_param.devId = devId;
	ret = mgmt_send(btmgmt.desc, opcode, devId, sizeof(cp),
			&cp, cmd_scan_complete, &btmgmt.cmd_param.devId, NULL);
	if (!ret) {
		ERR("cmd %s failed %d",
		    ((opcode == MGMT_OP_START_DISCOVERY) ?
		     "MGMT_OP_START_DISCOVERY" :
		     "MGMT_OP_STOP_DISCOVERY"), ret);

		cmd_send_status(devId, CMD_MGMT_SCAN, BTLE_ERROR_INTERNAL);

		return BTLE_ERROR_INTERNAL;
	}

	return BTLE_SUCCESS;
}

static void cmd_set_settings(uint8_t				devId,
			     const struct mgmt_rp_read_info *	info)
{
	uint32_t required_settings = MGMT_SETTING_LE;
	uint32_t supported_settings, current_settings;
	uint8_t val;

	supported_settings = le32_to_cpu(info->supported_settings);
	current_settings = le32_to_cpu(info->current_settings);

	if ((supported_settings & required_settings) != required_settings) {
		return;
	}
	if (current_settings & MGMT_SETTING_POWERED) {
		val = 0x00;
		mgmt_send(btmgmt.desc, MGMT_OP_SET_POWERED, devId, 1, &val,
			  NULL, NULL, NULL);
	}

	if (!(current_settings & MGMT_SETTING_LE)) {
		val = 0x01;
		mgmt_send(btmgmt.desc, MGMT_OP_SET_LE, devId, 1, &val,
			  NULL, NULL, NULL);
	}

	if (current_settings & MGMT_SETTING_BREDR) {
		val = 0x00;
		mgmt_send(btmgmt.desc, MGMT_OP_SET_BREDR, devId, 1, &val,
			  NULL, NULL, NULL);
	}
}
static void cmd_read_info_complete(uint8_t status, uint16_t length,
				   const void *param, void *user_data)
{
	uint8_t msg_len = 0;
	const struct mgmt_rp_read_info *info = param;
	uint8_t *devId = user_data;

	if (!status) {
		INFO("[%d] read info (%d)\n", *devId, status);
		INFO("[%d] version (%d)\n", *devId, info->version);
		INFO("[%d] manufacturer (%d)\n", *devId, info->manufacturer);
		INFO("[%d] supported_settings (%04X)\n", *devId,
		     info->supported_settings);
		INFO("[%d] current_settings (%04X)\n", *devId,
		     info->current_settings);
		INFO("[%d] dev_class (%02X %02X %02X)\n", *devId,
		     info->dev_class[0],
		     info->dev_class[1],
		     info->dev_class[2]);
		INFO("[%d] name (%s)\n", *devId, info->name);

		/* removing short name ; making name shorter */
		msg_len = sizeof(*info) - sizeof(info->short_name) + LE_NAME -
			  (sizeof(info->name));

#if 0
		/* works only if short_name and name are respectively last and second last member */
		BUILD_BUG_ON_ZERO(
			(void *)&info->short_name[sizeof(info->
							 short_name)]
			!= (void *)&info[sizeof(*info)]);
		BUILD_BUG_ON_ZERO((void *)&info->name[sizeof(info->name)] !=
				  (void *)&info->short_name[0]);
#endif

		cmd_set_settings(*devId, info);
	}

	cmd_send_status_msg(*devId, CMD_MGMT_READ_CONTROLLER_INFO, status,
			    (void *)info, msg_len);
}

static uint8_t cmd_read_controller_info(uint8_t devId, uint8_t *data,
					uint8_t data_len)
{
	uint16_t ret = BTLE_SUCCESS;

	btmgmt.cmd_param.devId = devId;
	ret = mgmt_send_nowait(btmgmt.desc, MGMT_OP_READ_INFO, devId, 0, NULL,
			       cmd_read_info_complete, &btmgmt.cmd_param.devId,
			       NULL);
	if (!ret) {
		ERR("cmd MGMT_OP_READ_INFO failed");
		cmd_send_status(devId, CMD_MGMT_READ_CONTROLLER_INFO,
				BTLE_ERROR_INTERNAL);

		ret = BTLE_ERROR_INTERNAL;
	}
	return ret;
}

struct cmd_adaper *cmd_get_adapter_by_id(uint8_t devId)
{
	struct cmd_adaper *adapter = NULL;

	if (devId < CMD_MAX_ADAPTER) {
		adapter = &btmgmt.adapter[devId];
	}
	return adapter;
}

uint8_t cmd_server_handler(uint8_t *data, uint8_t data_len)
{
	uint8_t ret = BTLE_ERROR_INVALID_ARG;;

	if (data_len < 2) {
		return BTLE_ERROR_INVALID_ARG;
	} else {
		/*
		 +----------+--------------+----------------+
		 |     0    |       1      | 2: data_len -2 |
		 +----------+--------------+----------------+
		 | DeviceId | Command Type | Parameters     |
		 +----------+--------------+----------------+
		 */
		uint8_t devId = data[0];
		uint8_t cmdtype = data[1];

		if (devId >= CMD_ADAPTER_MAX) {
			ret = BTLE_ERROR_INVALID_ARG;
			cmd_send_status(devId, cmdtype, ret);
			return ret;
		}
		if (cmdtype < CMD_MAX) {
			cmd_mgmt_init(devId);
			ret = cmd_table[cmdtype].cmd_fct(devId, &data[2],
							 data_len - 2);
		}

		if (ret) {
			cmd_send_status(devId, cmdtype, ret);
		}
	}
	return ret;
}

uint8_t cmd_server_init(void)
{
	return ipc_init(cmd_server_handler);
}

void cmd_server_close(void)
{
	uint8_t devId;

	for (devId = 0; devId < CMD_ADAPTER_MAX; devId++) {
		if (is_flag_set(devId)) {
			unset_flag(devId);
		}
		mgmt_unregister_index(btmgmt.desc, devId);
		mgmt_cancel_index(btmgmt.desc, devId);
	}
	mgmt_unref(btmgmt.desc);
	btmgmt.desc = NULL;
}
