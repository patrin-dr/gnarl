#ifndef _PUMP_CONFIG_H
#define _PUMP_CONFIG_H

// Medtronic MiniMed 722, Worldwide (WW) region

#define PUMP_ID		"970260"	// pump serial number (string constant)
#define PUMP_FREQUENCY	868350000	// WW pump frequency, Hz; refine with mmtune
#define MMTUNE_START	868200000	// starting frequency for mmtune scans, Hz

#endif // _PUMP_CONFIG_H
