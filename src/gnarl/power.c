#include "gnarl.h"

#include <stdbool.h>

#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_attr.h>
#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "battery.h"
#include "display.h"
#include "module.h"
#include "rfm95.h"

#define CHECK_INTERVAL_MS	(60 * 1000)

// Consecutive low readings required before shutting down.
#define CUTOFF_READINGS		3

// Readings below this mean no battery (USB only) or a failed measurement;
// never shut down because of them.
#define MIN_VALID_MV		2500

// While asleep for low battery, wake up this often to re-check
// (e.g. after USB power has been connected).
#define RECHECK_SLEEP_US	(30ULL * 60 * 1000000)

// Survives deep sleep: set when we slept because of a low battery,
// so that the higher resume threshold applies on wake-up.
static RTC_DATA_ATTR bool low_battery_sleep;

// at_boot: called from power_boot_check(), before Bluetooth is started
// and while no radio command can be running.
static void enter_low_battery_sleep(bool at_boot) {
	if (at_boot) {
		rfm95_sleep();
	} else {
		// ESP-IDF requires Bluetooth to be disabled before deep sleep.
		// The radio is already in Sleep after every command.
		ble_shutdown();
	}
#ifdef VEXT
	// Turn off the OLED and the battery divider while asleep.
	gpio_set_level(VEXT, 1);
	gpio_hold_en(VEXT);
	gpio_deep_sleep_hold_en();
#endif
#ifdef BUTTON
	// Wake on PRG button (active low); keep the pin pulled up while asleep.
	esp_sleep_enable_ext0_wakeup(BUTTON, 0);
	rtc_gpio_pullup_en(BUTTON);
	rtc_gpio_pulldown_dis(BUTTON);
#endif
	// Boards without a user button wake on the timer or on RST.
	esp_sleep_enable_timer_wakeup(RECHECK_SLEEP_US);
	low_battery_sleep = true;
	ESP_LOGW(TAG, "entering deep sleep to protect the battery");
	esp_deep_sleep_start();
}

void power_boot_check(void) {
#ifdef BUTTON
	// ext0 wake-up leaves the button pin routed to the RTC subsystem;
	// return it to the digital GPIO matrix so its interrupt works again.
	rtc_gpio_deinit(BUTTON);
#endif
#ifdef VEXT
	// Release the hold applied before a low-battery deep sleep.
	gpio_hold_dis(VEXT);
	gpio_deep_sleep_hold_dis();
#endif
	battery_init();
	// Let the divider settle after Vext is switched on.
	vTaskDelay(pdMS_TO_TICKS(20));
	int mv = battery_millivolts();
	ESP_LOGI(TAG, "battery at boot: %d mV", mv);
#ifdef BATTERY_CUTOFF_ENABLE
	int threshold = low_battery_sleep ? BATTERY_RESUME_MV : BATTERY_CUTOFF_MV;
	if (mv >= MIN_VALID_MV && mv < threshold) {
		ESP_LOGW(TAG, "battery %d mV below %d mV", mv, threshold);
		enter_low_battery_sleep(true);
	}
#endif
	low_battery_sleep = false;
}

static void power_monitor(void *unused) {
	int low_count = 0;
	for (;;) {
		int mv = battery_millivolts();
		if (mv >= 0) {
			int pct = battery_percent(mv);
			ESP_LOGI(TAG, "battery %d mV (%d%%)", mv, pct);
			ble_battery_update(pct);
		}
		display_update(BATTERY, mv);
#ifdef BATTERY_CUTOFF_ENABLE
		if (mv >= MIN_VALID_MV && mv < BATTERY_CUTOFF_MV) {
			low_count++;
			ESP_LOGW(TAG, "low battery reading %d of %d", low_count, CUTOFF_READINGS);
			if (low_count >= CUTOFF_READINGS) {
				display_update(BATTERY_LOW, mv);
				// Give the display time to show the warning.
				vTaskDelay(pdMS_TO_TICKS(6000));
				enter_low_battery_sleep(false);
			}
		} else {
			low_count = 0;
		}
#endif
		vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
	}
}

void start_power_monitor(void) {
	xTaskCreate(power_monitor, "power", 3072, 0, 5, 0);
}
