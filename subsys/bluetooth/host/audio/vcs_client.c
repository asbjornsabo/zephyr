/*  Bluetooth TBS - Call Control Profile - Client */

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
#include <bluetooth/services/vcs.h>
#include <bluetooth/services/vocs.h>

#include "vcs_internal.h"
#include "aics_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_VCS_CLIENT)
#define LOG_MODULE_NAME bt_vcs_client
#include "common/log.h"

#define FIRST_HANDLE				0x0001
#define LAST_HANDLE				0xFFFF

struct vcs_instance_t {
	struct vcs_state_t state;
	uint8_t flags;

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t state_handle;
	uint16_t control_handle;
	uint16_t flag_handle;
	struct bt_gatt_subscribe_params state_sub_params;
	struct bt_gatt_subscribe_params flag_sub_params;

	bool busy;
	struct vcs_control_vol cp_val;
	struct bt_gatt_write_params write_params;
	struct bt_gatt_read_params read_params;
	struct bt_gatt_discover_params discover_params;
	struct bt_uuid_16 uuid;

	uint8_t vocs_inst_cnt;
	struct bt_vocs *vocs[CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST];
	uint8_t aics_inst_cnt;
	struct bt_aics *aics[CONFIG_BT_VCS_CLIENT_MAX_AICS_INST];
};

/* Callback functions */
static struct bt_vcs_cb_t *vcs_client_cb;

static struct vcs_instance_t vcs_inst;
static int vcs_client_common_vcs_cp(struct bt_conn *conn, uint8_t opcode);

bool bt_vcs_client_valid_vocs_inst(struct bt_vocs *vocs)
{
	if (!vocs) {
		return false;
	}

	for (int i = 0; i < ARRAY_SIZE(vcs_inst.vocs); i++) {
		if (vcs_inst.vocs[i] == vocs) {
			return true;
		}
	}

	return false;
}

bool bt_vcs_client_valid_aics_inst(struct bt_aics *aics)
{
	if (!aics) {
		return false;
	}

	for (int i = 0; i < ARRAY_SIZE(vcs_inst.aics); i++) {
		if (vcs_inst.aics[i] == aics) {
			return true;
		}
	}

	return false;
}

