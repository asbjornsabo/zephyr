/*  Bluetooth MICS
 *
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/byteorder.h>

#include <device.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/mics.h>

#include "aics_internal.h"
#include "mics_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MICS)
#define LOG_MODULE_NAME bt_mics
#include "common/log.h"

#define MICS_MUTE_UNMUTED		0x00
#define MICS_MUTE_MUTED			0x01
#define MICS_MUTE_DISABLED		0x02

struct mics_inst_t {
	uint8_t mute;
	struct bt_mics_cb_t *cb;
	struct bt_gatt_service *service_p;
	/* TODO: Use instance pointers instead of indexes */
	struct bt_aics *aics_insts[CONFIG_BT_MICS_AICS_INSTANCE_COUNT];
};

#if defined(CONFIG_BT_MICS)
static struct mics_inst_t mics_inst;

static void mute_cfg_changed(const struct bt_gatt_attr *attr,
				     uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t read_mute(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	BT_DBG("Mute %u", mics_inst.mute);
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &mics_inst.mute, sizeof(mics_inst.mute));
}

static ssize_t write_mute(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  const void *buf, uint16_t len, uint16_t offset,
			  uint8_t flags)
{
	const uint8_t *val = buf;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(mics_inst.mute)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if ((conn && *val == MICS_MUTE_DISABLED) || *val > MICS_MUTE_DISABLED) {
		return BT_GATT_ERR(BT_MICS_ERR_VAL_OUT_OF_RANGE);
	}

	if (conn && mics_inst.mute == MICS_MUTE_DISABLED) {
		return BT_GATT_ERR(BT_MICS_ERR_MUTE_DISABLED);
	}

	BT_DBG("%u", *val);

	if (*val != mics_inst.mute) {
		mics_inst.mute = *val;


		bt_gatt_notify_uuid(NULL, BT_UUID_MICS_MUTE,
				    mics_inst.service_p->attrs,
				    &mics_inst.mute, sizeof(mics_inst.mute));

		if (mics_inst.cb && mics_inst.cb->mute) {
			mics_inst.cb->mute(NULL, 0, mics_inst.mute);
		}
	}

	return len;
}


#define DUMMY_INCLUDE(i, _) BT_GATT_INCLUDE_SERVICE(NULL),
#define AICS_INCLUDES(cnt) UTIL_LISTIFY(cnt, DUMMY_INCLUDE)

#define BT_MICS_SERVICE_DEFINITION \
	BT_GATT_PRIMARY_SERVICE(BT_UUID_MICS), \
	AICS_INCLUDES(CONFIG_BT_MICS_AICS_INSTANCE_COUNT) \
	BT_GATT_CHARACTERISTIC(BT_UUID_MICS_MUTE, \
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY, \
		BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT, \
		read_mute, write_mute, NULL), \
	BT_GATT_CCC(mute_cfg_changed, \
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)

#define MICS_ATTR_COUNT \
	ARRAY_SIZE(((struct bt_gatt_attr []){ BT_MICS_SERVICE_DEFINITION }))
#define MICS_INCL_COUNT (CONFIG_BT_MICS_AICS_INSTANCE_COUNT)

static struct bt_gatt_attr mics_attrs[] = { BT_MICS_SERVICE_DEFINITION };
static struct bt_gatt_service mics_svc;

int bt_mics_init(struct bt_mics_init *init)
{
	int err;
	int i;
	int j;

	for (j = 0, i = 0; i < ARRAY_SIZE(mics_attrs); i++) {
		if (!bt_uuid_cmp(mics_attrs[i].uuid, BT_UUID_GATT_INCLUDE)) {
			mics_inst.aics_insts[j] = bt_aics_free_instance_get();

			if (!mics_inst.aics_insts[j]) {
				BT_DBG("Could not get free AICS instances[%u]",
				       j);
				return -ENOMEM;
			}

			err = bt_aics_init(mics_inst.aics_insts[j],
					   init ? &init->aics_init[j] : NULL);
			if (err) {
				BT_DBG("Could not init AICS instance[%u]: %d",
				       j, err);
				return err;
			}

			mics_attrs[i].user_data =
				bt_aics_svc_decl_get(mics_inst.aics_insts[j]);
			j++;

			if (j == CONFIG_BT_MICS_AICS_INSTANCE_COUNT) {
				break;
			}
		}
	}

	mics_svc = (struct bt_gatt_service)BT_GATT_SERVICE(mics_attrs);
	mics_inst.service_p = &mics_svc;
	err = bt_gatt_service_register(&mics_svc);

	if (err) {
		BT_ERR("MICS service register failed: %d", err);
	}

	return err;
}


