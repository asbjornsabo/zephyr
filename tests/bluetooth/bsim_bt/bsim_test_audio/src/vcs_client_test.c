/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_VCS_CLIENT

#include "bluetooth/services/vcs.h"
#include "common.h"

static struct bt_conn_cb conn_callbacks;
extern enum bst_result_t bst_result;
static uint8_t expected_passes = 1;
static uint8_t passes;

static volatile bool g_bt_init;
static volatile bool g_is_connected;
static volatile bool g_discovery_complete;
struct bt_conn *g_conn;

static void vcs_discover_cb(struct bt_conn *conn, int err, uint8_t vocs_count,
			    uint8_t aics_count)
{
	printk("%s\n", __func__);
	if (err) {
		FAIL("VCS could not be discovered (%d)\n", err);
		return;
	}

	g_discovery_complete = true;
}

static struct bt_vcs_cb_t vcs_cbs = {
	.discover = vcs_discover_cb
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}
	printk("Connected to %s\n", addr);
	g_conn = conn;
	g_is_connected = true;
}

static void bt_ready(int err)
{
	if (err) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	g_bt_init = true;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void test_main(void)
{
	int err;
	int vcs_err;

	err = bt_enable(bt_ready);

	if (err) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);
	bt_vcs_client_cb_register(&vcs_cbs);

	WAIT_FOR(g_bt_init);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");

	WAIT_FOR(g_is_connected);

	vcs_err = bt_vcs_discover(g_conn);
	if (vcs_err) {
		FAIL("Failed to discover VCS for connection %d", vcs_err);
	}

	WAIT_FOR(g_discovery_complete);

	PASS("VCS client Passed");
}

static const struct bst_test_instance test_vcs[] = {
	{
		.test_id = "vcs_client",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_vcs_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_vcs);
}

#else

struct bst_test_list *test_vcs_client_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_VCS_CLIENT */
