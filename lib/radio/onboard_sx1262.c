// Put an unused on-board SX1262 (Heltec V3.x) into sleep.
//
// After power-up the SX1262 stays in STDBY_RC (about 0.6 mA). GNARL uses an
// external SX1276 instead, so send SetSleep (opcode 0x84) once at start-up.
// Bit-banged on the SX1262's own pins: two bytes, no driver needed.

#include "onboard_sx1262.h"

#include <stdbool.h>

#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_rom_sys.h>

#include "module.h"

#define TAG	"sx1262"

#ifdef ONBOARD_SX1262_NSS

#define SX126X_SET_SLEEP	0x84
#define SLEEP_COLD_START	0x00	// no RTC wake-up, configuration not kept

static bool wait_not_busy(int timeout_us) {
	while (gpio_get_level(ONBOARD_SX1262_BUSY) && timeout_us > 0) {
		esp_rom_delay_us(10);
		timeout_us -= 10;
	}
	return timeout_us > 0;
}

static void spi_byte(uint8_t b) {
	// SPI mode 0, MSB first.
	for (int i = 7; i >= 0; i--) {
		gpio_set_level(ONBOARD_SX1262_MOSI, (b >> i) & 1);
		esp_rom_delay_us(1);
		gpio_set_level(ONBOARD_SX1262_SCK, 1);
		esp_rom_delay_us(1);
		gpio_set_level(ONBOARD_SX1262_SCK, 0);
	}
}

void onboard_sx1262_sleep(void) {
	gpio_config_t out = {
		.pin_bit_mask = (1ULL << ONBOARD_SX1262_NSS) | (1ULL << ONBOARD_SX1262_SCK) |
				(1ULL << ONBOARD_SX1262_MOSI) | (1ULL << ONBOARD_SX1262_RST),
		.mode = GPIO_MODE_OUTPUT,
	};
	gpio_config(&out);
	gpio_config_t in = {
		.pin_bit_mask = (1ULL << ONBOARD_SX1262_BUSY) | (1ULL << ONBOARD_SX1262_MISO),
		.mode = GPIO_MODE_INPUT,
	};
	gpio_config(&in);
	gpio_set_level(ONBOARD_SX1262_NSS, 1);
	gpio_set_level(ONBOARD_SX1262_SCK, 0);

	// Reset, then wait until the chip is ready for commands.
	gpio_set_level(ONBOARD_SX1262_RST, 0);
	esp_rom_delay_us(2000);
	gpio_set_level(ONBOARD_SX1262_RST, 1);
	esp_rom_delay_us(5000);
	if (!wait_not_busy(100000)) {
		ESP_LOGW(TAG, "on-board SX1262 stays busy; not put to sleep");
		return;
	}

	gpio_set_level(ONBOARD_SX1262_NSS, 0);
	esp_rom_delay_us(1);
	spi_byte(SX126X_SET_SLEEP);
	spi_byte(SLEEP_COLD_START);
	gpio_set_level(ONBOARD_SX1262_NSS, 1);

	// Keep NSS high (a falling edge would wake the chip), also in deep sleep.
	gpio_hold_en(ONBOARD_SX1262_NSS);
	ESP_LOGI(TAG, "on-board SX1262 put to sleep");
}

#else

void onboard_sx1262_sleep(void) {
}

#endif
