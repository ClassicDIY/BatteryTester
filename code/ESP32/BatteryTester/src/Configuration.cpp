#include "Configuration.h"
#include "Log.h"

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

int deserialInt(byte *buffer)
{
	int i = 0;
	byte *b = (byte *)&i;
	b[0] = buffer[0];
	b[1] = buffer[1];
	return i;
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
}

void Configuration::LoadFactoryDefault()
{
	logi("loading factory default settings");

}

void serialFloat(byte *buffer, float f)
{
	byte *b = (byte *)&f;
	buffer[0] = b[0];
	buffer[1] = b[1];
	buffer[2] = b[2];
	buffer[3] = b[3];
}

void serialInt(byte *buffer, int f)
{
	byte *b = (byte *)&f;
	buffer[0] = b[0];
	buffer[1] = b[1];
}

void Configuration::Save()
{
	byte _buffer[STORAGE_SIZE + 1];
	for (int i = 0; i < STORAGE_SIZE; i++)
	{
		_buffer[i] = 0x00;
	}
	// serialFloat(&(_buffer[14]), _longitude);
	// serialInt(&(_buffer[18]), _horizontalLength);
	// if (_hasAnemometer)
	// 	_buffer[26] = 1;
	// else
	// 	_buffer[26] = 0;
	_buffer[28] = CalcChecksum(_buffer);
	_preferences.putBytes("Configuration", _buffer, STORAGE_SIZE);
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

} // namespace SkyeTracker