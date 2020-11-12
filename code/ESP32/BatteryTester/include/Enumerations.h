#pragma once

namespace BatteryTester
{
	enum TesterState
	{
		TesterState_Off,
		TesterState_Initializing,
		TesterState_Standby,
		TesterState_Manual,
		TesterState_Cycling,
		TesterState_Tracking,
		TesterState_Parked
	};

	enum TesterError
	{
		Tester_Ok,
		Tester_FailedToAccessRTC
	};
}
