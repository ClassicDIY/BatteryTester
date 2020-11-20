#pragma once

#include <ArduinoJson.h>
#include <ThreadController.h>
#include <Thread.h>
#include "Configuration.h"
#include "Enumerations.h"
#include "IOT.h"
#include "Constants.h"
#include "Battery.h"

extern BatteryTester::Configuration _config;
extern BatteryTester::IOT _iot;

namespace BatteryTester
{
	class Tester : public Thread
	{
	public:
		Tester(uint8_t batteryPosition, uint8_t highBatPin, uint8_t shuntPin, uint8_t gatePin, uint8_t tp4056Prog,
			   uint8_t tp4056Enable, uint8_t i2cAddress, uint8_t lowLoad, uint8_t tp4056Standby, uint8_t chargeCurrent4k, uint8_t chargeCurrent2k, uint8_t dischargeLed);
		~Tester();

		TesterError _errorState;
		void Setup(ThreadController *controller);
		// void Track();
		// void Resume();
		// void Park(bool protect);
		TesterState getState();
		void setState(TesterState state);
		void run();
		void SetChargeCurrent();
		Battery *pBattery() { return _pBattery; }

	private:
		uint8_t _batteryPosition; // 1 or 2;
		uint8_t _tp4056Enable;
		uint8_t _lowLoad;
		uint8_t _tp4056Standby;
		uint8_t _chargeCurrent4k;
		uint8_t _chargeCurrent2k;
		uint8_t _dischargeLed;
		uint8_t _dutyCycle;

		uint32_t _mAs = 0; //mA seconds
		unsigned long _previousMillis = 0;
		unsigned long _totalMillis = 0;
		uint16_t _MaxTemperature; // in �C * 10
		unsigned long _previousPoll = 0;
		uint32_t _internalResistance;

		Battery *_pBattery;
		TesterState _state;

		uint32_t DischargeTime();
		void TP4056_Off();
		void TP4056_On();
		boolean TP4056_OnStandby();
		boolean TP4056_Charging();
		void Load_Off();
		void Load_On();
		void LowLoad_Off();
		void LowLoad_On();
		void DischargeLed_Off();
		void DischargeLed_On();
		TesterState NextState();
	};

} // namespace BatteryTester