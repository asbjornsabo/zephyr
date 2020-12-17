/*  Bluetooth MICS client - Microphone Control Profile - Client */

/*
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <zephyr/types.h>

#include <device.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/mics.h>

#include "aics_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MICS_CLIENT)
#define LOG_MODULE_NAME bt_mics_client
#include "common/log.h"

#define FIRST_HANDLE				0x0001
#define LAST_HANDLE				0xFFFF
struct mics_instance_t {
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t mute_handle;
	struct bt_gatt_subscribe_params mute_sub_params;

	bool busy;
	uint8_t write_buf[1];
	struct bt_gatt_write_params write_params;
	struct bt_gatt_read_params read_params;

	uint8_t aics_inst_cnt;
	struct aics_instance_t aics[CONFIG_BT_MICS_CLIENT_MAX_AICS_INST];
};

/* Callback functions */
static struct bt_mics_cb_t *mics_client_cb;

static struct bt_gatt_discover_params discover_params;
static struct mics_instance_t *cur_mics_inst;
static struct aics_instance_t *cur_aics_inst;

static struct mics_instance_t mics_inst;
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

static uint8_t mute_notify_handler(struct bt_conn *conn,
				   struct bt_gatt_subscribe_params *params,
				   const void *data, uint16_t length)
{
	uint8_t *mute_val;

	if (data) {
		if (length == sizeof(*mute_val)) {
			mute_val = (uint8_t *)data;
			BT_DBG("Mute %u", *mute_val);
			if (mics_client_cb && mics_client_cb->mute) {
				mics_client_cb->mute(conn, 0, *mute_val);
			}
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(*mute_val));

		}
	}
	return BT_GATT_ITER_CONTINUE;
}


static uint8_t mics_client_read_mute_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	uint8_t *mute_val = NULL;

	mics_inst.busy = false;

	if (err) {
		BT_DBG("err: 0x%02X", err);
	} else if (data) {
		if (length == sizeof(*mute_val)) {
			mute_val = (uint8_t *)data;
			BT_DBG("Mute %u", *mute_val);
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(*mute_val));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (mics_client_cb && mics_client_cb->mute) {
		mics_client_cb->mute(conn, cb_err, *mute_val);
	}

	return BT_GATT_ITER_STOP;
}

static void mics_client_write_mics_mute_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_write_params *params)
{
	BT_DBG("Write %s (0x%02X)", err ? "failed" : "successful", err);

	mics_inst.busy = false;

	if (mics_client_cb && mics_client_cb->mute_write) {
		mics_client_cb->mute_write(conn, err,
					   ((uint8_t *)params->data)[0]);
	}

}

