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
		NoOp,
		TestCycleOperation,
		ChargeOperation,
		TestAndStoreOperation,
		TestAndChargeOperation,
		StorageOperation,
		InternalResistanceOperation,
		DischargeOperation,
	};
	static const char * const Operations[] = {"NoOp", "TestCycle", "Charge", "TestAndStore", "TestAndCharge", "Storage", "InternalResistance", "Discharge"};

	enum Id
	{
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
	static const char * const Elements[] = {"state", "energy", "maxTemperature", "duration", "voltage", "cycle", "current", "temperature", "internalResistance"};

	enum Subtopic
	{
		result,
		mode,
		monitor,
		config
	};
	static const char * const Subtopics[] = {"result", "mode", "monitor", "config"};
}
