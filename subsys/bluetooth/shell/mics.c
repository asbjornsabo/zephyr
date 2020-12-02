/** @file
 *  @brief Bluetooth MICS shell.
 *
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <bluetooth/conn.h>
#include <bluetooth/services/mics.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <stdio.h>

#include "bt.h"

static void bt_mics_mute_cb(struct bt_conn *conn, int err, uint8_t mute)
{
	if (err) {
		shell_error(ctx_shell, "Mute get failed (%d)", err);
	} else {
		shell_print(ctx_shell, "Mute value %u", mute);
	}
}

static void bt_mics_aics_state_cb(struct bt_conn *conn, uint8_t aics_index,
				  int err, int8_t gain, uint8_t mute,
				  uint8_t mode)
{
	if (err) {
		shell_error(ctx_shell, "AICS state get failed (%d) for "
			    "index %u", err, aics_index);
	} else {
		shell_print(ctx_shell, "AICS index %u state gain %d, mute %u, "
			    "mode %u", aics_index, gain, mute, mode);
	}

}
static void bt_mics_aics_gain_setting_cb(struct bt_conn *conn,
					 uint8_t aics_index, int err,
					 uint8_t units, int8_t minimum,
					 int8_t maximum)
{
	if (err) {
		shell_error(ctx_shell, "AICS gain settings get failed (%d) for "
			    "index %u", err, aics_index);
	} else {
		shell_print(ctx_shell, "AICS index %u gain settings units %u, "
			    "min %d, max %d", aics_index, units, minimum,
			    maximum);
	}

}
static void bt_mics_aics_input_type_cb(struct bt_conn *conn, uint8_t aics_index,
				       int err, uint8_t input_type)
{
	if (err) {
		shell_error(ctx_shell, "AICS input type get failed (%d) for "
			    "index %u", err, aics_index);
	} else {
		shell_print(ctx_shell, "AICS index %u input type %u",
			    aics_index, input_type);
	}

}
static void bt_mics_aics_status_cb(struct bt_conn *conn, uint8_t aics_index,
				   int err, bool active)
{
	if (err) {
		shell_error(ctx_shell, "AICS status get failed (%d) for "
			    "index %u", err, aics_index);
	} else {
		shell_print(ctx_shell, "AICS index %u status %s",
			    aics_index, active ? "active" : "inactive");
	}

}
static void bt_mics_aics_description_cb(struct bt_conn *conn,
					uint8_t aics_index, int err,
					char *description)
{
	if (err) {
		shell_error(ctx_shell, "AICS description get failed (%d) for "
			    "index %u", err, aics_index);
	} else {
		shell_print(ctx_shell, "AICS index %u description %s",
			    aics_index, description);
	}
}

static struct bt_mics_cb_t mics_cbs = {
	.mute = bt_mics_mute_cb,

	/* Audio Input Control Service */
	.aics_cb = {
		.state = bt_mics_aics_state_cb,
		.gain_setting = bt_mics_aics_gain_setting_cb,
		.type = bt_mics_aics_input_type_cb,
		.status = bt_mics_aics_status_cb,
		.description = bt_mics_aics_description_cb,
	}
};

static int cmd_mics_init(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	struct bt_mics_init mics_init;
	char input_desc[CONFIG_BT_MICS_AICS_INSTANCE_COUNT][16];

	memset(&mics_init, 0, sizeof(mics_init));


	for (int i = 0; i < ARRAY_SIZE(mics_init.aics_init); i++) {
		mics_init.aics_init[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]),
			 "Input %d", i + 1);
		mics_init.aics_init[i].input_desc = input_desc[i];
		mics_init.aics_init[i].input_type = AICS_INPUT_TYPE_LOCAL;
		mics_init.aics_init[i].input_state = true;
		mics_init.aics_init[i].mode = AICS_MODE_MANUAL;
		mics_init.aics_init[i].units = 1;
		mics_init.aics_init[i].min_gain = -100;
		mics_init.aics_init[i].max_gain = 100;
	}

	result = bt_mics_init(&mics_init);

	if (result) {
		shell_print(shell, "Fail: %d", result);
		return result;
	}
	shell_print(shell, "MICS initialized", result);

	bt_mics_server_cb_register(&mics_cbs);
	return result;
}

