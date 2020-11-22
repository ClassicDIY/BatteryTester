#pragma once

namespace BatteryTester
{
	enum State
	{
		Initialize, 
		Standby, 
		NoBatteryFound, 
		Monitor,
		Stabilize,
		InternalResistance, 
		FullCharge, 
		Discharge, 
		StorageCharge, 
		ThermalShutdown, 
		CycleConplete,
		Complete
	};

	enum TesterError
	{
		Tester_Ok,
		Tester_MCP9808_Failed
	};
}
