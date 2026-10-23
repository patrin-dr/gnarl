#ifndef _BOARD_HELTEC_V2_H
#define _BOARD_HELTEC_V2_H

// Heltec WiFi LoRa 32 V2 / V2.1 (ESP32 + on-board SX1276, 868 MHz).
// Build with: make gnarl            (or make gnarl BOARD=heltec_v2)
// SPI, RESET, DIO0 and OLED wiring is identical to TTGO ESP32 LoRa OLED v1;
// DIO1 and DIO2 are NOT (see below).

#define BOARD_NAME	"Heltec V2"

// Board revision, printed on the board ("V2" or "V2.1").
// V2.1 moved battery sensing from GPIO13 to GPIO37.
// Comment out for a V2.0 board.
#define HELTEC_V2_1

#define BUTTON		GPIO_NUM_0
#define LED		GPIO_NUM_25	// TTGO v1: GPIO_NUM_2

// Vext powers the OLED and enables the battery voltage divider:
// LOW = on, HIGH = off.
#define VEXT		GPIO_NUM_21

#define OLED_SDA	GPIO_NUM_4
#define OLED_SCL	GPIO_NUM_15
#define OLED_RST	GPIO_NUM_16

#define LORA_SCK	GPIO_NUM_5
#define LORA_SDI	GPIO_NUM_19	// MISO
#define LORA_SDO	GPIO_NUM_27	// MOSI
#define LORA_CS		GPIO_NUM_18
#define LORA_RST	GPIO_NUM_14

// SX1276 digital I/O lines (Heltec V2 schematic): DIO0 = 26, DIO1 = 35,
// DIO2 = 34 (TTGO v1: 26 / 33 / 32). Only DIO2 (SyncMatch) is used.
// GPIO34/35 are input-only without pull resistors; the SX1276 drives its
// DIO lines push-pull, so no pull is needed.
#define LORA_DIO0	GPIO_NUM_26
#define LORA_DIO1	GPIO_NUM_35
#define LORA_DIO2	GPIO_NUM_34

// Battery voltage: 220k/100k divider, switched on by Vext (LOW).
// V2.1: GPIO37 = ADC1 channel 1.  V2.0: GPIO13 = ADC2 channel 4.
#ifdef HELTEC_V2_1
#define BATTERY_ADC_UNIT	ADC_UNIT_1
#define BATTERY_ADC_CHANNEL	ADC_CHANNEL_1
#else
#define BATTERY_ADC_UNIT	ADC_UNIT_2
#define BATTERY_ADC_CHANNEL	ADC_CHANNEL_4
#endif
#define BATTERY_ADC_ATTEN	ADC_ATTEN_DB_6	// ~150-1750 mV at the ADC pin
#define BATTERY_DIVIDER_X100	320		// (220k + 100k) / 100k = 3.20

#endif // _BOARD_HELTEC_V2_H
