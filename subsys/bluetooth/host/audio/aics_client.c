/*  Bluetooth AICS client */

/*
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <zephyr/types.h>

#include <sys/check.h>

#include <device.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/aics.h>

#include "aics_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_AICS_CLIENT)
#define LOG_MODULE_NAME bt_aics_client
#include "common/log.h"

static struct bt_aics aics_insts
	[CONFIG_BT_MAX_CONN * CONFIG_BT_AICS_CLIENT_MAX_INSTANCE_COUNT];

static int aics_client_common_control(struct bt_conn *conn, uint8_t opcode,
				      struct bt_aics *inst);

static struct bt_aics *lookup_aics_by_handle(struct bt_conn *conn,
					     uint16_t handle)
{
	for (int i = 0; i < ARRAY_SIZE(aics_insts); i++) {
		if (aics_insts[i].cli.conn == conn &&
		    aics_insts[i].cli.active &&
		    aics_insts[i].cli.start_handle <= handle &&
		    aics_insts[i].cli.end_handle >= handle) {
			return &aics_insts[i];
		}
	}
	BT_DBG("Could not find AICS instance with handle 0x%04x", handle);
	return NULL;
}

uint8_t aics_client_notify_handler(struct bt_conn *conn,
				struct bt_gatt_subscribe_params *params,
				const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct bt_aics *inst = lookup_aics_by_handle(conn, handle);
	struct aics_state_t *state;
	uint8_t *status;
	char desc[MIN(CONFIG_BT_L2CAP_RX_MTU, BT_ATT_MAX_ATTRIBUTE_LEN) + 1];

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	if (data) {
		if (handle == inst->cli.state_handle) {
			if (length == sizeof(*state)) {
				state = (struct aics_state_t *)data;
				BT_DBG("Inst %p: Gain %d, mute %u, mode %u, "
				       "counter %u",
				       inst, state->gain,
				       state->mute, state->mode,
				       state->change_counter);
				inst->cli.change_counter =
					state->change_counter;
				if (inst->cli.cb && inst->cli.cb->state) {
					inst->cli.cb->state(conn, inst, 0,
							    state->gain,
							    state->mute,
							    state->mode);
				}
			}
		} else if (handle == inst->cli.status_handle) {
			if (length == sizeof(*status)) {
				status = (uint8_t *)data;
				BT_DBG("Inst %p: Status %u", inst, *status);
				if (inst->cli.cb && inst->cli.cb->status) {
					inst->cli.cb->status(conn, inst, 0,
							     *status);
				}
			}
		} else if (handle == inst->cli.desc_handle) {
			/* Truncate if too large */
			length = MIN(sizeof(desc) - 1, length);

			memcpy(desc, data, length);
			desc[length] = '\0';
			BT_DBG("Inst %p: Input description: %s",
			       inst, log_strdup(desc));
			if (inst->cli.cb && inst->cli.cb->description) {
				inst->cli.cb->description(conn, inst, 0, desc);
			}
		}
	}
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t aics_client_read_state_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_read_params *params,
					 const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	struct bt_aics *inst = lookup_aics_by_handle(conn,
						     params->single.handle);
	struct aics_state_t *state = (struct aics_state_t *)data;

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("Inst %p: err: 0x%02X", inst, err);
	inst->cli.busy = false;

	if (data) {
		if (length == sizeof(*state)) {
			BT_DBG("Gain %d, mute %u, mode %u, counter %u",
			       state->gain, state->mute, state->mode,
			       state->change_counter);
			inst->cli.change_counter = state->change_counter;
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(*state));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (inst->cli.cb && inst->cli.cb->state) {
		inst->cli.cb->state(conn, inst, cb_err, state->gain,
				    state->mute, state->mode);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t aics_client_read_gain_settings_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	struct bt_aics *inst = lookup_aics_by_handle(conn,
						     params->single.handle);
	struct aics_gain_settings_t *gain_settings =
		(struct aics_gain_settings_t *)data;

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("Inst %p: err: 0x%02X", inst, err);
	inst->cli.busy = false;

	if (data) {
		if (length == sizeof(*gain_settings)) {
			BT_DBG("Units %u, Max %d, Min %d",
			       gain_settings->units, gain_settings->maximum,
			       gain_settings->minimum);
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(*gain_settings));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (inst->cli.cb && inst->cli.cb->gain_setting) {
		inst->cli.cb->gain_setting(conn, inst, cb_err,
					   gain_settings->units,
					   gain_settings->minimum,
					   gain_settings->maximum);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t aics_client_read_type_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	uint8_t *input_type = (uint8_t *)data;
	struct bt_aics *inst = lookup_aics_by_handle(conn,
						     params->single.handle);

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("Inst %p: err: 0x%02X", inst, err);
	inst->cli.busy = false;

	if (data) {
		if (length == sizeof(*input_type)) {
			BT_DBG("Type %u", *input_type);
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(*input_type));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (inst->cli.cb && inst->cli.cb->type) {
		inst->cli.cb->type(conn, inst, cb_err, *input_type);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t aics_client_read_status_cb(struct bt_conn *conn, uint8_t err,
					  struct bt_gatt_read_params *params,
					  const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	uint8_t *status = (uint8_t *)data;
	struct bt_aics *inst = lookup_aics_by_handle(conn,
						     params->single.handle);

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("Inst %p: err: 0x%02X", inst, err);
	inst->cli.busy = false;

	if (data) {
		if (length == sizeof(*status)) {
			BT_DBG("Status %u", *status);
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(*status));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (inst->cli.cb && inst->cli.cb->status) {
		inst->cli.cb->status(conn, inst, cb_err, *status);
	}

	return BT_GATT_ITER_STOP;
}

static void aics_cp_notify_app(struct bt_conn *conn, struct bt_aics *inst,
			       uint8_t err)
{
	if (!inst->cli.cb) {
		return;
	}

	switch (inst->cli.cp_val.cp.opcode) {
	case AICS_OPCODE_SET_GAIN:
		if (inst->cli.cb->set_gain) {
			inst->cli.cb->set_gain(conn, inst, err);
		}
		break;
	case AICS_OPCODE_UNMUTE:
		if (inst->cli.cb->unmute) {
			inst->cli.cb->unmute(conn, inst, err);
		}
		break;
	case AICS_OPCODE_MUTE:
		if (inst->cli.cb->mute) {
			inst->cli.cb->mute(conn, inst, err);
		}
		break;
	case AICS_OPCODE_SET_MANUAL:
		if (inst->cli.cb->set_manual_mode) {
			inst->cli.cb->set_manual_mode(conn, inst, err);
		}
		break;
	case AICS_OPCODE_SET_AUTO:
		if (inst->cli.cb->set_auto_mode) {
			inst->cli.cb->set_auto_mode(conn, inst, err);
		}
		break;
	default:
		BT_DBG("Unknown opcode 0x%02x", inst->cli.cp_val.cp.opcode);
		break;
	}
}

static uint8_t internal_read_state_cb(struct bt_conn *conn, uint8_t err,
				      struct bt_gatt_read_params *params,
				      const void *data, uint16_t length)
{
	uint8_t cb_err = 0;
	struct bt_aics *inst = lookup_aics_by_handle(conn,
						     params->single.handle);
	struct aics_state_t *state = (struct aics_state_t *)data;

	if (!inst) {
		BT_ERR("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	if (err) {
		BT_WARN("Volume state read failed: %d", err);
		cb_err = BT_ATT_ERR_UNLIKELY;
	} else if (data) {
		if (length == sizeof(*state)) {
			int write_err;

			BT_DBG("Gain %d, mute %u, mode %u, counter %u",
			       state->gain, state->mute, state->mode,
			       state->change_counter);
			inst->cli.change_counter = state->change_counter;

			/* clear busy flag to reuse function */
			inst->cli.busy = false;

			if (inst->cli.cp_val.cp.opcode == AICS_OPCODE_SET_GAIN) {
				write_err = bt_aics_client_gain_set(
					conn, inst, inst->cli.cp_val.gain_setting);
			} else {
				write_err = aics_client_common_control(
					conn, inst->cli.cp_val.cp.opcode, inst);
			}

			if (write_err) {
				cb_err = BT_ATT_ERR_UNLIKELY;
			}
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(*state));
			cb_err = BT_ATT_ERR_UNLIKELY;
		}
	}

	if (cb_err) {
		inst->cli.busy = false;
		aics_cp_notify_app(conn, inst, cb_err);
	}

	return BT_GATT_ITER_STOP;
}


static void aics_client_write_aics_cp_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_write_params *params)
{
	struct bt_aics *inst = lookup_aics_by_handle(conn, params->handle);

	if (!inst) {
		BT_DBG("Instance not found");
		return;
	}

	BT_DBG("Inst %p: err: 0x%02X", inst, err);

	if (err == AICS_ERR_INVALID_COUNTER && inst->cli.state_handle) {
		int read_err;

		inst->cli.read_params.func = internal_read_state_cb;
		inst->cli.read_params.handle_count = 1;
		inst->cli.read_params.single.handle = inst->cli.state_handle;
		inst->cli.read_params.single.offset = 0U;

		read_err = bt_gatt_read(conn, &inst->cli.read_params);

		if (read_err) {
			BT_WARN("Could not read Volume state: %d", read_err);
		} else {
			return;
		}
	}

	inst->cli.busy = false;

	aics_cp_notify_app(conn, inst, err);
}

static int aics_client_common_control(struct bt_conn *conn, uint8_t opcode,
				      struct bt_aics *inst)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	} else if (!inst->cli.control_handle) {
		BT_DBG("Handle not set for opcode %u", opcode);
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	}

	inst->cli.cp_val.cp.opcode = opcode;
	inst->cli.cp_val.cp.counter = inst->cli.change_counter;

	inst->cli.write_params.offset = 0;
	inst->cli.write_params.data = &inst->cli.cp_val.cp;
	inst->cli.write_params.length = sizeof(inst->cli.cp_val.cp);
	inst->cli.write_params.handle = inst->cli.control_handle;
	inst->cli.write_params.func = aics_client_write_aics_cp_cb;

	err = bt_gatt_write(conn, &inst->cli.write_params);
	if (!err) {
		inst->cli.busy = true;
	}
	return err;
}


static uint8_t aics_client_read_desc_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	char desc[MIN(CONFIG_BT_L2CAP_RX_MTU, BT_ATT_MAX_ATTRIBUTE_LEN) + 1];
	struct bt_aics *inst = lookup_aics_by_handle(conn,
						     params->single.handle);

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	inst->cli.busy = false;

	if (err) {
		BT_DBG("err: 0x%02X", err);
	} else if (data) {
		BT_HEXDUMP_DBG(data, length, "Input description read");

		/* Truncate if too large */
		length = MIN(sizeof(desc) - 1, length);

		/* TODO: Handle long reads */

		memcpy(desc, data, length);
		desc[length] = '\0';
		BT_DBG("Input description: %s", log_strdup(desc));
	}

	if (inst->cli.cb && inst->cli.cb->description) {
		inst->cli.cb->description(conn, inst, cb_err, desc);
	}

	return BT_GATT_ITER_STOP;
}

static bool valid_inst_discovered(struct bt_aics *inst)
{
	return inst->cli.state_handle &&
	       inst->cli.gain_handle &&
	       inst->cli.type_handle &&
	       inst->cli.status_handle &&
	       inst->cli.control_handle &&
	       inst->cli.desc_handle;
}

static uint8_t aics_discover_func(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  struct bt_gatt_discover_params *params)
{
	struct bt_aics *inst = (struct bt_aics *)CONTAINER_OF(
				params, struct aics_client, discover_params);

	if (!attr) {
		BT_DBG("Discovery complete for AICS %p", inst);

		inst->cli.busy = false;

		if (inst->cli.cb && inst->cli.cb->discover) {
			int err = valid_inst_discovered(inst) ? 0 : -ENOENT;

			inst->cli.cb->discover(conn, inst, err);
		}

		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		struct bt_gatt_subscribe_params *sub_params = NULL;
		struct bt_gatt_chrc *chrc;

		chrc = (struct bt_gatt_chrc *)attr->user_data;
		if (inst->cli.start_handle == 0) {
			inst->cli.start_handle = chrc->value_handle;
		}
		inst->cli.end_handle = chrc->value_handle;

		if (!bt_uuid_cmp(chrc->uuid, BT_UUID_AICS_STATE)) {
			BT_DBG("Audio Input state");
			inst->cli.state_handle = chrc->value_handle;
			sub_params = &inst->cli.state_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_AICS_GAIN_SETTINGS)) {
			BT_DBG("Gain settings");
			inst->cli.gain_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_AICS_INPUT_TYPE)) {
			BT_DBG("Input type");
			inst->cli.type_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid,
					BT_UUID_AICS_INPUT_STATUS)) {
			BT_DBG("Input status");
			inst->cli.status_handle = chrc->value_handle;
			sub_params = &inst->cli.status_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_AICS_CONTROL)) {
			BT_DBG("Control point");
			inst->cli.control_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_AICS_DESCRIPTION)) {
			BT_DBG("Description");
			inst->cli.desc_handle = chrc->value_handle;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				sub_params =
					&inst->cli.desc_sub_params;
			}

			if (chrc->properties &
				BT_GATT_CHRC_WRITE_WITHOUT_RESP) {
				inst->cli.desc_writable = true;
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

static void aics_client_reset(struct bt_aics *inst, struct bt_conn *conn)
{
	inst->cli.desc_writable = 0;
	inst->cli.change_counter = 0;
	inst->cli.mode = 0;
	inst->cli.start_handle = 0;
	inst->cli.end_handle = 0;
	inst->cli.state_handle = 0;
	inst->cli.gain_handle = 0;
	inst->cli.type_handle = 0;
	inst->cli.status_handle = 0;
	inst->cli.control_handle = 0;
	inst->cli.desc_handle = 0;

	/* It's okay if these fail */
	(void)bt_gatt_unsubscribe(conn, &inst->cli.state_sub_params);
	(void)bt_gatt_unsubscribe(conn, &inst->cli.status_sub_params);
	(void)bt_gatt_unsubscribe(conn, &inst->cli.desc_sub_params);
}


int bt_aics_discover(struct bt_conn *conn, struct bt_aics *inst,
		     const struct bt_aics_discover_param *param)
{
	int err = 0;

	CHECKIF(!inst || !conn || !param) {
		BT_DBG("%s cannot be NULL",
		       inst == NULL ? "inst" : conn == NULL ? "conn" : "param");
		return -EINVAL;
	}

	CHECKIF(param->end_handle < param->start_handle) {
		BT_DBG("start_handle (%u) shall be less than end_handle (%u)",
		       param->start_handle, param->end_handle);
		return -EINVAL;
	}

	CHECKIF(!inst->cli.active) {
		BT_DBG("Inactive instance");
		return -EINVAL;
	}

	CHECKIF(inst->cli.busy) {
		BT_DBG("Instance is busy");
		return -EBUSY;
	}

	aics_client_reset(inst, conn);

	(void)memset(&inst->cli.discover_params, 0,
		     sizeof(inst->cli.discover_params));
	inst->cli.conn = conn;
	inst->cli.discover_params.start_handle = param->start_handle;
	inst->cli.discover_params.end_handle = param->end_handle;
	inst->cli.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	inst->cli.discover_params.func = aics_discover_func;

	err = bt_gatt_discover(conn, &inst->cli.discover_params);
	if (err) {
		BT_DBG("Discover failed (err %d)", err);
	} else {
		inst->cli.busy = true;
	}

	return err;
}

struct bt_aics *bt_aics_client_free_instance_get(void)
{
	for (int i = 0; i < ARRAY_SIZE(aics_insts); i++) {
		if (!aics_insts[i].cli.active) {
			aics_insts[i].cli.active = true;
			return &aics_insts[i];
		}
	}
	return NULL;
}

int bt_aics_client_state_get(struct bt_conn *conn, struct bt_aics *inst)
{
	int err;

	if (!inst->cli.state_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	}

	inst->cli.read_params.func = aics_client_read_state_cb;
	inst->cli.read_params.handle_count = 1;
	inst->cli.read_params.single.handle = inst->cli.state_handle;
	inst->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->cli.read_params);
	if (!err) {
		inst->cli.busy = true;
	}
	return err;
}

