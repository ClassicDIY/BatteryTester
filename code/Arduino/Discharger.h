// Discharger.h

#pragma once
#include <Arduino.h>
#include <Adafruit_ADS1015.h>
#include <MUX74HC4067.h>

#define ARDUINO_ANALOG 0
#define ADS 1
#define MUX 2
#define SAMPLESIZE 10 // tgemperature sensor

enum State {
	Standby,
	NoBatteryFound,
	MeasuringResistance,
	Discharging,
	ThermalShutdown,
	Complete
};

class Discharger
{
	int _highShuntPin;
	int _lowShuntPin;
	int _gatePin;
	int _Channel;
	boolean _discharging;
	boolean _standby = false;
	int _config;
	float _mAh = 0.0;
	float _shuntRes = 0.22;  // In Ohms - Shunt resistor resistance
	float _voltRef = 4.94; // Reference voltage (probe your 5V pin)
	float _battLow = 2.9;
	float _cutoffTemperature = 45.0;
	int _level = 255; // gate PWM level
	float _internalResistance= 0.0;
	unsigned long _previousMillis = 0;
	unsigned long _millisPassed = 0;
	unsigned long _totalMillis = 0;
	Adafruit_ADS1115* _ads;
	MUX74HC4067* _mux;
	int _index;
	uint16_t _temperatureReadings[SAMPLESIZE];
	State _state;

public:
	Discharger(Adafruit_ADS1115* ads, MUX74HC4067* mux, int channel, int gatePin);
	Discharger(Adafruit_ADS1115* ads, int highShuntPin, int lowShuntPin, int gatePin);
	Discharger(int highShuntPin, int lowShuntPin, int gatePin);
	~Discharger();
	void Init();
	boolean MeasureInternalResistance();
	State Discharge();
	void FindCurrent(float targetCurrent);
	float AdjustLevel(float targetCurrent);
	unsigned long InternalResistance();
	String InternalResistanceRanking();
	float BatteryVolt();
	float ShuntVolt();
	float Diff();
	unsigned long BatteryCurrent();
	unsigned long Capacity();
	unsigned long ElapsedTime();
	float Temperature();
	int State();
};

