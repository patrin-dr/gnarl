#ifndef _MODULE_H
#define _MODULE_H

// Configuration for Heltec WiFi LoRa 32 V2 (868 MHz) module
// (SX1276 and OLED wiring is identical to TTGO ESP32 LoRa OLED v1)

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

// Comment out the following line for a module without OLED
#define OLED_ENABLE

#endif // _MODULE_H
