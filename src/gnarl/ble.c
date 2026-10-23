#include "gnarl.h"

#include <stdbool.h>
#include <unistd.h>

#include <esp_bt.h>
#include <esp_nimble_hci.h>
#include <esp_timer.h>
#include <host/ble_gap.h>
#include <host/ble_store.h>
#include <host/util/util.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

#include "commands.h"
#include "display.h"
#include "module.h"

#define MAX_DATA		150
#define DEFAULT_NAME		"GNARL"

#define CUSTOM_NAME_SIZE	30
#define STORAGE_NAMESPACE	"GNARL"

// Advertising interval while disconnected, in units of 0.625 ms.
#define ADV_ITVL_MIN		240	// 150 ms
#define ADV_ITVL_MAX		400	// 250 ms

static uint8_t custom_name[CUSTOM_NAME_SIZE];

// Persistent (NVS) storage of bonding keys: CONFIG_BT_NIMBLE_NVS_PERSIST.
void ble_store_config_init(void);

#ifdef BLE_LOCK_TO_PHONE
// Characteristics that control the pump need an encrypted (bonded) link.
#define SEC_READ	(BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC)
#define SEC_WRITE	(BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC)
#else
#define SEC_READ	BLE_GATT_CHR_F_READ
#define SEC_WRITE	BLE_GATT_CHR_F_WRITE
#endif

// NVS key holding the identity address of the paired phone.
#define OWNER_KEY		"owner"

#define B0(x)	((x) & 0xFF)
#define B1(x)	(((x) >> 8) & 0xFF)
#define B2(x)	(((x) >> 16) & 0xFF)
#define B3(x)	(((x) >> 24) & 0xFF)
#define B4(x)	(((x) >> 32) & 0xFF)
#define B5(x)	(((x) >> 40) & 0xFF)

#define UUID128_CONST(a32, b16, c16, d16, e48)	\
	BLE_UUID128_INIT(						\
		B0(e48), B1(e48), B2(e48), B3(e48), B4(e48), B5(e48),	\
		B0(d16), B1(d16),					\
		B0(c16), B1(c16),					\
		B0(b16), B1(b16),					\
		B0(a32), B1(a32), B2(a32), B3(a32),			\
	)

static ble_uuid128_t service_uuid          = UUID128_CONST(0x0235733b, 0x99c5, 0x4197, 0xb856, 0x69219c2a3845);
static ble_uuid128_t data_uuid             = UUID128_CONST(0xc842e849, 0x5028, 0x42e2, 0x867c, 0x016adada9155);
static ble_uuid128_t response_count_uuid   = UUID128_CONST(0x6e6c7910, 0xb89e, 0x43a5, 0xa0fe, 0x50c5e2b81f4a);
static ble_uuid128_t timer_tick_uuid       = UUID128_CONST(0x6e6c7910, 0xb89e, 0x43a5, 0x78af, 0x50c5e2b86f7e);
static ble_uuid128_t custom_name_uuid      = UUID128_CONST(0xd93b2af0, 0x1e28, 0x11e4, 0x8c21, 0x0800200c9a66);
static ble_uuid128_t firmware_version_uuid = UUID128_CONST(0x30d99dc9, 0x7c91, 0x4295, 0xa051, 0x0a104d238cf2);
static ble_uuid128_t led_mode_uuid         = UUID128_CONST(0xc6d84241, 0xf1a7, 0x4f9c, 0xa25f, 0xfce16732f14e);

// Standard Bluetooth SIG Battery Service and Battery Level characteristic.
static ble_uuid16_t battery_service_uuid   = BLE_UUID16_INIT(0x180F);
static ble_uuid16_t battery_level_uuid     = BLE_UUID16_INIT(0x2A19);

static ble_gap_event_fn handle_gap_event;
static uint8_t addr_type;
static ble_gatt_access_fn data_access;
static ble_gatt_access_fn custom_name_access;
static ble_gatt_access_fn led_mode_access;
static ble_gatt_access_fn firmware_version_access;
static ble_gatt_access_fn battery_level_access;
static ble_gatt_access_fn no_access;

static volatile bool connected;
static volatile uint16_t connection_handle = BLE_HS_CONN_HANDLE_NONE;

