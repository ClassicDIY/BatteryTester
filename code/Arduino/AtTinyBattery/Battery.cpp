// 
// 
// 

#include "Battery.h"

Battery::Battery(unsigned long  voltRef, uint8_t highBatPin, uint8_t gatePin, uint8_t tp4056Enable, uint8_t tp4056Prog, uint8_t tp4056Standby, uint8_t mcp9700)
{
	_voltRef = voltRef;
	_highBatPin = highBatPin;
	_gatePin = gatePin;
	_tp4056Enable = tp4056Enable;
	_tp4056Prog = tp4056Prog;
	_tp4056Standby = tp4056Standby;
	_mcp9700 = mcp9700;
}

void Battery::Init()
{
	if (_gatePin != 0xff) pinMode(_gatePin, OUTPUT);
	if (_tp4056Enable != 0xff) pinMode(_tp4056Enable, OUTPUT);
	if (_tp4056Standby != 0xff) pinMode(_tp4056Standby, INPUT);
	if (_tp4056Prog != 0xff) pinMode(_tp4056Prog, INPUT);
	Reset();
}

void Battery::Reset()
{
	_state = Initialized;
	_internalResistance = 0;
	_previousMillis = 0;
	_millisPassed = 0;
	_totalMillis = 0;
	_mAh = 0;
	_MaxTemperature = 0;
	TP4056_Off();
	Load_Off();
}

void Battery::Charge()
{
	Load_Off();
	_state = FinalCharge;
	TP4056_On();
}

void Battery::Cycle()
{
	_state = InitialCharge;
	TP4056_On();
}

boolean Battery::MeasureInternalResistance()
{
	boolean rVal = false;
	//Load_Off();
	//_state = MeasuringResistance;
	//delay(500);
	//unsigned long voc = Volt();
	////Serial.print("voc: "); Serial.println(voc);
	//if (voc > 2.0) { // battery inserted?
	//	Load_On();
	//	delay(2000);
	//	unsigned long vLoad = Volt();
	//	unsigned long iMax = Current();
	//	Load_Off();
	//	_internalResistance = (((voc - vLoad) * 1000) / (iMax - 1)) * 1000;
	//	//Serial.print("Resistance: "); Serial.println(_internalResistance);
	//	//Serial.print(" voc: "); Serial.print(voc); Serial.print(" vLoad: "); Serial.print(vLoad); Serial.print(" dV: "); Serial.print(voc - vLoad);  Serial.print(" iMax: "); Serial.println(iMax);
	//	rVal = true;
	//}
	return rVal;
}

State Battery::Loop()
{
	if (_state == Initialized) {
		delay(1000);
		Load_Off();
		TP4056_Off();
		delay(1000);
		unsigned long voc = Volt();
		if (voc < 2.0) { // battery inserted?
			_state = NoBatteryFound;
			//Serial.print(_gatePin); Serial.print(" No Battery Found "); Serial.print("voc: "); Serial.println(voc);
		}
		else {
			_state = Standby;
		}
	}
	if (_state == InitialCharge) {
		if (TP4056_OnStandby()) {
			//Serial.print(_gatePin); Serial.println(" tp4056Standby => MeasureInternalResistance ");
			TP4056_Off();
			MeasureInternalResistance();
			//Serial.print(_gatePin); Serial.println(" MeasureInternalResistance => Discharge ");
			_state = Discharge;
			_previousMillis = millis();
			_totalMillis = millis();
			_mAh = 0;
			Load_On();
		}
	}
	if (_state == Discharge) {
		unsigned long battVolt = Volt();
		unsigned long current = Current();
		if (battVolt >= _battLow)
		{
			_millisPassed = millis() - _previousMillis;
			_mAh = _mAh + (current) * (_millisPassed / 3600000.0);
			//Serial.print("mAh: "); Serial.println(_mAh);
			_previousMillis = millis();
		}
		if (battVolt < _battLow)
		{
			Load_Off();
			_totalMillis = millis() - _totalMillis;
			_state = FinalCharge;
			TP4056_On();
			//Serial.print(_gatePin); Serial.println(" Discharge => FinalCharge ");
		}
	}
	if (_state == FinalCharge) {
		if (TP4056_OnStandby()) {
			TP4056_Off();
			_state = Complete;
			//Serial.print(_gatePin); Serial.println(" FinalCharge ==> Complete ");
		}
	}
	unsigned long temp = Temperature();
	if (temp > _MaxTemperature) {
		_MaxTemperature = temp;
		if (temp > 45.0) {
			_state = ThermalShutdown;
			TP4056_Off();
			Load_Off();
		}
	}
	return _state;
}