int bt_aics_client_gain_setting_get(struct bt_conn *conn, struct bt_aics *inst)
{
	int err;

	if (!inst->cli.gain_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	}

	inst->cli.read_params.func = aics_client_read_gain_settings_cb;
	inst->cli.read_params.handle_count = 1;
	inst->cli.read_params.single.handle = inst->cli.gain_handle;
	inst->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->cli.read_params);
	if (!err) {
		inst->cli.busy = true;
	}
	return err;
}

int bt_aics_client_type_get(struct bt_conn *conn, struct bt_aics *inst)
{
	int err;

	if (!inst->cli.type_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	}

	inst->cli.read_params.func = aics_client_read_type_cb;
	inst->cli.read_params.handle_count = 1;
	inst->cli.read_params.single.handle = inst->cli.type_handle;
	inst->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->cli.read_params);
	if (!err) {
		inst->cli.busy = true;
	}
	return err;
}

int bt_aics_client_status_get(struct bt_conn *conn, struct bt_aics *inst)
{
	int err;

	if (!inst->cli.status_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	}

	inst->cli.read_params.func = aics_client_read_status_cb;
	inst->cli.read_params.handle_count = 1;
	inst->cli.read_params.single.handle = inst->cli.status_handle;
	inst->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->cli.read_params);
	if (!err) {
		inst->cli.busy = true;
	}
	return err;
}