static uint16_t response_count_notify_handle;
static volatile int response_count_notify_state;
static uint8_t response_count;

static uint16_t timer_tick_notify_handle;
static volatile int timer_tick_notify_state;
static uint8_t timer_tick;
static void timer_tick_callback(void *);

static uint16_t battery_level_handle;
static volatile int battery_level_notify_state;
static volatile uint8_t battery_level = 100;

static const struct ble_gatt_svc_def service_list[] = {
	{
		.type = BLE_GATT_SVC_TYPE_PRIMARY,
		.uuid = &service_uuid.u,
		.characteristics = (struct ble_gatt_chr_def[]){
			{
				.uuid = &data_uuid.u,
				.access_cb = data_access,
				.flags = SEC_READ | SEC_WRITE,
			},
			{
				.uuid = &response_count_uuid.u,
				.access_cb = no_access,
				.val_handle = &response_count_notify_handle,
				.flags = BLE_GATT_CHR_F_NOTIFY,
			},
			{
				.uuid = &timer_tick_uuid.u,
				.access_cb = no_access,
				.val_handle = &timer_tick_notify_handle,
				.flags = BLE_GATT_CHR_F_NOTIFY,
			},
			{
				.uuid = &custom_name_uuid.u,
				.access_cb = custom_name_access,
				.flags = BLE_GATT_CHR_F_READ | SEC_WRITE,
			},
			{
				.uuid = &firmware_version_uuid.u,
				.access_cb = firmware_version_access,
				.flags = BLE_GATT_CHR_F_READ,
			},
			{
				.uuid = &led_mode_uuid.u,
				.access_cb = led_mode_access,
				.flags = BLE_GATT_CHR_F_READ | SEC_WRITE,
			},
			{}, // End of characteristic list.
		},
        },
	{
		.type = BLE_GATT_SVC_TYPE_PRIMARY,
		.uuid = &battery_service_uuid.u,
		.characteristics = (struct ble_gatt_chr_def[]){
			{
				.uuid = &battery_level_uuid.u,
				.access_cb = battery_level_access,
				.val_handle = &battery_level_handle,
				.flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
			},
			{}, // End of characteristic list.
		},
	},
	{}, // End of service list.
};

static void server_init(void) {
	int err;
	char u[60];

	ble_svc_gap_init();
	ble_svc_gatt_init();

	err = ble_gatts_count_cfg(service_list);
	assert(!err);

	err = ble_gatts_add_svcs(service_list);
	assert(!err);

	ble_uuid_to_str(&service_uuid.u, u);
	ESP_LOGD(TAG, "service UUID %s", u);

	esp_timer_handle_t t;
	esp_timer_create_args_t timer_args = {
		.callback = timer_tick_callback,
	};
	ESP_ERROR_CHECK(esp_timer_create(&timer_args, &t));
	ESP_ERROR_CHECK(esp_timer_start_periodic(t, 60*SECONDS));
}

// ---- Phone lock -------------------------------------------------------

#ifdef BLE_LOCK_TO_PHONE
static bool have_owner;
static ble_addr_t owner_addr;
static esp_timer_handle_t pairing_timer;

static void load_owner(void) {
	have_owner = false;
	nvs_handle h;
	if (nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &h) != ESP_OK) {
		return;
	}
	size_t len = sizeof(owner_addr);
	if (nvs_get_blob(h, OWNER_KEY, &owner_addr, &len) == ESP_OK && len == sizeof(owner_addr)) {
		have_owner = true;
	}
	nvs_close(h);
}

static void save_owner(const ble_addr_t *addr) {
	nvs_handle h;
	esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &h);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "save_owner: nvs_open: %s", esp_err_to_name(err));
		return;
	}
	err = nvs_set_blob(h, OWNER_KEY, addr, sizeof(*addr));
	if (err == ESP_OK) {
		err = nvs_commit(h);
	}
	nvs_close(h);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "save_owner: %s", esp_err_to_name(err));
		return;
	}
	owner_addr = *addr;
	have_owner = true;
}

static bool is_owner(const ble_addr_t *addr) {
	return have_owner && ble_addr_cmp(addr, &owner_addr) == 0;
}

