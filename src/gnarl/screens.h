#ifndef _SCREENS_H
#define _SCREENS_H

#include <stdbool.h>

// Drawing of every GNARL screen. Pure functions of their arguments with
// no ESP-IDF dependencies, so they can also be rendered on a computer
// (tools/screen_preview) to check that the text fits.
// Each function draws a complete screen and leaves the display on.

typedef enum {
	SCREEN_PAIRING_OFF,	// phone lock compiled out
	SCREEN_PAIRING_OPEN,	// no phone paired yet
	SCREEN_PAIRING_LOCKED,	// one phone paired
} screen_pairing_t;

// age_s: seconds since the last command; rssi values are ignored when
// !connected; battery_mv < 0 means unknown.
void screen_status(bool connected, int age_s, int phone_rssi, int pump_rssi,
		   int battery_mv, int battery_pct);

void screen_splash(const char *version, int freq_khz, screen_pairing_t pairing);

void screen_battery_low(int battery_mv, int battery_pct);
void screen_phone_unpaired(void);
void screen_phone_rejected(void);

#endif // _SCREENS_H
