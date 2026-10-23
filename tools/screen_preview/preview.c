// Renders every GNARL screen on a computer, using the real u8g2 library,
// lib/oled/oled.c and src/gnarl/screens.c, and writes PBM images.
// Build and run with tools/screen_preview/run.sh.

#include <stdio.h>
#include <string.h>

#include "../../lib/oled/oled.c"
#include "screens.h"
#include "display_text.h"

// The real callbacks talk to the I2C bus; on a computer do nothing.
uint8_t i2c_callback(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	return 1;
}
uint8_t gpio_and_delay_callback(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	return 1;
}

static int problems;

static void save(const char *name) {
	char path[256];
	snprintf(path, sizeof(path), "%s.pbm", name);
	FILE *f = fopen(path, "w");
	uint8_t *buf = u8g2_GetBufferPtr(&u8g2);
	fprintf(f, "P1\n%d %d\n", OLED_WIDTH, OLED_HEIGHT);
	int edge = 0;
	for (int y = 0; y < OLED_HEIGHT; y++) {
		for (int x = 0; x < OLED_WIDTH; x++) {
			int on = (buf[(y / 8) * OLED_WIDTH + x] >> (y % 8)) & 1;
			if (on && (x == 0 || x == OLED_WIDTH - 1)) {
				edge = 1;
			}
			fputc(on ? '1' : '0', f);
		}
		fputc('\n', f);
	}
	fclose(f);
	printf("%-22s %s\n", name, edge ? "ERROR: text touches the screen edge" : "ok");
	problems += edge;
}

static void width(const char *what, int size, const char *s) {
	if (size == 0) oled_font_small(); else oled_font_medium();
	int w = oled_string_width(s);
	// 128 px screen; leave at least 2 px on each side of a centred line.
	int too_wide = w > OLED_WIDTH - 4;
	printf("  %-6s %3d px %s  %s\n", size ? "medium" : "small", w, too_wide ? "TOO WIDE" : "        ", s);
	problems += too_wide;
}

int main(void) {
	oled_init();
	oled_font_sans_serif();

	printf("Text widths (limit 124 px):\n");
	width("", 1, S_CONNECTED);
	width("", 1, S_DISCONNECTED);
	width("", 1, S_BATTERY_LOW);
	width("", 1, S_PHONE_UNPAIRED);
	width("", 1, S_UNKNOWN_PHONE);
	width("", 0, S_PHONE_PAIRED);
	width("", 0, S_PHONE_NOT_PAIRED);
	width("", 0, S_PHONE_LOCK_OFF);
	width("", 0, S_PAIR_AGAIN);
	width("", 0, S_HOLD_TO_UNPAIR);
	width("", 0, S_TX_RFO);
	width("", 0, "TX: PA_BOOST 14 dBm");
	printf("\nScreens:\n");

	screen_splash("v0.1.0-3-gabc1234", 868350, SCREEN_PAIRING_OPEN);
	save("1_splash");
	screen_splash("v0.1.0", 868350, SCREEN_PAIRING_LOCKED);
	save("2_splash_paired");
	screen_status(true, 125, -58, -71, 3950, 74);
	save("3_status");
	screen_status(false, 46740, 0, 0, 3712, 24);
	save("4_status_disconnected");
	screen_battery_low(3290, 0);
	save("5_battery_low");
	screen_phone_unpaired();
	save("6_phone_unpaired");
	screen_phone_rejected();
	save("7_phone_rejected");
	if (problems) {
		printf("\n%d problem(s): shorten the text in src/gnarl/display_text.h\n", problems);
	}
	return problems ? 1 : 0;
}
