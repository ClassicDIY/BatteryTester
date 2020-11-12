#pragma once

#include "Preferences.h"

namespace BatteryTester
{
	class Configuration
	{
	public:
		Configuration();
		~Configuration();

	private:

		void LoadFactoryDefault();
		byte CalcChecksum(byte _buffer[]);
		bool _isDirty;
		

	public:
		
		bool isDirty() { return _isDirty; }
		void Load();
		void Save();
		void SaveTime();
		time_t GetTime();
	};

}