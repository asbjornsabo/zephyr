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
#include "../../../../subsys/bluetooth/host/audio/otc.h"

#include "common.h"

extern enum bst_result_t bst_result;
static uint8_t expected_passes = 1;
static uint8_t passes;

static struct bt_conn *default_conn;
static struct bt_mcc_cb_t mcc_cb;

uint64_t g_icon_object_id;
uint64_t g_current_track_object_id;
uint64_t g_next_track_object_id;
uint64_t g_track_segments_object_id;
uint64_t g_current_group_object_id;
uint64_t g_parent_group_object_id;

CREATE_FLAG(ble_is_initialized);
CREATE_FLAG(ble_link_is_ready);
CREATE_FLAG(mcc_is_initialized);
CREATE_FLAG(discovery_done);
CREATE_FLAG(icon_object_id_read);
CREATE_FLAG(track_segments_object_id_read);
CREATE_FLAG(current_track_object_id_read);
CREATE_FLAG(next_track_object_id_read);
CREATE_FLAG(current_group_object_id_read);
CREATE_FLAG(parent_group_object_id_read);
CREATE_FLAG(object_selected);
CREATE_FLAG(metadata_read);
CREATE_FLAG(object_read);


static void mcc_init_cb(struct bt_conn *conn, int err)
{
	if (err) {
		FAIL("MCC init failed (%d)\n", err);
		return;
	}

	printk("MCC init succeeded\n");
	SET_FLAG(mcc_is_initialized);
}

static void mcc_discover_mcs_cb(struct bt_conn *conn, int err)
{
	if (err) {
		FAIL("Discovery of MCS failed (%d)\n", err);
		return;
	}

	printk("Discovery of MCS succeeded\n");
	SET_FLAG(discovery_done);
}

static void mcc_icon_obj_id_read_cb(struct bt_conn *conn, int err, uint64_t id)
{
	if (err) {
		FAIL("Icon Object ID read failed (%d)", err);
		return;
	}

	printk("Icon Object ID read succeeded\n");
	g_icon_object_id = id;
	SET_FLAG(icon_object_id_read);
}

static void mcc_segments_obj_id_read_cb(struct bt_conn *conn, int err,
					uint64_t id)
{
	if (err) {
		FAIL("Track Segments ID read failed (%d)\n", err);
		return;
	}

	printk("Track Segments Object ID read succeeded\n");
	g_track_segments_object_id = id;
	SET_FLAG(track_segments_object_id_read);
}

static void mcc_current_track_obj_id_read_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	if (err) {
		FAIL("Current Track Object ID read failed (%d)\n", err);
		return;
	}

	printk("Current Track Object ID read succeeded\n");
	g_current_track_object_id = id;
	SET_FLAG(current_track_object_id_read);
}

static void mcc_next_track_obj_id_read_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	if (err) {
		FAIL("Next Track Object ID read failed (%d)\n", err);
		return;
	}

	printk("Next Track Object ID read succeeded\n");
	g_next_track_object_id = id;
	SET_FLAG(next_track_object_id_read);
}

static void mcc_current_group_obj_id_read_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	if (err) {
		FAIL("Current Group Object ID read failed (%d)\n", err);
		return;
	}

	printk("Current Group Object ID read succeeded\n");
	g_current_group_object_id = id;
	SET_FLAG(current_group_object_id_read);
}

static void mcc_parent_group_obj_id_read_cb(struct bt_conn *conn, int err,
					    uint64_t id)
{
	if (err) {
		FAIL("Parent Group Object ID read failed (%d)\n", err);
		return;
	}

	printk("Parent Group Object ID read succeeded\n");
	g_parent_group_object_id = id;
	SET_FLAG(parent_group_object_id_read);
}

static void mcc_otc_obj_selected_cb(struct bt_conn *conn, int err)
{
	if (err) {
		FAIL("Selecting object failed (%d)\n", err);
		return;
	}

	printk("Selecting object succeeded\n");
	SET_FLAG(object_selected);
}

