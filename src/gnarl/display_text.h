#ifndef _DISPLAY_TEXT_H
#define _DISPLAY_TEXT_H

// Text shown on the OLED. Select the language with DISPLAY_LANG_RU in
// include/module.h. Strings are UTF-8; the display is 128 x 64 pixels:
//   small font  (6x12): 21 characters per line,
//   medium font (8x13): 16 characters per line (Russian fonts).
// Keep technical terms (PA_BOOST, dBm, RFO, version) in Latin so they
// match the logs and the documentation.

#include "module.h"

#ifdef DISPLAY_LANG_RU

// Status screen
#define S_CONNECTED		"Подключено"
#define S_DISCONNECTED		"Нет связи"
#define S_LAST_COMMAND		"Посл. обмен:"
#define S_PHONE_RSSI		"Сигнал тел.:"
#define S_PUMP_RSSI		"Сигнал помпы:"
#define S_BATTERY		"Батарея:"
#define S_UNIT_SEC		"с"
#define S_UNIT_MIN		"м"
#define S_UNIT_HOUR		"ч"
#define S_VOLT			"В"
#define S_DECIMAL		","

// Boot screen
#define S_PUMP_FREQ		"Помпа: %d" S_DECIMAL "%03d МГц"
#define S_TX_RFO		"TX: RFO (оригинал)"
#define S_PHONE_PAIRED		"Телефон: привязан"
#define S_PHONE_NOT_PAIRED	"Телефон: не привязан"
#define S_PHONE_LOCK_OFF	"Привязка: выкл."

// Messages
#define S_BATTERY_LOW		"Батарея села"
#define S_TURNING_OFF		"Выключаюсь"
#define S_PHONE_UNPAIRED	"Телефон отвязан"
#define S_PAIR_AGAIN		"Привяжите в AAPS"
#define S_UNKNOWN_PHONE		"Чужой телефон"
#define S_REJECTED		"отклонён"
#define S_HOLD_TO_UNPAIR	"PRG 5 с: отвязать"

#else // English

#define S_CONNECTED		"Connected"
#define S_DISCONNECTED		"Disconnected"
#define S_LAST_COMMAND		"Last command:"
#define S_PHONE_RSSI		"Phone RSSI:"
#define S_PUMP_RSSI		"Pump  RSSI:"
#define S_BATTERY		"Battery:"
#define S_UNIT_SEC		"s"
#define S_UNIT_MIN		"m"
#define S_UNIT_HOUR		"h"
#define S_VOLT			"V"
#define S_DECIMAL		"."

#define S_PUMP_FREQ		"Pump: %d" S_DECIMAL "%03d MHz"
#define S_TX_RFO		"TX: RFO (upstream)"
#define S_PHONE_PAIRED		"Phone: paired"
#define S_PHONE_NOT_PAIRED	"Phone: not paired yet"
#define S_PHONE_LOCK_OFF	"Phone lock: off"

#define S_BATTERY_LOW		"Battery low"
#define S_TURNING_OFF		"Turning off"
#define S_PHONE_UNPAIRED	"Phone unpaired"
#define S_PAIR_AGAIN		"Pair again from AAPS"
#define S_UNKNOWN_PHONE		"Unknown phone"
#define S_REJECTED		"rejected"
#define S_HOLD_TO_UNPAIR	"Hold PRG 5 s to unpair"

#endif // DISPLAY_LANG_RU

#endif // _DISPLAY_TEXT_H
