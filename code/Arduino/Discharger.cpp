// 
// 
// 

#include "Discharger.h"

Discharger::Discharger(Adafruit_ADS1115 * ads, MUX74HC4067 * mux, int channel, int gatePin)
{
	_ads = ads;
	_mux = mux;
	_Channel = channel;
	_gatePin = gatePin;
	_config = MUX;
}

Discharger::Discharger(Adafruit_ADS1115 * ads, int highShuntPin, int lowShuntPin, int gatePin)
{
	_ads = ads;
	_highShuntPin = highShuntPin;
	_lowShuntPin = lowShuntPin;
	_gatePin = gatePin;
	_config = ADS;
}

Discharger::Discharger(int highShuntPin, int lowShuntPin, int gatePin)
{
	_highShuntPin = highShuntPin;
	_lowShuntPin = lowShuntPin;
	_gatePin = gatePin;
	_config = ARDUINO_ANALOG;
}


Discharger::~Discharger()
{
}

void Discharger::Init()
{
	_state = Standby;
	pinMode(_gatePin, OUTPUT);
	digitalWrite(_gatePin, LOW); // turn off load
	_index = 0;
	for (int i = 0; i < SAMPLESIZE; i++)
	{
		_temperatureReadings[i] = 0.0;
	}
}

boolean Discharger::MeasureInternalResistance()
{
	boolean rVal = false;
	digitalWrite(_gatePin, LOW);
	delay(1000);
	float voc = BatteryVolt();
	Serial.print("voc: "); Serial.println(voc);
	if (voc > 2.0) { // battery inserted?
		digitalWrite(_gatePin, LOW);
		delay(20);
		_level = 127;
		FindCurrent(5);
		float level5 = _level;
		float v5 = BatteryVolt() * 1000;
		float i5 = BatteryCurrent();
		digitalWrite(_gatePin, HIGH);
		delay(2000);

		float vLoad = BatteryVolt() * 1000;
		float iMax = BatteryCurrent();
		digitalWrite(_gatePin, LOW);
		if (Temperature() >= _cutoffTemperature) {
			_state = ThermalShutdown;
		}
		_internalResistance = ((v5 - vLoad) / (iMax - i5)) * 1000;
		Serial.print("Resistance: "); Serial.print(_internalResistance); Serial.print(" v5: "); Serial.print(v5); Serial.print(" v500: "); Serial.print(vLoad); Serial.print(" dV: "); Serial.print(v5 - vLoad); Serial.print(" i5: "); Serial.print(i5); Serial.print(" i500: "); Serial.print(iMax); Serial.print(" dI: "); Serial.print(iMax - i5); Serial.print(" level 5: "); Serial.print(level5);
		rVal = true;
	}
	digitalWrite(_gatePin, LOW);
	return rVal;
}

State Discharger::Discharge()
{

	if (_state == Standby) {
		float voc = BatteryVolt();
		if (voc < 2.0) { // battery inserted?
			_state = NoBatteryFound;
		}
		else {
			_state = Discharging;
			digitalWrite(_gatePin, HIGH);
			_totalMillis = millis();
		}
	}
	Serial.print(_gatePin); Serial.print(" state: "); Serial.println(_state);

	if (_state == Discharging) {

		float battVolt = BatteryVolt();
		float current = BatteryCurrent();
		if (battVolt >= _battLow)
		{
			_millisPassed = millis() - _previousMillis;
			_mAh = _mAh + (current) * (_millisPassed / 3600000.0);
			Serial.print("mAh: "); Serial.println(_mAh);
			_previousMillis = millis();
		}
		if (battVolt < _battLow)
		{
			digitalWrite(_gatePin, LOW);
			_totalMillis = millis() - _totalMillis;
			_state = Complete;
		}
	}
	if (Temperature() >= _cutoffTemperature) {
		_state = ThermalShutdown;
		digitalWrite(_gatePin, LOW);
	}
	return _state;
}

void Discharger::FindCurrent(float targetCurrent) {
	int countdown = 255;
	int stable = 0;
	int lastLevel = 0;

	do {
		lastLevel = _level;
		AdjustLevel(targetCurrent);
		if (_level == lastLevel) {
			stable++;
		}
		else {
			stable = 0;
		}
		countdown--;
	} while (countdown > 0 && stable < 5);
	//Serial.print("countdown "); Serial.print(countdown); Serial.print(" stable "); Serial.println(stable);
}

