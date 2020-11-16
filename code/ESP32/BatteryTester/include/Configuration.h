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
		uint16_t _lowCutoff;
		uint16_t _thermalShutdownTemperature;
		uint16_t _storageVoltage;
		uint8_t _chargeCurrent;

		byte CalcChecksum(byte _buffer[]);
		bool _isDirty;
		

	public:
		uint16_t getLowCutoff() { return _lowCutoff; }
		uint16_t getThermalShutdownTemperature() { return _thermalShutdownTemperature; }
		uint16_t getStorageVoltage() { return _storageVoltage; }
		uint8_t getChargeCurrent() { return _chargeCurrent; }

		void setLowCutoff(uint16_t lowCutoff);
		void setThermalShutdownTemperature(uint16_t thermalShutdownTemperature);
		void setStorageVoltage(uint16_t storageVoltage);
		void setChargeCurrent(uint8_t chargeCurrent); // 0:100 mA 1:400 mA 2: 700mA 3:1000 mA

		bool isDirty() { return _isDirty; }
		void Load();
		void Save();
		void SaveTime();
		time_t GetTime();
		void PrintConfiguration();
		void LoadFactoryDefault();
	};

}