/****************************** PUBLIC API ******************************/
int bt_mics_aics_deactivate(uint8_t aics_index)
{
	if (aics_index >= CONFIG_BT_MICS_AICS_INSTANCE_COUNT) {
		return -EINVAL;
	}

	return bt_aics_deactivate(AICS_MICS_INDEX(aics_index));
}

int bt_mics_aics_activate(uint8_t aics_index)
{
	if (aics_index >= CONFIG_BT_MICS_AICS_INSTANCE_COUNT) {
		return -EINVAL;
	}

	return bt_aics_activate(AICS_MICS_INDEX(aics_index));
}


int bt_mics_mute_disable(void)
{
	uint8_t val = MICS_MUTE_DISABLED;
	int err = write_mute(NULL, NULL, &val, sizeof(val), 0, 0);

	return err > 0 ? 0 : err;
}

void bt_mics_server_cb_register(struct bt_mics_cb_t *cb)
{
	int err;

	mics_inst.cb = cb;

	for (int i = 0; i < CONFIG_BT_MICS_AICS_INSTANCE_COUNT; i++) {
		if (cb) {
			err = bt_aics_cb_register(AICS_MICS_INDEX(i),
						  &mics_inst.cb->aics_cb);
		} else {
			err = bt_aics_cb_register(i, NULL);
		}

		if (err) {
			BT_WARN("[%d] Could not register AICS callbacks", i);
		}
	}
}
#endif /* CONFIG_BT_MICS */

int bt_mics_unmute(struct bt_conn *conn)
{
#if defined(CONFIG_BT_MICS_CLIENT)
	if (conn) {
		return bt_mics_client_unmute(conn);
	}
#endif /* CONFIG_BT_MICS_CLIENT */

#if defined(CONFIG_BT_MICS)
	if (!conn) {
		uint8_t val = MICS_MUTE_UNMUTED;
		int err = write_mute(NULL, NULL, &val, sizeof(val), 0, 0);

		return err > 0 ? 0 : err;
	}
#endif /* CONFIG_BT_MICS */
	return -EOPNOTSUPP;
}

int bt_mics_mute(struct bt_conn *conn)
{
#if defined(CONFIG_BT_MICS_CLIENT)
	if (conn) {
		return bt_mics_client_mute(conn);
	}
#endif /* CONFIG_BT_MICS_CLIENT */

#if defined(CONFIG_BT_MICS)
	if (!conn) {
		uint8_t val = MICS_MUTE_MUTED;
		int err = write_mute(NULL, NULL, &val, sizeof(val), 0, 0);

		return err > 0 ? 0 : err;
	}
#endif /* CONFIG_BT_MICS */
	return -EOPNOTSUPP;
}

int bt_mics_mute_get(struct bt_conn *conn)
{
#if defined(CONFIG_BT_MICS_CLIENT)
	if (conn) {
		return bt_mics_client_mute_get(conn);
	}
#endif /* CONFIG_BT_MICS_CLIENT */

#if defined(CONFIG_BT_MICS)
	if (!conn) {
		if (mics_inst.cb && mics_inst.cb->mute) {
			mics_inst.cb->mute(NULL, 0, mics_inst.mute);
		}

		return 0;
	}
#endif /* CONFIG_BT_MICS */
	return -EOPNOTSUPP;
}

int bt_mics_aics_state_get(struct bt_conn *conn, uint8_t aics_index)
{
#if CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0
	if (conn) {
		return bt_mics_client_aics_input_state_get(conn, aics_index);
	}
#endif /* CONFIG_BT_MICS_CLIENT_MAX_AICS_INST */

#if CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0
	if (!conn) {
		return bt_aics_input_state_get(AICS_MICS_INDEX(aics_index));
	}
#endif /* CONFIG_BT_MICS_AICS_INSTANCE_COUNT */
	return -EOPNOTSUPP;
}

int bt_mics_aics_gain_setting_get(struct bt_conn *conn, uint8_t aics_index)
{
#if CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0
	if (conn) {
		return bt_mics_client_aics_gain_setting_get(conn, aics_index);
	}
#endif /* CONFIG_BT_MICS_CLIENT_MAX_AICS_INST */

#if CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0
	if (!conn) {
		return bt_aics_gain_setting_get(AICS_MICS_INDEX(aics_index));
	}
#endif /* CONFIG_BT_MICS_AICS_INSTANCE_COUNT */
	return -EOPNOTSUPP;
}

int bt_mics_aics_type_get(struct bt_conn *conn, uint8_t aics_index)
{
#if CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0
	if (conn) {
		return bt_mics_client_aics_input_type_get(conn, aics_index);
	}
#endif /* CONFIG_BT_MICS_CLIENT_MAX_AICS_INST */

#if CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0
	if (!conn) {
		return bt_aics_input_type_get(AICS_MICS_INDEX(aics_index));
	}
#endif /* CONFIG_BT_MICS_AICS_INSTANCE_COUNT */
	return -EOPNOTSUPP;
}

