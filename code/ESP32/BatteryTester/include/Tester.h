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
		void Perform(Operation op);
		void setState(State state);
		void run();
		Battery *pBattery() { return _pBattery; }

	private:
		void SetChargeCurrent();
        const char* StateText();
		const char* StateText(State s);
		void MQTTMonitor();

		StaticJsonDocument<MaxMQTTPayload> _doc;
		uint8_t _batteryPosition; // 1 or 2;
		uint8_t _tp4056Enable;
		uint8_t _tp4056Standby;
		uint8_t _chargeCurrent4k;
		uint8_t _chargeCurrent2k;
		uint8_t _dischargeLed;
		uint8_t _dutyCycle;
		boolean _blinker;
		int _modulo;
		uint32_t _mAs = 0; //mA seconds
		unsigned long _timeStamp = 0;
		uint16_t _MaxTemperature; // in �C * 10
		unsigned long _previousPoll = 0;
		uint32_t _internalResistance;

		Battery *_pBattery;
		State _state = Unspecified; // current published state
		int _currentStage; // index of the current state of execution
		State* _currentOperation = 0;
		int _cycleCount = 0;

		void TP4056_Off();
		void TP4056_On();
		boolean TP4056_OnStandby();
		boolean TP4056_Charging();
		void Load_Off();
		void Load_On();
		void BlinkLED();
		void DischargeLed_Off();
		void DischargeLed_On();
		State NextState();
	};

} // namespace BatteryTester