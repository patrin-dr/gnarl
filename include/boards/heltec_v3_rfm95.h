#ifndef _BOARD_HELTEC_V3_RFM95_H
#define _BOARD_HELTEC_V3_RFM95_H

// Heltec WiFi LoRa 32 V3 / V3.1 / V3.2 (ESP32-S3) with an EXTERNAL
// RFM95W 868 MHz module (SX1276).
// Build with: make gnarl BOARD=heltec_v3_rfm95
//
// The on-board SX1262 cannot do OOK, which Medtronic pumps use, so it is
// put to sleep at start-up and an RFM95W is wired to free header pins:
//
//   RFM95W   Heltec V3 (label printed on the board)
//   3.3V     3V3
//   GND      GND
//   SCK      GPIO5
//   MISO     GPIO7
//   MOSI     GPIO6
//   NSS/CS   GPIO4
//   RESET    GPIO2
//   DIO2     GPIO38      (DIO0 and DIO1 are not needed)
//
// These are the only free header pins without a boot (strapping) role,
// besides GPIO26/33/34 on the other header row.

#define BOARD_NAME	"Heltec V3 + RFM95W"

#define BUTTON		GPIO_NUM_0
#define LED		GPIO_NUM_35

// Vext powers the OLED: LOW = on, HIGH = off.
#define VEXT		GPIO_NUM_36

#define OLED_SDA	GPIO_NUM_17
#define OLED_SCL	GPIO_NUM_18
#define OLED_RST	GPIO_NUM_21

// External RFM95W (SX1276).
#define LORA_SCK	GPIO_NUM_5
#define LORA_SDI	GPIO_NUM_7	// MISO
#define LORA_SDO	GPIO_NUM_6	// MOSI
#define LORA_CS		GPIO_NUM_4
#define LORA_RST	GPIO_NUM_2
#define LORA_DIO0	GPIO_NUM_NC	// not connected, not used
#define LORA_DIO1	GPIO_NUM_NC	// not connected, not used
#define LORA_DIO2	GPIO_NUM_38

// On-board SX1262, only to put it into sleep (it stays in standby,
// ~0.6 mA, after power-up otherwise).
#define ONBOARD_SX1262_NSS	GPIO_NUM_8
#define ONBOARD_SX1262_SCK	GPIO_NUM_9
#define ONBOARD_SX1262_MOSI	GPIO_NUM_10
#define ONBOARD_SX1262_MISO	GPIO_NUM_11
#define ONBOARD_SX1262_RST	GPIO_NUM_12
#define ONBOARD_SX1262_BUSY	GPIO_NUM_13

// Battery voltage: GPIO1 (ADC1 channel 0) through a 390k/100k divider,
// enabled by ADC_Ctrl (GPIO37). V3/V3.1: ADC_Ctrl has a pull-up and is
// active LOW; V3.2: transistor, active HIGH. The firmware reads the pin
// at start-up to tell them apart.
#define BATTERY_ADC_UNIT	ADC_UNIT_1
#define BATTERY_ADC_CHANNEL	ADC_CHANNEL_0
#define BATTERY_ADC_ATTEN	ADC_ATTEN_DB_2_5	// 0-1250 mV at the pin
#define BATTERY_DIVIDER_X100	490			// (390k + 100k) / 100k
#define BATTERY_CTRL		GPIO_NUM_37
#define BATTERY_CTRL_AUTO_POLARITY

#endif // _BOARD_HELTEC_V3_RFM95_H
