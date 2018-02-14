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
#include <glib.h>

#include "lib/bluetooth.h"
#include "lib/sdp.h"

#include <btio/btio.h>
#include "lib/uuid.h"
#include "attrib/gattrib.h"
#include "attrib/att.h"
#include "attrib/gatt.h"
#include "attrib/gatttool.h"

#include "lib/mgmt.h"
#include "src/shared/mgmt.h"
#include "src/shared/util.h"

#define MODULE "gatts"
#include "btprint.h"

#include "btle_error.h"
#include "gatts.h"
#include "cmd.h"

#ifndef BIT
#define BIT(n) (1UL << (n))
#endif

enum {
	BT_GATT_CHRC_READ                               = BIT(0),
	BT_GATT_CHRC_WRITE_WITHOUT_RESP = BIT(1),
	BT_GATT_CHRC_WRITE                              = BIT(2),
	BT_GATT_CHRC_NOTIFY                             = BIT(3),
};

struct gatts_characteristic;

struct bt_gatt_attr {
	const struct bt_uuid *uuid;
	void *user_data;
	uint16_t handle;
	uint8_t perm;
};

/**@brief GATT Characteristic Definition Handles. */
typedef struct {
	uint16_t value_handle;
	uint16_t user_desc_handle;
	uint16_t cccd_handle;
	uint16_t sccd_handle;
} gatts_char_handles_t;

struct gatts_characteristic {
	bt_uuid_t uuid;
	uint16_t handle;
	uint8_t *char_value[23];
	uint8_t perm;
};

struct gatts_service {
	struct gatts_service *next;
	bt_uuid_t uuid;
	struct att_range range;
	uint8_t charac_cnt;
	struct gatts_characteristic *charac;
};

struct {
	struct gatts_service *svc_head;
} gatts_db[CMD_MAX_ADAPTER] = {};

static struct gatts_service *gatts_get_last(struct gatts_service *head)
{
	struct gatts_service *rover;

	if (!head)
		return NULL;

	/* Look for the last element of the queue */
	for (rover = head;
	     rover->next != NULL;
	     rover = rover->next) {
	}

	return rover;
}

uint8_t gatts_addService(uint8_t devId, uint8_t *data, uint8_t data_len)
{
	struct gatts_service *svc;
	uint8_t ret = BTLE_ERROR_MEMORY;

	svc = malloc(sizeof(*svc));
	if (svc) {
		svc->next = NULL;
		bt_string_to_uuid(&svc->uuid, (const char *)&data[0]);
		svc->charac_cnt =
			data[bt_uuid_len((const bt_uuid_t *)&svc->uuid)];
		svc->charac = malloc(svc->charac_cnt * sizeof(*svc->charac));
		memset(svc->charac, 0, sizeof(*svc->charac));

		if (!gatts_db[devId].svc_head) {
			gatts_db[devId].svc_head = svc;
			svc->range.start = 0x01;
		} else {
			struct gatts_service *rover;

			rover = gatts_get_last(gatts_db[devId].svc_head);
			rover->next = svc;
			svc->range.start =
				rover->charac[rover->charac_cnt - 1].handle + 1;
		}
		ret = BTLE_SUCCESS;
	}

	cmd_send_status(devId, CMD_GATTS_ADD_SVC, ret);
	return ret;
}

uint8_t gatts_addCharacteristic(uint8_t devId, uint8_t *data, uint8_t data_len)
{
	struct gatts_service *svc;
	uint8_t ret = BTLE_ERROR_INVALID_STATE;

	svc = gatts_get_last(gatts_db[devId].svc_head);
	if (svc) {
		uint8_t idx;

		for (idx = 0; idx < svc->charac_cnt; idx++) {
			if (!svc->charac[idx].handle) {
				struct gatts_characteristic *charac;
				charac = &svc->charac[idx];
				if (idx)
					charac->handle =
						svc->charac[idx - 1].handle + 1;
				else
					charac->handle = svc->range.start + 1;

				bt_string_to_uuid(&charac->uuid,
						  (const char *)&data[0]);
				charac->perm =
					data[bt_uuid_len((const bt_uuid_t *)&
							 charac->
							 uuid)];
				break;
			}
		}
		ret = BTLE_SUCCESS;
	}

	cmd_send_status(devId, CMD_GATTS_ADD_CHARACTERISTIC, ret);
	return ret;
}

uint8_t gatts_adddescriptor(uint8_t devId, uint8_t *data, uint8_t data_len)
{
	cmd_send_status(devId, CMD_GATTS_ADD_DESCRIPTOR,
			BTLE_ERROR_NOT_IMPLEMENTED);
	return BTLE_ERROR_NOT_IMPLEMENTED;
}

void gatts_unregister(uint8_t devId)
{
}
