#ifndef _BOARD_LILYGO_T3_V161_H
#define _BOARD_LILYGO_T3_V161_H

// LilyGO (TTGO) LoRa32 T3 V1.6.1, 868 MHz (ESP32 + on-board SX1276).
// Build with: make gnarl BOARD=lilygo_t3_v161
// Pins per the ESPHome device page for this board; same ESP32 and radio as
// Heltec V2, no extra wiring.

#define BOARD_NAME	"LilyGO T3 V1.6.1"

#define BUTTON		GPIO_NUM_0
#define LED		GPIO_NUM_25

// No Vext switch: the OLED is always powered.

#define OLED_SDA	GPIO_NUM_21
#define OLED_SCL	GPIO_NUM_22
#define OLED_RST	GPIO_NUM_16

#define LORA_SCK	GPIO_NUM_5
#define LORA_SDI	GPIO_NUM_19	// MISO
#define LORA_SDO	GPIO_NUM_27	// MOSI
#define LORA_CS		GPIO_NUM_18
#define LORA_RST	GPIO_NUM_23
#define LORA_DIO0	GPIO_NUM_26
#define LORA_DIO1	GPIO_NUM_33
#define LORA_DIO2	GPIO_NUM_32

// Battery: GPIO35 (ADC1 channel 7) through a 1:2 divider, always on.
#define BATTERY_ADC_UNIT	ADC_UNIT_1
#define BATTERY_ADC_CHANNEL	ADC_CHANNEL_7
#define BATTERY_ADC_ATTEN	ADC_ATTEN_DB_11	// up to ~3100 mV at the pin
#define BATTERY_DIVIDER_X100	200

#endif // _BOARD_LILYGO_T3_V161_H