static void log_addr(const char *what, const ble_addr_t *a) {
	ESP_LOGI(TAG, "%s %02x:%02x:%02x:%02x:%02x:%02x (type %d)", what,
		 a->val[5], a->val[4], a->val[3], a->val[2], a->val[1], a->val[0], a->type);
}

// Runs in the esp_timer task: drop a link that never became encrypted.
static void pairing_timeout(void *arg) {
	uint16_t h = connection_handle;
	if (h == BLE_HS_CONN_HANDLE_NONE) {
		return;
	}
	struct ble_gap_conn_desc desc;
	if (ble_gap_conn_find(h, &desc) == 0 && !desc.sec_state.encrypted) {
		ESP_LOGW(TAG, "no encryption after %d s; disconnecting", BLE_PAIRING_TIMEOUT_S);
		ble_gap_terminate(h, BLE_ERR_AUTH_FAIL);
	}
}

// Called when a link becomes encrypted: accept the owner, adopt the first
// phone as owner, reject (and forget) anyone else.
static void check_encrypted_peer(uint16_t conn_handle) {
	struct ble_gap_conn_desc desc;
	if (ble_gap_conn_find(conn_handle, &desc) != 0) {
		return;
	}
	esp_timer_stop(pairing_timer);
	if (!have_owner) {
		save_owner(&desc.peer_id_addr);
		log_addr("paired with phone", &desc.peer_id_addr);
		return;
	}
	if (is_owner(&desc.peer_id_addr)) {
		ESP_LOGI(TAG, "paired phone connected");
		return;
	}
	log_addr("rejecting unknown device", &desc.peer_id_addr);
	display_update(PHONE_REJECTED, 0);
	ble_gap_terminate(conn_handle, BLE_ERR_AUTH_FAIL);
	ble_store_util_delete_peer(&desc.peer_id_addr);
}
#endif // BLE_LOCK_TO_PHONE

ble_pairing_state_t ble_pairing_state(void) {
#ifdef BLE_LOCK_TO_PHONE
	return have_owner ? PAIRING_LOCKED : PAIRING_OPEN;
#else
	return PAIRING_DISABLED;
#endif
}

void ble_forget_phone(void) {
#ifdef BLE_LOCK_TO_PHONE
	ESP_LOGW(TAG, "forgetting paired phone");
	nvs_handle h;
	if (nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &h) == ESP_OK) {
		nvs_erase_key(h, OWNER_KEY);
		nvs_commit(h);
		nvs_close(h);
	}
	have_owner = false;
	ble_store_clear();
	if (connection_handle != BLE_HS_CONN_HANDLE_NONE) {
		ble_gap_terminate(connection_handle, BLE_ERR_REM_USER_CONN_TERM);
	}
#endif
}

static void advertise(void) {
	struct ble_hs_adv_fields fields, fields_ext;
	char short_name[6]; // 5 plus zero byte
	memset(&fields, 0, sizeof(fields));
	memset(&fields_ext, 0, sizeof(fields_ext));

	fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

	fields.tx_pwr_lvl_is_present = 1;
	fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

	const char *name = ble_svc_gap_device_name();
	strncpy(short_name, name, sizeof(short_name) - 1);
	short_name[sizeof(short_name) - 1] = 0;
	fields.name = (uint8_t *)short_name;
	fields.name_len = strlen(short_name);
	if (strlen(name) <= 5) {
		fields.name_is_complete = 1;
	} else {
		fields.name_is_complete = 0;
		ESP_LOGD(TAG, "device name shortened to %s", short_name);
	}

	ESP_LOGD(TAG, "gap_device_name %d %s", fields.name_len, fields.name);

	fields.uuids128 = &service_uuid;
	fields.num_uuids128 = 1;
	fields.uuids128_is_complete = 1;

	int err = ble_gap_adv_set_fields(&fields);
	if (err) {
		ESP_LOGE(TAG, "ble_gap_adv_set_fields err %d", err);
	}
	assert(!err);

	if (!fields.name_is_complete) {
		// Not sure if we have to set the flags again.
		// We should determine and check the maximum
		// length here as well to prevent crashing.
		fields_ext.flags = fields.flags;
		fields_ext.name = (uint8_t *)name;
		fields_ext.name_len = strlen(name);
		fields_ext.name_is_complete = 1;
		int err = ble_gap_adv_set_fields(&fields_ext);
		if (err) {
			ESP_LOGE(TAG, "ble_gap_adv_set_fields fields_ext, name might be too long, err %d", err);
		}
		// Do not crash, but keep going. It is hard to recover otherwise.
	}

	// Begin advertising.
	struct ble_gap_adv_params adv;
	memset(&adv, 0, sizeof(adv));
	adv.conn_mode = BLE_GAP_CONN_MODE_UND;
	adv.disc_mode = BLE_GAP_DISC_MODE_GEN;
	adv.itvl_min = ADV_ITVL_MIN;
	adv.itvl_max = ADV_ITVL_MAX;

	err = ble_gap_adv_start(addr_type, 0, BLE_HS_FOREVER, &adv, handle_gap_event, 0);
	if (err && err != BLE_HS_EALREADY) {
		ESP_LOGE(TAG, "ble_gap_adv_start err %d", err);
	}

	ESP_LOGD(TAG, "advertising started");
}

