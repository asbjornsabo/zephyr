/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_VOCS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_VOCS_H_

/** @brief Volume Offset Control Service (VOCS)
 *
 *  @defgroup bt_gatt_vocs Volume Offset Control Service (VOCS)
 *
 *  @ingroup bluetooth
 *  @{
 *
 *  Volume Offset Control Service is a secondary service,
 *  and as such should not be used own its own,
 *  but rather in the context of another (primary) service.
 *
 *  [Experimental] Users should note that the APIs can change
 *  as a part of ongoing development.
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Volume Offset Control Service Error codes */
#define BT_VOCS_ERR_INVALID_COUNTER                0x80
#define BT_VOCS_ERR_OP_NOT_SUPPORTED               0x81
#define BT_VOCS_ERR_OUT_OF_RANGE                   0x82

#define BT_VOCS_MIN_OFFSET                         -255
#define BT_VOCS_MAX_OFFSET                         255

/** @brief Opaque Volume Offset Control Service instance. */
struct bt_vocs;

/** @brief Structure for initializing a Volume Offset Control Service instance.
 */
struct bt_vocs_init {
	/** @brief Audio Location bitmask
	 *
	 * Values TBD.
	*/
	uint8_t location;

	/** Boolean to set whether the location is writable by clients */
	bool location_writable;

	/** Initial volume offset (-255 to 255) */
	int16_t offset;

	/** Initial audio output description */
	char *output_desc;

	/** Boolean to set whether the description is writable by clients */
	bool desc_writable;
};

/** @brief Structure for discovering a Volume Offset Control Service instance.
 */