static uint8_t aics_discover_func(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  struct bt_gatt_discover_params *params)
{
	int err;
	struct bt_gatt_chrc *chrc;
	uint8_t next_idx;
	uint8_t aics_cnt;
	struct bt_gatt_subscribe_params *sub_params = NULL;

	if (!attr) {
		bt_aics_client_register(
			cur_aics_inst,
			AICS_CLI_MICS_CLIENT_INDEX(cur_aics_inst->index));
		aics_cnt = mics_inst.aics_inst_cnt;
		next_idx = cur_aics_inst->index + 1;
		BT_DBG("Setup complete for AICS %u / %u",
		       next_idx, mics_inst.aics_inst_cnt);
		(void)memset(params, 0, sizeof(*params));

		if (next_idx < mics_inst.aics_inst_cnt) {
			/* Discover characeristics */
			cur_aics_inst = &mics_inst.aics[next_idx];
			discover_params.start_handle =
				cur_aics_inst->start_handle;
			discover_params.end_handle = cur_aics_inst->end_handle;
			discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
			discover_params.func = aics_discover_func;

			err = bt_gatt_discover(conn, &discover_params);
			if (err) {
				BT_DBG("Discover failed (err %d)", err);
				cur_mics_inst = NULL;
				cur_aics_inst = NULL;
				if (mics_client_cb && mics_client_cb->init) {
					mics_client_cb->init(conn, err,
							     aics_cnt);
				}
			}
		} else {
			cur_mics_inst = NULL;
			cur_aics_inst = NULL;
			if (mics_client_cb && mics_client_cb->init) {
				mics_client_cb->init(conn, 0, aics_cnt);
			}
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		chrc = (struct bt_gatt_chrc *)attr->user_data;
		if (!bt_uuid_cmp(chrc->uuid, BT_UUID_AICS_STATE)) {
			BT_DBG("Audio Input state");
			cur_aics_inst->state_handle = chrc->value_handle;
			sub_params = &cur_aics_inst->state_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_AICS_GAIN_SETTINGS)) {
			BT_DBG("Gain settings");
			cur_aics_inst->gain_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_AICS_INPUT_TYPE)) {
			BT_DBG("Input type");
			cur_aics_inst->type_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_AICS_INPUT_STATUS)) {
			BT_DBG("Input status");
			cur_aics_inst->status_handle = chrc->value_handle;
			sub_params = &cur_aics_inst->status_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_AICS_CONTROL)) {
			BT_DBG("Control point");
			cur_aics_inst->control_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_AICS_DESCRIPTION)) {
			BT_DBG("Description");
			cur_aics_inst->desc_handle = chrc->value_handle;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				sub_params = &cur_aics_inst->desc_sub_params;
			}

			if (chrc->properties &
				BT_GATT_CHRC_WRITE_WITHOUT_RESP) {
				cur_aics_inst->desc_writable = true;
			}
		}

		if (sub_params) {
			sub_params->value = BT_GATT_CCC_NOTIFY;
			sub_params->value_handle = chrc->value_handle;
			/*
			 * TODO: Don't assume that CCC is at handle + 2;
			 * do proper discovery;
			 */
			sub_params->ccc_handle = attr->handle + 2;
			sub_params->notify = aics_client_notify_handler;
			bt_gatt_subscribe(conn, sub_params);
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t mics_discover_include_func(
	struct bt_conn *conn, const struct bt_gatt_attr *attr,
	struct bt_gatt_discover_params *params)
{
	struct bt_gatt_include *include;
	uint8_t inst_idx;
	int err;

	if (!attr) {
		BT_DBG("Discover include complete for MICS: %u AICS",
		       mics_inst.aics_inst_cnt);
		(void)memset(params, 0, sizeof(*params));
		if (mics_inst.aics_inst_cnt) {
			/* Discover characeristics */
			cur_aics_inst = &mics_inst.aics[0];
			discover_params.start_handle =
				cur_aics_inst->start_handle;
			discover_params.end_handle = cur_aics_inst->end_handle;
			discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
			discover_params.func = aics_discover_func;

			err = bt_gatt_discover(conn, &discover_params);
			if (err) {
				BT_DBG("Discover failed (err %d)", err);
				cur_mics_inst = NULL;
				cur_aics_inst = NULL;
				if (mics_client_cb && mics_client_cb->init) {
					mics_client_cb->init(conn, err, 0);
				}
			}
		} else {
			cur_mics_inst = NULL;
			cur_aics_inst = NULL;
			if (mics_client_cb && mics_client_cb->init) {
				mics_client_cb->init(conn, 0, 0);
			}
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_INCLUDE) {
		include = (struct bt_gatt_include *)attr->user_data;
		BT_DBG("Include UUID %s", bt_uuid_str(include->uuid));
		if (!bt_uuid_cmp(include->uuid, BT_UUID_AICS) &&
		    mics_inst.aics_inst_cnt <
			CONFIG_BT_MICS_CLIENT_MAX_AICS_INST) {
			inst_idx = mics_inst.aics_inst_cnt;
			mics_inst.aics[inst_idx].start_handle =
				include->start_handle;
			mics_inst.aics[inst_idx].end_handle =
				include->end_handle;
			mics_inst.aics[inst_idx].index = inst_idx;
			mics_inst.aics_inst_cnt++;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

/**
 * @brief This will discover all characteristics on the server, retrieving the
 * handles of the writeable characteristics and subscribing to all notify and
 * indicate characteristics.
 */
static uint8_t mics_discover_func(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  struct bt_gatt_discover_params *params)
{
	int err = 0;
	struct bt_gatt_chrc *chrc;
	struct bt_gatt_subscribe_params *sub_params = NULL;

	if (!attr) {
		BT_DBG("Setup complete for MICS");
		(void)memset(params, 0, sizeof(*params));
		if (CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {

			/* Discover included services */
			discover_params.start_handle = mics_inst.start_handle;
			discover_params.end_handle = mics_inst.end_handle;
			discover_params.type = BT_GATT_DISCOVER_INCLUDE;
			discover_params.func = mics_discover_include_func;

			err = bt_gatt_discover(conn, &discover_params);
			if (err) {
				BT_DBG("Discover failed (err %d)", err);
				cur_mics_inst = NULL;
				cur_aics_inst = NULL;
				if (mics_client_cb && mics_client_cb->init) {
					mics_client_cb->init(conn, err, 0);
				}
			}
		} else {
			cur_aics_inst = NULL;
			cur_mics_inst = NULL;
			if (mics_client_cb && mics_client_cb->init) {
				mics_client_cb->init(conn, err, 0);
			}
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		chrc = (struct bt_gatt_chrc *)attr->user_data;
		if (!bt_uuid_cmp(chrc->uuid, BT_UUID_MICS_MUTE)) {
			BT_DBG("Mute");
			mics_inst.mute_handle = chrc->value_handle;
			sub_params = &mics_inst.mute_sub_params;
		}

		if (sub_params) {
			sub_params->value = BT_GATT_CCC_NOTIFY;
			sub_params->value_handle = chrc->value_handle;
			/*
			 * TODO: Don't assume that CCC is at handle + 2;
			 * do proper discovery;
			 */
			sub_params->ccc_handle = attr->handle + 2;
			sub_params->notify = mute_notify_handler;
			bt_gatt_subscribe(conn, sub_params);
		}
	}

	return BT_GATT_ITER_CONTINUE;
}


/**
 * @brief This will discover all characteristics on the server, retrieving the
 * handles of the writeable characteristics and subscribing to all notify and
 * indicate characteristics.
 */
static uint8_t primary_discover_func(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	int err;
	struct bt_gatt_service_val *prim_service;

	if (!attr) {
		BT_DBG("Could not find a MICS instance on the server");
		cur_mics_inst = NULL;
		cur_aics_inst = NULL;
		if (mics_client_cb && mics_client_cb->init) {
			mics_client_cb->init(conn, -ENODATA, 0);
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		BT_DBG("Primary discover complete");
		prim_service = (struct bt_gatt_service_val *)attr->user_data;
		cur_mics_inst = &mics_inst;
		mics_inst.start_handle = attr->handle + 1;
		mics_inst.end_handle = prim_service->end_handle;

		/* Discover characeristics */
		discover_params.start_handle = attr->handle + 1;
		discover_params.uuid = NULL;
		discover_params.start_handle = mics_inst.start_handle;
		discover_params.end_handle = mics_inst.end_handle;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		discover_params.func = mics_discover_func;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			BT_DBG("Discover failed (err %d)", err);
			cur_mics_inst = NULL;
			cur_aics_inst = NULL;
			if (mics_client_cb && mics_client_cb->init) {
				mics_client_cb->init(conn, err, 0);
			}
		}

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}


int bt_mics_client_write_mute(struct bt_conn *conn, bool mute)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mics_inst.mute_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mics_inst.busy) {
		return -EBUSY;
	}

	mics_inst.write_buf[0] = mute;
	mics_inst.write_params.offset = 0;
	mics_inst.write_params.data = mics_inst.write_buf;
	mics_inst.write_params.length = sizeof(mute);
	mics_inst.write_params.handle = mics_inst.mute_handle;
	mics_inst.write_params.func = mics_client_write_mics_mute_cb;

	err = bt_gatt_write(conn, &mics_inst.write_params);
	if (!err) {
		mics_inst.busy = true;
	}
	return err;
}

int bt_mics_discover(struct bt_conn *conn)
{
	/*
	 * This will initiate a discover procedure. The procedure will do the
	 * following sequence:
	 * 1) Primary discover for the MICS
	 * 2) Characteristic discover of the MICS
	 * 3) Discover services included in MICS (AICS)
	 * 4) For each included service found; discovery of the characteristiscs
	 * 5) When everything above have been discovered, the callback is called
	 */

	if (!conn) {
		return -ENOTCONN;
	} else if (cur_mics_inst) {
		return -EBUSY;
	}

	cur_aics_inst = NULL;
	memset(&discover_params, 0, sizeof(discover_params));
	memset(&mics_inst, 0, sizeof(mics_inst));
	memcpy(&uuid, BT_UUID_MICS, sizeof(uuid));
	for (int i = 0; i < ARRAY_SIZE(mics_inst.aics); i++) {
		if (mics_client_cb) {
			mics_inst.aics[i].cb = &mics_client_cb->aics_cb;
		}
		bt_aics_client_unregister(AICS_CLI_MICS_CLIENT_INDEX(i));
	}
	discover_params.func = primary_discover_func;
	discover_params.uuid = &uuid.uuid;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.start_handle = FIRST_HANDLE;
	discover_params.end_handle = LAST_HANDLE;
	return bt_gatt_discover(conn, &discover_params);
}

void bt_mics_client_cb_register(struct bt_mics_cb_t *cb)
{
	mics_client_cb = cb;
	for (int i = 0; i < ARRAY_SIZE(mics_inst.aics); i++) {
		if (mics_client_cb) {
			mics_inst.aics[i].cb = &mics_client_cb->aics_cb;
		} else {
			mics_inst.aics[i].cb = NULL;
		}
	}
}

int bt_mics_client_service_get(struct bt_conn *conn, struct bt_mics *service)
{
	static struct bt_aics *aics[CONFIG_BT_MICS_CLIENT_MAX_AICS_INST];

	if (!service) {
		return -EINVAL;
	}

	service->aics_cnt = mics_inst.aics_inst_cnt;
	for (int i = 0; i < mics_inst.aics_inst_cnt; i++) {
		aics[i] = (struct bt_aics *)&mics_inst.aics[i];
	}

	service->aics = aics;

	return 0;
}

int bt_mics_client_mute_get(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!mics_inst.mute_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mics_inst.busy) {
		return -EBUSY;
	}

	mics_inst.read_params.func = mics_client_read_mute_cb;
	mics_inst.read_params.handle_count = 1;
	mics_inst.read_params.single.handle = mics_inst.mute_handle;
	mics_inst.read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mics_inst.read_params);
	if (!err) {
		mics_inst.busy = true;
	}
	return err;
}

int bt_mics_client_mute(struct bt_conn *conn)
{
	return bt_mics_client_write_mute(conn, true);
}

int bt_mics_client_unmute(struct bt_conn *conn)
{
	return bt_mics_client_write_mute(conn, false);
}

int bt_mics_client_aics_input_state_get(struct bt_conn *conn,
					uint8_t aics_index)
{
	if (CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {
		return bt_aics_client_input_state_get(
			conn, AICS_CLI_MICS_CLIENT_INDEX(aics_index));
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

int bt_mics_client_aics_gain_setting_get(struct bt_conn *conn,
					 uint8_t aics_index)
{
	if (CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {
		return bt_aics_client_gain_setting_get(
			conn, AICS_CLI_MICS_CLIENT_INDEX(aics_index));
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

int bt_mics_client_aics_input_type_get(struct bt_conn *conn, uint8_t aics_index)
{
	if (CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {
		return bt_aics_client_input_type_get(
			conn, AICS_CLI_MICS_CLIENT_INDEX(aics_index));
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

int bt_mics_client_aics_input_status_get(struct bt_conn *conn,
					 uint8_t aics_index)
{
	if (CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {
		return bt_aics_client_input_status_get(
			conn, AICS_CLI_MICS_CLIENT_INDEX(aics_index));
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

int bt_mics_client_aics_input_unmute(struct bt_conn *conn, uint8_t aics_index)
{
	if (CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {
		return bt_aics_client_input_unmute(
			conn, AICS_CLI_MICS_CLIENT_INDEX(aics_index));
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

int bt_mics_client_aics_input_mute(struct bt_conn *conn, uint8_t aics_index)
{

	if (CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {
		return bt_aics_client_input_mute(
			conn, AICS_CLI_MICS_CLIENT_INDEX(aics_index));
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

int bt_mics_client_aics_manual_input_gain_set(struct bt_conn *conn,
					      uint8_t aics_index)
{
	if (CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {
		return bt_aics_client_manual_input_gain_set(
			conn, AICS_CLI_MICS_CLIENT_INDEX(aics_index));
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

int bt_mics_client_aics_automatic_input_gain_set(struct bt_conn *conn,
						 uint8_t aics_index)
{
	if (CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {
		return bt_aics_client_automatic_input_gain_set(
			conn, AICS_CLI_MICS_CLIENT_INDEX(aics_index));
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

int bt_mics_client_aics_gain_set(struct bt_conn *conn, uint8_t aics_index,
				 int8_t gain)
{
	if (CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {
		return bt_aics_client_gain_set(
			conn, AICS_CLI_MICS_CLIENT_INDEX(aics_index), gain);
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

int bt_mics_client_aics_input_description_get(struct bt_conn *conn,
					      uint8_t aics_index)
{
	if (CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {
		return bt_aics_client_input_description_get(
			conn, AICS_CLI_MICS_CLIENT_INDEX(aics_index));
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}

int bt_mics_client_aics_input_description_set(struct bt_conn *conn,
					      uint8_t aics_index,
					      const char *description)
{
	if (CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {
		return bt_aics_client_input_description_set(
			conn, AICS_CLI_MICS_CLIENT_INDEX(aics_index),
			description);
	}

	BT_DBG("Not supported");
	return -EOPNOTSUPP;
}