static int handle_gap_event(struct ble_gap_event *e, void *arg) {
	switch (e->type) {
	case BLE_GAP_EVENT_CONNECT:
		if (e->connect.status != 0) {
			ESP_LOGE(TAG, "connection failed");
			advertise();
			return 0;
		}
		connection_handle = e->connect.conn_handle;
		connected = true;
		display_update(CONNECTED, true);
		ESP_LOGI(TAG, "connected");
#ifdef BLE_LOCK_TO_PHONE
		// Ask the phone to encrypt (pair the first time, or resume the
		// existing bond); drop it if that does not happen in time.
		ble_gap_security_initiate(connection_handle);
		esp_timer_stop(pairing_timer);
		esp_timer_start_once(pairing_timer, BLE_PAIRING_TIMEOUT_S * 1000000ULL);
#endif
		ESP_LOGD(TAG, "connection handle %04X", connection_handle);
		ESP_LOGD(TAG, "response count notify handle %04X", response_count_notify_handle);
		ESP_LOGD(TAG, "timer tick notify handle %04X", timer_tick_notify_handle);
		break;
	case BLE_GAP_EVENT_DISCONNECT:
		// Forget subscriptions: they belong to the old connection.
		connected = false;
		response_count_notify_state = 0;
		timer_tick_notify_state = 0;
		battery_level_notify_state = 0;
		connection_handle = BLE_HS_CONN_HANDLE_NONE;
#ifdef BLE_LOCK_TO_PHONE
		esp_timer_stop(pairing_timer);
#endif
		display_update(CONNECTED, false);
		ESP_LOGI(TAG, "disconnected, reason %d", e->disconnect.reason);
		advertise();
		break;
	case BLE_GAP_EVENT_ADV_COMPLETE:
		ESP_LOGD(TAG, "advertising complete");
		advertise();
		break;
#ifdef BLE_LOCK_TO_PHONE
	case BLE_GAP_EVENT_ENC_CHANGE:
		if (e->enc_change.status != 0) {
			ESP_LOGW(TAG, "encryption failed: %d", e->enc_change.status);
			break;
		}
		check_encrypted_peer(e->enc_change.conn_handle);
		break;
	case BLE_GAP_EVENT_REPEAT_PAIRING: {
		// The owner phone lost its bond (e.g. "Forget device") and pairs
		// again: replace the old keys. Ignore anyone else.
		struct ble_gap_conn_desc desc;
		if (ble_gap_conn_find(e->repeat_pairing.conn_handle, &desc) != 0) {
			return BLE_GAP_REPEAT_PAIRING_IGNORE;
		}
		if (!is_owner(&desc.peer_id_addr)) {
			log_addr("ignoring repeat pairing from", &desc.peer_id_addr);
			return BLE_GAP_REPEAT_PAIRING_IGNORE;
		}
		ble_store_util_delete_peer(&desc.peer_id_addr);
		return BLE_GAP_REPEAT_PAIRING_RETRY;
	}
#endif
	case BLE_GAP_EVENT_SUBSCRIBE:
		if (e->subscribe.attr_handle == response_count_notify_handle) {
			ESP_LOGD(TAG, "notify %d for response count", e->subscribe.cur_notify);
			response_count_notify_state = e->subscribe.cur_notify;
			break;
		}
		if (e->subscribe.attr_handle == timer_tick_notify_handle) {
			ESP_LOGD(TAG, "notify %d for timer tick", e->subscribe.cur_notify);
			timer_tick_notify_state = e->subscribe.cur_notify;
			break;
		}
		if (e->subscribe.attr_handle == battery_level_handle) {
			ESP_LOGD(TAG, "notify %d for battery level", e->subscribe.cur_notify);
			battery_level_notify_state = e->subscribe.cur_notify;
			break;
		}
		ESP_LOGD(TAG, "notify %d for unknown handle %04X", e->subscribe.cur_notify, e->subscribe.attr_handle);
		break;
	default:
		ESP_LOGD(TAG, "GAP event %d", e->type);
		break;
	}
	return 0;
}

