/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_MICS
#include "bluetooth/services/mics.h"
#include "common.h"

extern enum bst_result_t bst_result;
static uint8_t expected_passes = 1;
static uint8_t passes;

#if defined(CONFIG_BT_AICS)
#define AICS_DESC_SIZE CONFIG_BT_AICS_MAX_INPUT_DESCRIPTION_SIZE
#else
#define AICS_DESC_SIZE 0
#endif /* CONFIG_BT_AICS */

static struct bt_mics mics;

static volatile uint8_t g_mute;
static volatile int8_t g_aics_gain;
static volatile uint8_t g_aics_input_mute;
static volatile uint8_t g_aics_mode;
static volatile uint8_t g_aics_input_type;
static volatile uint8_t g_aics_units;
static volatile uint8_t g_aics_gain_max;
static volatile uint8_t g_aics_gain_min;
static volatile bool g_aics_active = 1;
static char g_aics_desc[AICS_DESC_SIZE];
static volatile bool g_cb;

static void mics_mute_cb(struct bt_conn *conn, int err, uint8_t mute)
{
	if (err) {
		FAIL("MICS mute cb err (%d)", err);
		return;
	}

	g_mute = mute;

	if (!conn) {
		g_cb = true;
	}
}

static void aics_state_cb(struct bt_conn *conn, struct bt_aics *inst, int err,
			  int8_t gain, uint8_t mute, uint8_t mode)
{
	if (err) {
		FAIL("AICS state cb err (%d)", err);
		return;
	}

	g_aics_gain = gain;
	g_aics_input_mute = mute;
	g_aics_mode = mode;

	if (!conn) {
		g_cb = true;
	}
}

static void aics_gain_setting_cb(struct bt_conn *conn, struct bt_aics *inst,
				 int err, uint8_t units, int8_t minimum,
				 int8_t maximum)
{
	if (err) {
		FAIL("AICS gain setting cb err (%d)", err);
		return;
	}

	g_aics_units = units;
	g_aics_gain_min = minimum;
	g_aics_gain_max = maximum;

	if (!conn) {
		g_cb = true;
	}
}

static void aics_input_type_cb(struct bt_conn *conn, struct bt_aics *inst,
			       int err, uint8_t input_type)
{
	if (err) {
		FAIL("AICS input type cb err (%d)", err);
		return;
	}

	g_aics_input_type = input_type;

	if (!conn) {
		g_cb = true;
	}
}

static void aics_status_cb(struct bt_conn *conn, struct bt_aics *inst, int err,
			   bool active)
{
	if (err) {
		FAIL("AICS status cb err (%d)", err);
		return;
	}

	g_aics_active = active;

	if (!conn) {
		g_cb = true;
	}
}

static void aics_description_cb(struct bt_conn *conn, struct bt_aics *inst,
				int err, char *description)
{
	if (err) {
		FAIL("AICS description cb err (%d)", err);
		return;
	}

	strcpy(g_aics_desc, description);

	if (!conn) {
		g_cb = true;
	}
}

static struct bt_mics_cb_t mics_cb = {
	.mute = mics_mute_cb,
	.aics_cb  = {
		.state = aics_state_cb,
		.gain_setting = aics_gain_setting_cb,
		.type = aics_input_type_cb,
		.status = aics_status_cb,
		.description = aics_description_cb
	}
};

