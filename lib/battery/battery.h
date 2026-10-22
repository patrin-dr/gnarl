#ifndef _BATTERY_H
#define _BATTERY_H

// Set up battery voltage sensing. Safe to call more than once.
void battery_init(void);

// Battery voltage in millivolts, or -1 if sensing is unavailable.
int battery_millivolts(void);

// Approximate state of charge (0-100) of a single LiPo cell.
// Only meaningful on battery power: while charging from USB the
// voltage reads high.
int battery_percent(int mV);

#endif // _BATTERY_H