static uint8_t vcs_notify_handler(struct bt_conn *conn,
				  struct bt_gatt_subscribe_params *params,
				  const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;

	if (data) {
		if (handle == vcs_inst.state_handle) {
			if (length == sizeof(vcs_inst.state)) {
				memcpy(&vcs_inst.state, data, length);
				BT_DBG("Volume %u, mute %u, counter %u",
				       vcs_inst.state.volume,
				       vcs_inst.state.mute,
				       vcs_inst.state.change_counter);
				if (vcs_client_cb && vcs_client_cb->state) {
					vcs_client_cb->state(
						conn, 0, vcs_inst.state.volume,
						vcs_inst.state.mute);
				}
			}
		} else if (handle == vcs_inst.flag_handle) {
			if (length == sizeof(vcs_inst.flags)) {
				memcpy(&vcs_inst.flags, data, length);
				BT_DBG("Flags %u", vcs_inst.flags);
				if (vcs_client_cb && vcs_client_cb->flags) {
					vcs_client_cb->flags(conn, 0,
							     vcs_inst.flags);
				}
			}
		}
	}
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t vcs_client_read_volume_state_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length)
{
	uint8_t cb_err = err;

	vcs_inst.busy = false;

	if (err) {
		BT_DBG("err: 0x%02X", err);
	} else if (data) {
		if (length == sizeof(vcs_inst.state)) {
			memcpy(&vcs_inst.state, data, length);
			BT_DBG("Volume %u, mute %u, counter %u",
			       vcs_inst.state.volume,
			       vcs_inst.state.mute,
			       vcs_inst.state.change_counter);
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(vcs_inst.state));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (vcs_client_cb && vcs_client_cb->state) {
		vcs_client_cb->state(conn, cb_err,
				  vcs_inst.state.volume,
				  vcs_inst.state.mute);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t vcs_client_read_flag_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	uint8_t cb_err = err;

	vcs_inst.busy = false;

	if (err) {
		BT_DBG("err: 0x%02X", err);
	} else if (data) {
		if (length == sizeof(vcs_inst.flags)) {
			memcpy(&vcs_inst.flags, data, length);
			BT_DBG("Flags %u", vcs_inst.flags);
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(vcs_inst.flags));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (vcs_client_cb && vcs_client_cb->flags) {
		vcs_client_cb->flags(conn, cb_err, vcs_inst.flags);
	}

	return BT_GATT_ITER_STOP;
}

static void vcs_cp_notify_app(struct bt_conn *conn, uint8_t opcode, uint8_t err)
{
	if (!vcs_client_cb) {
		return;
	}

	switch (opcode) {
	case VCS_OPCODE_REL_VOL_DOWN:
		if (vcs_client_cb->vol_down) {
			vcs_client_cb->vol_down(conn, err);
		}
		break;
	case VCS_OPCODE_REL_VOL_UP:
		if (vcs_client_cb->vol_up) {
			vcs_client_cb->vol_up(conn, err);
		}
		break;
	case VCS_OPCODE_UNMUTE_REL_VOL_DOWN:
		if (vcs_client_cb->vol_down_unmute) {
			vcs_client_cb->vol_down_unmute(conn, err);
		}
		break;
	case VCS_OPCODE_UNMUTE_REL_VOL_UP:
		if (vcs_client_cb->vol_up_unmute) {
			vcs_client_cb->vol_up_unmute(conn, err);
		}
		break;
	case VCS_OPCODE_SET_ABS_VOL:
		if (vcs_client_cb->vol_set) {
			vcs_client_cb->vol_set(conn, err);
		}
		break;
	case VCS_OPCODE_UNMUTE:
		if (vcs_client_cb->unmute) {
			vcs_client_cb->unmute(conn, err);
		}
		break;
	case VCS_OPCODE_MUTE:
		if (vcs_client_cb->mute) {
			vcs_client_cb->mute(conn, err);
		}
		break;
	default:
		BT_DBG("Unknown opcode 0x%02x", opcode);
		break;
	}
}

static uint8_t internal_read_volume_state_cb(
	struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
	const void *data, uint16_t length)
{
	uint8_t cb_err = 0;
	uint8_t opcode = vcs_inst.cp_val.cp.opcode;

	if (err) {
		BT_WARN("Volume state read failed: %d", err);
		cb_err = BT_ATT_ERR_UNLIKELY;
	} else if (data) {
		if (length == sizeof(vcs_inst.state)) {
			int write_err;

			memcpy(&vcs_inst.state, data, length);
			BT_DBG("Volume %u, mute %u, counter %u",
			       vcs_inst.state.volume,
			       vcs_inst.state.mute,
			       vcs_inst.state.change_counter);

			/* clear busy flag to reuse function */
			vcs_inst.busy = false;
			if (vcs_inst.cp_val.cp.opcode == VCS_OPCODE_SET_ABS_VOL) {
				write_err = bt_vcs_client_set_volume(conn, vcs_inst.cp_val.volume);
			} else {
				write_err = vcs_client_common_vcs_cp(conn,
								     opcode);
			}
			if (write_err) {
				cb_err = BT_ATT_ERR_UNLIKELY;
			}
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(vcs_inst.state));
			cb_err = BT_ATT_ERR_UNLIKELY;
		}
	}

	if (cb_err) {
		vcs_inst.busy = false;
		vcs_cp_notify_app(conn, opcode, BT_ATT_ERR_UNLIKELY);
	}

	return BT_GATT_ITER_STOP;
}

static void vcs_client_write_vcs_cp_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_write_params *params)
{
	uint8_t opcode = vcs_inst.cp_val.cp.opcode;

	BT_DBG("err: 0x%02X", err);

	if (err == VCS_ERR_INVALID_COUNTER && vcs_inst.state_handle) {
		int read_err;

		vcs_inst.read_params.func = internal_read_volume_state_cb;
		vcs_inst.read_params.handle_count = 1;
		vcs_inst.read_params.single.handle = vcs_inst.state_handle;
		vcs_inst.read_params.single.offset = 0U;

		read_err = bt_gatt_read(conn, &vcs_inst.read_params);

		if (read_err) {
			BT_WARN("Could not read Volume state: %d", read_err);
		} else {
			return;
		}
	}

	vcs_inst.busy = false;

	vcs_cp_notify_app(conn, opcode, err);
}

#if (CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0 || \
	CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0)
