#include "gnarl.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <esp_app_desc.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "battery.h"
#include "display.h"
#include "module.h"
#include "oled.h"

typedef struct {
	display_op_t op;
	int arg;
} display_command_t;

#define QUEUE_LENGTH	100

static QueueHandle_t display_queue;

static int connected = false;
static int phone_rssi;
static int pump_rssi;
static int command_time;  // seconds
static int battery_mv = -1;

#define DISPLAY_TIMEOUT	5  // seconds
#define SPLASH_TIMEOUT	4  // seconds

// Set when a screen that must be seen at once (warning, unpair) is queued;
// a screen being shown then gives way early instead of blocking 5 s.
static volatile bool urgent_pending;

static TaskHandle_t button_task_handle;

// Keep the current screen on for `seconds`, or less if an urgent screen
// is waiting (that one will redraw the display, so don't blank it).
static void hold_screen(int seconds) {
	for (int i = 0; i < seconds * 10; i++) {
		if (urgent_pending) {
			return;
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	oled_off();
}

static void show_message(const char *line1, const char *line2, const char *line3) {
	oled_on();
	oled_clear();
	oled_font_medium();
	oled_align_center();
	oled_draw_string(64, 24, line1);
	oled_font_small();
	if (line2) {
		oled_draw_string(64, 42, line2);
	}
	if (line3) {
		oled_draw_string(64, 58, line3);
	}
	oled_update();
	hold_screen(DISPLAY_TIMEOUT);
}

// Boot screen: firmware version and the settings that matter in the field.
static void show_splash(int freq_khz) {
	char buf[40];
	const esp_app_desc_t *app = esp_app_get_description();
	oled_on();
	oled_clear();
	oled_font_medium();
	oled_align_center();
	oled_draw_string(64, 13, "GNARL");
	oled_font_small();
	oled_draw_string(64, 26, app->version);
	snprintf(buf, sizeof(buf), "Pump: %d.%03d MHz", freq_khz / 1000, freq_khz % 1000);
	oled_draw_string(64, 38, buf);
#ifdef LORA_TX_PA_BOOST
	snprintf(buf, sizeof(buf), "TX: PA_BOOST %d dBm", LORA_TX_POWER_DBM);
#else
	snprintf(buf, sizeof(buf), "TX: RFO (upstream)");
#endif
	oled_draw_string(64, 50, buf);
	switch (ble_pairing_state()) {
	case PAIRING_LOCKED:
		oled_draw_string(64, 62, "Phone: paired");
		break;
	case PAIRING_OPEN:
		oled_draw_string(64, 62, "Phone: not paired yet");
		break;
	default:
		oled_draw_string(64, 62, "Phone lock: off");
		break;
	}
	oled_update();
	hold_screen(SPLASH_TIMEOUT);
}

static void format_time_ago(char *buf) {
	int now = esp_timer_get_time() / 1000000;
	int delta = now - command_time;
	int min = delta / 60;
	if (min == 0) {
		sprintf(buf, "%ds", delta);
		return;
	}
	if (min < 60) {
		sprintf(buf, "%dm", min);
		return;
	}
	int hr = min / 60;
	min = min % 60;
	sprintf(buf, "%dh%dm", hr, min);
}

static void format_battery(char *buf, int mv) {
	if (mv < 0) {
		strcpy(buf, "--");
		return;
	}
	sprintf(buf, "%d%% %d.%02dV", battery_percent(mv), mv / 1000, (mv % 1000) / 10);
}

static void show_battery_low(int mv) {
	char buf[20];
	format_battery(buf, mv);
	show_message("Battery low", "Turning off", buf);
}

static void update(display_command_t cmd) {
	switch (cmd.op) {
	case PHONE_RSSI:
		phone_rssi = cmd.arg;
		ESP_LOGD(TAG, "phone RSSI = %d", phone_rssi);
		return;
	case PUMP_RSSI:
		pump_rssi = cmd.arg;
		ESP_LOGD(TAG, "pump RSSI = %d", pump_rssi);
		return;
	case COMMAND_TIME:
		command_time = cmd.arg;
		ESP_LOGD(TAG, "command time = %d", command_time);
		return;
	case BATTERY:
		battery_mv = cmd.arg;
		ESP_LOGD(TAG, "battery = %d mV", battery_mv);
		return;
	case BATTERY_LOW:
		urgent_pending = false;
		show_battery_low(cmd.arg);
		return;
	case PHONE_UNPAIRED:
		urgent_pending = false;
		show_message("Phone unpaired", "Pair again from AAPS", NULL);
		return;
	case PHONE_REJECTED:
		urgent_pending = false;
		show_message("Unknown phone", "rejected", "Hold PRG 5 s to unpair");
		return;
	case SHOW_SPLASH:
		show_splash(cmd.arg);
		return;
	case CONNECTED:
		connected = cmd.arg;
		break;
	default:
		break;
	}
	oled_on();
	oled_clear();

	oled_font_medium();
	oled_align_center();
	oled_draw_string(64, 13, connected ? "Connected" : "Disconnected");

	oled_font_small();
	oled_align_left();
	oled_draw_string(5, 26, "Last command:");
	oled_draw_string(5, 38, "Phone RSSI:");
	oled_draw_string(5, 50, "Pump  RSSI:");
	oled_draw_string(5, 62, "Battery:");

	oled_align_right();
	char buf[20];
	format_time_ago(buf);
	oled_draw_string(122, 26, buf);
	if (connected) {
		sprintf(buf, "%d", phone_rssi);
		oled_draw_string(122, 38, buf);
		sprintf(buf, "%d", pump_rssi);
		oled_draw_string(122, 50, buf);
	} else {
		oled_draw_string(122, 38, "--");
		oled_draw_string(122, 50, "--");
	}
	format_battery(buf, battery_mv);
	oled_draw_string(122, 62, buf);

	oled_update();
	hold_screen(DISPLAY_TIMEOUT);
}

static void display_loop(void *unused) {
	for (;;) {
		display_command_t cmd;
		if (!xQueueReceive(display_queue, &cmd, pdMS_TO_TICKS(100))) {
			continue;
		}
		ESP_LOGD(TAG, "display_loop: op %d arg %d", cmd.op, cmd.arg);
		update(cmd);
	}
}

static void button_interrupt(void *unused) {
	display_command_t cmd = { .op = SHOW_STATUS };
	xQueueSendFromISR(display_queue, &cmd, 0);
	if (button_task_handle) {
		vTaskNotifyGiveFromISR(button_task_handle, 0);
	}
}

// Long press of PRG forgets the paired phone (BLE_LOCK_TO_PHONE).
static void button_task(void *unused) {
	for (;;) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
#ifdef BLE_LOCK_TO_PHONE
		int held = 0;
		while (gpio_get_level(BUTTON) == 0 && held < BLE_UNPAIR_HOLD_MS) {
			vTaskDelay(pdMS_TO_TICKS(100));
			held += 100;
		}
		if (held >= BLE_UNPAIR_HOLD_MS) {
			display_update(PHONE_UNPAIRED, 0);
			ble_forget_phone();
			// Wait for release so one long press acts only once.
			while (gpio_get_level(BUTTON) == 0) {
				vTaskDelay(pdMS_TO_TICKS(100));
			}
		}
#endif
		// Ignore bounces collected while the button was held.
		ulTaskNotifyTake(pdTRUE, 0);
	}
}

void display_update(display_op_t op, int arg) {
	display_command_t cmd = { .op = op, .arg = arg };
	if (op == BATTERY_LOW || op == PHONE_UNPAIRED || op == PHONE_REJECTED) {
		urgent_pending = true;
	}
	if (!xQueueSend(display_queue, &cmd, 0)) {
		ESP_LOGE(TAG, "display_update: queue full");
	}
}

void display_init(void) {
	oled_init();
	display_queue = xQueueCreate(QUEUE_LENGTH, sizeof(display_command_t));
	xTaskCreate(display_loop, "display", 3072, 0, 10, 0);
	xTaskCreate(button_task, "button", 2048, 0, 5, &button_task_handle);
	// The boot screen (SHOW_SPLASH) is requested by app_main once
	// Bluetooth is initialised, so that it can show the pairing state.

	// Enable interrupt on button press.
	gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
	gpio_set_intr_type(BUTTON, GPIO_INTR_NEGEDGE);
	gpio_install_isr_service(0);
	gpio_isr_handler_add(BUTTON, button_interrupt, 0);
	gpio_intr_enable(BUTTON);
}