struct bt_vocs_discover_param {
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
 *  @param vocs Volume Offset Control Service instance.
 *
 *  @return Pointer to the attributes of the service.
 */
void *bt_vocs_svc_decl_get(struct bt_vocs *vocs);

/** @brief Initialize the Volume Offset Control Service instance.
 *
 *  @param vocs      Volume Offset Control Service instance.
 *  @param init      Volume Offset Control Service initialization structure.
 *                   May be NULL to use default values.
 *
 *  @return 0 if success, ERRNO on failure.
 */
int bt_vocs_init(struct bt_vocs *vocs, struct bt_vocs_init *init);

/** @brief Get a free instance of Volume Offset Control Service from the pool.
 *
 *  @return Volume Offset Control Service instance in case of success or
 *          NULL in case of error.
 */
struct bt_vocs *bt_vocs_free_instance_get(void);

/** @brief Callback function for the offset state.
 *
 *  Called when the value is read,
 *  or if the value is changed by either the server or client.
 *
 *  @param conn         Connection to peer device, or NULL if local server read.
 *  @param inst         The instance pointer.
 *  @param err          Error value. 0 on success, GATT error or ERRNO on fail.
 *                      For notifications, this will always be 0.
 *  @param offset       The offset value.
 */
typedef void (*bt_vocs_state_cb_t)(
	struct bt_conn *conn, struct bt_vocs *inst, int err, int16_t offset);


/** @brief Callback function for writes.
 *
 *  @param conn        Connection to peer device, or NULL if local server write.
 *  @param inst        The instance pointer.
 *  @param err         Error value. 0 on success, GATT error on fail.
 */
typedef void (*bt_vocs_write_cb_t)(
	struct bt_conn *conn, struct bt_vocs *inst, int err);

/** @brief Callback function for the location.
 *
 *  Called when the value is read,
 *  or if the value is changed by either the server or client.
 *
 *  @param conn         Connection to peer device, or NULL if local server read.
 *  @param inst         The instance pointer.
 *  @param err          Error value. 0 on success, GATT error or ERRNO on fail.
 *                      For notifications, this will always be 0.
 *  @param location     The location value.
 */
typedef void (*bt_vocs_location_cb_t)(
	struct bt_conn *conn, struct bt_vocs *inst, int err, uint8_t location);

/** @brief Callback function for the description.
 *
 *  Called when the value is read,
 *  or if the value is changed by either the server or client.
 *
 *  @param conn         Connection to peer device, or NULL if local server read.
 *  @param inst         The instance pointer.
 *  @param err          Error value. 0 on success, GATT error or ERRNO on fail.
 *                      For notifications, this will always be 0.
 *  @param description  The description as a string.
 */
typedef void (*bt_vocs_description_cb_t)(
	struct bt_conn *conn, struct bt_vocs *inst, int err, char *description);

/** @brief Callback function for bt_vocs_discover.
 *
 *  This callback will usually be overwritten by the primary service that
 *  includes the Volume Control Offset Service client.
 *
 *  @param conn         Connection to peer device, or NULL if local server read.
 *  @param inst         The instance pointer.
 *  @param err          Error value. 0 on success, GATT error or ERRNO on fail.
 *                      For notifications, this will always be 0.
 */
typedef void (*bt_vocs_discover_cb_t)(
	struct bt_conn *conn, struct bt_vocs *inst, int err);

struct bt_vocs_cb {
	bt_vocs_state_cb_t              state;
	bt_vocs_location_cb_t           location;
	bt_vocs_description_cb_t        description;

#if defined(CONFIG_BT_VOCS_CLIENT)
	/* Client only */
	bt_vocs_discover_cb_t           discover;
	bt_vocs_write_cb_t              set_offset;
#endif /* CONFIG_BT_VCS_CLIENT */
};

/** @brief Read the Volume Offset Control Service offset state.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Volume Offset Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_vocs_state_get(struct bt_conn *conn, struct bt_vocs *inst);

/** @brief Set the Volume Offset Control Service offset state.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to set local server value.
 *  @param inst          Pointer to the Volume Offset Control Service instance.
 *  @param offset        The offset to set (-255 to 255).
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_vocs_state_set(struct bt_conn *conn, struct bt_vocs *inst,
		      int16_t offset);

/** @brief Read the Volume Offset Control Service location.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Volume Offset Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_vocs_location_get(struct bt_conn *conn, struct bt_vocs *inst);

/** @brief Set the Volume Offset Control Service location.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Volume Offset Control Service instance.
 *  @param location      The location to set.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_vocs_location_set(struct bt_conn *conn, struct bt_vocs *inst,
			 uint8_t location);

/** @brief Read the Volume Offset Control Service output description.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to read local server value.
 *  @param inst          Pointer to the Volume Offset Control Service instance.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_vocs_description_get(struct bt_conn *conn, struct bt_vocs *inst);

/** @brief Set the Volume Offset Control Service description.
 *
 *  @param conn          Connection to peer device,
 *                       or NULL to set local server value.
 *  @param inst          Pointer to the Volume Offset Control Service instance.
 *  @param description   The description to set. Value will be copied.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_vocs_description_set(struct bt_conn *conn, struct bt_vocs *inst,
			    const char *description);

/** @brief Register callbacks for the Volume Offset Control Service.
 *
 *  @param inst          Pointer to the Volume Offset Control Service instance.
 *  @param cb            Pointer to the callback structure.
 *
 *  @return 0 on success, GATT error value on fail.
 */
int bt_vocs_cb_register(struct bt_vocs *inst, struct bt_vocs_cb *cb);

/** @brief Registers the callbacks for the Volume Offset Control Service client.
 *
 *  @param inst  Pointer to the Volume Offset Control Service client instance.
 *  @param cb    Pointer to the callback structure.
 */
void bt_vocs_client_cb_register(struct bt_vocs *inst, struct bt_vocs_cb *cb);

/** @brief Returns a pointer to a Volume Offset Control Service client instance.
 *
 * @return Pointer to the instance, or NULL if no free instances are left.
 */
struct bt_vocs *bt_vocs_client_free_instance_get(void);

/** @brief Discover a Volume Offset Control Service.
 *
 * Attempts to discover a Volume Offset Control Service on a server given the
 * @p param.
 *
 * @param conn  Connection to the peer with the Volume Offset Control Service.
 * @param inst  Pointer to the Volume Offset Control Service client instance.
 * @param param Pointer to the parameters.
 *
 * @return 0 on success, ERRNO on fail.
 */
int bt_vocs_discover(struct bt_conn *conn, struct bt_vocs *inst,
		     const struct bt_vocs_discover_param *param);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_VOCS_H_ */
