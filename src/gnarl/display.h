#ifndef _DISPLAY_H
#define _DISPLAY_H

typedef enum {
	CONNECTED,
	PHONE_RSSI,
	PUMP_RSSI,
	COMMAND_TIME,
	SHOW_STATUS,
	BATTERY,	// arg = battery voltage in mV, or -1 if unknown
	BATTERY_LOW,	// arg = battery voltage in mV; shown before shutdown
	SHOW_SPLASH,	// arg = radio frequency in kHz; version screen at boot
	PHONE_UNPAIRED,	// shown after a long press of PRG forgot the phone
	PHONE_REJECTED,	// an unknown device tried to pair and was dropped
} display_op_t;

void display_init(void);

void display_update(display_op_t op, int arg);

#endif // _DISPLAY_H
