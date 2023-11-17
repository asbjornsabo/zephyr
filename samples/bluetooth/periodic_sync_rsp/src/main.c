/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>


/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0});
static struct gpio_callback button_cb_data;


#define NAME_LEN 30
#define DEVICE_ID 0 /* Temporary hack to identify a device - must be set to */
                    /* separate value for each device */

static K_SEM_DEFINE(sem_per_adv, 0, 1);
static K_SEM_DEFINE(sem_per_sync, 0, 1);
static K_SEM_DEFINE(sem_per_sync_lost, 0, 1);

static struct bt_conn *default_conn;
static struct bt_le_per_adv_sync *default_sync;
static struct __packed {
	uint8_t subevent;
	uint8_t response_slot;
} pawr_timing;

static struct {
	bool connected;       /* Whether we are currently connected to the periodic advertiser */
	bool conn_requested;  /* Whether we want to connect to the periodic advertiser */
} conn_state;

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
	if (conn_state.connected) {
		printk("Already connected\n)");
	} else if (conn_state.conn_requested) {
		printk("Connection already requested\n");
	} else {
		printk("Not connected - scheduling connection\n");
		conn_state.conn_requested = true;
	}
}

static void sync_cb(struct bt_le_per_adv_sync *sync, struct bt_le_per_adv_sync_synced_info *info)
{
	struct bt_le_per_adv_sync_subevent_params params;
	uint8_t subevents[1];
	char le_addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	printk("Synced to %s with %d subevents\n", le_addr, info->num_subevents);

	default_sync = sync;

	params.properties = 0;
	params.num_subevents = 1;
	params.subevents = subevents;
	subevents[0] = pawr_timing.subevent;

	err = bt_le_per_adv_sync_subevent(sync, &params);
	if (err) {
		printk("Failed to set subevents to sync to (err %d)\n", err);
	}

	k_sem_give(&sem_per_sync);
}

static void term_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("Sync terminated (reason %d)\n", info->reason);

	default_sync = NULL;

	k_sem_give(&sem_per_sync_lost);
}

static bool print_ad_field(struct bt_data *data, void *user_data)
{
	ARG_UNUSED(user_data);

	printk("    0x%02X: ", data->type);
	for (size_t i = 0; i < data->data_len; i++) {
		printk("%02X", data->data[i]);
	}

	printk("\n");

	return true;
}

int bt_le_per_adv_set_response_data(struct bt_le_per_adv_sync *per_adv_sync,
				    const struct bt_le_per_adv_response_params *params,
				    const struct net_buf_simple *data);

static struct bt_le_per_adv_response_params rsp_params;

NET_BUF_SIMPLE_DEFINE_STATIC(rsp_buf, 247);

static void recv_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info, struct net_buf_simple *buf)
{
	int err;

	if (buf && buf->len) {
		/* Echo the data back to the advertiser, excdept that */
		/* that we modify the second byte according to which device we are */
		/* (This overwrites the Nordic manufactuerer ID. */
		/* And we set the third byte to indicate whether we request a connection. */
		net_buf_simple_reset(&rsp_buf);
		net_buf_simple_add_mem(&rsp_buf, buf->data, buf->len);
		rsp_buf.data[2] = DEVICE_ID;
		if (conn_state.conn_requested) {
			rsp_buf.data[3] = 1;
		} else {
			rsp_buf.data[3] = 0;
		}

		rsp_params.request_event = info->periodic_event_counter;
		rsp_params.request_subevent = info->subevent;
		/* Respond in current subevent and assigned response slot */
		rsp_params.response_subevent = info->subevent;
		rsp_params.response_slot = pawr_timing.response_slot;

		printk("Indication: subevent %d, responding in slot %d\n", info->subevent,
		       pawr_timing.response_slot);
		bt_data_parse(buf, print_ad_field, NULL);

		err = bt_le_per_adv_set_response_data(sync, &rsp_params, &rsp_buf);
		if (err) {
			printk("Failed to send response (err %d)\n", err);
		}
	} else if (buf) {
		printk("Received empty indication: subevent %d\n", info->subevent);
	} else {
		printk("Failed to receive indication: subevent %d\n", info->subevent);
	}
}

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = sync_cb,
	.term = term_cb,
	.recv = recv_cb,
};

