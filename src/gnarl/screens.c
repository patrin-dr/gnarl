#include "screens.h"

#include <stdio.h>
#include <string.h>

#include "module.h"
#include "oled.h"
#include "display_text.h"

#define LEFT	5
#define RIGHT	122
#define CENTER	64

static void format_age(char *buf, size_t size, int age_s) {
	if (age_s < 0) {
		age_s = 0;
	}
	int min = age_s / 60;
	if (min == 0) {
		snprintf(buf, size, "%d" S_UNIT_SEC, age_s);
	} else if (min < 60) {
		snprintf(buf, size, "%d" S_UNIT_MIN, min);
	} else {
		snprintf(buf, size, "%d" S_UNIT_HOUR "%d" S_UNIT_MIN, min / 60, min % 60);
	}
}

static void format_battery(char *buf, size_t size, int mv, int pct) {
	if (mv < 0) {
		snprintf(buf, size, "--");
		return;
	}
	snprintf(buf, size, "%d%% %d" S_DECIMAL "%02d" S_VOLT, pct, mv / 1000, (mv % 1000) / 10);
}

static void message(const char *line1, const char *line2, const char *line3) {
	oled_on();
	oled_clear();
	oled_font_medium();
	oled_align_center();
	oled_draw_string(CENTER, 24, line1);
	oled_font_small();
	if (line2) {
		oled_draw_string(CENTER, 42, line2);
	}
	if (line3) {
		oled_draw_string(CENTER, 58, line3);
	}
	oled_update();
}

void screen_status(bool connected, int age_s, int phone_rssi, int pump_rssi,
		   int battery_mv, int battery_pct) {
	char buf[24];
	oled_on();
	oled_clear();

	oled_font_medium();
	oled_align_center();
	oled_draw_string(CENTER, 13, connected ? S_CONNECTED : S_DISCONNECTED);

	oled_font_small();
	oled_align_left();
	oled_draw_string(LEFT, 26, S_LAST_COMMAND);
	oled_draw_string(LEFT, 38, S_PHONE_RSSI);
	oled_draw_string(LEFT, 50, S_PUMP_RSSI);
	oled_draw_string(LEFT, 62, S_BATTERY);

	oled_align_right();
	format_age(buf, sizeof(buf), age_s);
	oled_draw_string(RIGHT, 26, buf);
	if (connected) {
		snprintf(buf, sizeof(buf), "%d", phone_rssi);
		oled_draw_string(RIGHT, 38, buf);
		snprintf(buf, sizeof(buf), "%d", pump_rssi);
		oled_draw_string(RIGHT, 50, buf);
	} else {
		oled_draw_string(RIGHT, 38, "--");
		oled_draw_string(RIGHT, 50, "--");
	}
	format_battery(buf, sizeof(buf), battery_mv, battery_pct);
	oled_draw_string(RIGHT, 62, buf);

	oled_update();
}

void screen_splash(const char *version, int freq_khz, screen_pairing_t pairing) {
	char buf[40];
	oled_on();
	oled_clear();
	oled_font_medium();
	oled_align_center();
	oled_draw_string(CENTER, 13, "GNARL");
	oled_font_small();
	oled_draw_string(CENTER, 26, version);
	snprintf(buf, sizeof(buf), S_PUMP_FREQ, freq_khz / 1000, freq_khz % 1000);
	oled_draw_string(CENTER, 38, buf);
#ifdef LORA_TX_PA_BOOST
	snprintf(buf, sizeof(buf), "TX: PA_BOOST %d dBm", LORA_TX_POWER_DBM);
#else
	snprintf(buf, sizeof(buf), S_TX_RFO);
#endif
	oled_draw_string(CENTER, 50, buf);
	switch (pairing) {
	case SCREEN_PAIRING_LOCKED:
		oled_draw_string(CENTER, 62, S_PHONE_PAIRED);
		break;
	case SCREEN_PAIRING_OPEN:
		oled_draw_string(CENTER, 62, S_PHONE_NOT_PAIRED);
		break;
	default:
		oled_draw_string(CENTER, 62, S_PHONE_LOCK_OFF);
		break;
	}
	oled_update();
}

void screen_battery_low(int battery_mv, int battery_pct) {
	char buf[24];
	format_battery(buf, sizeof(buf), battery_mv, battery_pct);
	message(S_BATTERY_LOW, S_TURNING_OFF, buf);
}

void screen_phone_unpaired(void) {
	message(S_PHONE_UNPAIRED, S_PAIR_AGAIN, NULL);
}

void screen_phone_rejected(void) {
	message(S_UNKNOWN_PHONE, S_REJECTED, S_HOLD_TO_UNPAIR);
}