int bt_aics_client_unmute(struct bt_conn *conn, struct bt_aics *inst)
{
	return aics_client_common_control(conn, AICS_OPCODE_UNMUTE, inst);
}

int bt_aics_client_mute(struct bt_conn *conn, struct bt_aics *inst)
{
	return aics_client_common_control(conn, AICS_OPCODE_MUTE, inst);
}

int bt_aics_client_manual_gain_set(struct bt_conn *conn, struct bt_aics *inst)
{
	return aics_client_common_control(conn, AICS_OPCODE_SET_MANUAL, inst);
}

int bt_aics_client_automatic_gain_set(struct bt_conn *conn,
				      struct bt_aics *inst)
{
	return aics_client_common_control(conn, AICS_OPCODE_SET_AUTO, inst);
}

int bt_aics_client_gain_set(struct bt_conn *conn, struct bt_aics *inst,
			    int8_t gain)
{
	int err;

	if (!inst->cli.control_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	}

	inst->cli.cp_val.cp.opcode = AICS_OPCODE_SET_GAIN;
	inst->cli.cp_val.cp.counter = inst->cli.change_counter;
	inst->cli.cp_val.gain_setting = gain;

	inst->cli.write_params.offset = 0;
	inst->cli.write_params.data = &inst->cli.cp_val;
	inst->cli.write_params.length = sizeof(inst->cli.cp_val);
	inst->cli.write_params.handle = inst->cli.control_handle;
	inst->cli.write_params.func = aics_client_write_aics_cp_cb;

	err = bt_gatt_write(conn, &inst->cli.write_params);
	if (!err) {
		inst->cli.busy = true;
	}
	return err;
}

int bt_aics_client_description_get(struct bt_conn *conn, struct bt_aics *inst)
{
	int err;

	if (!inst->cli.desc_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	}

	inst->cli.read_params.func = aics_client_read_desc_cb;
	inst->cli.read_params.handle_count = 1;
	inst->cli.read_params.single.handle = inst->cli.desc_handle;
	inst->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->cli.read_params);
	if (!err) {
		inst->cli.busy = true;
	}
	return err;
}

int bt_aics_client_description_set(struct bt_conn *conn, struct bt_aics *inst,
				   const char *description)
{
	int err;

	if (!inst->cli.desc_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	} else if (!inst->cli.desc_writable) {
		BT_DBG("Description is not writable on peer "
			"service instance");
		return -EPERM;
	}

	err = bt_gatt_write_without_response(conn, inst->cli.desc_handle,
					     description, strlen(description),
					     false);
	return err;
}

void bt_aics_client_cb_register(struct bt_aics *inst, struct bt_aics_cb *cb)
{
	CHECKIF(!inst) {
		BT_DBG("inst cannot be NULL");
		return;
	}

	inst->cli.cb = cb;
}