static inline int cmd_mics_mute_get(
	const struct shell *shell, size_t argc, char **argv)
{
	int result = bt_mics_mute_get(NULL);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_mute(
	const struct shell *shell, size_t argc, char **argv)
{
	int result = bt_mics_mute(NULL);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_unmute(
	const struct shell *shell, size_t argc, char **argv)
{
	int result = bt_mics_unmute(NULL);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_mute_disable(
	const struct shell *shell, size_t argc, char **argv)
{
	int result = bt_mics_mute_disable();

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_aics_deactivate(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index > CONFIG_BT_MICS_AICS_INSTANCE_COUNT) {
		shell_error(shell, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_MICS_AICS_INSTANCE_COUNT, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_deactivate(index);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_aics_activate(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index > CONFIG_BT_MICS_AICS_INSTANCE_COUNT) {
		shell_error(shell, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_MICS_AICS_INSTANCE_COUNT, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_activate(index);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_aics_input_state_get(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index > CONFIG_BT_MICS_CLIENT_MAX_AICS_INST) {
		shell_error(shell, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_MICS_CLIENT_MAX_AICS_INST, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_state_get(NULL, index);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_aics_gain_setting_get(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index > CONFIG_BT_MICS_CLIENT_MAX_AICS_INST) {
		shell_error(shell, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_MICS_CLIENT_MAX_AICS_INST, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_gain_setting_get(NULL, index);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_aics_input_type_get(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index > CONFIG_BT_MICS_CLIENT_MAX_AICS_INST) {
		shell_error(shell, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_MICS_CLIENT_MAX_AICS_INST, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_type_get(NULL, index);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_aics_input_status_get(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index > CONFIG_BT_MICS_CLIENT_MAX_AICS_INST) {
		shell_error(shell, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_MICS_CLIENT_MAX_AICS_INST, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_status_get(NULL, index);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_aics_input_unmute(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index > CONFIG_BT_MICS_CLIENT_MAX_AICS_INST) {
		shell_error(shell, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_MICS_CLIENT_MAX_AICS_INST, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_unmute(NULL, index);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_aics_input_mute(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index > CONFIG_BT_MICS_CLIENT_MAX_AICS_INST) {
		shell_error(shell, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_MICS_CLIENT_MAX_AICS_INST, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_mute(NULL, index);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_aics_manual_input_gain_set(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index > CONFIG_BT_MICS_CLIENT_MAX_AICS_INST) {
		shell_error(shell, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_MICS_CLIENT_MAX_AICS_INST, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_manual_gain_set(NULL, index);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_aics_automatic_input_gain_set(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index > CONFIG_BT_MICS_CLIENT_MAX_AICS_INST) {
		shell_error(shell, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_MICS_CLIENT_MAX_AICS_INST, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_automatic_gain_set(NULL, index);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_aics_gain_set(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	int gain = strtol(argv[2], NULL, 0);

	if (index > CONFIG_BT_MICS_CLIENT_MAX_AICS_INST) {
		shell_error(shell, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_MICS_CLIENT_MAX_AICS_INST, index);
		return -ENOEXEC;
	}

	if (gain > INT8_MAX || gain < INT8_MIN) {
		shell_error(shell, "Offset shall be %d-%d, was %d",
			    INT8_MIN, INT8_MAX, gain);
		return -ENOEXEC;
	}

	result = bt_mics_aics_gain_set(NULL, index, gain);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_aics_input_description_get(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index > CONFIG_BT_MICS_CLIENT_MAX_AICS_INST) {
		shell_error(shell, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_MICS_CLIENT_MAX_AICS_INST, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_description_get(NULL, index);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_aics_input_description_set(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	char *description = argv[2];

	if (index > CONFIG_BT_MICS_CLIENT_MAX_AICS_INST) {
		shell_error(shell, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_MICS_CLIENT_MAX_AICS_INST, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_description_set(NULL, index, description);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;

}

static inline int cmd_mics(
	const struct shell *shell, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(shell, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(shell, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(mics_cmds,
	SHELL_CMD_ARG(init, NULL,
		      "Initialize the service and register callbacks",
		      cmd_mics_init, 1, 0),
	SHELL_CMD_ARG(mute_get, NULL,
		      "Get the mute state",
		      cmd_mics_mute_get, 1, 0),
	SHELL_CMD_ARG(mute, NULL,
		      "Mute the MICS server",
		      cmd_mics_mute, 1, 0),
	SHELL_CMD_ARG(unmute, NULL,
		      "Unmute the MICS server",
		      cmd_mics_unmute, 1, 0),
	SHELL_CMD_ARG(mute_disable, NULL,
		      "Disable the MICS mute",
		      cmd_mics_mute_disable, 1, 0),
	SHELL_CMD_ARG(aics_deactivate, NULL,
		      "Deactivates a AICS instance <inst_index>",
		      cmd_mics_aics_deactivate, 2, 0),
	SHELL_CMD_ARG(aics_activate, NULL,
		      "Activates a AICS instance <inst_index>",
		      cmd_mics_aics_activate, 2, 0),
	SHELL_CMD_ARG(aics_input_state_get, NULL,
		      "Get the input state of a AICS instance <inst_index>",
		      cmd_mics_aics_input_state_get, 2, 0),
	SHELL_CMD_ARG(aics_gain_setting_get, NULL,
		      "Get the gain settings of a AICS instance <inst_index>",
		      cmd_mics_aics_gain_setting_get, 2, 0),
	SHELL_CMD_ARG(aics_input_type_get, NULL,
		      "Get the input type of a AICS instance <inst_index>",
		      cmd_mics_aics_input_type_get, 2, 0),
	SHELL_CMD_ARG(aics_input_status_get, NULL,
		      "Get the input status of a AICS instance <inst_index>",
		      cmd_mics_aics_input_status_get, 2, 0),
	SHELL_CMD_ARG(aics_input_unmute, NULL,
		      "Unmute the input of a AICS instance <inst_index>",
		      cmd_mics_aics_input_unmute, 2, 0),
	SHELL_CMD_ARG(aics_input_mute, NULL,
		      "Mute the input of a AICS instance <inst_index>",
		      cmd_mics_aics_input_mute, 2, 0),
	SHELL_CMD_ARG(aics_manual_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to manual "
		      "<inst_index>",
		      cmd_mics_aics_manual_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_automatic_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to automatic "
		      "<inst_index>",
		      cmd_mics_aics_automatic_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_gain_set, NULL,
		      "Set the gain in dB of a AICS instance <inst_index> "
		      "<gain (-128 to 127)>",
		      cmd_mics_aics_gain_set, 3, 0),
	SHELL_CMD_ARG(aics_input_description_get, NULL,
		      "Get the input description of a AICS instance "
		      "<inst_index>",
		      cmd_mics_aics_input_description_get, 2, 0),
	SHELL_CMD_ARG(aics_input_description_set, NULL,
		      "Set the input description of a AICS instance "
		      "<inst_index> <description>",
		      cmd_mics_aics_input_description_set, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(mics, &mics_cmds, "Bluetooth MICS shell commands",
		       cmd_mics, 1, 1);
