#ifndef _BOARD_LILYGO_T3_V161_H
#define _BOARD_LILYGO_T3_V161_H

// LilyGO (TTGO) LoRa32 T3 V1.6.1, 868 MHz (ESP32-PICO-D4 + on-board SX1276).
// Build with: make gnarl BOARD=lilygo_t3_v161
// Pins from LilyGO's own definitions (LilyGo-LoRa-Series, utilities.h,
// T3_V1_6_SX1276) and the T3_V1.6.1 schematic.
//
// Differences from Heltec boards:
// - No user button: only RST (reboot) and a battery power switch.
//   GPIO0 is wired to the USB auto-reset circuit, not to a button.
//   Without BUTTON the firmware shows the status screen on events only,
//   and the paired phone is forgotten by pressing RST 3 times in a row
//   (see BLE_UNPAIR_RESETS in module.h).
// - The OLED has no reset line and is always powered (no Vext).
// - GPIO16/17 belong to the ESP32-PICO-D4's internal flash: never use them.
// - Battery connector: JST PH 2.0 mm, pin 1 GND, pin 2 VBAT.

#define BOARD_NAME	"LilyGO T3 V1.6.1"

#define LED		GPIO_NUM_25	// green, active HIGH

#define OLED_SDA	GPIO_NUM_21
#define OLED_SCL	GPIO_NUM_22

#define LORA_SCK	GPIO_NUM_5
#define LORA_SDI	GPIO_NUM_19	// MISO
#define LORA_SDO	GPIO_NUM_27	// MOSI
#define LORA_CS		GPIO_NUM_18
#define LORA_RST	GPIO_NUM_23
#define LORA_DIO0	GPIO_NUM_26
#define LORA_DIO1	GPIO_NUM_33
#define LORA_DIO2	GPIO_NUM_32

// Battery: GPIO35 (ADC1 channel 7) through a 100k/100k divider, always on.
#define BATTERY_ADC_UNIT	ADC_UNIT_1
#define BATTERY_ADC_CHANNEL	ADC_CHANNEL_7
#define BATTERY_ADC_ATTEN	ADC_ATTEN_DB_11	// up to ~3100 mV at the pin
#define BATTERY_DIVIDER_X100	200

#endif // _BOARD_LILYGO_T3_V161_H