static int test_aics_standalone(void)
{
	int err;
	int8_t expected_gain;
	uint8_t expected_input_mute;
	uint8_t expected_mode;
	uint8_t expected_input_type;
	bool expected_aics_active;
	char expected_aics_desc[AICS_DESC_SIZE];

	printk("Deactivating AICS\n");
	expected_aics_active = false;
	err = bt_mics_aics_deactivate(mics.aics[0]);
	if (err) {
		FAIL("Could not deactivate AICS (err %d)\n", err);
		return err;
	}
	WAIT_FOR(expected_aics_active == g_aics_active);
	printk("AICS deactivated\n");

	printk("Activating AICS\n");
	expected_aics_active = true;
	err = bt_mics_aics_activate(mics.aics[0]);
	if (err) {
		FAIL("Could not activate AICS (err %d)\n", err);
		return err;
	}
	WAIT_FOR(expected_aics_active == g_aics_active);
	printk("AICS activated\n");

	printk("Getting AICS state\n");
	g_cb = false;
	err = bt_mics_aics_state_get(NULL, mics.aics[0]);
	if (err) {
		FAIL("Could not get AICS state (err %d)\n", err);
		return err;
	}
	WAIT_FOR(g_cb);
	printk("AICS state get\n");

	printk("Getting AICS gain setting\n");
	g_cb = false;
	err = bt_mics_aics_gain_setting_get(NULL, mics.aics[0]);
	if (err) {
		FAIL("Could not get AICS gain setting (err %d)\n", err);
		return err;
	}
	WAIT_FOR(g_cb);
	printk("AICS gain setting get\n");

	printk("Getting AICS input type\n");
	expected_input_type = AICS_INPUT_TYPE_DIGITAL;
	err = bt_mics_aics_type_get(NULL, mics.aics[0]);
	if (err) {
		FAIL("Could not get AICS input type (err %d)\n", err);
		return err;
	}
	/* Expect and wait for input_type from init */
	WAIT_FOR(expected_input_type == g_aics_input_type);
	printk("AICS input type get\n");

	printk("Getting AICS status\n");
	g_cb = false;
	err = bt_mics_aics_status_get(NULL, mics.aics[0]);
	if (err) {
		FAIL("Could not get AICS status (err %d)\n", err);
		return err;
	}
	WAIT_FOR(g_cb);
	printk("AICS status get\n");

	printk("Getting AICS description\n");
	g_cb = false;
	err = bt_mics_aics_description_get(NULL, mics.aics[0]);
	if (err) {
		FAIL("Could not get AICS description (err %d)\n", err);
		return err;
	}
	WAIT_FOR(g_cb);
	printk("AICS description get\n");

	printk("Setting AICS mute\n");
	expected_input_mute = AICS_STATE_MUTED;
	err = bt_mics_aics_mute(NULL, mics.aics[0]);
	if (err) {
		FAIL("Could not set AICS mute (err %d)\n", err);
		return err;
	}
	WAIT_FOR(expected_input_mute == g_aics_input_mute);
	printk("AICS mute set\n");

	printk("Setting AICS unmute\n");
	expected_input_mute = AICS_STATE_UNMUTED;
	err = bt_mics_aics_unmute(NULL, mics.aics[0]);
	if (err) {
		FAIL("Could not set AICS unmute (err %d)\n", err);
		return err;
	}
	WAIT_FOR(expected_input_mute == g_aics_input_mute);
	printk("AICS unmute set\n");

	printk("Setting AICS auto mode\n");
	expected_mode = AICS_MODE_AUTO;
	err = bt_mics_aics_automatic_gain_set(NULL, mics.aics[0]);
	if (err) {
		FAIL("Could not set AICS auto mode (err %d)\n", err);
		return err;
	}
	WAIT_FOR(expected_input_mute == g_aics_input_mute);
	printk("AICS auto mode set\n");

	printk("Setting AICS manual mode\n");
	expected_mode = AICS_MODE_MANUAL;
	err = bt_mics_aics_manual_gain_set(NULL, mics.aics[0]);
	if (err) {
		FAIL("Could not set AICS manual mode (err %d)\n", err);
		return err;
	}
	WAIT_FOR(expected_input_mute == g_aics_input_mute);
	printk("AICS manual mode set\n");

	printk("Setting AICS gain\n");
	expected_gain = g_aics_gain_max - 1;
	err = bt_mics_aics_gain_set(NULL, mics.aics[0], expected_gain);
	if (err) {
		FAIL("Could not set AICS gain (err %d)\n", err);
		return err;
	}
	WAIT_FOR(expected_gain == g_aics_gain);
	printk("AICS gain set\n");

	printk("Setting AICS Description\n");
	strncpy(expected_aics_desc, "New Input Description",
		sizeof(expected_aics_desc));
	g_cb = false;
	err = bt_mics_aics_description_set(NULL, mics.aics[0],
					   expected_aics_desc);
	if (err) {
		FAIL("Could not set AICS Description (err %d)\n", err);
		return err;
}
	WAIT_FOR(g_cb && !strncmp(expected_aics_desc, g_aics_desc,
				  sizeof(expected_aics_desc)));
	printk("AICS Description set\n");

	return 0;
}

static void test_standalone(void)
{
	int err;
	struct bt_mics_init mics_init;
	char input_desc[CONFIG_BT_MICS_AICS_INSTANCE_COUNT][16];
	uint8_t expected_mute;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	memset(&mics_init, 0, sizeof(mics_init));

	for (int i = 0; i < ARRAY_SIZE(mics_init.aics_init); i++) {
		mics_init.aics_init[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]),
			 "Input %d", i + 1);
		mics_init.aics_init[i].input_desc = input_desc[i];
		mics_init.aics_init[i].input_type = AICS_INPUT_TYPE_DIGITAL;
		mics_init.aics_init[i].input_state = g_aics_active;
		mics_init.aics_init[i].mode = AICS_MODE_MANUAL;
		mics_init.aics_init[i].units = 1;
		mics_init.aics_init[i].min_gain = 0;
		mics_init.aics_init[i].max_gain = 100;
	}

	err = bt_mics_init(&mics_init);
	if (err) {
		FAIL("MICS init failed (err %d)\n", err);
		return;
	}

	bt_mics_server_cb_register(&mics_cb);


	err = bt_mics_get(NULL, &mics);

	printk("MICS initialized\n");

	printk("Getting MICS mute\n");
	g_cb = false;
	err = bt_mics_mute_get(NULL);
	if (err) {
		FAIL("Could not get MICS mute (err %d)\n", err);
		return;
	}
	WAIT_FOR(g_cb);
	printk("MICS mute get\n");

	printk("Setting MICS mute\n");
	expected_mute = BT_MICS_MUTE_MUTED;
	err = bt_mics_mute(NULL);
	if (err) {
		FAIL("MICS mute failed (err %d)\n", err);
		return;
	}
	WAIT_FOR(expected_mute == g_mute);
	printk("MICS mute set\n");

	printk("Setting MICS unmute\n");
	expected_mute = BT_MICS_MUTE_UNMUTED;
	err = bt_mics_unmute(NULL);
	if (err) {
		FAIL("MICS unmute failed (err %d)\n", err);
		return;
	}
	WAIT_FOR(expected_mute == g_mute);
	printk("MICS unmute set\n");

	printk("Setting MICS disable\n");
	expected_mute = BT_MICS_MUTE_DISABLED;
	err = bt_mics_mute_disable();
	if (err) {
		FAIL("MICS disable failed (err %d)\n", err);
		return;
	}
	WAIT_FOR(expected_mute == g_mute);
	printk("MICS disable set\n");

	if (CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0) {
		if (test_aics_standalone()) {
			return;
		}
	}

	PASS("MICS passed\n");
}

static const struct bst_test_instance test_mics[] = {
	{
		.test_id = "mics_standalone",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_standalone
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_mics_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_mics);
}
#else
struct bst_test_list *test_mics_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_MICS */
