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

	void Configuration::setLowCutoff(uint16_t val)
	{
		if (val > 2000)
		{
			_lowCutoff = val;
			_isDirty = true;
		}
	}

	void Configuration::setThermalShutdownTemperature(uint16_t val)
	{
		if (val < 700)
		{
			_thermalShutdownTemperature = val;
			_isDirty = true;
		}
	}

	void Configuration::setStorageVoltage(uint16_t val)
	{
		if (val < 4200)
		{
			_storageVoltage = val;
			_isDirty = true;
		}
	}

	void Configuration::setChargeCurrent(uint8_t val)
	{
		if (val < 4)
		{
			_chargeCurrent = val;
			_isDirty = true;
		}
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

		if (_preferences.begin("BatteryTester", false))
		{
			if (digitalRead(FACTORY_RESET_PIN) == LOW)
			{
				logi("FACTORY_RESET_PIN LOW, loading default settings");
				LoadFactoryDefault();
				_preferences.clear();
				Save();
			}
			byte _buffer[STORAGE_SIZE + 1];
			_preferences.getBytes("Configuration", _buffer, STORAGE_SIZE);
			if (CalcChecksum(_buffer) == 0)
			{
				logi("Checksum passed, loading saved settings");
				_lowCutoff = deserialWord(&(_buffer[0]));
				_thermalShutdownTemperature = deserialWord(&(_buffer[2]));
				_storageVoltage = deserialWord(&(_buffer[4]));
				_chargeCurrent = deserialByte(&(_buffer[6]));
				_isDirty = false;
			}
			else
			{
				logi("Checksum failed, loading default settings");
				LoadFactoryDefault();
			}
		}
		else
		{
			logi("Could not initialize preferences, loading default settings");
			LoadFactoryDefault();
		}
		PrintConfiguration();
	}

	void Configuration::LoadFactoryDefault()
	{
		logi("loading factory default settings");
		_thermalShutdownTemperature = ThermalShutdownTemperature;
		_lowCutoff = LowCutoff;
		_storageVoltage = StorageVoltage;
		_chargeCurrent = DefaultChargeCurrent;
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
		serialByte(&(buffer[6]), _chargeCurrent);
		buffer[STORAGE_SIZE] = CalcChecksum(buffer);
		_preferences.putBytes("Configuration", buffer, STORAGE_SIZE);
		_isDirty = false;
		logi("Saved settings");
	}

	byte Configuration::CalcChecksum(byte _buffer[])
	{
		byte crc = 0;
		for (int i = 0; i < STORAGE_SIZE; i++)
			crc += _buffer[i];
		crc = -crc;
		logi("Checksum cal: %x", crc);
		return crc;
	}

	time_t GetSystemTime()
	{
		uint32_t start = millis();
		time_t now;
		while ((millis() - start) <= 100)
		{
			time(&now);
			if (now > 1573996930)
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
		logi("LowCutoff: %d ThermalShutdownTemperature: %d StorageVoltage: %d  ChargeCurrent: %d", getLowCutoff(), getThermalShutdownTemperature(), getStorageVoltage(), getChargeCurrent());
	}
} // namespace BatteryTester