static void sync_callback(void) {
	int err;

	err = ble_hs_util_ensure_addr(0);
	assert(!err);

	err = ble_hs_id_infer_auto(0, &addr_type);
	assert(!err);

	uint8_t addr[6];
	ble_hs_id_copy_addr(addr_type, addr, 0);
	if (LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG) {
		printf("device address: ");
		for (int i = sizeof(addr)-1; i >= 0; i--) {
			printf("%02x%s", addr[i], i == 0 ? "" : ":");
		}
		printf("\n");
	}
	advertise();
}

static uint8_t data_in[MAX_DATA];
static uint16_t data_in_len;

static uint8_t data_out[MAX_DATA];
static uint16_t data_out_len;

// Send a notification on the current connection.
// Failures (e.g. the phone just disconnected) are logged, never fatal.
static void notify(uint16_t attr_handle, int subscribed, const uint8_t *value, int len, const char *what) {
	if (!connected || !subscribed || connection_handle == BLE_HS_CONN_HANDLE_NONE) {
		ESP_LOGD(TAG, "not notifying for %s", what);
		return;
	}
	struct os_mbuf *om = ble_hs_mbuf_from_flat(value, len);
	if (om == NULL) {
		ESP_LOGE(TAG, "notify %s: out of mbufs", what);
		return;
	}
	int err = ble_gattc_notify_custom(connection_handle, attr_handle, om);
	if (err) {
		ESP_LOGW(TAG, "notify %s failed: err %d", what, err);
		return;
	}
	ESP_LOGD(TAG, "notify for %s", what);
}

static void response_notify(void) {
	response_count++;
	notify(response_count_notify_handle, response_count_notify_state,
	       &response_count, sizeof(response_count), "response count");
}

void send_code(const uint8_t code) {
	ESP_LOGD(TAG, "send_code %02X", code);
	data_out[0] = code;
	data_out_len = 1;
	response_notify();
}

void send_bytes(const uint8_t *buf, int count) {
	if (count > MAX_DATA - 1) {
		ESP_LOGE(TAG, "send_bytes: %d bytes is too long", count);
		count = MAX_DATA - 1;
	}
	print_bytes("send_bytes (%d):", buf, count);
	data_out[0] = RESPONSE_CODE_SUCCESS;
	memcpy(data_out + 1, buf, count);
	data_out_len = count + 1;
	response_notify();
}

void print_bytes(const char *msg, const uint8_t *buf, int count) {
	if (LOG_LOCAL_LEVEL < ESP_LOG_DEBUG) {
		return;
	}
	printf(msg, count);
	for (int i = 0; i < count; i++) {
		printf(" %02X", buf[i]);
	}
	printf("\n");
}

static void timer_tick_callback(void *arg) {
	timer_tick++;
	ESP_LOGD(TAG, "timer tick %d", timer_tick);
	notify(timer_tick_notify_handle, timer_tick_notify_state,
	       &timer_tick, sizeof(timer_tick), "timer tick");
}

void ble_battery_update(uint8_t percent) {
	if (percent > 100) {
		percent = 100;
	}
	if (percent == battery_level) {
		return;
	}
	battery_level = percent;
	uint8_t level = percent;
	notify(battery_level_handle, battery_level_notify_state,
	       &level, sizeof(level), "battery level");
}