static struct bt_uuid_128 pawr_svc_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0));
static struct bt_uuid_128 pawr_char_uuid =
	BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));

static ssize_t write_timing(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			    uint16_t len, uint16_t offset, uint8_t flags)
{
	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(pawr_timing)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&pawr_timing, buf, len);

	printk("New timing: subevent %d, response slot %d\n", pawr_timing.subevent,
	       pawr_timing.response_slot);

	struct bt_le_per_adv_sync_subevent_params params;
	uint8_t subevents[1];
	int err;

	params.properties = 0;
	params.num_subevents = 1;
	params.subevents = subevents;
	subevents[0] = pawr_timing.subevent;

	if (default_sync) {
		err = bt_le_per_adv_sync_subevent(default_sync, &params);
		if (err) {
			printk("Failed to set subevents to sync to (err %d)\n", err);
		}
	} else {
		printk("Not synced yet\n");
	}

	return len;
}

BT_GATT_SERVICE_DEFINE(pawr_svc, BT_GATT_PRIMARY_SERVICE(&pawr_svc_uuid.uuid),
		       BT_GATT_CHARACTERISTIC(&pawr_char_uuid.uuid, BT_GATT_CHRC_WRITE,
					      BT_GATT_PERM_WRITE, NULL, write_timing,
					      &pawr_timing));

void connected(struct bt_conn *conn, uint8_t err)
{
	printk("Connected (err 0x%02X)\n", err);

	if (err) {
		default_conn = NULL;

		return;
	}

	default_conn = bt_conn_ref(conn);
}

void disconnected(struct bt_conn *conn, uint8_t reason)
{
	bt_conn_unref(default_conn);
	default_conn = NULL;

	printk("Disconnected (reason 0x%02X)\n", reason);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected,
	.disconnected = disconnected,
};

int main(void)
{
	struct bt_le_per_adv_sync_transfer_param past_param;
	int err;

	printk("Starting Periodic Advertising with Responses Synchronization Demo\n");

	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n",
		       button.port->name);
		return 0;
	}

	err = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (err != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       err, button.port->name, button.pin);
		return 0;
	}

	err = gpio_pin_interrupt_configure_dt(&button,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (err != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			err, button.port->name, button.pin);
		return 0;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);

		return 0;
	}

	bt_le_per_adv_sync_cb_register(&sync_callbacks);

	past_param.skip = 1;
	past_param.timeout = 1000; /* 10 seconds */
	past_param.options = BT_LE_PER_ADV_SYNC_TRANSFER_OPT_NONE;
	err = bt_le_per_adv_sync_transfer_subscribe(NULL, &past_param);
	if (err) {
		printk("PAST subscribe failed (err %d)\n", err);

		return 0;
	}

	do {
		err = bt_le_adv_start(
			BT_LE_ADV_PARAM(BT_LE_ADV_OPT_ONE_TIME | BT_LE_ADV_OPT_CONNECTABLE |
						BT_LE_ADV_OPT_USE_NAME |
						BT_LE_ADV_OPT_FORCE_NAME_IN_AD,
					BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL),
			NULL, 0, NULL, 0);
		if (err && err != -EALREADY) {
			printk("Advertising failed to start (err %d)\n", err);

			return 0;
		}

		printk("Waiting for periodic sync...\n");
		err = k_sem_take(&sem_per_sync, K_SECONDS(10));
		if (err) {
			printk("Timed out while synchronizing\n");

			continue;
		}

		printk("Periodic sync established.\n");

		err = k_sem_take(&sem_per_sync_lost, K_FOREVER);
		if (err) {
			printk("failed (err %d)\n", err);

			return 0;
		}

		printk("Periodic sync lost.\n");
	} while (true);

	return 0;
}
