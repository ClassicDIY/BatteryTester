#pragma once

namespace BatteryTester
{
	enum State
	{
		Initialize, 
		Begin,
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
		Complete,
		Unspecified
	};
	static const char * const States[] = {"Initialize", "Begin", "Standby"
	, "NoBatteryFound", "Monitor", "Stabilize", "InternalResistance", "FullCharge", "Discharge", "StorageCharge", "ThermalShutdown", "CycleConplete", "Complete", "Unspecified"};

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
		capacity,
		maxTemperature,
		duration,
		voltage,
		cycle,
		current,
		temperature,
		internalResistance,
		stage
	};
	static const char * const Elements[] = {"index", "cell", "state", "capacity", "maxTemperature", "duration", "voltage", "cycle", "current", "temperature", "internalResistance", "stage"};

	enum Subtopic
	{
		result,
		operation,
		mode,
		monitor,
		config,
		ping,
		outcome,
		flash
	};
	static const char * const Subtopics[] = {"result", "operation", "mode", "monitor", "config", "ping", "outcome", "flash"};
}
