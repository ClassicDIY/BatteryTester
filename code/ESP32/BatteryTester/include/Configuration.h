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
		uint16_t _stabilizeDuration; // in seconds
		uint8_t _chargeCurrent;
		uint8_t _chargeDischargeCycleCount;

		byte CalcChecksum(byte _buffer[]);
		bool _isDirty;

	public:
		uint16_t getLowCutoff() { return _lowCutoff; }
		uint16_t getThermalShutdownTemperature() { return _thermalShutdownTemperature; }
		uint16_t getStorageVoltage() { return _storageVoltage; }
		uint16_t getStabilizeDuration() { return _stabilizeDuration; }
		uint8_t getChargeCurrent() { return _chargeCurrent; }
		uint8_t getChargeDischargeCycleCount() { return _chargeDischargeCycleCount; }

		void setLowCutoff(uint16_t lowCutoff); // depth of discharge 
		void setThermalShutdownTemperature(uint16_t thermalShutdownTemperature); // Temperature limit 
		void setStorageVoltage(uint16_t storageVoltage); // Voltage setpoint for battery storage charge (3.7V)
		void setStabilizeDuration(uint16_t stabilizeDuration); // amount of seconds to monitor during stabilize operation
		void setChargeCurrent(uint8_t chargeCurrent); // 0:100 mA 1:400 mA 2: 700mA 3:1000 mA
		void setChargeDischargeCycleCount(uint8_t _chargeDischargeCycleCount); // number of charge-discharge cycles to perform during Cycle() operation

		bool isDirty() { return _isDirty; }
		void Load();
		void Save();
		void SaveTime();
		time_t GetTime();
		void PrintConfiguration();
		void LoadFactoryDefault();
	};

} // namespace BatteryTester