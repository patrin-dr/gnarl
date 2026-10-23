#ifndef _GNARL_H
#define _GNARL_H

#include <stdint.h>

#include "pump_config.h"
#include "version.h"

#define TAG		"GNARL"

// Uncomment for verbose logging, including hex dumps of every radio
// and BLE packet. Useful when debugging with AAPS; costs power and
// adds latency in the radio task, so leave it off for daily use.
// #define GNARL_DEBUG

#ifdef GNARL_DEBUG
#define LOG_LOCAL_LEVEL	ESP_LOG_DEBUG
#else
#define LOG_LOCAL_LEVEL	ESP_LOG_INFO
#endif
#include <esp_log.h>

#define MILLISECONDS	1000
#define SECONDS		1000000

#define MHz		1000000

// Track github.com/ps2/rileylink/firmware/ble113_rfspy/gatt.xml
#define BLE_RFSPY_VERSION  "ble_rfspy 2.0"

#define STATE_OK "OK"

void gnarl_init(void);
void start_gnarl_task(void);
void rfspy_command(const uint8_t *buf, int count, int rssi);
void send_code(const uint8_t code);
void send_bytes(const uint8_t *buf, int count);
void print_bytes(const char* msg, const uint8_t *buf, int count);

// Update the BLE Battery Service level (0-100) and notify subscribers.
void ble_battery_update(uint8_t percent);

// Stop the Bluetooth host and controller (before deep sleep).
void ble_shutdown(void);

// Phone lock (BLE_LOCK_TO_PHONE in module.h).
typedef enum {
	PAIRING_DISABLED,	// lock compiled out: any device may connect
	PAIRING_OPEN,		// lock on, no phone paired yet
	PAIRING_LOCKED,		// lock on, one phone paired
} ble_pairing_state_t;

ble_pairing_state_t ble_pairing_state(void);

// Forget the paired phone: the next phone that pairs becomes the owner.
void ble_forget_phone(void);

// Low-battery check at boot: returns only if the battery is good enough
// to run (or sensing is unavailable); otherwise enters deep sleep.
void power_boot_check(void);

// Start the periodic battery monitor (display, BLE level, cutoff).
void start_power_monitor(void);

#endif // _GNARL_H
