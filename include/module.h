#ifndef _MODULE_H
#define _MODULE_H

// Configuration for Heltec WiFi LoRa 32 V2 (868 MHz) module.
// SPI, RESET, DIO0 and OLED wiring is identical to TTGO ESP32 LoRa OLED v1;
// DIO1 and DIO2 are NOT (see below).

#define BUTTON		GPIO_NUM_0
#define LED		GPIO_NUM_25	// TTGO v1: GPIO_NUM_2

// Vext powers the OLED on Heltec boards: LOW = on, HIGH = off.
// Comment out for TTGO v1.
#define VEXT		GPIO_NUM_21

#define OLED_SDA	GPIO_NUM_4
#define OLED_SCL	GPIO_NUM_15
#define OLED_RST	GPIO_NUM_16

#define LORA_SCK	GPIO_NUM_5
#define LORA_SDI	GPIO_NUM_19
#define LORA_SDO	GPIO_NUM_27
#define LORA_CS		GPIO_NUM_18
#define LORA_RST	GPIO_NUM_14
#define LORA_IRQ	GPIO_NUM_26

// SX1276 digital I/O lines.
// Heltec V2 (per Heltec V2 schematic): DIO0 = 26, DIO1 = 35, DIO2 = 34.
// TTGO v1:                             DIO0 = 26, DIO1 = 33, DIO2 = 32.
// GPIO34/35 are input-only without internal pull resistors; the SX1276
// drives its DIO lines push-pull, so no pull is needed.
#define LORA_DIO0	GPIO_NUM_26
#define LORA_DIO1	GPIO_NUM_35
#define LORA_DIO2	GPIO_NUM_34

// Comment out the following line for a module without OLED
#define OLED_ENABLE

#endif // _MODULE_H