int bt_mics_aics_status_get(struct bt_conn *conn, uint8_t aics_index)
{
#if CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0
	if (conn) {
		return bt_mics_client_aics_input_status_get(conn, aics_index);
	}
#endif /* CONFIG_BT_MICS_CLIENT_MAX_AICS_INST */

#if CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0
	if (!conn) {
		return bt_aics_input_status_get(AICS_MICS_INDEX(aics_index));
	}
#endif /* CONFIG_BT_MICS_AICS_INSTANCE_COUNT */
	return -EOPNOTSUPP;
}
int bt_mics_aics_unmute(struct bt_conn *conn, uint8_t aics_index)
{
#if CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0
	if (conn) {
		return bt_mics_client_aics_input_unmute(conn, aics_index);
	}
#endif /* CONFIG_BT_MICS_CLIENT_MAX_AICS_INST */

#if CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0
	if (!conn) {
		return bt_aics_input_unmute(AICS_MICS_INDEX(aics_index));
	}
#endif /* CONFIG_BT_MICS_AICS_INSTANCE_COUNT */
	return -EOPNOTSUPP;
}

int bt_mics_aics_mute(struct bt_conn *conn, uint8_t aics_index)
{
#if CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0
	if (conn) {
		return bt_mics_client_aics_input_mute(conn, aics_index);
	}
#endif /* CONFIG_BT_MICS_CLIENT_MAX_AICS_INST */

#if CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0
	if (!conn) {
		return bt_aics_input_mute(AICS_MICS_INDEX(aics_index));
	}
#endif /* CONFIG_BT_MICS_AICS_INSTANCE_COUNT */
	return -EOPNOTSUPP;
}

int bt_mics_aics_manual_gain_set(struct bt_conn *conn, uint8_t aics_index)
{
#if CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0
	if (conn) {
		return bt_mics_client_aics_manual_input_gain_set(conn,
								 aics_index);
	}
#endif /* CONFIG_BT_MICS_CLIENT_MAX_AICS_INST */

#if CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0
	if (!conn) {
		return bt_aics_manual_input_gain_set(
			AICS_MICS_INDEX(aics_index));
	}
#endif /* CONFIG_BT_MICS_AICS_INSTANCE_COUNT */
	return -EOPNOTSUPP;
}

int bt_mics_aics_automatic_gain_set(struct bt_conn *conn, uint8_t aics_index)
{
#if CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0
	if (conn) {
		return bt_mics_client_aics_automatic_input_gain_set(conn,
								    aics_index);
	}
#endif /* CONFIG_BT_MICS_CLIENT_MAX_AICS_INST */

#if CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0
	if (!conn) {
		return bt_aics_automatic_input_gain_set(
			AICS_MICS_INDEX(aics_index));
	}
#endif /* CONFIG_BT_MICS_AICS_INSTANCE_COUNT */
	return -EOPNOTSUPP;
}

int bt_mics_aics_gain_set(struct bt_conn *conn, uint8_t aics_index, int8_t gain)
{
#if CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0
	if (conn) {
		return bt_mics_client_aics_gain_set(conn, aics_index, gain);
	}
#endif /* CONFIG_BT_MICS_CLIENT_MAX_AICS_INST */

#if CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0
	if (!conn) {
		return bt_aics_gain_set(AICS_MICS_INDEX(aics_index), gain);
	}
#endif /* CONFIG_BT_MICS_AICS_INSTANCE_COUNT */
	return -EOPNOTSUPP;
}

int bt_mics_aics_description_get(struct bt_conn *conn, uint8_t aics_index)
{
#if CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0
	if (conn) {
		return bt_mics_client_aics_input_description_get(conn,
								 aics_index);
	}
#endif /* CONFIG_BT_MICS_CLIENT_MAX_AICS_INST */

#if CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0
	if (!conn) {
		return bt_aics_input_description_get(
			AICS_MICS_INDEX(aics_index));
	}
#endif /* CONFIG_BT_MICS_AICS_INSTANCE_COUNT */
	return -EOPNOTSUPP;
}

int bt_mics_aics_description_set(struct bt_conn *conn, uint8_t aics_index,
				 const char *description)
{
#if CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0
	if (conn) {
		return bt_mics_client_aics_input_description_set(conn,
								 aics_index,
								 description);
	}
#endif /* CONFIG_BT_MICS_CLIENT_MAX_AICS_INST */

#if CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0
	if (!conn) {
		return bt_aics_input_description_set(
			AICS_MICS_INDEX(aics_index), description);
	}
#endif /* CONFIG_BT_MICS_AICS_INSTANCE_COUNT */
	return -EOPNOTSUPP;
}