static int battery_level_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
	if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
		return BLE_ATT_ERR_UNLIKELY;
	}
	uint8_t level = battery_level;
	ESP_LOGD(TAG, "battery level read: %d%%", level);
	if (os_mbuf_append(ctxt->om, &level, sizeof(level)) != 0) {
		return BLE_ATT_ERR_INSUFFICIENT_RES;
	}
	return 0;
}

static int data_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
	int err;
	int8_t rssi = 0;
	switch (ctxt->op) {
	case BLE_GATT_ACCESS_OP_READ_CHR:
		print_bytes("data_access: sending %d bytes:", data_out, data_out_len);
		if (os_mbuf_append(ctxt->om, data_out, data_out_len) != 0) {
			return BLE_ATT_ERR_INSUFFICIENT_RES;
		}
		return 0;
	case BLE_GATT_ACCESS_OP_WRITE_CHR:
		err = ble_hs_mbuf_to_flat(ctxt->om, data_in, sizeof(data_in), &data_in_len);
		if (err) {
			ESP_LOGE(TAG, "data_access: write too long (err %d)", err);
			return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
		}
		print_bytes("data_access: received %d bytes:", data_in, data_in_len);
		ble_gap_conn_rssi(conn_handle, &rssi);
		rfspy_command(data_in, data_in_len, (int)rssi);
		return 0;
	default:
		return BLE_ATT_ERR_UNLIKELY;
	}
}

static void read_custom_name(void) {
	ESP_LOGD(TAG, "read_custom_name from nvs");
	strcpy((char *)custom_name, DEFAULT_NAME);
	nvs_handle my_handle;
	esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "read_custom_name: nvs_open: %s", esp_err_to_name(err));
		return;
	}
	size_t required_size = CUSTOM_NAME_SIZE;
	err = nvs_get_blob(my_handle, "custom_name", custom_name, &required_size);
	if (err == ESP_ERR_NVS_NOT_FOUND) {
		strcpy((char *)custom_name, DEFAULT_NAME);
		ESP_LOGD(TAG, "set default custom name: %s", custom_name);
	} else if (err != ESP_OK) {
		strcpy((char *)custom_name, DEFAULT_NAME);
		ESP_LOGE(TAG, "read_custom_name: nvs_get_blob: %s", esp_err_to_name(err));
	} else {
		ESP_LOGD(TAG, "read_custom_name success: %s", custom_name);
	}
	// Guarantee termination even if the stored blob is corrupt.
	custom_name[CUSTOM_NAME_SIZE - 1] = 0;
	nvs_close(my_handle);
}

static void write_custom_name(void) {
	ESP_LOGD(TAG, "write_custom_name to nvs");
	nvs_handle my_handle;
	esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "write_custom_name: nvs_open: %s", esp_err_to_name(err));
		return;
	}
	err = nvs_set_blob(my_handle, "custom_name", custom_name, CUSTOM_NAME_SIZE);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "write_custom_name: nvs_set_blob: %s", esp_err_to_name(err));
		nvs_close(my_handle);
		return;
	}
	err = nvs_commit(my_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "write_custom_name: nvs_commit: %s", esp_err_to_name(err));
	}
	nvs_close(my_handle);
}

static int custom_name_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
	int err;
	uint8_t new_name[CUSTOM_NAME_SIZE];
	uint16_t len;
	switch (ctxt->op) {
	case BLE_GATT_ACCESS_OP_READ_CHR:
		len = strlen((char *)custom_name);
		print_bytes("custom_name_access: sending %d bytes:", custom_name, len);
		if (os_mbuf_append(ctxt->om, custom_name, len) != 0) {
			return BLE_ATT_ERR_INSUFFICIENT_RES;
		}
		return 0;
	case BLE_GATT_ACCESS_OP_WRITE_CHR:
		// Leave room for the terminating zero byte.
		err = ble_hs_mbuf_to_flat(ctxt->om, new_name, sizeof(new_name) - 1, &len);
		if (err || len == 0) {
			ESP_LOGE(TAG, "custom_name_access: invalid name length (err %d)", err);
			return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
		}
		new_name[len] = 0;
		memset(custom_name, 0, sizeof(custom_name));
		memcpy(custom_name, new_name, len + 1);
		print_bytes("custom_name_access: received %d bytes:", custom_name, len);
		ESP_LOGI(TAG, "new custom name: %s", custom_name);
		write_custom_name();
		esp_restart();
		return 0;
	default:
		return BLE_ATT_ERR_UNLIKELY;
	}
}

