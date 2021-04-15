/**
 * @file
 * @brief Shell APIs for Bluetooth CSIS
 *
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#include <zephyr.h>
#include <zephyr/types.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <bluetooth/gatt.h>
#include <bluetooth/bluetooth.h>
#include "../host/audio/csis.h"
#include "bt.h"

extern const struct shell *ctx_shell;
static uint8_t sirk_read_rsp = BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT;

static void locked_cb(struct bt_conn *conn, bool locked)
{
	if (!conn) {
		shell_error(ctx_shell, "Server %s the device",
			    locked ? "locked" : "released");
	} else {
		char addr[BT_ADDR_LE_STR_LEN];

		conn_addr_str(conn, addr, sizeof(addr));

		shell_print(ctx_shell, "Client %s %s the device",
			    addr, locked ? "locked" : "released");
	}
}

static uint8_t sirk_read_req_cb(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	static const char *rsp_strings[] = {
		"Accept", "Accept Enc", "Reject", "OOB only"
	};

	conn_addr_str(conn, addr, sizeof(addr));

	shell_print(ctx_shell, "Client %s requested to read the sirk. "
		    "Responding with %s", addr, rsp_strings[sirk_read_rsp]);

	return sirk_read_rsp;
}

struct bt_csis_cb_t csis_cbs = {
	.lock_changed = locked_cb,
	.sirk_read_req = sirk_read_req_cb,
};

static int cmd_csis_init(const struct shell *shell, size_t argc, char **argv)
{
	bt_csis_register_cb(&csis_cbs);

	return 0;
}

static int cmd_csis_advertise(const struct shell *shell, size_t argc,
				     char *argv[])
{
	int err;
	if (!strcmp(argv[1], "off")) {
		err = bt_csis_advertise(false);
		if (err) {
			shell_error(shell, "Failed to stop advertising %d",
				    err);
			return -ENOEXEC;
		}
		shell_print(shell, "Advertising stopped");
	} else if (!strcmp(argv[1], "on")) {
		err = bt_csis_advertise(true);
		if (err) {
			shell_error(shell, "Failed to start advertising %d",
				    err);
			return -ENOEXEC;
		}
		shell_print(shell, "Advertising started");
	} else {
		shell_error(shell, "Invalid argument: %s", argv[1]);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_csis_update_psri(const struct shell *shell, size_t argc,
				       char *argv[])
{
	int err;

	if (bt_csis_advertise(false) != 0) {
		shell_error(shell,
			    "Failed to stop advertising - psri not updated");
		return -ENOEXEC;
	}
	err = bt_csis_advertise(true);
	if (err != 0) {
		shell_error(shell,
			    "Failed to start advertising  - psri not updated");
		return -ENOEXEC;
	}

	shell_print(shell, "PSRI and optionally RPA updated");

	return 0;
}

static int cmd_csis_print_sirk(const struct shell *shell, size_t argc,
				      char *argv[])
{
	bt_csis_print_sirk();
	return 0;
}

static int cmd_csis_lock(const struct shell *shell, size_t argc,
				char *argv[])
{
	if (bt_csis_lock(true, false) != 0) {
		shell_error(shell, "Failed to set lock");
		return -ENOEXEC;
	}

	shell_print(shell, "Set locked");

	return 0;
}

static int cmd_csis_release(const struct shell *shell, size_t argc,
				   char *argv[])
{
	bool force = false;

	if (argc > 1) {
		if (strcmp(argv[1], "force") == 0) {
			force = true;
		} else {
			shell_error(shell, "Unknown parameter: %s", argv[1]);
			return -ENOEXEC;
		}
	}

	if (bt_csis_lock(false, force) != 0) {
		shell_error(shell, "Failed to release lock");
		return -ENOEXEC;
	}

	shell_print(shell, "Set release");

	return 0;
}

static int cmd_csis_set_sirk_rsp(const struct shell *shell, size_t argc,
				 char *argv[])
{
	if (strcmp(argv[1], "accept") == 0) {
		sirk_read_rsp = BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT;
	} else if (strcmp(argv[1], "accept_enc") == 0) {
		sirk_read_rsp = BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT_ENC;
	} else if (strcmp(argv[1], "reject") == 0) {
		sirk_read_rsp = BT_CSIS_READ_SIRK_REQ_RSP_REJECT;
	} else if (strcmp(argv[1], "oob") == 0) {
		sirk_read_rsp = BT_CSIS_READ_SIRK_REQ_RSP_OOB_ONLY;
	} else {
		shell_error(shell, "Unknown parameter: %s", argv[1]);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_csis(const struct shell *shell, size_t argc, char **argv)
{
	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(csis_cmds,
	SHELL_CMD_ARG(init, NULL,
		      "Initialize the service and register callbacks",
		      cmd_csis_init, 1, 0),
	SHELL_CMD_ARG(advertise, NULL,
		      "Start/stop advertising CSIS PSRIs <on/off>",
		      cmd_csis_advertise, 2, 0),
	SHELL_CMD_ARG(update_psri, NULL,
		      "Update the advertised PSRI",
		      cmd_csis_update_psri, 1, 0),
	SHELL_CMD_ARG(lock, NULL,
		      "Lock the set",
		      cmd_csis_lock, 1, 0),
	SHELL_CMD_ARG(release, NULL,
		      "Release the set [force]",
		      cmd_csis_release, 1, 1),
	SHELL_CMD_ARG(print_sirk, NULL,
		      "Print the currently used SIRK",
			  cmd_csis_print_sirk, 1, 0),
	SHELL_CMD_ARG(set_sirk_rsp, NULL,
		      "Set the response used in SIRK requests "
		      "<accept, accept_enc, reject, oob>",
		      cmd_csis_set_sirk_rsp, 2, 0),
		      SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(csis, &csis_cmds, "Bluetooth CSIS shell commands",
		       cmd_csis, 1, 1);
