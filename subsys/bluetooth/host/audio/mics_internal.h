/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICS_INTERNAL_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICS_INTERNAL_
#include <zephyr/types.h>
#include <bluetooth/gatt.h>

int bt_mics_client_service_get(struct bt_conn *conn, struct bt_mics *service);
int bt_mics_client_mute_get(struct bt_conn *conn);
int bt_mics_client_mute(struct bt_conn *conn);
int bt_mics_client_unmute(struct bt_conn *conn);

int bt_mics_client_aics_input_state_get(struct bt_conn *conn,
					uint8_t aics_index);
int bt_mics_client_aics_gain_setting_get(struct bt_conn *conn,
					 uint8_t aics_index);
int bt_mics_client_aics_input_type_get(struct bt_conn *conn,
				       uint8_t aics_index);
int bt_mics_client_aics_input_status_get(struct bt_conn *conn,
					 uint8_t aics_index);
int bt_mics_client_aics_input_unmute(struct bt_conn *conn, uint8_t aics_index);
int bt_mics_client_aics_input_mute(struct bt_conn *conn, uint8_t aics_index);
int bt_mics_client_aics_manual_input_gain_set(struct bt_conn *conn,
					      uint8_t aics_index);
int bt_mics_client_aics_automatic_input_gain_set(struct bt_conn *conn,
						 uint8_t aics_index);
int bt_mics_client_aics_gain_set(struct bt_conn *conn, uint8_t aics_index,
				 int8_t gain);
int bt_mics_client_aics_input_description_get(struct bt_conn *conn,
					      uint8_t aics_index);
int bt_mics_client_aics_input_description_set(struct bt_conn *conn,
					      uint8_t aics_index,
					      const char *description);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICS_INTERNAL_ */