static void mcc_otc_obj_metadata_cb(struct bt_conn *conn, int err)
{
	if (err) {
		FAIL("Reading object metadata failed (%d)\n", err);
		return;
	}

	printk("Reading object metadata succeeded\n");
	SET_FLAG(metadata_read);
}

static void mcc_icon_object_read_cb(struct bt_conn *conn, int err,
				    struct net_buf_simple *buf)
{
	if (err) {
		FAIL("Reading Icon Object failed (%d)", err);
		return;
	}

	printk("Reading Icon Object succeeded\n");
	SET_FLAG(object_read);
}

static void mcc_track_segments_object_read_cb(struct bt_conn *conn, int err,
					      struct net_buf_simple *buf)
{
	if (err) {
		FAIL("Reading Track Segments Object failed (%d)", err);
		return;
	}

	printk("Reading Track Segments Object succeeded\n");
	SET_FLAG(object_read);
}

static void mcc_otc_read_current_track_object_cb(struct bt_conn *conn, int err,
						 struct net_buf_simple *buf)
{
	if (err) {
		FAIL("Current Track Object read failed (%d)", err);
		return;
	}

	printk("Current Track Object read succeeded\n");
	SET_FLAG(object_read);
}

static void mcc_otc_read_next_track_object_cb(struct bt_conn *conn, int err,
						 struct net_buf_simple *buf)
{
	if (err) {
		FAIL("Next Track Object read failed (%d)", err);
		return;
	}

	printk("Next Track Object read succeeded\n");
	SET_FLAG(object_read);
}

static void mcc_otc_read_current_group_object_cb(struct bt_conn *conn, int err,
						 struct net_buf_simple *buf)
{
	if (err) {
		FAIL("Current Group Object read failed (%d)", err);
		return;
	}

	printk("Current Group Object read succeeded\n");
	SET_FLAG(object_read);
}

static void mcc_otc_read_parent_group_object_cb(struct bt_conn *conn, int err,
						struct net_buf_simple *buf)
{
	if (err) {
		FAIL("Parent Group Object read failed (%d)", err);
		return;
	}

	printk("Parent Group Object read succeeded\n");
	SET_FLAG(object_read);
}

int do_mcc_init(void)
{
	/* Set up the callbacks */
	mcc_cb.init             = &mcc_init_cb;
	mcc_cb.discover_mcs     = &mcc_discover_mcs_cb;
	mcc_cb.icon_obj_id_read = &mcc_icon_obj_id_read_cb;
	mcc_cb.current_track_obj_id_read = &mcc_current_track_obj_id_read_cb;
	mcc_cb.next_track_obj_id_read    = &mcc_next_track_obj_id_read_cb;
	mcc_cb.segments_obj_id_read      = &mcc_segments_obj_id_read_cb;
	mcc_cb.current_group_obj_id_read = &mcc_current_group_obj_id_read_cb;
	mcc_cb.parent_group_obj_id_read  = &mcc_parent_group_obj_id_read_cb;
	mcc_cb.otc_obj_selected = &mcc_otc_obj_selected_cb;
	mcc_cb.otc_obj_metadata = &mcc_otc_obj_metadata_cb;
	mcc_cb.otc_icon_object  = &mcc_icon_object_read_cb;
	mcc_cb.otc_track_segments_object = &mcc_track_segments_object_read_cb;
	mcc_cb.otc_current_track_object  = &mcc_otc_read_current_track_object_cb;
	mcc_cb.otc_next_track_object     = &mcc_otc_read_next_track_object_cb;
	mcc_cb.otc_current_group_object  = &mcc_otc_read_current_group_object_cb;
	mcc_cb.otc_parent_group_object   = &mcc_otc_read_parent_group_object_cb;

	/* Initialize the module */
	return bt_mcc_init(default_conn, &mcc_cb);
}

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

/* Helper function - select object and read the object metadata
 *
 * Will FAIL the test on errors calling select and read metadata.
 * Will WAIT (hang) until callbacks are received.
 * If callbacks are not received, the test fill FAIL due to timeout.
 *
 * @param object_id    ID of the object to select and read metadata for
 */