static uint8_t vcs_discover_include_func(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 struct bt_gatt_discover_params *params)
{
	struct bt_gatt_include *include;
	uint8_t inst_idx;
	int err;

	if (!attr) {
		BT_DBG("Discover include complete for VCS: %u AICS and %u VOCS",
		       vcs_inst.aics_inst_cnt,
		       vcs_inst.vocs_inst_cnt);
		(void)memset(params, 0, sizeof(*params));

		if (vcs_client_cb && vcs_client_cb->discover) {
			vcs_client_cb->discover(conn, 0, vcs_inst.vocs_inst_cnt,
						vcs_inst.aics_inst_cnt);
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_INCLUDE) {
		include = (struct bt_gatt_include *)attr->user_data;
		BT_DBG("Include UUID %s", bt_uuid_str(include->uuid));
#if CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0
		if (!bt_uuid_cmp(include->uuid, BT_UUID_AICS) &&
		    vcs_inst.aics_inst_cnt <
			CONFIG_BT_VCS_CLIENT_MAX_AICS_INST) {

			struct bt_aics_discover_param param = {
				.start_handle = include->start_handle,
				.end_handle = include->end_handle,
			};

			/* Update discover params so we can continue where we
			 * left off after bt_vocs_discover
			 */
			vcs_inst.discover_params.start_handle =
				attr->handle + 1;

			inst_idx = vcs_inst.aics_inst_cnt++;
			err = bt_aics_discover(conn, vcs_inst.aics[inst_idx],
					       &param);
			if (err) {
				BT_DBG("AICS Discover failed (err %d)", err);
				if (vcs_client_cb && vcs_client_cb->discover) {
					vcs_client_cb->discover(conn, err, 0,
								0);
				}
			}
			return BT_GATT_ITER_STOP;
		}
#endif /* CONFIG_BT_VCS_CLIENT_MAX_AICS_INST */
#if CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0
		if (!bt_uuid_cmp(include->uuid, BT_UUID_VOCS) &&
		    vcs_inst.vocs_inst_cnt <
			CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST) {

			struct bt_vocs_discover_param param = {
				.start_handle = include->start_handle,
				.end_handle = include->end_handle,
			};

			/* Update discover params so we can continue where we
			 * left off after bt_vocs_discover
			 */
			vcs_inst.discover_params.start_handle =
				attr->handle + 1;

			inst_idx = vcs_inst.vocs_inst_cnt++;
			err = bt_vocs_discover(conn, vcs_inst.vocs[inst_idx],
					       &param);
			if (err) {
				BT_DBG("VOCS Discover failed (err %d)", err);
				if (vcs_client_cb && vcs_client_cb->discover) {
					vcs_client_cb->discover(conn, err, 0,
								0);
				}
			}
			return BT_GATT_ITER_STOP;
		}
#endif /* CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST */
	}

	return BT_GATT_ITER_CONTINUE;
}
#endif /* (CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0 ||
	*  CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0)
	*/

/**
 * @brief This will discover all characteristics on the server, retrieving the
 * handles of the writeable characteristics and subscribing to all notify and
 * indicate characteristics.
 */
