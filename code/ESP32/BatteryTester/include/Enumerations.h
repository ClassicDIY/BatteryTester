#pragma once

namespace BatteryTester
{
	enum TesterState
	{
		Initialize, 
		Standby, 
		NoBatteryFound, 
		Monitor,
		InternalResistance, 
		Charge, 
		Discharge, 
		Storage, 
		ThermalShutdown, 
		Complete, 
		
	};

	enum TesterError
	{
		Tester_Ok,
		Tester_FailedToAccessRTC
	};
}
