/** @file
 *  @brief Media Player / Media control service shell implementation
 *
 */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <shell/shell.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include "bt.h"

#include "../host/audio/uint48_util.h"
#include "../host/audio/mpl.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MCS)
#define LOG_MODULE_NAME bt_mpl_shell
#include "common/log.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BT_MCS)

#if defined(CONFIG_BT_DEBUG_MCS) && defined(CONFIG_BT_TESTING)
int cmd_mpl_test_set_media_state(const struct shell *shell, size_t argc,
				  char *argv[])
{
	uint8_t state = strtol(argv[1], NULL, 0);

	mpl_test_media_state_set(state);

	return 0;
}

#ifdef CONFIG_BT_OTS_TEMP
int cmd_mpl_test_unset_parent_group(const struct shell *shell, size_t argc,
				    char *argv[])
{
	mpl_test_unset_parent_group();

	return 0;
}
#endif /* CONFIG_BT_OTS_TEMP */

/* Interface to _local_ control point, for testing and debugging */
int cmd_mpl_test_set_operation(const struct shell *shell, size_t argc,
				char *argv[])
{
	struct mpl_op_t op;


	if (argc > 1) {
		op.opcode = strtol(argv[1], NULL, 0);
	} else {
		shell_error(shell, "Invalid parameter");
		return -ENOEXEC;
	}

	if (argc > 2) {
		op.use_param = true;
		op.param = strtol(argv[2], NULL, 0);
	} else {
		op.use_param = false;
		op.param = 0;
	}

	mpl_operation_set(op);

	return 0;
}
#endif /* CONFIG_BT_DEBUG_MCS && CONFIG_BT_TESTING */

#if defined(CONFIG_BT_DEBUG_MCS)
int cmd_mpl_debug_dump_state(const struct shell *shell, size_t argc,
			     char *argv[])
{
	mpl_debug_dump_state();

	return 0;
}
#endif /* CONFIG_BT_DEBUG_MCS */

int cmd_mpl_init(const struct shell *shell, size_t argc, char *argv[])
{
	if (!ctx_shell) {
		ctx_shell = shell;
	}

	int err = mpl_init();

	if (err) {
		shell_error(shell, "Could not init mpl");
	}

	return err;
}


int cmd_mpl_track_changed_cb(const struct shell *shell, size_t argc,
			     char *argv[])
{
	mpl_track_changed_cb();

	return 0;
}

int cmd_mpl_title_changed_cb(const struct shell *shell, size_t argc,
			     char *argv[])
{
	mpl_track_title_cb("Interlude #3");

	return 0;
}

int cmd_mpl_duration_changed_cb(const struct shell *shell, size_t argc,
				char *argv[])
{
	mpl_track_duration_cb(12000);

	return 0;
}

int cmd_mpl_position_changed_cb(const struct shell *shell, size_t argc,
				char *argv[])
{
	mpl_track_position_cb(2048);

	return 0;
}

int cmd_mpl_playback_speed_changed_cb(const struct shell *shell, size_t argc,
				      char *argv[])
{
	mpl_playback_speed_cb(96);

	return 0;
}

int cmd_mpl_seeking_speed_changed_cb(const struct shell *shell, size_t argc,
				     char *argv[])
{
	mpl_seeking_speed_cb(4);

	return 0;
}

#ifdef CONFIG_BT_OTS_TEMP
int cmd_mpl_current_track_id_changed_cb(const struct shell *shell, size_t argc,
					char *argv[])
{
	mpl_current_track_id_cb(16);

	return 0;
}

int cmd_mpl_next_track_id_changed_cb(const struct shell *shell, size_t argc,
				     char *argv[])
{
	mpl_next_track_id_cb(17);

	return 0;
}

int cmd_mpl_group_id_changed_cb(const struct shell *shell, size_t argc,
				char *argv[])
{
	mpl_group_id_cb(19);

	return 0;
}

int cmd_mpl_parent_group_id_changed_cb(const struct shell *shell, size_t argc,
				       char *argv[])
{
	mpl_parent_group_id_cb(23);

	return 0;
}
#endif /* CONFIG_BT_OTS_TEMP */

int cmd_mpl_playing_order_changed_cb(const struct shell *shell, size_t argc,
				     char *argv[])
{
	mpl_playing_order_cb(1);

	return 0;
}

int cmd_mpl_state_changed_cb(const struct shell *shell, size_t argc,
			     char *argv[])
{
	mpl_media_state_cb(MPL_MEDIA_STATE_SEEKING);

	return 0;
}

int cmd_mpl_media_opcodes_changed_cb(const struct shell *shell, size_t argc,
				     char *argv[])
{
	mpl_operations_supported_cb(0x00aa55aa);

	return 0;
}

