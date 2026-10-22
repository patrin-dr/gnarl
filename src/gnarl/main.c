#include <stdio.h>

#include <esp_pm.h>
#include <esp_task_wdt.h>

#include "display.h"
#include "gnarl.h"
#include "rfm95.h"

// Lowest CPU frequency while idle: the 40 MHz crystal frequency.
#define MIN_CPU_FREQ_MHZ	40

static void power_init(void) {
#ifdef CONFIG_PM_ENABLE
	esp_pm_config_esp32_t pm = {
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
	ESP_LOGI(TAG, "%s", SUBG_RFSPY_VERSION);
	power_init();
	rfm95_init();
	uint8_t v = read_version();
	ESP_LOGD(TAG, "radio version %d.%d", version_major(v), version_minor(v));
	set_frequency(PUMP_FREQUENCY);
	ESP_LOGD(TAG, "frequency set to %lu Hz", read_frequency());
	display_init();
	gnarl_init();
}
