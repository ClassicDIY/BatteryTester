// Battery.h

#ifndef _BATTERY_h
#define _BATTERY_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif
#include "State.h"

#define SAMPLESIZE 10 // temperature sensor

class Battery
{
private:
	uint8_t _highBatPin = 0xff;
	uint8_t _gatePin = 0xff;
	uint8_t _tp4056Prog = A1;
	uint8_t _tp4056Enable = 1;
	uint8_t _mcp9700 = 0xff;
	uint8_t _tp4056Standby = 0;

	unsigned long  _shuntRes = 6800;  // In mOhms - Shunt resistor resistance
	unsigned long  _voltRef = 48; // Reference voltage (probe your 5V pin) in V * 10

	unsigned long  _mAh = 0;
	unsigned long  _battLow = 2900; // discharge cutoff in mV
	unsigned long  _internalResistance = 0; // in mOhms
	unsigned long _previousMillis = 0;
	unsigned long _millisPassed = 0;
	unsigned long _totalMillis = 0;
	enum State _state;
	unsigned long  _MaxTemperature; // in degrees C * 10

public:
	Battery(unsigned long  voltRef, uint8_t highBatPin, uint8_t gatePin, uint8_t tp4056Enable, uint8_t tp4056Prog, uint8_t tp4056Standby, uint8_t mcp9700);
	void Init();
	void Reset();
	void Charge();
	void Cycle();

	boolean MeasureInternalResistance();
	State Loop();
	unsigned long InternalResistance();
	unsigned long Volt();
	unsigned long Current();
	unsigned long ChargeCurrent();
	unsigned long DischargeCurrent();
	unsigned long Capacity();
	unsigned long DischargeTime();
	unsigned long Temperature();
	unsigned long MaxTemperature();
	State GetState();

private:
	unsigned long Diff();
	void TP4056_Off();
	boolean TP4056_OnStandby();
	void TP4056_On();
	void Load_Off();
	void Load_On();
};

#endif