static uint8_t vcs_discover_func(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 struct bt_gatt_discover_params *params)
{
	int err = 0;
	struct bt_gatt_chrc *chrc;
	struct bt_gatt_subscribe_params *sub_params = NULL;

	if (!attr) {
		BT_DBG("Setup complete for VCS");
		(void)memset(params, 0, sizeof(*params));
#if (CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0 || \
	CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0)
		/* Discover included services */
		vcs_inst.discover_params.start_handle = vcs_inst.start_handle;
		vcs_inst.discover_params.end_handle = vcs_inst.end_handle;
		vcs_inst.discover_params.type = BT_GATT_DISCOVER_INCLUDE;
		vcs_inst.discover_params.func = vcs_discover_include_func;

		err = bt_gatt_discover(conn, &vcs_inst.discover_params);
		if (err) {
			BT_DBG("Discover failed (err %d)", err);
			if (vcs_client_cb && vcs_client_cb->discover) {
				vcs_client_cb->discover(conn, err, 0, 0);
			}
		}
#else
		if (vcs_client_cb && vcs_client_cb->discover) {
			vcs_client_cb->discover(conn, err, 0, 0);
		}
#endif /* (CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0 ||
	* CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0)
	*/
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		chrc = (struct bt_gatt_chrc *)attr->user_data;
		if (!bt_uuid_cmp(chrc->uuid, BT_UUID_VCS_STATE)) {
			BT_DBG("Volume state");
			vcs_inst.state_handle = chrc->value_handle;
			sub_params = &vcs_inst.state_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_VCS_CONTROL)) {
			BT_DBG("Control Point");
			vcs_inst.control_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_VCS_FLAGS)) {
			BT_DBG("Flags");
			vcs_inst.flag_handle = chrc->value_handle;
			sub_params = &vcs_inst.flag_sub_params;
		}

		if (sub_params) {
			sub_params->value = BT_GATT_CCC_NOTIFY;
			sub_params->value_handle = chrc->value_handle;
			/*
			 * TODO: Don't assume that CCC is at handle + 2;
			 * do proper discovery;
			 */
			sub_params->ccc_handle = attr->handle + 2;
			sub_params->notify = vcs_notify_handler;
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
		BT_DBG("Could not find a VCS instance on the server");
		if (vcs_client_cb && vcs_client_cb->discover) {
			vcs_client_cb->discover(conn, -ENODATA, 0, 0);
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		BT_DBG("Primary discover complete");
		prim_service = (struct bt_gatt_service_val *)attr->user_data;
		vcs_inst.discover_params.start_handle = attr->handle + 1;

		vcs_inst.start_handle = attr->handle + 1;
		vcs_inst.end_handle = prim_service->end_handle;

		/* Discover characteristics */
		vcs_inst.discover_params.uuid = NULL;
		vcs_inst.discover_params.start_handle = vcs_inst.start_handle;
		vcs_inst.discover_params.end_handle = vcs_inst.end_handle;
		vcs_inst.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		vcs_inst.discover_params.func = vcs_discover_func;

		err = bt_gatt_discover(conn, &vcs_inst.discover_params);
		if (err) {
			BT_DBG("Discover failed (err %d)", err);
			if (vcs_client_cb && vcs_client_cb->discover) {
				vcs_client_cb->discover(conn, err, 0, 0);
			}
		}

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static int vcs_client_common_vcs_cp(struct bt_conn *conn, uint8_t opcode)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!vcs_inst.control_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (vcs_inst.busy) {
		return -EBUSY;
	}

	vcs_inst.busy = true;
	vcs_inst.cp_val.cp.opcode = opcode;
	vcs_inst.cp_val.cp.counter = vcs_inst.state.change_counter;
	vcs_inst.write_params.offset = 0;
	vcs_inst.write_params.data = &vcs_inst.cp_val.cp;
	vcs_inst.write_params.length = sizeof(vcs_inst.cp_val.cp);
	vcs_inst.write_params.handle = vcs_inst.control_handle;
	vcs_inst.write_params.func = vcs_client_write_vcs_cp_cb;

	err = bt_gatt_write(conn, &vcs_inst.write_params);
	if (!err) {
		vcs_inst.busy = true;
	}
	return err;
}

static void aics_discover_cb(struct bt_conn *conn, struct bt_aics *inst,
			     int err)
{
	if (!err) {
		/* Continue discovery of included services */
		err = bt_gatt_discover(conn, &vcs_inst.discover_params);
	}

	if (err) {
		BT_DBG("Discover failed (err %d)", err);
		if (vcs_client_cb && vcs_client_cb->discover) {
			vcs_client_cb->discover(conn, err, 0, 0);
		}
	}
}

static void vocs_discover_cb(struct bt_conn *conn, struct bt_vocs *inst,
			     int err)
{
	if (!err) {
		/* Continue discovery of included services */
		err = bt_gatt_discover(conn, &vcs_inst.discover_params);
	}

	if (err) {
		BT_DBG("Discover failed (err %d)", err);
		if (vcs_client_cb && vcs_client_cb->discover) {
			vcs_client_cb->discover(conn, err, 0, 0);
		}
	}
}

static void vcs_client_reset(struct bt_conn *conn)
{
	memset(&vcs_inst.state, 0, sizeof(vcs_inst.state));
	vcs_inst.flags = 0;
	vcs_inst.start_handle = 0;
	vcs_inst.end_handle = 0;
	vcs_inst.state_handle = 0;
	vcs_inst.control_handle = 0;
	vcs_inst.flag_handle = 0;
	vcs_inst.vocs_inst_cnt = 0;
	vcs_inst.aics_inst_cnt = 0;

	memset(&vcs_inst.discover_params, 0, sizeof(vcs_inst.discover_params));

	/* It's okay if these fail */
	(void)bt_gatt_unsubscribe(conn, &vcs_inst.state_sub_params);
	(void)bt_gatt_unsubscribe(conn, &vcs_inst.flag_sub_params);
}

int bt_vcs_discover(struct bt_conn *conn)
{
	static bool initialized;
	/*
	 * This will initiate a discover procedure. The procedure will do the
	 * following sequence:
	 * 1) Primary discover for the VCS
	 * 2) Characteristic discover of the VCS
	 * 3) Discover services included in VCS (VOCS and AICS)
	 * 4) For each included service found; discovery of the characteristics
	 * 5) When everything above have been discovered, the callback is called
	 */

	if (!conn) {
		return -ENOTCONN;
	} else if (vcs_inst.busy) {
		return -EBUSY;
	}

	vcs_client_reset(conn);
	memcpy(&vcs_inst.uuid, BT_UUID_VCS, sizeof(vcs_inst.uuid));

	if (IS_ENABLED(CONFIG_BT_VOCS_CLIENT) &&
	    CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0) {
		for (int i = 0; i < ARRAY_SIZE(vcs_inst.vocs); i++) {
			if (!initialized) {
				vcs_inst.vocs[i] =
					bt_vocs_client_free_instance_get();

				if (!vcs_inst.vocs[i]) {
					return -ENOMEM;
				}

				bt_vocs_client_cb_register(
					vcs_inst.vocs[i],
					&vcs_client_cb->vocs_cb);
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) &&
	    CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0) {
		for (int i = 0; i < ARRAY_SIZE(vcs_inst.aics); i++) {
			if (!initialized) {
				vcs_inst.aics[i] =
					bt_aics_client_free_instance_get();

				if (!vcs_inst.aics[i]) {
					return -ENOMEM;
				}

				bt_aics_client_cb_register(
					vcs_inst.aics[i],
					&vcs_client_cb->aics_cb);
			}
		}
	}

	vcs_inst.discover_params.func = primary_discover_func;
	vcs_inst.discover_params.uuid = &vcs_inst.uuid.uuid;
	vcs_inst.discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	vcs_inst.discover_params.start_handle = FIRST_HANDLE;
	vcs_inst.discover_params.end_handle = LAST_HANDLE;

	initialized = true;
	return bt_gatt_discover(conn, &vcs_inst.discover_params);
}

void bt_vcs_client_cb_register(struct bt_vcs_cb_t *cb)
{
	vcs_client_cb = cb;
	if (IS_ENABLED(CONFIG_BT_VOCS_CLIENT)) {
		if (cb) {
			if (cb->vocs_cb.discover) {
				BT_WARN("VCS overwrote discover callback "
					"of VOCS");
			}
			cb->vocs_cb.discover = vocs_discover_cb;

			for (int i = 0; i < ARRAY_SIZE(vcs_inst.vocs); i++) {
				if (vcs_inst.vocs[i]) {
					bt_vocs_client_cb_register(
						vcs_inst.vocs[i], &cb->vocs_cb);
				}
			}
		}
	}
	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT)) {
		if (cb) {
			if (cb->aics_cb.discover) {
				BT_WARN("VCS overwrote discover callback "
					"of AICS");
			}
			cb->aics_cb.discover = aics_discover_cb;

			for (int i = 0; i < ARRAY_SIZE(vcs_inst.aics); i++) {
				if (vcs_inst.aics[i]) {
					bt_aics_client_cb_register(
						vcs_inst.aics[i], &cb->aics_cb);
				}
			}
		}
	}
}

