/** @file
 *  @brief Internal APIs for Bluetooth AICS.
 */

/*
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_AICS_INTERNAL_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_AICS_INTERNAL_
#include <zephyr/types.h>
#include <bluetooth/gatt.h>

#if defined(CONFIG_BT_AICS)
#define AICS_MAX_DESC_SIZE CONFIG_BT_AICS_MAX_INPUT_DESCRIPTION_SIZE
#else
#define AICS_MAX_DESC_SIZE 0
#endif /* CONFIG_BT_AICS */

/* AICS opcodes */
#define AICS_OPCODE_SET_GAIN                    0x01
#define AICS_OPCODE_UNMUTE                      0x02
#define AICS_OPCODE_MUTE                        0x03
#define AICS_OPCODE_SET_MANUAL                  0x04
#define AICS_OPCODE_SET_AUTO                    0x05

/* AICS status */
#define AICS_STATUS_INACTIVE                    0x00
#define AICS_STATUS_ACTIVE                      0x01

/* AICS macros */
#if CONFIG_BT_VCS
#define VCS_COUNT CONFIG_BT_VCS_AICS_INSTANCE_COUNT
#else
#define VCS_COUNT 0
#endif

#if CONFIG_BT_MICS
#define MICS_COUNT CONFIG_BT_MICS_AICS_INSTANCE_COUNT
#else
#define MICS_COUNT 0
#endif

#define AICS_INST_COUNT (MICS_COUNT + VCS_COUNT)

/* AICS client macros */
#if CONFIG_BT_VCS_CLIENT
#define VCS_CLIENT_COUNT CONFIG_BT_VCS_CLIENT_MAX_AICS_INST
#else
#define VCS_CLIENT_COUNT 0
#endif

#if CONFIG_BT_MICS_CLIENT
#define MICS_CLIENT_COUNT CONFIG_BT_MICS_CLIENT_MAX_AICS_INST
#else
#define MICS_CLIENT_COUNT 0
#endif

#define AICS_CLIENT_INST_COUNT (MICS_CLIENT_COUNT + VCS_CLIENT_COUNT)

#define AICS_INPUT_MODE_IMMUTABLE(mode) \
	((mode) == AICS_MODE_MANUAL_ONLY || (mode) == AICS_MODE_AUTO_ONLY)


#define AICS_INPUT_MODE_SETTABLE(mode) \
	((mode) == AICS_MODE_MANUAL_ONLY || (mode) == AICS_MODE_MANUAL)

struct aics_control_t {
	uint8_t opcode;
	uint8_t counter;
} __packed;

struct aics_gain_control_t {
	struct aics_control_t cp;
	int8_t gain_setting;
} __packed;

struct aics_client {
	uint8_t change_counter;
	uint8_t mode;
	bool desc_writable;
	bool active;

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t state_handle;
	uint16_t gain_handle;
	uint16_t type_handle;
	uint16_t status_handle;
	uint16_t control_handle;
	uint16_t desc_handle;
	struct bt_gatt_subscribe_params state_sub_params;
	struct bt_gatt_subscribe_params status_sub_params;
	struct bt_gatt_subscribe_params desc_sub_params;
	uint8_t subscribe_cnt;

	bool busy;
	uint8_t write_buf[sizeof(struct aics_gain_control_t)];
	struct bt_gatt_write_params write_params;
	struct bt_gatt_read_params read_params;
	struct bt_gatt_discover_params discover_params;
	struct bt_aics_cb *cb;
	struct bt_conn *conn;
};

struct aics_state_t {
	int8_t gain;
	uint8_t mute;
	uint8_t mode;
	uint8_t change_counter;
} __packed;

struct aics_gain_settings_t {
	uint8_t units;
	int8_t minimum;
	int8_t maximum;
} __packed;

struct aics_server {
	struct aics_state_t state;
	struct aics_gain_settings_t gain_settings;
	bool initialized;
	uint8_t type;
	uint8_t status;
	struct bt_aics *inst;
	char input_desc[AICS_MAX_DESC_SIZE];
	struct bt_aics_cb *cb;

	struct bt_gatt_service *service_p;
};

/* Struct used as a common type for the api */
struct bt_aics {
	union {
		struct aics_server srv;
		struct aics_client cli;
	};

};

uint8_t aics_client_notify_handler(struct bt_conn *conn,
				   struct bt_gatt_subscribe_params *params,
				   const void *data, uint16_t length);

int bt_aics_client_register(struct bt_aics *inst);
int bt_aics_client_unregister(struct bt_aics *inst);
int bt_aics_client_state_get(struct bt_conn *conn, struct bt_aics *inst);
int bt_aics_client_gain_setting_get(struct bt_conn *conn, struct bt_aics *inst);
int bt_aics_client_type_get(struct bt_conn *conn, struct bt_aics *inst);
int bt_aics_client_status_get(struct bt_conn *conn, struct bt_aics *inst);
int bt_aics_client_unmute(struct bt_conn *conn, struct bt_aics *inst);
int bt_aics_client_mute(struct bt_conn *conn, struct bt_aics *inst);
int bt_aics_client_manual_gain_set(struct bt_conn *conn,
					 struct bt_aics *inst);
int bt_aics_client_automatic_gain_set(struct bt_conn *conn,
					    struct bt_aics *inst);
int bt_aics_client_gain_set(struct bt_conn *conn, struct bt_aics *inst,
			    int8_t gain);
int bt_aics_client_description_get(struct bt_conn *conn,
					 struct bt_aics *inst);
int bt_aics_client_description_set(struct bt_conn *conn,
					 struct bt_aics *inst,
					 const char *description);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_AICS_INTERNAL_ */
