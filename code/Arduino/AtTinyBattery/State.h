// State.h

#ifndef _STATE_h
#define _STATE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

enum State {
	Initialized, //0
	Standby, //1
	NoBatteryFound, //2
	MeasuringResistance, //3
	InitialCharge, //4
	Discharge, //5
	FinalCharge, //6
	ThermalShutdown, //7
	Complete //8
};

#endif

