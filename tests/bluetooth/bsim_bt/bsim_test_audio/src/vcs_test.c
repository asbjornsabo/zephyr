/*
 * Copyright (c) 2019 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_VCS
#include "bluetooth/services/vcs.h"
#include "common.h"

extern enum bst_result_t bst_result;
static uint8_t expected_passes = 1;
static uint8_t passes;

static volatile uint8_t g_volume;
static volatile uint8_t g_mute;
static volatile uint8_t g_flags;
static volatile bool g_cb;

static void vcs_state_cb(
	struct bt_conn *conn, int err, uint8_t volume, uint8_t mute)
{
	if (err) {
		FAIL("VCS state cb err (%d)", err);
		return;
	}

	g_volume = volume;
	g_mute = mute;

	if (!conn) {
		g_cb = true;
	}
}

static void vcs_flags_cb(struct bt_conn *conn, int err, uint8_t flags)
{
	if (err) {
		FAIL("VCS flags cb err (%d)", err);
		return;
	}

	g_flags = flags;

	if (!conn) {
		g_cb = true;
	}
}

static struct bt_vcs_cb_t vcs_cb = {
	.state = vcs_state_cb,
	.flags = vcs_flags_cb,
	.vocs_cb = {
		.state = NULL,
		.location = NULL,
		.description = NULL
	},
	.aics_cb  = {
		.state = NULL,
		.gain_setting = NULL,
		.type = NULL,
		.status = NULL,
		.description = NULL
	}
};

static void test_standalone(void)
{
	int err;
	struct bt_vcs_init vcs_init;
	char input_desc[CONFIG_BT_VCS_AICS_INSTANCE_COUNT][16];
	char output_desc[CONFIG_BT_VCS_VOCS_INSTANCE_COUNT][16];
	const uint8_t volume_step = 5;
	uint8_t expected_volume;
	uint8_t expected_mute;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	memset(&vcs_init, 0, sizeof(vcs_init));

	for (int i = 0; i < ARRAY_SIZE(vcs_init.vocs_init); i++) {
		vcs_init.vocs_init[i].location_writable = true;
		vcs_init.vocs_init[i].desc_writable = true;
		snprintf(output_desc[i], sizeof(output_desc[i]),
			 "Output %d", i + 1);
		vcs_init.vocs_init[i].output_desc = output_desc[i];
	}

	for (int i = 0; i < ARRAY_SIZE(vcs_init.aics_init); i++) {
		vcs_init.aics_init[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]),
			 "Input %d", i + 1);
		vcs_init.aics_init[i].input_desc = input_desc[i];
	}

	err = bt_vcs_init(&vcs_init);
	if (err) {
		FAIL("VCS init failed (err %d)\n", err);
		return;
	}

	bt_vcs_server_cb_register(&vcs_cb);

	printk("VCS initialized\n");


	printk("Setting VCS step\n");
	err = bt_vcs_volume_step_set(volume_step);
	if (err) {
		FAIL("VCS step set failed (err %d)\n", err);
		return;
	}
	printk("VCS step set\n");

	printk("Getting VCS volume state\n");
	g_cb = false;
	err = bt_vcs_volume_get(NULL);
	if (err) {
		FAIL("Could not get VCS volume (err %d)\n", err);
		return;
	}
	WAIT_FOR(g_cb);
	printk("VCS volume get\n");

	printk("Getting VCS flags\n");
	g_cb = false;
	err = bt_vcs_flags_get(NULL);
	if (err) {
		FAIL("Could not get VCS flags (err %d)\n", err);
		return;
	}
	WAIT_FOR(g_cb);
	printk("VCS flags get\n");

	printk("Downing VCS volume\n");
	expected_volume = g_volume - volume_step;
	err = bt_vcs_volume_down(NULL);
	if (err) {
		FAIL("Could not get down VCS volume (err %d)\n", err);
		return;
	}
	WAIT_FOR(expected_volume == g_volume);
	printk("VCS volume downed\n");

	printk("Upping VCS volume\n");
	expected_volume = g_volume + volume_step;
	err = bt_vcs_volume_up(NULL);
	if (err) {
		FAIL("Could not up VCS volume (err %d)\n", err);
		return;
	}
	WAIT_FOR(expected_volume == g_volume);
	printk("VCS volume upped\n");

	printk("Muting VCS\n");
	expected_mute = 1;
	err = bt_vcs_mute(NULL);
	if (err) {
		FAIL("Could not mute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR(expected_mute == g_mute);
	printk("VCS muted\n");

	printk("Downing and unmuting VCS\n");
	expected_volume = g_volume - volume_step;
	expected_mute = 0;
	err = bt_vcs_unmute_volume_down(NULL);
	if (err) {
		FAIL("Could not down and unmute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR(expected_volume == g_volume && expected_mute == g_mute);
	printk("VCS volume downed and unmuted\n");

	printk("Muting VCS\n");
	expected_mute = 1;
	err = bt_vcs_mute(NULL);
	if (err) {
		FAIL("Could not mute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR(expected_mute == g_mute);
	printk("VCS muted\n");

	printk("Upping and unmuting VCS\n");
	expected_volume = g_volume + volume_step;
	expected_mute = 0;
	err = bt_vcs_unmute_volume_up(NULL);
	if (err) {
		FAIL("Could not up and unmute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR(expected_volume == g_volume && expected_mute == g_mute);
	printk("VCS volume upped and unmuted\n");

	printk("Muting VCS\n");
	expected_mute = 1;
	err = bt_vcs_mute(NULL);
	if (err) {
		FAIL("Could not mute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR(expected_mute == g_mute);
	printk("VCS volume upped\n");

	printk("Unmuting VCS\n");
	expected_mute = 0;
	err = bt_vcs_unmute(NULL);
	if (err) {
		FAIL("Could not unmute VCS (err %d)\n", err);
		return;
	}
	WAIT_FOR(expected_mute == g_mute);
	printk("VCS volume unmuted\n");

	expected_volume = g_volume - 5;
	err = bt_vcs_volume_set(NULL, expected_volume);
	if (err) {
		FAIL("Could not set VCS volume (err %d)\n", err);
		return;
	}
	WAIT_FOR(expected_volume == g_volume);
	printk("VCS volume set\n");

	PASS("VCS passed\n");
}

static const struct bst_test_instance test_vcs[] = {
	{
		.test_id = "vcs_standalone",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_standalone
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_vcs_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_vcs);
}
#else
struct bst_test_list *test_vcs_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_VCS */
