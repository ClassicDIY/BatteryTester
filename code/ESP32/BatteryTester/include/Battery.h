#pragma once

#include <ArduinoJson.h>
#include <ThreadController.h>
#include <Thread.h>
#include "Configuration.h"
#include "Enumerations.h"
#include "IOT.h"

extern BatteryTester::Configuration _config;
extern BatteryTester::IOT _iot;

// const char c_Track[] = "Track";
// const char c_Cycle[] = "Cycle";
// const char c_Stop[] = "Stop";
// const char c_Park[] = "Park";
// const char c_Protect[] = "Protect";
// const char c_GetConfiguration[] = "GetConfiguration";
// const char c_GetDateTime[] = "GetDateTime";
// const char c_BroadcastPosition[] = "BroadcastPosition";
// const char c_StopBroadcast[] = "StopBroadcast";
// const char c_SetC[] = "SetC";
// const char c_SetL[] = "SetL";
// const char c_SetA[] = "SetA";
// const char c_SetO[] = "SetO";
// const char c_SetDateTime[] = "SetDateTime";
// const char c_MoveTo[] = "MoveTo";
// const char c_East[] = "East";
// const char c_West[] = "West";
// const char c_Up[] = "Up";
// const char c_Down[] = "Down";

namespace BatteryTester
{
	class Battery : public Thread
	{
	public:
		Battery();
		~Battery();

		TesterError _errorState;
		void Initialize(ThreadController* controller);
		// void Track();
		// void Resume();
		// void Park(bool protect);
		TesterState getState();
		void setState(TesterState state);
		void ProcessCommand(const char* input);

	private:


		void run();
		TesterState _testerState;

	};

}