int bt_vcs_client_get(struct bt_conn *conn, struct bt_vcs *client)
{
	static struct bt_vocs *vocs[CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST];
	static struct bt_aics *aics[CONFIG_BT_VCS_CLIENT_MAX_AICS_INST];

	if (!client) {
		return -EINVAL;
	}

	client->vocs_cnt = vcs_inst.vocs_inst_cnt;
	for (int i = 0; i < vcs_inst.vocs_inst_cnt; i++) {
		vocs[i] = vcs_inst.vocs[i];
	}

	client->aics_cnt = vcs_inst.aics_inst_cnt;
	for (int i = 0; i < vcs_inst.aics_inst_cnt; i++) {
		aics[i] = vcs_inst.aics[i];
	}

	client->vocs = vocs;
	client->aics = aics;

	return 0;
}

int bt_vcs_client_read_volume_state(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!vcs_inst.state_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (vcs_inst.busy) {
		return -EBUSY;
	}

	vcs_inst.read_params.func = vcs_client_read_volume_state_cb;
	vcs_inst.read_params.handle_count = 1;
	vcs_inst.read_params.single.handle = vcs_inst.state_handle;
	vcs_inst.read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &vcs_inst.read_params);
	if (!err) {
		vcs_inst.busy = true;
	}
	return err;
}

