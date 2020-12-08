/** @file
 *  @brief Bluetooth MICS client shell.
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

static struct bt_mics mics;

static void bt_mics_discover_cb(struct bt_conn *conn, int err,
				uint8_t aics_count)
{
	if (err) {
		shell_error(ctx_shell, "MICS discover failed (%d)", err);
	} else {
		shell_print(ctx_shell, "MICS discover done with %u AICS",
			    aics_count);

		if (bt_mics_get(conn, &mics)) {
			shell_error(ctx_shell, "Could not get MICS context");
		}
	}
}

static void bt_mics_mute_write_cb(struct bt_conn *conn, int err,
				  uint8_t req_val)
{
	if (err) {
		shell_error(ctx_shell, "Mute write failed (%d)", err);
	} else {
		shell_print(ctx_shell, "Mute write value %u", req_val);
	}
}

static void bt_mics_aics_set_gain_cb(struct bt_conn *conn, struct bt_aics *inst,
				     int err)
{
	if (err) {
		shell_error(ctx_shell, "Set gain failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Gain set for inst %p", inst);
	}
}

static void bt_mics_aics_unmute_cb(struct bt_conn *conn, struct bt_aics *inst,
				   int err)
{
	if (err) {
		shell_error(ctx_shell, "Unmute failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Unmuted inst %p", inst);
	}
}

static void bt_mics_aics_mute_cb(struct bt_conn *conn, struct bt_aics *inst,
				 int err)
{
	if (err) {
		shell_error(ctx_shell, "Mute failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Muted inst %p", inst);
	}
}

static void bt_mics_aics_set_manual_mode_cb(struct bt_conn *conn,
					    struct bt_aics *inst,
					    int err)
{
	if (err) {
		shell_error(ctx_shell,
			    "Set manual mode failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Manuel mode set for inst %p", inst);
	}
}

static void bt_mics_aics_automatic_mode_cb(struct bt_conn *conn,
					   struct bt_aics *inst,
					   int err)
{
	if (err) {
		shell_error(ctx_shell,
			    "Set automatic mode failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Automatic mode set for inst %p",
			    inst);
	}
}

static void bt_mics_mute_cb(struct bt_conn *conn, int err, uint8_t mute)
{
	if (err) {
		shell_error(ctx_shell, "Mute get failed (%d)", err);
	} else {
		shell_print(ctx_shell, "Mute value %u", mute);
	}
}

static void bt_mics_aics_state_cb(struct bt_conn *conn, struct bt_aics *inst,
				  int err, int8_t gain, uint8_t mute,
				  uint8_t mode)
{
	if (err) {
		shell_error(ctx_shell, "AICS state get failed (%d) for "
			    "inst %p", err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p state gain %d, mute %u, "
			    "mode %u", inst, gain, mute, mode);
	}

}

static void bt_mics_aics_gain_setting_cb(struct bt_conn *conn,
					 struct bt_aics *inst, int err,
					 uint8_t units, int8_t minimum,
					 int8_t maximum)
{
	if (err) {
		shell_error(ctx_shell, "AICS gain settings get failed (%d) for "
			    "inst %p", err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p gain settings units %u, "
			    "min %d, max %d", inst, units, minimum,
			    maximum);
	}

}

static void bt_mics_aics_input_type_cb(struct bt_conn *conn,
				       struct bt_aics *inst,
				       int err, uint8_t input_type)
{
	if (err) {
		shell_error(ctx_shell, "AICS input type get failed (%d) for "
			    "inst %p", err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p input type %u",
			    inst, input_type);
	}

}

static void bt_mics_aics_status_cb(struct bt_conn *conn, struct bt_aics *inst,
				   int err, bool active)
{
	if (err) {
		shell_error(ctx_shell, "AICS status get failed (%d) for "
			    "inst %p", err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p status %s",
			    inst, active ? "active" : "inactive");
	}

}

static void bt_mics_aics_description_cb(struct bt_conn *conn,
					struct bt_aics *inst, int err,
					char *description)
{
	if (err) {
		shell_error(ctx_shell, "AICS description get failed (%d) for "
			    "inst %p", err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p description %s",
			    inst, description);
	}
}

static struct bt_mics_cb_t mics_cbs = {
	.init = bt_mics_discover_cb,
	.mute_write = bt_mics_mute_write_cb,

	.mute = bt_mics_mute_cb,

	/* Audio Input Control Service */
	.aics_cb = {
		.state = bt_mics_aics_state_cb,
		.gain_setting = bt_mics_aics_gain_setting_cb,
		.type = bt_mics_aics_input_type_cb,
		.status = bt_mics_aics_status_cb,
		.description = bt_mics_aics_description_cb,
		.set_gain = bt_mics_aics_set_gain_cb,
		.unmute = bt_mics_aics_unmute_cb,
		.mute = bt_mics_aics_mute_cb,
		.set_manual_mode = bt_mics_aics_set_manual_mode_cb,
		.set_auto_mode = bt_mics_aics_automatic_mode_cb,
	}
};

