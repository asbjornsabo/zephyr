/**
 * @file
 * @brief Internal Header for Bluetooth Volumen Control Service (VCS).
 *
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_VCS_INTERNAL_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_VCS_INTERNAL_

/* VCS opcodes */
#define VCS_OPCODE_REL_VOL_DOWN			0x00
#define VCS_OPCODE_REL_VOL_UP			0x01
#define VCS_OPCODE_UNMUTE_REL_VOL_DOWN		0x02
#define VCS_OPCODE_UNMUTE_REL_VOL_UP		0x03
#define VCS_OPCODE_SET_ABS_VOL			0x04
#define VCS_OPCODE_UNMUTE			0x05
#define VCS_OPCODE_MUTE				0x06

struct vcs_state_t {
	uint8_t volume;
	uint8_t mute;
	uint8_t change_counter;
} __packed;

struct vcs_control {
	uint8_t opcode;
	uint8_t counter;
} __packed;

struct vcs_control_vol {
	struct vcs_control cp;
	uint8_t volume;
} __packed;

int bt_vcs_client_get(struct bt_conn *conn, struct bt_vcs *client);
int bt_vcs_client_read_volume_state(struct bt_conn *conn);
int bt_vcs_client_read_flags(struct bt_conn *conn);
int bt_vcs_client_volume_down(struct bt_conn *conn);
int bt_vcs_client_volume_up(struct bt_conn *conn);
int bt_vcs_client_unmute_volume_down(struct bt_conn *conn);
int bt_vcs_client_unmute_volume_up(struct bt_conn *conn);
int bt_vcs_client_set_volume(struct bt_conn *conn, uint8_t volume);
int bt_vcs_client_unmute(struct bt_conn *conn);
int bt_vcs_client_mute(struct bt_conn *conn);
int bt_vcs_client_vocs_read_location(struct bt_conn *conn,
				     struct bt_vocs *inst);
int bt_vcs_client_vocs_set_location(struct bt_conn *conn, struct bt_vocs *inst,
				    uint8_t location);
int bt_vcs_client_vocs_set_offset(struct bt_conn *conn,
				  struct bt_vocs *vocs_inst,
				  int16_t offset);
int bt_vcs_client_vocs_read_output_description(struct bt_conn *conn,
					       struct bt_vocs *inst);
int bt_vcs_client_vocs_set_output_description(struct bt_conn *conn,
					      struct bt_vocs *inst,
					      const char *description);
int bt_vcs_client_aics_read_input_state(struct bt_conn *conn,
					struct bt_aics *inst);
int bt_vcs_client_aics_read_gain_setting(struct bt_conn *conn,
					 struct bt_aics *inst);
int bt_vcs_client_aics_read_input_type(struct bt_conn *conn,
				       struct bt_aics *inst);
int bt_vcs_client_aics_read_input_status(struct bt_conn *conn,
					 struct bt_aics *inst);
int bt_vcs_client_aics_input_unmute(struct bt_conn *conn, struct bt_aics *inst);
int bt_vcs_client_aics_input_mute(struct bt_conn *conn, struct bt_aics *inst);
int bt_vcs_client_aics_set_manual_input_gain(struct bt_conn *conn,
					     struct bt_aics *inst);
int bt_vcs_client_aics_set_automatic_input_gain(struct bt_conn *conn,
						struct bt_aics *inst);
int bt_vcs_client_aics_set_gain(struct bt_conn *conn, struct bt_aics *inst,
				int8_t gain);
int bt_vcs_client_aics_read_input_description(struct bt_conn *conn,
					      struct bt_aics *inst);
int bt_vcs_client_aics_set_input_description(struct bt_conn *conn,
					     struct bt_aics *inst,
					     const char *description);

bool bt_vcs_client_valid_vocs_inst(struct bt_vocs *vocs);
bool bt_vcs_client_valid_aics_inst(struct bt_aics *aics);
#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_VCS_INTERNAL_*/
