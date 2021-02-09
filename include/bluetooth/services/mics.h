/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_MICS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_MICS_H_

/** @brief Microphone Input Control Service (MICS)
 *
 *  @defgroup bt_gatt_mics Microphone Input Control Service (MICS)
 *
 *  @ingroup bluetooth
 *  @{
 *
 *  [Experimental] Users should note that the APIs can change
 *  as a part of ongoing development.
 */

#include <zephyr/types.h>
#include <bluetooth/services/aics.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BT_MICS)
#define MICS_AICS_CNT CONFIG_BT_MICS_AICS_INSTANCE_COUNT
#else
#define MICS_AICS_CNT 0
#endif /* CONFIG_BT_MICS */

/* Application error codes */
#define BT_MICS_ERR_MUTE_DISABLED               0x80
#define BT_MICS_ERR_VAL_OUT_OF_RANGE            0x81

/** Microphone Input Control Service mute states */
#define BT_MICS_MUTE_UNMUTED                       0x00
#define BT_MICS_MUTE_MUTED                         0x01
#define BT_MICS_MUTE_DISABLED                      0x02

/** @brief Initializing structure for Microphone Input Control Service */
struct bt_mics_init {
	/** Initializing structure for Audio Input Control Services */
	struct bt_aics_init aics_init[MICS_AICS_CNT];
};

/** @brief MICS service instance
 *
 * Used for to represent a MICS service, either for a client
 * or a server instance. The instance pointers either represent local
 * server instances, or remote service instances.
 */
struct bt_mics {
	/** Number of Audio Input Control Service instances */
	uint8_t aics_cnt;
	/** Array of pointers to Audio Input Control Service instances */
	struct bt_aics **aics;
};

/** @brief Initialize the Microphone Input Control Service
 *
 *  This will enable the service and make it discoverable by clients.
 *
 *  @param init  Pointer to a initialization structure. May be NULL to use
 *               default values.
 *
 *  @return 0 if success, ERRNO on failure.
 */
int bt_mics_init(struct bt_mics_init *init);

/** @brief Get MICS service pointer
 *
 *  Returns a pointer to a struct that contains information about the MICS
 *  instance, such as pointers to the AICS instances.
 *
 *  @param conn          Connection to peer device, or NULL to get server value.
 *  @param[out] service  Pointer to store the result in.
 *
 *  @return 0 if success, ERRNO on failure.
 */
int bt_mics_get(struct bt_conn *conn, struct bt_mics *service);

/** @brief Callback function for @ref bt_mics_discover.
 *
 *  @param conn         The connection that was used to discover MICS.
 *  @param err          Error value. 0 on success, GATT error or ERRNO on fail.
 *  @param aics_count   Number of Audio Input Control Service instances on
 *                      peer device.
 */
typedef void (*bt_mics_discover_cb_t)(
	struct bt_conn *conn, int err, uint8_t aics_count);

/** @brief Callback function for MICS mute.
 *
 *  Called when the value is read,
 *  or if the value is changed by either the server or client.
 *
 *  @param conn     Connection to peer device, or NULL if local server read.
 *  @param err      Error value. 0 on success, GATT error or ERRNO on fail.
 *                  For notifications, this will always be 0. For reads, if
 *                  this is 0, the @p volume and @p mute values are the last
 *                  known values.
 *  @param mute     The mute setting of the MICS server.
 */
typedef void (*bt_mics_mute_read_cb_t)(
	struct bt_conn *conn, int err, uint8_t mute);

/** @brief Callback function for MICS mute/unmute.
 *
 *  @param conn      Connection to peer device, or NULL if local server read.
 *  @param err       Error value. 0 on success, GATT error or ERRNO on fail.
 *  @param req_val   The requested mute value.
 */
typedef void (*bt_mics_mute_write_cb_t)(
	struct bt_conn *conn, int err, uint8_t req_val);

struct bt_mics_cb_t {
#if defined(CONFIG_BT_MICS_CLIENT)
	bt_mics_discover_cb_t           discover;
	bt_mics_mute_write_cb_t         mute_write;
#endif /* CONFIG_BT_MICS_CLIENT */

	bt_mics_mute_read_cb_t          mute;

	/* Audio Input Control Service */
	struct bt_aics_cb               aics_cb;
};

/** @brief Discover MICS
 *
 *  This will start a GATT discovery and setup handles and subscriptions.
 *  This shall be called once before any other actions can completed for
 *  the peer device.
 *
 *  @param conn          The connection to initialize the profile for.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_discover(struct bt_conn *conn);

/** @brief Unmute the server.
 *
 *  @param conn   Connection to peer device, or NULL to read local server value.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_unmute(struct bt_conn *conn);

/** @brief Mute the server.
 *
 *  @param conn   Connection to peer device, or NULL to read local server value.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_mute(struct bt_conn *conn);

/** @brief Disable the mute functionality.
 *
 *  Can be reenabled by called @ref bt_mics_mute or @ref bt_mics_unmute.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_mute_disable(void);

/** @brief Read the mute state of a MICS server.
 *
 *  @param conn   Connection to peer device, or NULL to read local server value.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_mute_get(struct bt_conn *conn);

/** @brief Read the Audio Input Control Service input state.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_state_get(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Read the Audio Input Control Service gain settings.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_gain_setting_get(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Read the Audio Input Control Service input type.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_type_get(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Read the Audio Input Control Service input status.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_status_get(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Unmute the Audio Input Control Service input.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_unmute(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Mute the Audio Input Control Service input.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_mute(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Set input gain to manual.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to set local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_manual_gain_set(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Set Audio Input Control Service input gain to automatic.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to set local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_automatic_gain_set(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Set Audio Input Control Service input gain.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to set local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *  @param gain          The gain in dB to set (-128 to 127).
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_gain_set(struct bt_conn *conn, struct bt_aics *inst,
			  int8_t gain);

/** @brief Read the Audio Input Control Service description.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_description_get(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Set the Audio Input Control Service description.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to set local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *  @param description	 The description to set.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_mics_aics_description_set(struct bt_conn *conn, struct bt_aics *inst,
				 const char *description);

/** @brief Deactivates a Audio Input Control Service instance.
 *
 *  Audio Input Control Services are activated by default, but this will allow
 *  the server deactivate a Audio Input Control Service.
 *
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 if success, ERRNO on failure.
 */
int bt_mics_aics_deactivate(struct bt_aics *inst);

/** @brief Activates a Audio Input Control Service instance.
 *
 *  Audio Input Control Services are activated by default, but this will allow
 *  the server reactivate a Audio Input Control Service instance after it has
 *  been deactivated with @ref bt_mics_aics_deactivate.
 *
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 if success, ERRNO on failure.
 */
int bt_mics_aics_activate(struct bt_aics *inst);

/** @brief Registers the callbacks used by MICS client.
 *
 *  @param cb    The callback structure.
 */
void bt_mics_client_cb_register(struct bt_mics_cb_t *cb);

/** @brief Registers the callbacks used by MICS server.
 *
 *  @param cb    The callback structure.
 */
void bt_mics_server_cb_register(struct bt_mics_cb_t *cb);

#ifdef __cplusplus
}
#endif

/**
 *  @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_MICS_H_ */
