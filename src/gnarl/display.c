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
#include "screens.h"

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

#ifdef BUTTON
static TaskHandle_t button_task_handle;
#endif

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

static int seconds_since_command(void) {
	return esp_timer_get_time() / 1000000 - command_time;
}

static screen_pairing_t pairing_for_screen(void) {
	switch (ble_pairing_state()) {
	case PAIRING_LOCKED:
		return SCREEN_PAIRING_LOCKED;
	case PAIRING_OPEN:
		return SCREEN_PAIRING_OPEN;
	default:
		return SCREEN_PAIRING_OFF;
	}
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
		screen_battery_low(cmd.arg, battery_percent(cmd.arg));
		hold_screen(DISPLAY_TIMEOUT);
		return;
	case PHONE_UNPAIRED:
		urgent_pending = false;
		screen_phone_unpaired();
		hold_screen(DISPLAY_TIMEOUT);
		return;
	case PHONE_REJECTED:
		urgent_pending = false;
		screen_phone_rejected();
		hold_screen(DISPLAY_TIMEOUT);
		return;
	case SHOW_SPLASH:
		screen_splash(esp_app_get_description()->version, cmd.arg, pairing_for_screen());
		hold_screen(SPLASH_TIMEOUT);
		return;
	case CONNECTED:
		connected = cmd.arg;
		break;
	default:
		break;
	}
	screen_status(connected, seconds_since_command(), phone_rssi, pump_rssi,
		      battery_mv, battery_mv >= 0 ? battery_percent(battery_mv) : -1);
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

#ifdef BUTTON
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
#endif // BUTTON

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
#ifdef BUTTON
	xTaskCreate(button_task, "button", 2048, 0, 5, &button_task_handle);
#endif
	// The boot screen (SHOW_SPLASH) is requested by app_main once
	// Bluetooth is initialised, so that it can show the pairing state.

#ifdef BUTTON
	// Enable interrupt on button press.
	gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
	gpio_set_intr_type(BUTTON, GPIO_INTR_NEGEDGE);
	gpio_install_isr_service(0);
	gpio_isr_handler_add(BUTTON, button_interrupt, 0);
	gpio_intr_enable(BUTTON);
#endif
}
