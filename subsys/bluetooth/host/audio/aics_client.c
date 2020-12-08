/*  Bluetooth AICS client */

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
#include <bluetooth/services/aics.h>

#include "aics_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_AICS_CLIENT)
#define LOG_MODULE_NAME bt_aics_client
#include "common/log.h"

/* The profile clients that uses the AICS are responsible for discovery and
 * will simply register any found AICS instances as pointers, which is stored
 * here
 */
struct aics_client *insts[CONFIG_BT_AICS_CLIENT_MAX_INSTANCE_COUNT];

static int aics_client_common_control(struct bt_conn *conn, uint8_t opcode,
				      struct aics_client *inst);

static struct aics_client *lookup_aics_by_handle(uint16_t handle)
{
	for (int i = 0; i < ARRAY_SIZE(insts); i++) {
		if (insts[i] &&
		    insts[i]->start_handle <= handle &&
		    insts[i]->end_handle >= handle) {
			return insts[i];
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
	struct aics_client *inst = lookup_aics_by_handle(handle);
	struct aics_state_t *state;
	uint8_t *status;
	char desc[MIN(CONFIG_BT_L2CAP_RX_MTU, BT_ATT_MAX_ATTRIBUTE_LEN) + 1];

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	if (data) {
		if (handle == inst->state_handle) {
			if (length == sizeof(*state)) {
				state = (struct aics_state_t *)data;
				BT_DBG("Inst %p: Gain %d, mute %u, mode %u, "
				       "counter %u",
				       inst, state->gain,
				       state->mute, state->mode,
				       state->change_counter);
				inst->change_counter = state->change_counter;
				if (inst->cb && inst->cb->state) {
					inst->cb->state(
						conn, (struct bt_aics *)inst, 0,
						state->gain, state->mute,
						state->mode);
				}
			}
		} else if (handle == inst->status_handle) {
			if (length == sizeof(*status)) {
				status = (uint8_t *)data;
				BT_DBG("Inst %p: Status %u", inst, *status);
				if (inst->cb && inst->cb->status) {
					inst->cb->status(
						conn, (struct bt_aics *)inst,
						0, *status);
				}
			}
		} else if (handle == inst->desc_handle) {
			if (length > BT_ATT_MAX_ATTRIBUTE_LEN) {
				BT_DBG("Length (%u) too large", length);
				return BT_GATT_ITER_CONTINUE;
			}
			memcpy(desc, data, length);
			desc[length] = '\0';
			BT_DBG("Inst %p: Input description: %s",
			       inst, log_strdup(desc));
			if (inst->cb && inst->cb->description) {
				inst->cb->description(
					conn, (struct bt_aics *)inst, 0,
					desc);
			}
		}
	}
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t aics_client_read_input_state_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	struct aics_client *inst =
		lookup_aics_by_handle(params->single.handle);
	struct aics_state_t *state = (struct aics_state_t *)data;

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("Inst %p: err: 0x%02X", inst, err);
	inst->busy = false;

	if (data) {
		if (length == sizeof(*state)) {
			BT_DBG("Gain %d, mute %u, mode %u, counter %u",
			       state->gain, state->mute, state->mode,
			       state->change_counter);
			inst->change_counter = state->change_counter;
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(*state));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (inst->cb && inst->cb->state) {
		inst->cb->state(conn, (struct bt_aics *)inst, cb_err,
				     state->gain, state->mute, state->mode);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t aics_client_read_gain_settings_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	struct aics_client *inst =
		lookup_aics_by_handle(params->single.handle);
	struct aics_gain_settings_t *gain_settings =
		(struct aics_gain_settings_t *)data;

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("Inst %p: err: 0x%02X", inst, err);
	inst->busy = false;

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

	if (inst->cb && inst->cb->gain_setting) {
		inst->cb->gain_setting(conn, (struct bt_aics *)inst, cb_err,
				       gain_settings->units,
				       gain_settings->minimum,
				       gain_settings->maximum);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t aics_client_read_input_type_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_read_params *params,
					   const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	uint8_t *input_type = (uint8_t *)data;
	struct aics_client *inst =
		lookup_aics_by_handle(params->single.handle);

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("Inst %p: err: 0x%02X", inst, err);
	inst->busy = false;

	if (data) {
		if (length == sizeof(*input_type)) {
			BT_DBG("Type %u", *input_type);
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(*input_type));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (inst->cb && inst->cb->type) {
		inst->cb->type(conn, (struct bt_aics *)inst, cb_err,
			       *input_type);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t aics_client_read_input_status_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	uint8_t *status = (uint8_t *)data;
	struct aics_client *inst =
		lookup_aics_by_handle(params->single.handle);

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("Inst %p: err: 0x%02X", inst, err);
	inst->busy = false;

	if (data) {
		if (length == sizeof(*status)) {
			BT_DBG("Status %u", *status);
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(*status));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (inst->cb && inst->cb->status) {
		inst->cb->status(conn, (struct bt_aics *)inst, cb_err, *status);
	}

	return BT_GATT_ITER_STOP;
}

static void aics_cp_notify_app(struct bt_conn *conn,
			       struct aics_client *inst,
			       uint8_t err)
{
	struct aics_control_t *cp = (struct aics_control_t *)inst->write_buf;

	if (!inst->cb) {
		return;
	}

	switch (cp->opcode) {
	case AICS_OPCODE_SET_GAIN:
		if (inst->cb->set_gain) {
			inst->cb->set_gain(conn, (struct bt_aics *)inst, err);
		}
		break;
	case AICS_OPCODE_UNMUTE:
		if (inst->cb->unmute) {
			inst->cb->unmute(conn, (struct bt_aics *)inst, err);
		}
		break;
	case AICS_OPCODE_MUTE:
		if (inst->cb->mute) {
			inst->cb->mute(conn, (struct bt_aics *)inst, err);
		}
		break;
	case AICS_OPCODE_SET_MANUAL:
		if (inst->cb->set_manual_mode) {
			inst->cb->set_manual_mode(
				conn, (struct bt_aics *)inst, err);
		}
		break;
	case AICS_OPCODE_SET_AUTO:
		if (inst->cb->set_auto_mode) {
			inst->cb->set_auto_mode(
				conn, (struct bt_aics *)inst, err);
		}
		break;
	default:
		BT_DBG("Unknown opcode 0x%02x", cp->opcode);
		break;
	}
}

static uint8_t internal_read_input_state_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length)
{
	uint8_t cb_err = 0;
	struct aics_client *inst =
		lookup_aics_by_handle(params->single.handle);
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
			struct aics_control_t *cp;

			BT_DBG("Gain %d, mute %u, mode %u, counter %u",
			       state->gain, state->mute, state->mode,
			       state->change_counter);
			inst->change_counter = state->change_counter;

			/* clear busy flag to reuse function */
			inst->busy = false;

			cp = (struct aics_control_t *)inst->write_buf;
			if (cp->opcode == AICS_OPCODE_SET_GAIN) {
				struct aics_gain_control_t *set_gain_cp =
					(struct aics_gain_control_t *)cp;

				write_err = bt_aics_client_gain_set(
					conn, (struct bt_aics *)inst,
					set_gain_cp->gain_setting);
			} else {
				write_err = aics_client_common_control(
					conn, cp->opcode, inst);
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
		inst->busy = false;
		aics_cp_notify_app(conn, inst, cb_err);
	}

	return BT_GATT_ITER_STOP;
}


static void aics_client_write_aics_cp_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_write_params *params)
{
	struct aics_client *inst = lookup_aics_by_handle(params->handle);

	if (!inst) {
		BT_DBG("Instance not found");
		return;
	}

	BT_DBG("Inst %p: err: 0x%02X", inst, err);

	if (err == AICS_ERR_INVALID_COUNTER && inst->state_handle) {
		int read_err;

		inst->read_params.func = internal_read_input_state_cb;
		inst->read_params.handle_count = 1;
		inst->read_params.single.handle = inst->state_handle;
		inst->read_params.single.offset = 0U;

		read_err = bt_gatt_read(conn, &inst->read_params);

		if (read_err) {
			BT_WARN("Could not read Volume state: %d", read_err);
		} else {
			return;
		}
	}

	inst->busy = false;

	aics_cp_notify_app(conn, inst, err);
}

static int aics_client_common_control(struct bt_conn *conn, uint8_t opcode,
				      struct aics_client *inst)
{
	int err;
	struct aics_control_t *cp;

	if (!conn) {
		return -ENOTCONN;
	} else if (!inst->control_handle) {
		BT_DBG("Handle not set for opcode %u", opcode);
		return -EINVAL;
	} else if (inst->busy) {
		return -EBUSY;
	}

	cp = (struct aics_control_t *)inst->write_buf;
	cp->opcode = opcode;
	cp->counter = inst->change_counter;
	inst->write_params.offset = 0;
	inst->write_params.data = inst->write_buf;
	inst->write_params.length =
		sizeof(opcode) + sizeof(inst->change_counter);
	inst->write_params.handle = inst->control_handle;
	inst->write_params.func = aics_client_write_aics_cp_cb;

	err = bt_gatt_write(conn, &inst->write_params);
	if (!err) {
		inst->busy = true;
	}
	return err;
}


static uint8_t aics_client_read_input_desc_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_read_params *params,
					   const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	char desc[MIN(CONFIG_BT_L2CAP_RX_MTU, BT_ATT_MAX_ATTRIBUTE_LEN) + 1];
	struct aics_client *inst =
		lookup_aics_by_handle(params->single.handle);

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	inst->busy = false;

	if (err) {
		BT_DBG("err: 0x%02X", err);
	} else if (data) {
		BT_HEXDUMP_DBG(data, length, "Input description read");

		if (length > BT_ATT_MAX_ATTRIBUTE_LEN) {
			BT_DBG("Length (%u) too large", length);
			return BT_GATT_ITER_CONTINUE;
		}

		/* TODO: Handle long reads */

		memcpy(desc, data, length);
		desc[length] = '\0';
		BT_DBG("Input description: %s", log_strdup(desc));
	}

	if (inst->cb && inst->cb->description) {
		inst->cb->description(conn, (struct bt_aics *)inst, cb_err,
				      desc);
	}

	return BT_GATT_ITER_STOP;
}

int bt_aics_client_register(struct bt_aics *inst)
{
	struct aics_client **free = NULL;

	__ASSERT(inst, "inst is null");

	for (int i = 0; i < ARRAY_SIZE(insts); i++) {
		if (free == NULL && insts[i] == NULL) {
			free = &insts[i];
		}

		if ((struct aics_client *)inst == insts[i]) {
			return -EALREADY;
		}
	}

	if (free) {
		*free = (struct aics_client *)inst;
	}

	return 0;
}

int bt_aics_client_unregister(struct bt_aics *inst)
{
	__ASSERT(inst, "inst is null");

	for (int i = 0; i < ARRAY_SIZE(insts); i++) {
		if ((struct aics_client *)inst == insts[i]) {
			insts[i] = NULL;
			return 0;
		}
	}

	return -EINVAL;
}

int bt_aics_client_input_state_get(struct bt_conn *conn,
				   struct bt_aics *inst)
{
	int err;

	if (!inst->cli.state_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	}

	inst->cli.read_params.func = aics_client_read_input_state_cb;
	inst->cli.read_params.handle_count = 1;
	inst->cli.read_params.single.handle = inst->cli.state_handle;
	inst->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->cli.read_params);
	if (!err) {
		inst->cli.busy = true;
	}
	return err;
}

int bt_aics_client_gain_setting_get(struct bt_conn *conn,
				    struct bt_aics *inst)
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

int bt_aics_client_input_type_get(struct bt_conn *conn,
				  struct bt_aics *inst)
{
	int err;

	if (!inst->cli.type_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	}

	inst->cli.read_params.func = aics_client_read_input_type_cb;
	inst->cli.read_params.handle_count = 1;
	inst->cli.read_params.single.handle = inst->cli.type_handle;
	inst->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->cli.read_params);
	if (!err) {
		inst->cli.busy = true;
	}
	return err;
}

int bt_aics_client_input_status_get(struct bt_conn *conn,
				    struct bt_aics *inst)
{
	int err;

	if (!inst->cli.status_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	}

	inst->cli.read_params.func = aics_client_read_input_status_cb;
	inst->cli.read_params.handle_count = 1;
	inst->cli.read_params.single.handle = inst->cli.status_handle;
	inst->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->cli.read_params);
	if (!err) {
		inst->cli.busy = true;
	}
	return err;
}

int bt_aics_client_input_unmute(struct bt_conn *conn,
				struct bt_aics *inst)
{
	return aics_client_common_control(conn, AICS_OPCODE_UNMUTE, &inst->cli);
}

int bt_aics_client_input_mute(struct bt_conn *conn, struct bt_aics *inst)
{
	return aics_client_common_control(conn, AICS_OPCODE_MUTE, &inst->cli);
}

int bt_aics_client_manual_input_gain_set(struct bt_conn *conn,
					 struct bt_aics *inst)
{
	return aics_client_common_control(conn, AICS_OPCODE_SET_MANUAL,
					  &inst->cli);
}

int bt_aics_client_automatic_input_gain_set(struct bt_conn *conn,
					    struct bt_aics *inst)
{
	return aics_client_common_control(conn, AICS_OPCODE_SET_AUTO,
					  &inst->cli);
}

int bt_aics_client_gain_set(struct bt_conn *conn, struct bt_aics *inst,
			    int8_t gain)
{
	int err;
	struct aics_gain_control_t cp = {
		.cp.opcode = AICS_OPCODE_SET_GAIN,
		.gain_setting = gain
	};

	if (!inst->cli.control_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	}

	cp.cp.counter = inst->cli.change_counter;

	memcpy(inst->cli.write_buf, &cp, sizeof(cp));
	inst->cli.write_params.offset = 0;
	inst->cli.write_params.data = inst->cli.write_buf;
	inst->cli.write_params.length = sizeof(cp);
	inst->cli.write_params.handle = inst->cli.control_handle;
	inst->cli.write_params.func = aics_client_write_aics_cp_cb;

	err = bt_gatt_write(conn, &inst->cli.write_params);
	if (!err) {
		inst->cli.busy = true;
	}
	return err;
}

int bt_aics_client_input_description_get(struct bt_conn *conn,
					 struct bt_aics *inst)
{
	int err;

	if (!inst->cli.desc_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	}

	inst->cli.read_params.func = aics_client_read_input_desc_cb;
	inst->cli.read_params.handle_count = 1;
	inst->cli.read_params.single.handle = inst->cli.desc_handle;
	inst->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &inst->cli.read_params);
	if (!err) {
		inst->cli.busy = true;
	}
	return err;
}

int bt_aics_client_input_description_set(struct bt_conn *conn,
					 struct bt_aics *inst,
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