float Discharger::AdjustLevel(float targetCurrent) {
	boolean found = true;
	analogWrite(_gatePin, _level);
	delay(100);
	float current = BatteryCurrent();
	//Serial.print(_gatePin); Serial.print(": "); Serial.print(current); Serial.print(" mA "); Serial.print(_level);
	if (current > targetCurrent + (targetCurrent / 15)) {
		//Serial.print(" -");
		int decrement = 1;
		_level = _level - decrement;
		if (_level <= 0) {
			_level = 0;
		}
		else {
			found = false;
		}
	}
	if (current < targetCurrent - (targetCurrent / 15)) {
		//Serial.print(" +");
		int increment = 1;
		_level = _level + increment;
		if (_level >= 255) {
			_level = 255;
		}
		else {
			found = false;
		}
	}
	//Serial.println(" ");
	return current;
}

/*
Milli-Ohm	Battery Voltage	Ranking
75-150mOhm	3.6V	Excellent
150-250mOhm	3.6V	Good
250-350mOhm	3.6V	Marginal
350-500mOhm	3.6V	Poor
> 500mOhm	3.6V	Fail*/

unsigned long Discharger::InternalResistance()
{
	return _internalResistance; // in mOhms
}

String Discharger::InternalResistanceRanking()
{
	if (_internalResistance == 0) {
		return "";
	}
	if (InternalResistance() < 150) {
		return "Excellent";
	}
	else if (InternalResistance() < 250) {
		return "Good";
	}
	else if (InternalResistance() < 350) {
		return "Marginal";
	}
	else if (InternalResistance() < 500) {
		return "Poor";
	}
	else {
		return "Fail";
	}
}

float Discharger::BatteryVolt()
{
	if (_config == ADS) {
		uint16_t high = _ads->readADC_SingleEnded(_highShuntPin);
		if (high > 32768) { high = 0; }
		return high * 0.1875 / 1000;
	}
	else if (_config == MUX) {
		_mux->setChannel(_Channel);
		delay(100);
		uint16_t high = _ads->readADC_SingleEnded(0);
		if (high > 32768) { high = 0; }
		return high * 0.1875 / 1000;
	}
	else {
		return analogRead(_highShuntPin) * _voltRef / 1024.0;
	}
}

float Discharger::ShuntVolt()
{
	if (_config == ADS) {
		uint16_t low = _ads->readADC_SingleEnded(_lowShuntPin);
		if (low > 32768) { low = 0; } // ignore negative volt reading (open circuit)
		return low * 0.1875 / 1000;
	}
	else if (_config == MUX) {
		_mux->setChannel(_Channel);
		uint16_t low = _ads->readADC_SingleEnded(1);
		if (low > 32768) { low = 0; } // ignore negative volt reading (open circuit)
		return low * 0.1875 / 1000;
	}
	else {
		return analogRead(_lowShuntPin) * _voltRef / 1024.0;
	}
}

float Discharger::Diff()
{
	if (_config == ADS) {
		uint16_t diff = 0;
		if (_highShuntPin > 1) {
			diff = _ads->readADC_Differential_2_3();
		}
		else {
			diff = _ads->readADC_Differential_0_1();
		}
		if (diff > 32768) { diff = 0; }
		return diff * 0.1875 / 1000;
	}
	else if (_config == MUX) {
		_mux->setChannel(_Channel);
		uint16_t diff = 0;
		diff = _ads->readADC_Differential_0_1();
		if (diff > 32768) { diff = 0; }
		return diff * 0.1875 / 1000;
	}
	else {
		return (analogRead(_highShuntPin) - analogRead(_lowShuntPin)) * _voltRef / 1024.0;
	}
}

unsigned long Discharger::BatteryCurrent()
{
	float current = 0.0;
	float diff = Diff();
	if (diff > 0) {
		current = (diff)*1000.0 / _shuntRes;
	}
	if (current < 0) {
		current = 0;
	}
	return current;
}

unsigned long Discharger::Capacity()
{
	return _mAh;
}

unsigned long Discharger::ElapsedTime()
{
	if (_state == Discharging) {
		return (millis() - _totalMillis) / 1000;
	}
	else {
		return _totalMillis / 1000;
	}
}

float Discharger::Temperature()
{
	float temperature = 0.0;
	float ten_samples = 0.0;

	_mux->setChannel(_Channel - 5);
	for (int sample = 0; sample < 10; sample++) {
		temperature = (float)_ads->readADC_SingleEnded(1) * 0.1875;
		temperature = temperature - 500;
		delay(50);
		ten_samples = ten_samples + temperature;
	}
	temperature = ten_samples / 100.0;
	return temperature;
}

int Discharger::State()
{
	return _state;
}