static void select_read_meta(int64_t id)
{
	int err;

	/* TODO: Fix the instance pointer - it is neither valid nor used */
	err = bt_otc_select_id(default_conn, bt_mcc_otc_inst(), id);
	if (err) {
		FAIL("Failed to select object\n");
		return;
	}

	WAIT_FOR_FLAG(object_selected);
	UNSET_FLAG(object_selected);    /* Clear flag for later use */

	/* TODO: Fix the instance pointer - it is neither valid nor used */
	err = bt_otc_obj_metadata_read(default_conn, bt_mcc_otc_inst(),
				       BT_OTC_METADATA_REQ_ALL);
	if (err) {
		FAIL("Failed to read object metadata\n");
		return;
	}

	WAIT_FOR_FLAG(metadata_read);
	UNSET_FLAG(metadata_read);
}

/* This function tests all commands in the API in sequence
 * The order of the sequence follows the order of the characterstics in the
 * Media Control Service specification
 */
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

	/* Connect ******************************************/
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		FAIL("Failed to start scanning (err %d\n)", err);
	} else {
		printk("Scanning started successfully\n");
	}

	WAIT_FOR_FLAG(ble_link_is_ready);

	/* Initialize MCC  ********************************************/
	do_mcc_init();
	WAIT_FOR_FLAG(mcc_is_initialized);

	/* Discover MCS, subscribe to notifications *******************/
	err = bt_mcc_discover_mcs(default_conn, true);
	if (err) {
		FAIL("Failed to start discovery of MCS: %d\n", err);
	}

	WAIT_FOR_FLAG(discovery_done);

	/* Read icon object ******************************************/
	err = bt_mcc_read_icon_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read icon object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(icon_object_id_read);

	select_read_meta(g_icon_object_id);
	err = bt_mcc_otc_read_icon_object(default_conn);

	if (err) {
		FAIL("Failed to read icon object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	UNSET_FLAG(object_read);


	/* Read track segments object *****************************************/
	err = bt_mcc_read_segments_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read track segments object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(track_segments_object_id_read);

	select_read_meta(g_track_segments_object_id);
	err = bt_mcc_otc_read_track_segments_object(default_conn);

	if (err) {
		FAIL("Failed to read current track object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	UNSET_FLAG(object_read);


	/* Read current track object ******************************************/
	err = bt_mcc_read_current_track_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read current track object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_track_object_id_read);

	select_read_meta(g_current_track_object_id);
	err = bt_mcc_otc_read_current_track_object(default_conn);

	if (err) {
		FAIL("Failed to current track object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	UNSET_FLAG(object_read);


	/* Read next track object ******************************************/
	err = bt_mcc_read_next_track_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read next track object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(next_track_object_id_read);

	select_read_meta(g_next_track_object_id);
	err = bt_mcc_otc_read_next_track_object(default_conn);

	if (err) {
		FAIL("Failed to read next track object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	UNSET_FLAG(object_read);


	/* Read current group object ******************************************/
	err = bt_mcc_read_current_group_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read current group object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(current_group_object_id_read);

	select_read_meta(g_current_group_object_id);
	err = bt_mcc_otc_read_current_group_object(default_conn);

	if (err) {
		FAIL("Failed to read current group object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	UNSET_FLAG(object_read);

	/* Read parent group object ******************************************/
	err = bt_mcc_read_parent_group_obj_id(default_conn);
	if (err) {
		FAIL("Failed to read parent group object ID: %d", err);
		return;
	}

	WAIT_FOR_FLAG(parent_group_object_id_read);

	select_read_meta(g_parent_group_object_id);
	err = bt_mcc_otc_read_parent_group_object(default_conn);

	if (err) {
		FAIL("Failed to read parent group object\n");
		return;
	}

	WAIT_FOR_FLAG(object_read);
	UNSET_FLAG(object_read);

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