#ifdef CONFIG_BT_OTS_TEMP
int cmd_mpl_search_results_changed_cb(const struct shell *shell, size_t argc,
				      char *argv[])
{
	mpl_search_results_id_cb(19);

	return 0;
}
#endif /* CONFIG_BT_OTS_TEMP */

static int cmd_mpl(const struct shell *shell, size_t argc, char **argv)
{
	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(mpl_cmds,
#if defined(CONFIG_BT_DEBUG_MCS) && defined(CONFIG_BT_TESTING)
	SHELL_CMD_ARG(test_set_media_state, NULL,
		      "Set the media player state (test) <state>",
		      cmd_mpl_test_set_media_state, 2, 0),
#if CONFIG_BT_OTS_TEMP
	SHELL_CMD_ARG(test_unset_parent_group, NULL,
		      "Set current group to be its own parent (test)",
		      cmd_mpl_test_unset_parent_group, 1, 0),
#endif /* CONFIG_BT_OTS_TEMP */
	SHELL_CMD_ARG(test_set_operation, NULL,
		      "Write opcode to local control point (test) <opcode> [argument]",
		      cmd_mpl_test_set_operation, 2, 1),
#endif /* CONFIG_BT_DEBUG_MCS && CONFIG_BT_TESTING */
#if defined(CONFIG_BT_DEBUG_MCS)
	SHELL_CMD_ARG(debug_dump_state, NULL,
		      "Dump media player's state as debug output (debug)",
		      cmd_mpl_debug_dump_state, 1, 0),
#endif /* CONFIG_BT_DEBUG_MCC */
	SHELL_CMD_ARG(init, NULL,
		      "Initialize media player",
		      cmd_mpl_init, 1, 0),
	SHELL_CMD_ARG(track_changed_cb, NULL,
		      "Send Track Changed notification",
		      cmd_mpl_track_changed_cb, 1, 0),
	SHELL_CMD_ARG(title_changed_cb, NULL,
		      "Send (fake) Track Title notification",
		      cmd_mpl_title_changed_cb, 1, 0),
	SHELL_CMD_ARG(duration_changed_cb, NULL,
		      "Send Track Duration notification",
		      cmd_mpl_duration_changed_cb, 1, 0),
	SHELL_CMD_ARG(position_changed_cb, NULL,
		      "Send Track Position notification",
		      cmd_mpl_position_changed_cb, 1, 0),
	SHELL_CMD_ARG(playback_speed_changed_cb, NULL,
		      "Send Playback Speed notification",
		      cmd_mpl_playback_speed_changed_cb, 1, 0),
	SHELL_CMD_ARG(seeking_speed_changed_cb, NULL,
		      "Send Seeking Speed notification",
		      cmd_mpl_seeking_speed_changed_cb, 1, 0),
#ifdef CONFIG_BT_OTS_TEMP
	SHELL_CMD_ARG(current_track_id_changed_cb, NULL,
		      "Send Current Track notification",
		      cmd_mpl_current_track_id_changed_cb, 1, 0),
	SHELL_CMD_ARG(next_track_id_changed_cb, NULL,
		      "Send Next Track notification",
		      cmd_mpl_next_track_id_changed_cb, 1, 0),
	SHELL_CMD_ARG(group_id_changed_cb, NULL,
		      "Send Current Group notification",
		      cmd_mpl_group_id_changed_cb, 1, 0),
	SHELL_CMD_ARG(parent_group_id_changed_cb, NULL,
		      "Send Parent Group notification",
		      cmd_mpl_parent_group_id_changed_cb, 1, 0),
#endif /* CONFIG_BT_OTS_TEMP */
	SHELL_CMD_ARG(playing_order_changed_cb, NULL,
		      "Send Playing Order notification",
		      cmd_mpl_playing_order_changed_cb, 1, 0),
	SHELL_CMD_ARG(state_changed_cb, NULL,
		      "Send Media State notification",
		      cmd_mpl_state_changed_cb, 1, 0),
	SHELL_CMD_ARG(media_opcodes_changed_cb, NULL,
		      "Send Supported Opcodes notfication",
		      cmd_mpl_media_opcodes_changed_cb, 1, 0),
#ifdef CONFIG_BT_OTS_TEMP
	SHELL_CMD_ARG(search_results_changed_cb, NULL,
		      "Send Search Results Object ID notification",
		      cmd_mpl_search_results_changed_cb, 1, 0),
#endif /* CONFIG_BT_OTS_TEMP */
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(mpl, &mpl_cmds, "Media player (MCS) related commands",
		       cmd_mpl, 1, 1);

#endif /* CONFIG_BT_MCS */

#ifdef __cplusplus
}
#endif
