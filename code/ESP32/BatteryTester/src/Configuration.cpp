#include "Configuration.h"
#include "Log.h"
#include "Constants.h"

#define STORAGE_SIZE 30

Preferences _preferences;

namespace BatteryTester
{
	Configuration::Configuration()
	{
		_isDirty = false;
	}

	Configuration::~Configuration()
	{
	}

	float deserialFloat(byte *buffer)
	{
		float f = 0;
		byte *b = (byte *)&f;
		b[0] = buffer[0];
		b[1] = buffer[1];
		b[2] = buffer[2];
		b[3] = buffer[3];
		return f;
	}

	uint16_t deserialWord(byte *buffer)
	{
		uint16_t w = 0;
		byte *b = (byte *)&w;
		b[0] = buffer[0];
		b[1] = buffer[1];
		return w;
	}

	uint8_t deserialByte(byte *buffer)
	{
		return buffer[0];
	}

	void Configuration::Load()
	{
		if (_preferences.begin(TAG, false))
		{
			if (FACTORY_RESET_PIN != -1 && digitalRead(FACTORY_RESET_PIN) == LOW)
			{
				logw("FACTORY_RESET_PIN LOW, loading default settings");
				LoadFactoryDefault();
				_preferences.clear();
				Save();
			}
			byte _buffer[STORAGE_SIZE + 1];
			_preferences.getBytes("Configuration", _buffer, STORAGE_SIZE);
			if (CalcChecksum(_buffer) == 0)
			{
				logd("Checksum passed, loading saved settings");
				_lowCutoff = deserialWord(&(_buffer[0]));
				_thermalShutdownTemperature = deserialWord(&(_buffer[2]));
				_storageVoltage = deserialWord(&(_buffer[4]));
				_lowCutoff = deserialWord(&(_buffer[6]));
				_chargeCurrent = deserialByte(&(_buffer[8]));
				_chargeDischargeCycleCount = deserialByte(&(_buffer[9]));
				
				_isDirty = false;
			}
			else
			{
				logw("Checksum failed, loading default settings");
				LoadFactoryDefault();
			}
		}
		else
		{
			logw("Could not initialize preferences, loading default settings");
			LoadFactoryDefault();
		}
		PrintConfiguration();
	}

	void Configuration::LoadFactoryDefault()
	{
		_thermalShutdownTemperature = ThermalShutdownTemperature;
		_lowCutoff = LowCutoff;
		_storageVoltage = StorageVoltage;
		_stabilizeDuration = StabilizeDuration;
		_chargeCurrent = DefaultChargeCurrent;
		_chargeDischargeCycleCount = ChargeDischargeCycleCount;
		_isDirty = true;
	}

	void serialFloat(byte *buffer, float f)
	{
		byte *b = (byte *)&f;
		buffer[0] = b[0];
		buffer[1] = b[1];
		buffer[2] = b[2];
		buffer[3] = b[3];
	}

	void serialWord(byte *buffer, uint16_t v)
	{
		byte *b = (byte *)&v;
		buffer[0] = b[0];
		buffer[1] = b[1];
	}

	void serialByte(byte *buffer, uint8_t v)
	{
		byte *b = (byte *)&v;
		buffer[0] = b[0];
	}

	void Configuration::Save()
	{
		byte buffer[STORAGE_SIZE + 1];
		for (int i = 0; i < STORAGE_SIZE; i++)
		{
			buffer[i] = 0x00;
		}
		serialWord(&(buffer[0]), _lowCutoff);
		serialWord(&(buffer[2]), _thermalShutdownTemperature);
		serialWord(&(buffer[4]), _storageVoltage);
		serialWord(&(buffer[6]), _stabilizeDuration);
		serialByte(&(buffer[8]), _chargeCurrent);
		serialByte(&(buffer[9]), _chargeDischargeCycleCount);
		buffer[STORAGE_SIZE-1] = CalcChecksum(buffer);
		_preferences.putBytes("Configuration", buffer, STORAGE_SIZE);
		_isDirty = false;
		logd("Saved settings");
	}

	byte Configuration::CalcChecksum(byte _buffer[])
	{
		byte crc = 0;
		for (int i = 0; i < STORAGE_SIZE; i++)
			crc += _buffer[i];
		crc = -crc;
		logd("Checksum cal: %x", crc);
		return crc;
	}

	time_t GetSystemTime()
	{
		uint32_t start = millis();
		time_t now;
		while ((millis() - start) <= 100)
		{
			time(&now);
			if (now > 1573996930) // manufacture date
			{
				return now;
			}
			delay(10);
		}
		return (time_t)0;
	}

	// try to get valid time from system, last resort use last saved time from watchdog reset
	time_t Configuration::GetTime()
	{
		time_t now = GetSystemTime();
		if (now == 0)
		{
			return (time_t)_preferences.getLong("time");
		}
		return now;
	}

	void Configuration::SaveTime()
	{
		time_t now = GetSystemTime();
		if (now != 0)
		{
			_preferences.putLong("time", (int32_t)now);
		}
	}

	void Configuration::PrintConfiguration()
	{
		logi("LowCutoff: %d ThermalShutdownTemperature: %d StorageVoltage: %d  StabilizeDuration: %d ChargeCurrent: %d ChargeDischargeCycleCount: %d"
		, getLowCutoff(), getThermalShutdownTemperature(), getStorageVoltage(), getStabilizeDuration(), getChargeCurrent(), getChargeDischargeCycleCount());
	}
} // namespace BatteryTester