#ifndef _MODULE_H
#define _MODULE_H

// Configuration for Heltec WiFi LoRa 32 V2 (868 MHz) module.
// SPI, RESET, DIO0 and OLED wiring is identical to TTGO ESP32 LoRa OLED v1;
// DIO1 and DIO2 are NOT (see below).

// Board revision. The revision is printed on the board ("V2" or "V2.1").
// V2.1 moved battery sensing from GPIO13 to GPIO37.
// Comment out for a V2.0 board.
#define HELTEC_V2_1

#define BUTTON		GPIO_NUM_0
#define LED		GPIO_NUM_25	// TTGO v1: GPIO_NUM_2

// Vext powers the OLED and enables the battery voltage divider on Heltec
// boards: LOW = on, HIGH = off. Comment out for TTGO v1.
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

// SX1276 transmit amplifier.
// The chip has two outputs: RFO (chip default, up to +14 dBm) and PA_BOOST
// (+2..+17 dBm). RFM95 modules, and most Heltec/TTGO boards, connect only
// PA_BOOST to the antenna, so transmitting through RFO leaves only leakage.
// Upstream GNARL never selected an output, i.e. it used RFO.
// Comment out LORA_TX_PA_BOOST to get the upstream behaviour back
// (useful to compare range with mmtune / pumpstat).
#define LORA_TX_PA_BOOST
// Output power on PA_BOOST in dBm, 2..17.
// 10 dBm = maximum of the CC1110 in a real RileyLink.
// 14 dBm (25 mW) is the limit for the 868 MHz short-range-device band.
#define LORA_TX_POWER_DBM	10

// Bluetooth security: accept only one phone.
// The first phone that pairs becomes the owner (stored in flash); other
// devices can still connect but cannot read or write the RileyLink
// characteristics (they require an encrypted, bonded link) and are
// disconnected. Android shows a one-time "Pair with GNARL?" prompt.
// Hold the PRG button for BLE_UNPAIR_HOLD_MS to forget the phone.
// Comment out to get the upstream behaviour (any device may connect).
#define BLE_LOCK_TO_PHONE
#define BLE_UNPAIR_HOLD_MS	5000
// Disconnect a device that does not start encryption within this time,
// so a stranger cannot keep the bridge's only advertising slot busy.
#define BLE_PAIRING_TIMEOUT_S	30

// Bluetooth transmit power (ESP_PWR_LVL_N12 .. ESP_PWR_LVL_P9, 3 dB steps).
// The phone is usually within a few metres, so the ESP32 default (+3 dBm)
// is more than needed; 0 dBm saves a little current. Raise it if the phone
// connection drops (e.g. phone in a bag on the other side of the body).
#define BLE_TX_POWER		ESP_PWR_LVL_N0

// Battery voltage sensing: 220k/100k divider switched on by Vext (LOW).
// V2.1: GPIO37 = ADC1 channel 1.  V2.0: GPIO13 = ADC2 channel 4.
// Comment out BATTERY_ADC_UNIT to disable battery sensing entirely.
#ifdef HELTEC_V2_1
#define BATTERY_ADC_UNIT	ADC_UNIT_1
#define BATTERY_ADC_CHANNEL	ADC_CHANNEL_1
#else
#define BATTERY_ADC_UNIT	ADC_UNIT_2
#define BATTERY_ADC_CHANNEL	ADC_CHANNEL_4
#endif
#define BATTERY_ADC_ATTEN	ADC_ATTEN_DB_6	// ~150-1750 mV at the ADC pin
#define BATTERY_DIVIDER_X100	320		// (220k + 100k) / 100k = 3.20
// Multiply readings by this percentage to correct for divider tolerance;
// calibrate against a multimeter: actual mV / reported mV * 100.
#define BATTERY_CALIBRATION_PCT	100

// Low-battery protection: the board itself has no over-discharge cutoff.
// Below BATTERY_CUTOFF_MV (under load) the bridge goes into deep sleep;
// it wakes on the PRG button or every 30 minutes and resumes only above
// BATTERY_RESUME_MV. Comment out to disable.
#define BATTERY_CUTOFF_ENABLE
#define BATTERY_CUTOFF_MV	3300
#define BATTERY_RESUME_MV	3450

// Comment out the following line for a module without OLED
#define OLED_ENABLE

// Display language: Russian with this line, English without it.
// Text lives in src/gnarl/display_text.h.
#define DISPLAY_LANG_RU

#endif // _MODULE_H
