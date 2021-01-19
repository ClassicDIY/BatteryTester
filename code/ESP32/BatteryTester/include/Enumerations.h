#pragma once

namespace BatteryTester
{
	enum State
	{
		Unspecified,
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
	static const char * const States[] = {"Unspecified", "Initialize", "Standby"
	, "NoBatteryFound", "Monitor", "Stabilize", "InternalResistance", "FullCharge", "Discharge", "StorageCharge", "ThermalShutdown", "CycleConplete", "Complete"};

	enum TesterError
	{
		Tester_Ok,
		Tester_MCP9808_Failed
	};

	enum Operation
	{
		MonitorOperation,
		TestCycleOperation,
		ChargeOperation,
		TestAndStoreOperation,
		TestAndChargeOperation,
		StorageOperation,
		InternalResistanceOperation,
		DischargeOperation,
	};
	static const char * const Operations[] = {"Monitor", "TestCycle", "Charge", "TestAndStore", "TestAndCharge", "Storage", "InternalResistance", "Discharge"};

	// json key string to enum
	enum Id
	{
		index,
		cell,
		state,
		energy,
		maxTemperature,
		duration,
		voltage,
		cycle,
		current,
		temperature,
		internalResistance
	};
	static const char * const Elements[] = {"index", "cell", "state", "energy", "maxTemperature", "duration", "voltage", "cycle", "current", "temperature", "internalResistance"};

	enum Subtopic
	{
		result,
		operation,
		mode,
		monitor,
		config,
		ping
	};
	static const char * const Subtopics[] = {"result", "operation", "mode", "monitor", "config", "ping"};
}