static inline int cmd_mics_client_discover(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;

	if (!ctx_shell) {
		ctx_shell = shell;
	}

	bt_mics_client_cb_register(&mics_cbs);

	if (!default_conn) {
		return -ENOTCONN;
	}

	result = bt_mics_discover(default_conn);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_client_mute_get(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;

	if (!default_conn) {
		return -ENOTCONN;
	}

	result = bt_mics_mute_get(default_conn);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_client_mute(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;

	if (!default_conn) {
		return -ENOTCONN;
	}

	result = bt_mics_mute(default_conn);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_client_unmute(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;

	if (!default_conn) {
		return -ENOTCONN;
	}

	result = bt_mics_unmute(default_conn);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_client_aics_input_state_get(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics.aics_cnt) {
		shell_error(shell, "Index shall be less than %u, was %u",
			    mics.aics_cnt, index);
		return -ENOEXEC;
	}

	if (!default_conn) {
		return -ENOTCONN;
	}

	result = bt_mics_aics_state_get(default_conn, mics.aics[index]);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_client_aics_gain_setting_get(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics.aics_cnt) {
		shell_error(shell, "Index shall be less than %u, was %u",
			    mics.aics_cnt, index);
		return -ENOEXEC;
	}

	if (!default_conn) {
		return -ENOTCONN;
	}

	result = bt_mics_aics_gain_setting_get(default_conn, mics.aics[index]);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_client_aics_input_type_get(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics.aics_cnt) {
		shell_error(shell, "Index shall be less than %u, was %u",
			    mics.aics_cnt, index);
		return -ENOEXEC;
	}

	if (!default_conn) {
		return -ENOTCONN;
	}

	result = bt_mics_aics_type_get(default_conn, mics.aics[index]);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_client_aics_input_status_get(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics.aics_cnt) {
		shell_error(shell, "Index shall be less than %u, was %u",
			    mics.aics_cnt, index);
		return -ENOEXEC;
	}

	if (!default_conn) {
		return -ENOTCONN;
	}

	result = bt_mics_aics_status_get(default_conn, mics.aics[index]);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_client_aics_input_unmute(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics.aics_cnt) {
		shell_error(shell, "Index shall be less than %u, was %u",
			    mics.aics_cnt, index);
		return -ENOEXEC;
	}

	if (!default_conn) {
		return -ENOTCONN;
	}

	result = bt_mics_aics_unmute(default_conn, mics.aics[index]);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_client_aics_input_mute(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics.aics_cnt) {
		shell_error(shell, "Index shall be less than %u, was %u",
			    mics.aics_cnt, index);
		return -ENOEXEC;
	}

	if (!default_conn) {
		return -ENOTCONN;
	}

	result = bt_mics_aics_mute(default_conn, mics.aics[index]);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_client_aics_manual_input_gain_set(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics.aics_cnt) {
		shell_error(shell, "Index shall be less than %u, was %u",
			    mics.aics_cnt, index);
		return -ENOEXEC;
	}

	if (!default_conn) {
		return -ENOTCONN;
	}

	result = bt_mics_aics_manual_gain_set(default_conn, mics.aics[index]);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_client_aics_automatic_input_gain_set(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics.aics_cnt) {
		shell_error(shell, "Index shall be less than %u, was %u",
			    mics.aics_cnt, index);
		return -ENOEXEC;
	}

	if (!default_conn) {
		return -ENOTCONN;
	}

	result = bt_mics_aics_automatic_gain_set(default_conn, mics.aics[index]);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_client_aics_gain_set(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	int gain = strtol(argv[2], NULL, 0);

	if (index >= mics.aics_cnt) {
		shell_error(shell, "Index shall be less than %u, was %u",
			    mics.aics_cnt, index);
		return -ENOEXEC;
	}

	if (gain > INT8_MAX || gain < INT8_MIN) {
		shell_error(shell, "Offset shall be %d-%d, was %d",
			    INT8_MIN, INT8_MAX, gain);
		return -ENOEXEC;
	}

	if (!default_conn) {
		return -ENOTCONN;
	}

	result = bt_mics_aics_gain_set(default_conn, mics.aics[index], gain);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_client_aics_input_description_get(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics.aics_cnt) {
		shell_error(shell, "Index shall be less than %u, was %u",
			    mics.aics_cnt, index);
		return -ENOEXEC;
	}

	if (!default_conn) {
		return -ENOTCONN;
	}

	result = bt_mics_aics_description_get(default_conn, mics.aics[index]);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_mics_client_aics_input_description_set(
	const struct shell *shell, size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	char *description = argv[2];

	if (index >= mics.aics_cnt) {
		shell_error(shell, "Index shall be less than %u, was %u",
			    mics.aics_cnt, index);
		return -ENOEXEC;
	}

	if (!default_conn) {
		return -ENOTCONN;
	}

	result = bt_mics_aics_description_set(default_conn, mics.aics[index],
					      description);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;

}

static inline int cmd_mics_client(
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

SHELL_STATIC_SUBCMD_SET_CREATE(mics_client_cmds,
	SHELL_CMD_ARG(discover, NULL,
		      "Discover MICS on remote device",
		      cmd_mics_client_discover, 1, 0),
	SHELL_CMD_ARG(mute_get, NULL,
		      "Read the mute state of the MICS server.",
		      cmd_mics_client_mute_get, 1, 0),
	SHELL_CMD_ARG(mute, NULL,
		      "Mute the MICS server",
		      cmd_mics_client_mute, 1, 0),
	SHELL_CMD_ARG(unmute, NULL,
		      "Unmute the MICS server",
		      cmd_mics_client_unmute, 1, 0),
	SHELL_CMD_ARG(aics_input_state_get, NULL,
		      "Read the input state of a AICS instance <inst_index>",
		      cmd_mics_client_aics_input_state_get, 2, 0),
	SHELL_CMD_ARG(aics_gain_setting_get, NULL,
		      "Read the gain settings of a AICS instance <inst_index>",
		      cmd_mics_client_aics_gain_setting_get, 2, 0),
	SHELL_CMD_ARG(aics_input_type_get, NULL,
		      "Read the input type of a AICS instance <inst_index>",
		      cmd_mics_client_aics_input_type_get, 2, 0),
	SHELL_CMD_ARG(aics_input_status_get, NULL,
		      "Read the input status of a AICS instance <inst_index>",
		      cmd_mics_client_aics_input_status_get, 2, 0),
	SHELL_CMD_ARG(aics_input_unmute, NULL,
		      "Unmute the input of a AICS instance <inst_index>",
		      cmd_mics_client_aics_input_unmute, 2, 0),
	SHELL_CMD_ARG(aics_input_mute, NULL,
		      "Mute the input of a AICS instance <inst_index>",
		      cmd_mics_client_aics_input_mute, 2, 0),
	SHELL_CMD_ARG(aics_manual_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to manual "
		      "<inst_index>",
		      cmd_mics_client_aics_manual_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_automatic_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to automatic "
		      "<inst_index>",
		      cmd_mics_client_aics_automatic_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_gain_set, NULL,
		      "Set the gain of a AICS instance <inst_index> <gain>",
		      cmd_mics_client_aics_gain_set, 3, 0),
	SHELL_CMD_ARG(aics_input_description_get, NULL,
		      "Read the input description of a AICS instance "
		      "<inst_index>",
		      cmd_mics_client_aics_input_description_get, 2, 0),
	SHELL_CMD_ARG(aics_input_description_set, NULL,
		      "Set the input description of a AICS instance "
		      "<inst_index> <description>",
		      cmd_mics_client_aics_input_description_set, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(mics_client, &mics_client_cmds,
		       "Bluetooth MICS client shell commands",
		       cmd_mics_client, 1, 1);
