#ifndef _MODULE_H
#define _MODULE_H

// Settings shared by all boards. Pins and board details are in
// include/boards/<board>.h; mk/make-project.sh writes include/board.h
// for the board chosen with "make <project> BOARD=<board>":
//   heltec_v2         Heltec WiFi LoRa 32 V2 / V2.1 (default)
//   heltec_v3_rfm95   Heltec WiFi LoRa 32 V3.x + external RFM95W
#include "board.h"

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
// Boards without a user button (LilyGO T3): forget the phone by pressing
// RST BLE_UNPAIR_RESETS times in a row, each press within
// BLE_UNPAIR_RESET_WINDOW_S seconds of the previous start-up.
#define BLE_UNPAIR_RESETS		3
#define BLE_UNPAIR_RESET_WINDOW_S	5
// Disconnect a device that does not start encryption within this time,
// so a stranger cannot keep the bridge's only advertising slot busy.
#define BLE_PAIRING_TIMEOUT_S	30

// Bluetooth transmit power (ESP_PWR_LVL_N12 .. ESP_PWR_LVL_P9, 3 dB steps).
// The phone is usually within a few metres, so the ESP32 default (+3 dBm)
// is more than needed; 0 dBm saves a little current. Raise it if the phone
// connection drops (e.g. phone in a bag on the other side of the body).
#define BLE_TX_POWER		ESP_PWR_LVL_N0

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
