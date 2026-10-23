#include <stdio.h>

#include <esp_app_desc.h>
#include <esp_pm.h>
#include <esp_task_wdt.h>

#include "display.h"
#include "gnarl.h"
#include "module.h"
#include "onboard_sx1262.h"
#include "rfm95.h"

// Lowest CPU frequency while idle: the 40 MHz crystal frequency.
#define MIN_CPU_FREQ_MHZ	40

static void power_init(void) {
#ifdef CONFIG_PM_ENABLE
#if CONFIG_IDF_TARGET_ESP32
	esp_pm_config_esp32_t pm = {
#elif CONFIG_IDF_TARGET_ESP32S3
	esp_pm_config_esp32s3_t pm = {
#else
#error "add the esp_pm_config type for this chip"
#endif
		.max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
		.min_freq_mhz = MIN_CPU_FREQ_MHZ,
		.light_sleep_enable = false,
	};
	esp_err_t err = esp_pm_configure(&pm);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp_pm_configure: %s", esp_err_to_name(err));
		return;
	}
	ESP_LOGI(TAG, "power management: CPU %d-%d MHz", MIN_CPU_FREQ_MHZ, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
#else
	ESP_LOGI(TAG, "power management disabled");
#endif
}

void app_main(void) {
#ifdef GNARL_DEBUG
	esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
	ESP_LOGI(TAG, "GNARL %s on %s (%s)", esp_app_get_description()->version,
		 BOARD_NAME, SUBG_RFSPY_VERSION);
	power_init();
	// Heltec V3.x: the on-board SX1262 is unused (no OOK); stop its standby current.
	onboard_sx1262_sleep();
	rfm95_init();
	// May not return: deep sleep if the battery is too low to run.
	power_boot_check();
	uint8_t v = read_version();
	ESP_LOGD(TAG, "radio version %d.%d", version_major(v), version_minor(v));
	set_frequency(PUMP_FREQUENCY);
	ESP_LOGD(TAG, "frequency set to %lu Hz", read_frequency());
	display_init();
	gnarl_init();
	display_update(SHOW_SPLASH, read_frequency() / 1000);
	start_power_monitor();
}