void Battery::TP4056_Off()
{
	if (_tp4056Enable != 0xff) digitalWrite(_tp4056Enable, LOW);
}

boolean Battery::TP4056_OnStandby()
{
	return (_tp4056Standby != 0xff) ? digitalRead(_tp4056Standby) == LOW : false;
}

void Battery::TP4056_On()
{
	if (_tp4056Enable != 0xff) digitalWrite(_tp4056Enable, HIGH);
}

void Battery::Load_Off()
{
	if (_gatePin != 0xff) digitalWrite(_gatePin, LOW);
}

void Battery::Load_On()
{
	if (_gatePin != 0xff) digitalWrite(_gatePin, HIGH);
}

unsigned long Battery::InternalResistance()
{
	return _internalResistance; // in mOhms
}

unsigned long Battery::Volt()
{
	if (_highBatPin == 0xff) return 0;
	unsigned int t = 0;
	unsigned int samples = 0;

	for (int sample = 0; sample < SAMPLESIZE; sample++) {
		t = analogRead(_highBatPin);
		samples = samples + t;
	}
	t = samples / SAMPLESIZE;
	return t * _voltRef - 5;
}

unsigned long Battery::Diff()
{
	if (_highBatPin == 0xff) return 0;
	return Volt() - 10; // minus mosfet internal resistance
}

unsigned long Battery::Current()
{
	if (_state == Discharge) {
		return DischargeCurrent();
	}
	else if (_state == InitialCharge || _state == FinalCharge) {
		return ChargeCurrent();
	}
	return 0;
}

unsigned long Battery::DischargeCurrent()
{
	if (_highBatPin == 0xff) return 888;
	unsigned long current = 0;
	unsigned long diff = Diff();
	if (diff > 0) {
		current = (diff)*1000.0 / _shuntRes;
	}
	if (current < 0) {
		current = 0;
	}
	return current;
}

unsigned long Battery::ChargeCurrent()
{

	if (_tp4056Prog == 0xff) return 999;
	unsigned long current = 0;
	unsigned int a = 0;
	unsigned int samples = 0;

	for (int sample = 0; sample < SAMPLESIZE; sample++) {
		a = analogRead(_tp4056Prog);
		samples = samples + a;
	}
	a = samples / SAMPLESIZE;
	current = a * _voltRef / 10;
	return current;
}

unsigned long Battery::Capacity()
{
	return _mAh;
}

unsigned long Battery::DischargeTime()
{
	if (_state == Discharge) {
		return (millis() - _totalMillis) / 1000;
	}
	else if (_state == FinalCharge || _state == Complete) {
		return _totalMillis / 1000;
	}
	else {
		return 0;
	}
}

State Battery::GetState()
{
	return _state;
}

unsigned long Battery::Temperature()
{
	if (_mcp9700 == 0xff) return 999;
	unsigned int t = 0;
	unsigned int samples = 0;
	for (int sample = 0; sample < SAMPLESIZE; sample++) {
		t = analogRead(_mcp9700);
		samples = samples + t;
	}
	t = samples / SAMPLESIZE;
	return t * _voltRef - 5;
}

unsigned long Battery::MaxTemperature() {
	return _MaxTemperature;
}