int bt_vcs_client_read_flags(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!vcs_inst.flag_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (vcs_inst.busy) {
		return -EBUSY;
	}

	vcs_inst.read_params.func = vcs_client_read_flag_cb;
	vcs_inst.read_params.handle_count = 1;
	vcs_inst.read_params.single.handle = vcs_inst.flag_handle;
	vcs_inst.read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &vcs_inst.read_params);
	if (!err) {
		vcs_inst.busy = true;
	}
	return err;
}

int bt_vcs_client_volume_down(struct bt_conn *conn)
{
	return vcs_client_common_vcs_cp(conn, VCS_OPCODE_REL_VOL_DOWN);
}

int bt_vcs_client_volume_up(struct bt_conn *conn)
{
	return vcs_client_common_vcs_cp(conn, VCS_OPCODE_REL_VOL_UP);
}

int bt_vcs_client_unmute_volume_down(struct bt_conn *conn)
{
	return vcs_client_common_vcs_cp(conn, VCS_OPCODE_UNMUTE_REL_VOL_DOWN);
}

int bt_vcs_client_unmute_volume_up(struct bt_conn *conn)
{
	return vcs_client_common_vcs_cp(conn, VCS_OPCODE_UNMUTE_REL_VOL_UP);
}

int bt_vcs_client_set_volume(struct bt_conn *conn, uint8_t volume)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!vcs_inst.control_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (vcs_inst.busy) {
		return -EBUSY;
	}

	vcs_inst.cp_val.cp.opcode = VCS_OPCODE_SET_ABS_VOL;
	vcs_inst.cp_val.cp.counter = vcs_inst.state.change_counter;
	vcs_inst.cp_val.volume = volume;

	vcs_inst.busy = true;
	vcs_inst.write_params.offset = 0;
	vcs_inst.write_params.data = &vcs_inst.cp_val;
	vcs_inst.write_params.length = sizeof(vcs_inst.cp_val);
	vcs_inst.write_params.handle = vcs_inst.control_handle;
	vcs_inst.write_params.func = vcs_client_write_vcs_cp_cb;

	err = bt_gatt_write(conn, &vcs_inst.write_params);
	if (!err) {
		vcs_inst.busy = true;
	}
	return err;
}

int bt_vcs_client_unmute(struct bt_conn *conn)
{
	return vcs_client_common_vcs_cp(conn, VCS_OPCODE_UNMUTE);
}

int bt_vcs_client_mute(struct bt_conn *conn)
{
	return vcs_client_common_vcs_cp(conn, VCS_OPCODE_MUTE);
}
