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
		DischargeOperation
	};
}