static int firmware_version_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
	if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
		return BLE_ATT_ERR_UNLIKELY;
	}
	ESP_LOGD(TAG, "BLE firmware version = %s", BLE_RFSPY_VERSION);
	if (os_mbuf_append(ctxt->om, (const uint8_t *)BLE_RFSPY_VERSION, strlen(BLE_RFSPY_VERSION)) != 0) {
		return BLE_ATT_ERR_INSUFFICIENT_RES;
	}
	return 0;
}

static uint8_t led_mode;

static int led_mode_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
	int err;
	uint16_t n;
	switch (ctxt->op) {
	case BLE_GATT_ACCESS_OP_READ_CHR:
		ESP_LOGD(TAG, "led_mode_access: mode = %d", led_mode);
		if (os_mbuf_append(ctxt->om, &led_mode, sizeof(led_mode))) {
			return BLE_ATT_ERR_INSUFFICIENT_RES;
		}
		return 0;
	case BLE_GATT_ACCESS_OP_WRITE_CHR:
		err = ble_hs_mbuf_to_flat(ctxt->om, &led_mode, sizeof(led_mode), &n);
		if (err) {
			return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
		}
		ESP_LOGD(TAG, "led_mode_access: set mode = %d", led_mode);
		return 0;
	default:
		return BLE_ATT_ERR_UNLIKELY;
	}
}

static int no_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
	char u[60];
	ble_uuid_to_str(ctxt->chr->uuid, u);
	ESP_LOGE(TAG, "should not happen: op %d, attr handle %04X, uuid %s", ctxt->op, attr_handle, u);
	return 0;
}

static void host_task(void *arg) {
	// Returns only after nimble_port_stop().
	nimble_port_run();
	// A FreeRTOS task must never return from its function.
	nimble_port_freertos_deinit();
}

void ble_shutdown(void) {
	// Stop notifications from other tasks before tearing down the stack.
	connected = false;
	connection_handle = BLE_HS_CONN_HANDLE_NONE;
	int rc = nimble_port_stop();
	if (rc != 0) {
		ESP_LOGW(TAG, "nimble_port_stop: %d", rc);
		return;
	}
	nimble_port_deinit();
	ESP_LOGI(TAG, "Bluetooth stopped");
}

void gnarl_init(void) {
	start_gnarl_task();

	ESP_ERROR_CHECK(nvs_flash_init());
	nimble_port_init();

	ble_hs_cfg.sync_cb = sync_callback;
	ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
#ifdef BLE_LOCK_TO_PHONE
	// "Just Works" bonding with LE Secure Connections: no PIN to type
	// (the bridge has no keyboard); keys are kept in flash.
	ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;
	ble_hs_cfg.sm_bonding = 1;
	ble_hs_cfg.sm_mitm = 0;
	ble_hs_cfg.sm_sc = 1;
	ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
	ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
	load_owner();
	esp_timer_create_args_t pt = {
		.callback = pairing_timeout,
		.name = "pairing",
	};
	ESP_ERROR_CHECK(esp_timer_create(&pt, &pairing_timer));
	ESP_LOGI(TAG, "phone lock on: %s", have_owner ? "paired" : "waiting for first phone");
#endif

	server_init();

	read_custom_name();

	int err = ble_svc_gap_device_name_set((char *)custom_name);
	assert(!err);

	ble_store_config_init();

	// Lower Bluetooth transmit power (see BLE_TX_POWER in module.h).
	esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, BLE_TX_POWER);
	esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, BLE_TX_POWER);
	esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN, BLE_TX_POWER);
	for (int i = ESP_BLE_PWR_TYPE_CONN_HDL0; i <= ESP_BLE_PWR_TYPE_CONN_HDL8; i++) {
		esp_ble_tx_power_set(i, BLE_TX_POWER);
	}

	nimble_port_freertos_init(host_task);
}
