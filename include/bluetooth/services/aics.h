/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_AICS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_AICS_H_

/** @brief Audio Input Control Service (AICS)
 *
 *  @defgroup bt_gatt_aics Audio Input Control Service (AICS)
 *
 *  @ingroup bluetooth
 *  @{
 *
 *  AICS is currently only implemented as a secondary service, and as such do
 *  not have any public API, but defines the callbacks used by the primary
 *  services that include AICS.
 *
 *  [Experimental] Users should note that the APIs can change
 *  as a part of ongoing development.
 */

#include <zephyr/types.h>
#include <bluetooth/bluetooth.h>

#ifdef __cplusplus
extern "C" {
#endif

/* AICS mute states */
#define AICS_STATE_UNMUTED                      0x00
#define AICS_STATE_MUTED                        0x01
#define AICS_STATE_MUTE_DISABLED                0x02

/* AICS input modes */
#define AICS_MODE_MANUAL_ONLY                   0x00
#define AICS_MODE_AUTO_ONLY                     0x01
#define AICS_MODE_MANUAL                        0x02
#define AICS_MODE_AUTO                          0x03

/* AICS input types (Values are TBD) */
#define AICS_INPUT_TYPE_LOCAL                   0x00
#define AICS_INPUT_TYPE_ISO                     0x01
#define AICS_INPUT_TYPE_ANALOG                  0x02
#define AICS_INPUT_TYPE_DIGITAL                 0x03
#define AICS_INPUT_TYPE_RADIO                   0x04
#define AICS_INPUT_TYPE_PHYS_MEDIA              0x05
#define AICS_INPUT_TYPE_NETWORK                 0x06
#define AICS_INPUT_TYPE_OTHER                   0x255

/* AICS Error codes */
#define AICS_ERR_INVALID_COUNTER                0x80
#define AICS_ERR_OP_NOT_SUPPORTED               0x81
#define AICS_ERR_MUTE_DISABLED                  0x82
#define AICS_ERR_OUT_OF_RANGE                   0x83
#define AICS_ERR_GAIN_MODE_NO_SUPPORT           0x84

/** @brief Opaque AICS instance. */
struct bt_aics;

/** @brief Structure for initializing a Audio Input Control Service instance.
 */
struct bt_aics_init {
	/** Initial audio input gain (-128 to 127) */
	int8_t gain;

	/** Initial audio input mute state */
	uint8_t mute;

	/** Initial audio input mode */
	uint8_t mode;

	/** Initial audio input gain units (N * 0.1 dB) */
	uint8_t units;

	/** Initial audio input minimum gain */
	int8_t min_gain;

	/** Initial audio input maximum gain */
	int8_t max_gain;

	/** @brief Initial audio input type */
	int8_t input_type;

	/** Initial audio input state (enabled/disabled) */
	bool input_state;

	/** Boolean to set whether the description is writable by clients */
	bool desc_writable;

	/** Initial audio input description */
	char *input_desc;
};

/** @brief Structure for discovering a Audio Input Control Service instance.
 */
struct bt_aics_discover_param {
	/** @brief The start handle of the discovering.
	 *
	 * Typically the @p start_handle of a @ref bt_gatt_include.
	 */
	uint16_t start_handle;
	/** @brief The end handle of the discovering.
	 *
	 * Typically the @p end_handle of a @ref bt_gatt_include.
	 */
	uint16_t end_handle;
};

/** @brief Get the service declaration attribute.
 *
 *  The first service attribute returned can be included in any other
 *  GATT service.
 *
 *  @param aics Audio Input Control Service instance.
 *
 *  @return Pointer to the attributes of the service.
 */
void *bt_aics_svc_decl_get(struct bt_aics *aics);

/** @brief Initialize the Audio Input Control Service instance.
 *
 *  @param aics      Audio Input Control Service instance.
 *  @param init      Audio Input Control Service initialization structure.
 *                   May be NULL to use default values.
 *
 *  @return 0 if success, ERRNO on failure.
 */
int bt_aics_init(struct bt_aics *aics, struct bt_aics_init *init);

/** @brief Get a free instance of Audio Input Control Service from the pool.
 *
 *  @return Audio Input Control Service instance in case of success or
 *          NULL in case of error.
 */
struct bt_aics *bt_aics_free_instance_get(void);

/** @brief Callback function for writes.
 *
 *  @param conn        Connection to peer device, or NULL if local server write.
 *  @param inst        The service pointer (as there may be multiple).
 *  @param err         Error value. 0 on success, GATT error on fail.
 */
typedef void (*bt_aics_write_cb_t)(
	struct bt_conn *conn, struct bt_aics *inst, int err);

/** @brief Callback function for the input state.
 *
 *  Called when the value is read,
 *  or if the value is changed by either the server or client.
 *
 *  @param conn         Connection to peer device, or NULL if local server read.
 *  @param inst         The service pointer (as there may be multiple).
 *  @param err          Error value. 0 on success, GATT error or ERRNO on fail.
 *                      For notifications, this will always be 0.
 *  @param gain         The gain setting value.
 *  @param mute         The mute value.
 *  @param mode         The mode value.
 */
typedef void (*bt_aics_state_cb_t)(
	struct bt_conn *conn, struct bt_aics *inst, int err, int8_t gain,
	uint8_t mute, uint8_t mode);

/** @brief Callback function for the gain settings.
 *
 *  Called when the value is read,
 *  or if the value is changed by either the server or client.
 *
 *  @param conn         Connection to peer device, or NULL if local server read.
 *  @param inst         The service pointer (as there may be multiple).
 *  @param err          Error value. 0 on success, GATT error or ERRNO on fail.
 *                      For notifications, this will always be 0.
 *  @param units        The value that reflect the size of a single increment
 *                      or decrement of the Gain Setting value in 0.1 decibel
 *                      units.
 *  @param minimum      The minimum gain allowed for the gain setting.
 *  @param maximum      The maximum gain allowed for the gain setting.
 */
typedef void (*bt_aics_gain_setting_cb_t)(
	struct bt_conn *conn, struct bt_aics *inst, int err, uint8_t units,
	int8_t minimum, int8_t maximum);

/** @brief Callback function for the input type.
 *
 *  Called when the value is read,
 *  or if the value is changed by either the server or client.
 *
 *  @param conn         Connection to peer device, or NULL if local server read.
 *  @param inst         The service pointer (as there may be multiple).
 *  @param err          Error value. 0 on success, GATT error or ERRNO on fail.
 *                      For notifications, this will always be 0.
 *  @param input_type   The input type.
 */
typedef void (*bt_aics_input_type_cb_t)(
	struct bt_conn *conn, struct bt_aics *inst, int err,
	uint8_t input_type);


/** @brief Callback function for the input status.
 *
 *  Called when the value is read,
 *  or if the value is changed by either the server or client.
 *
 *  @param conn         Connection to peer device, or NULL if local server read.
 *  @param inst         The service pointer (as there may be multiple).
 *  @param err          Error value. 0 on success, GATT error or ERRNO on fail.
 *                      For notifications, this will always be 0.
 *  @param active       Whether the instance is active or inactive.
 */
typedef void (*bt_aics_status_cb_t)(
	struct bt_conn *conn, struct bt_aics *inst, int err, bool active);

/** @brief Callback function for the description.
 *
 *  Called when the value is read,
 *  or if the value is changed by either the server or client.
 *
 *  @param conn         Connection to peer device, or NULL if local server read.
 *  @param inst         The service pointer (as there may be multiple).
 *  @param err          Error value. 0 on success, GATT error or ERRNO on fail.
 *                      For notifications, this will always be 0.
 *  @param description  The description as a string (may have been clipped).
 */
typedef void (*bt_aics_description_cb_t)(
	struct bt_conn *conn, struct bt_aics *inst, int err, char *description);

/** @brief Callback function for bt_aics_discover.
 *
 *  This callback will usually be overwritten by the primary service that
 *  includes the Audio Input Control Service client.
 *
 *  @param conn         Connection to peer device, or NULL if local server read.
 *  @param inst         The instance pointer.
 *  @param err          Error value. 0 on success, GATT error or ERRNO on fail.
 *                      For notifications, this will always be 0.
 */
typedef void (*bt_aics_discover_cb_t)(
	struct bt_conn *conn, struct bt_aics *inst, int err);

struct bt_aics_cb {
	bt_aics_state_cb_t                 state;
	bt_aics_gain_setting_cb_t          gain_setting;
	bt_aics_input_type_cb_t            type;
	bt_aics_status_cb_t                status;
	bt_aics_description_cb_t           description;

#if defined(CONFIG_BT_AICS_CLIENT)
	bt_aics_discover_cb_t              discover;
	bt_aics_write_cb_t                 set_gain;
	bt_aics_write_cb_t                 unmute;
	bt_aics_write_cb_t                 mute;
	bt_aics_write_cb_t                 set_manual_mode;
	bt_aics_write_cb_t                 set_auto_mode;
#endif /* CONFIG_BT_AICS_CLIENT */
};


/** @brief Discover a Audio Input Control Service.
 *
 * Attempts to discover a Audio Input Control Service on a server given the
 * @p param.
 *
 * @param conn  Connection to the peer with the Audio Input Control Service.
 * @param inst  Pointer to the Audio Input Control Service client instance.
 * @param param Pointer to the parameters.
 *
 * @return 0 on success, ERRNO on fail.
 */
int bt_aics_discover(struct bt_conn *conn, struct bt_aics *inst,
		     const struct bt_aics_discover_param *param);

/** @brief Deactivates a Audio Input Control Service instance.
 *
 *  Audio Input Control Services are activated by default, but this will allow
 *  the server deactivate a Audio Input Control Service.
 *
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 if success, ERRNO on failure.
 */
int bt_aics_deactivate(struct bt_aics *inst);

/** @brief Activates a Audio Input Control Service instance.
 *
 *  Audio Input Control Services are activated by default, but this will allow
 *  the server reactivate a Audio Input Control Service instance after it has
 *  been deactivated with @ref bt_aics_deactivate.
 *
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 if success, ERRNO on failure.
 */
int bt_aics_activate(struct bt_aics *inst);

/** @brief Read the Audio Input Control Service input state.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_aics_state_get(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Read the Audio Input Control Service gain settings.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_aics_gain_setting_get(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Read the Audio Input Control Service input type.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_aics_type_get(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Read the Audio Input Control Service input status.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_aics_status_get(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Unmute the Audio Input Control Service input.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_aics_unmute(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Mute the Audio Input Control Service input.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_aics_mute(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Set input gain to manual.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to set local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_aics_manual_gain_set(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Set the input gain to automatic.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to set local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_aics_automatic_gain_set(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Set the input gain.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to set local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *  @param gain          The gain in dB to set (-128 to 127).
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_aics_gain_set(struct bt_conn *conn, struct bt_aics *inst, int8_t gain);

/** @brief Read the Audio Input Control Service description.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_aics_description_get(struct bt_conn *conn, struct bt_aics *inst);

/** @brief Set the Audio Input Control Service description.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to set local server value.
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *  @param description   The description to set.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_aics_description_set(struct bt_conn *conn, struct bt_aics *inst,
			    const char *description);

/** @brief Register callbacks for the Audio Input Control Service.
 *
 *  @param inst          Pointer to the Audio Input Control Service instance.
 *  @param cb            Pointer to the callback structure.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_aics_cb_register(struct bt_aics *inst, struct bt_aics_cb *cb);

/** @brief Get a new Audio Input Control Service client instance.
 *
 * @return Pointer to the instance, or NULL if no free instances are left.
 */
struct bt_aics *bt_aics_client_free_instance_get(void);

/** @brief Registers the callbacks for the Audio Input Control Service client.
 *
 *  @param inst  Pointer to the Audio Input Control Service client instance.
 *  @param cb    Pointer to the callback structure.
 */
void bt_aics_client_cb_register(struct bt_aics *inst, struct bt_aics_cb *cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_AICS_H_ */
