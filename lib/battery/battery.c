#include "battery.h"

#include <stdbool.h>

#include <driver/gpio.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_log.h>
#include <esp_rom_sys.h>

#include "module.h"

#define TAG		"battery"

#define NUM_SAMPLES	16

#ifdef BATTERY_ADC_UNIT

// Approximate full-scale input per attenuation, used only without calibration.
static int uncalibrated_full_scale_mv(void) {
	switch (BATTERY_ADC_ATTEN) {
	case ADC_ATTEN_DB_0:
		return 950;
	case ADC_ATTEN_DB_2_5:
		return 1250;
	case ADC_ATTEN_DB_6:
		return 1750;
	default:
		return 3100;
	}
}
#define ADC_MAX_RAW			4095

static adc_oneshot_unit_handle_t adc;
static adc_cali_handle_t cali;
static bool ready;

void battery_init(void) {
	if (ready) {
		return;
	}
#ifdef VEXT
	// The divider is switched by the same Vext line that powers the OLED.
	gpio_set_level(VEXT, 0);
	gpio_config_t vext = {
		.pin_bit_mask = 1ULL << VEXT,
		.mode         = GPIO_MODE_OUTPUT,
		.pull_up_en   = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type    = GPIO_INTR_DISABLE,
	};
	gpio_config(&vext);
	gpio_set_level(VEXT, 0);
#endif
#ifdef BATTERY_CTRL
	// Separate divider switch (Heltec V3.x: ADC_Ctrl, GPIO37).
	int active = 1;
#ifdef BATTERY_CTRL_AUTO_POLARITY
	// V3/V3.1 pull the pin up and switch on LOW; V3.2 has a transistor
	// switched on HIGH and no pull-up. Read the idle level to tell.
	gpio_reset_pin(BATTERY_CTRL);
	gpio_set_direction(BATTERY_CTRL, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BATTERY_CTRL, GPIO_FLOATING);
	esp_rom_delay_us(100);
	active = gpio_get_level(BATTERY_CTRL) ? 0 : 1;
	ESP_LOGI(TAG, "battery divider control is active %s (%s)",
		 active ? "HIGH" : "LOW", active ? "V3.2" : "V3/V3.1");
#endif
	gpio_set_direction(BATTERY_CTRL, GPIO_MODE_OUTPUT);
	gpio_set_level(BATTERY_CTRL, active);
#endif
	adc_oneshot_unit_init_cfg_t unit_cfg = {
		.unit_id = BATTERY_ADC_UNIT,
	};
	esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &adc);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "adc_oneshot_new_unit: %s", esp_err_to_name(err));
		return;
	}
	adc_oneshot_chan_cfg_t chan_cfg = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		.atten    = BATTERY_ADC_ATTEN,
	};
	err = adc_oneshot_config_channel(adc, BATTERY_ADC_CHANNEL, &chan_cfg);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "adc_oneshot_config_channel: %s", esp_err_to_name(err));
		return;
	}
	cali = NULL;
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
	// ESP32-S3 and newer chips.
	adc_cali_curve_fitting_config_t cali_cfg = {
		.unit_id  = BATTERY_ADC_UNIT,
		.atten    = BATTERY_ADC_ATTEN,
		.bitwidth = ADC_BITWIDTH_DEFAULT,
	};
	err = adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali);
	if (err != ESP_OK) {
		ESP_LOGW(TAG, "no ADC calibration (%s); readings are approximate", esp_err_to_name(err));
		cali = NULL;
	}
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
	adc_cali_line_fitting_config_t cali_cfg = {
		.unit_id  = BATTERY_ADC_UNIT,
		.atten    = BATTERY_ADC_ATTEN,
		.bitwidth = ADC_BITWIDTH_DEFAULT,
#if CONFIG_IDF_TARGET_ESP32
		.default_vref = 1100,
#endif
	};
	err = adc_cali_create_scheme_line_fitting(&cali_cfg, &cali);
	if (err != ESP_OK) {
		ESP_LOGW(TAG, "no ADC calibration (%s); readings are approximate", esp_err_to_name(err));
		cali = NULL;
	}
#endif
	ready = true;
	ESP_LOGI(TAG, "battery sensing ready (%s)", cali ? "calibrated" : "uncalibrated");
}

int battery_millivolts(void) {
	if (!ready) {
		return -1;
	}
	int sum = 0;
	int n = 0;
	for (int i = 0; i < NUM_SAMPLES; i++) {
		int raw;
		if (adc_oneshot_read(adc, BATTERY_ADC_CHANNEL, &raw) == ESP_OK) {
			sum += raw;
			n++;
		}
	}
	if (n == 0) {
		return -1;
	}
	int raw = sum / n;
	int mv;
	if (cali != NULL) {
		if (adc_cali_raw_to_voltage(cali, raw, &mv) != ESP_OK) {
			return -1;
		}
	} else {
		mv = raw * uncalibrated_full_scale_mv() / ADC_MAX_RAW;
	}
	return mv * BATTERY_DIVIDER_X100 / 100 * BATTERY_CALIBRATION_PCT / 100;
}

#else // BATTERY_ADC_UNIT

void battery_init(void) {
}

int battery_millivolts(void) {
	return -1;
}

#endif // BATTERY_ADC_UNIT

// Typical resting voltage of a LiPo cell versus remaining capacity.
static const struct {
	int mV;
	int pct;
} lipo_curve[] = {
	{ 4200, 100 },
	{ 4100,  90 },
	{ 4000,  78 },
	{ 3900,  62 },
	{ 3800,  45 },
	{ 3750,  35 },
	{ 3700,  22 },
	{ 3650,  12 },
	{ 3600,   6 },
	{ 3500,   2 },
	{ 3300,   0 },
};

#define CURVE_LEN	(sizeof(lipo_curve) / sizeof(lipo_curve[0]))

int battery_percent(int mV) {
	if (mV < 0) {
		return -1;
	}
	if (mV >= lipo_curve[0].mV) {
		return 100;
	}
	for (int i = 1; i < CURVE_LEN; i++) {
		if (mV >= lipo_curve[i].mV) {
			int v0 = lipo_curve[i].mV, v1 = lipo_curve[i-1].mV;
			int p0 = lipo_curve[i].pct, p1 = lipo_curve[i-1].pct;
			return p0 + (mV - v0) * (p1 - p0) / (v1 - v0);
		}
	}
	return 0;
}
