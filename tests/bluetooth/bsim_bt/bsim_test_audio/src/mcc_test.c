/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <sys/byteorder.h>
#include <sys/sem.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/mcc.h>

#include "../../../../subsys/bluetooth/host/audio/mpl.h"

#include "common.h"

extern enum bst_result_t bst_result;
static uint8_t expected_passes = 1;
static uint8_t passes;

static struct bt_conn *default_conn;
static struct bt_mcc_cb_t mcc_cb;

CREATE_FLAG(ble_is_initialized);
CREATE_FLAG(ble_link_is_ready);

/* Callback after Bluetoot initialization attempt */
static void bt_ready(int err)
{
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");
	SET_FLAG(ble_is_initialized);
}

/* Callback on connection */
static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected: %s\n", addr);
	default_conn = conn;
	SET_FLAG(ble_link_is_ready);
}

void test_main(void)
{
	int err;
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
	};

	printk("Media Control Client test application.  Board: %s\n", CONFIG_BOARD);

	err = bt_enable(bt_ready);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(ble_is_initialized);

	bt_conn_cb_register(&conn_callbacks);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		FAIL("Failed to start scanning (err %d\n)", err);
	} else {
		printk("Scanning started successfully\n");
	}

	WAIT_FOR_FLAG(ble_link_is_ready);

	PASS("MCC passed\n");
}

static const struct bst_test_instance test_mcs[] = {
	{
		.test_id = "mcc",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_mcc_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_mcs);
}
