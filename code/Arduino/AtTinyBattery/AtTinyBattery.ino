#include <arduino.h>
#include <SoftwareSerial.h>
#include "State.h"

//#define OscillatorCalibration 125 // OSCCAL value
#define InternalResistanceCalibration 300 // resistance from battery holder/wires (mOhms).
#define ThermometerCalibration -30 // mcp9700 offset value (0.1 °C)
#define ThermalShutdownTemperature 500 // °C * 10
#define Rx 10
#define Tx 9
#define VoltageRef 5000 // mV
#define AverageCount 100
#define PollInterval 500 // ms

SoftwareSerial swSerial(Rx, Tx);

uint8_t _hallEffectPin = 1;
uint8_t _highBatPin = 4;
uint8_t _shuntPin = 2;
uint8_t _gatePin = 5;
uint8_t _tp4056Prog = 7;
uint8_t _tp4056Enable = 8;
uint8_t _mcp9700 = 6;
uint8_t _tp4056Standby = 3;
uint8_t _gatePWM = 64;

const uint16_t  _shuntRes = 1000;  // In mOhms - Shunt resistor resistance
const uint16_t  _battLow = 2900; // discharge cutoff in mV

uint32_t  _mAs = 0; //mA seconds
unsigned long _previousMillis = 0;
unsigned long _totalMillis = 0;
enum State _state;
uint16_t  _MaxTemperature; // in °C * 10
unsigned long _previousPoll = 0;
unsigned int _pollInterval = PollInterval; // ms
uint32_t _movingAverageChargeCurrent;
uint32_t _movingAverageSumChargeCurrent;
uint32_t _movingAverageTemperature;
uint32_t _movingAverageSumTemperature;
uint32_t _movingAverageBatteryVolt;
uint32_t _movingAverageSumBatteryVolt;
uint32_t _movingAverageShuntVolt;
uint32_t _movingAverageSumShuntVolt;
uint32_t _internalResistance;
int8_t _index = -1; // cell index

void setup()
{
	//OSCCAL = OscillatorCalibration;
	swSerial.begin(9600);
	delay(200);
	pinMode(_gatePin, OUTPUT);
	pinMode(_tp4056Enable, OUTPUT);
	pinMode(_tp4056Standby, INPUT);
	pinMode(_tp4056Prog, INPUT);
	pinMode(_highBatPin, INPUT);
	pinMode(_hallEffectPin, INPUT);
	pinMode(_shuntPin, INPUT);
	pinMode(_mcp9700, INPUT);
	Reset();
	_index = -1;
	//swSerial.println("ATtiny Ready");
}

void Reset()
{
	_state = Initialized;
	_index = -1;
	_movingAverageChargeCurrent = 0;
	_movingAverageSumChargeCurrent = 0;
	_movingAverageTemperature = 0;
	_movingAverageSumTemperature = 0;
	_movingAverageBatteryVolt = 0;
	_movingAverageSumBatteryVolt = 0;
	_movingAverageShuntVolt = 0;
	_movingAverageSumShuntVolt = 0;
	_internalResistance = 0;
	_previousMillis = 0;
	_totalMillis = 0;
	_mAs = 0;
	_MaxTemperature = 0;
	TP4056_Off();
	Load_Off();
}

void loop()
{
	MMA();
	MMABat();
	if (millis() - _previousPoll >= _pollInterval) {
		_previousPoll = millis();
		switch (_state) {
		case Initialized:
		{
			uint32_t voc = Volt();
			if (voc < 2000) { // battery inserted?
				_state = NoBatteryFound;
			}
			else {
				_state = Standby;
			}
		}
		break;
		case InitialCharge:
		{
			if (TP4056_OnStandby()) {
				TP4056_Off();
				_state = MeasuringResistance;
			}
		}
		break;
		case MeasuringResistance:
		{
			analogWrite(_gatePin, 1);
			delay(200);
			_movingAverageBatteryVolt = 0;
			_movingAverageSumBatteryVolt = 0;
			_movingAverageShuntVolt = 0;
			_movingAverageSumShuntVolt = 0;
			_previousPoll = millis();
			while (millis() - _previousPoll < 500) {
				MMABat();
			}
			uint16_t voc = Volt();
			uint16_t iMin = Current();
			Load_On();
			delay(200);
			_movingAverageBatteryVolt = 0;
			_movingAverageSumBatteryVolt = 0;
			_movingAverageShuntVolt = 0;
			_movingAverageSumShuntVolt = 0;
			_previousPoll = millis();
			while (millis() - _previousPoll < 500) {
				MMABat();
			}
			uint32_t vLoad = Volt();
			uint32_t iMax = Current();
			_internalResistance = (voc - vLoad) - InternalResistanceCalibration;
			_internalResistance *= 1000;
			_internalResistance /= (iMax - iMin);
			_state = Discharge;
			_previousMillis = millis();
			_totalMillis = millis();
			_mAs = 0;
		}
		break;
		case Discharge:
		{
			uint32_t battVolt = Volt();
			if (battVolt >= _battLow)
			{
				uint32_t current = Current();
				unsigned long t = ((millis() - _previousMillis));
				_mAs += current * t / 1000;
				_previousMillis = millis();
				if (current > 550) {
					_gatePWM--;
					if (_gatePWM >= 0) analogWrite(_gatePin, _gatePWM);
				}
				else if (current < 450) {
					_gatePWM++;
					if (_gatePWM < 200) analogWrite(_gatePin, _gatePWM);
				}
			}
			else {
				Load_Off();
				_totalMillis = millis() - _totalMillis;
				_state = FinalCharge;
				TP4056_On();
			}
		}
		break;
		case FinalCharge:
		{
			if (TP4056_OnStandby()) {
				TP4056_Off();
				_state = Complete;
			}
		}
		break;
		}
		uint32_t temp = Temperature();
		if (temp > _MaxTemperature) {
			_MaxTemperature = temp;
			if (temp > ThermalShutdownTemperature) {
				_state = ThermalShutdown;
				TP4056_Off();
				Load_Off();
			}
		}
		if (swSerial.available()) {
			swSerial.readStringUntil('[');
			String received = swSerial.readStringUntil(']');
			//swSerial.println(); swSerial.print("<<<"); swSerial.print(received); swSerial.println(">>>");
			swSerial.read();
			if (received.length() < 5) {
				if (received[0] == '?') { // reset request
					swSerial.print("[?]");
					Reset();
				}
				else if (_index == -1) {

					if (received[0] == '!') { // first one in chain
						_index = 0;
					}
					else {
						uint8_t i = received.substring(0, 2).toInt();
						_index = i + 1;
					}
					swSerial.print("[");
					swSerial.print(_index, HEX);
					swSerial.print("]");
					if (_state == Standby) {
						Load_Off();
						_state = InitialCharge;
						TP4056_On();
					}
					//TP4056_Off();
					//_state = MeasuringResistance;
				}
				else {
					uint8_t receivedIndex = received.substring(0, 2).toInt();
					if (receivedIndex == _index) { // request for this cell
						swSerial.print("[");
						swSerial.print(_index, HEX);
						swSerial.print(",");
						swSerial.print(_state, HEX);
						swSerial.print(",");
						swSerial.print(_internalResistance, HEX);
						swSerial.print(",");
						swSerial.print(Volt(), HEX);
						swSerial.print(",");
						swSerial.print(Current(), HEX);
						swSerial.print(",");
						swSerial.print(_mAs / 3600, HEX);
						swSerial.print(",");
						swSerial.print(Temperature(), HEX);
						swSerial.print(",");
						swSerial.print(DischargeTime(), HEX);
						swSerial.print("]");
					}
					else { // next cell request
						swSerial.print("[");
						swSerial.print(receivedIndex, HEX);
						swSerial.print("]");
					}
				}
			}
			else {
				swSerial.print("[");
				swSerial.print(received); // forward previous cell data
				swSerial.print("]");
			}
		}
	}
}

// in mV
uint16_t Volt()
{
	return Scale(_movingAverageBatteryVolt);
}

// in mA
uint16_t Current()
{
	if (_state == Discharge || _state == MeasuringResistance) {
		uint32_t current = 0;
		if (_movingAverageBatteryVolt > _movingAverageShuntVolt) {
			float diff = _movingAverageBatteryVolt - _movingAverageShuntVolt; // one ohm shunt
			return Scale(diff + 0.5);
		}
		return 0;
	}
	else if (_state == InitialCharge || _state == FinalCharge) {
		return Scale(_movingAverageChargeCurrent);
	}
	return 0;
}

// in Seconds
uint32_t DischargeTime()
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

// °C * 10
uint16_t Temperature()
{
	return Scale(_movingAverageTemperature) - 500 + ThermometerCalibration; // MCP9700 0°C is at 500mv
}

uint16_t Scale(uint32_t v) {
	float f = v;
	f *= VoltageRef;
	f /= 1024;
	return (f + 0.5); // round float by adding 0.5 before cast
}

void TP4056_Off()
{
	digitalWrite(_tp4056Enable, LOW);
}

boolean TP4056_OnStandby()
{
	return digitalRead(_tp4056Standby) == LOW;
}

void TP4056_On()
{
	digitalWrite(_tp4056Enable, HIGH);
}

void Load_Off()
{
	analogWrite(_gatePin, 0);
}

void Load_On()
{
	analogWrite(_gatePin, _gatePWM);
}

// Modified Moving Average
void MMA() {
	// Temperature
	_movingAverageSumTemperature = _movingAverageSumTemperature - _movingAverageTemperature; // Remove previous sample from the sum
	_movingAverageSumTemperature = _movingAverageSumTemperature + analogRead(_mcp9700); // Replace it with the current sample
	_movingAverageTemperature = _movingAverageSumTemperature / AverageCount; // Recalculate moving average
	// charge current
	_movingAverageSumChargeCurrent = _movingAverageSumChargeCurrent - _movingAverageChargeCurrent;
	_movingAverageSumChargeCurrent = _movingAverageSumChargeCurrent + analogRead(_tp4056Prog);
	_movingAverageChargeCurrent = _movingAverageSumChargeCurrent / AverageCount;
}

void MMABat() {
	//Battery volts
	_movingAverageSumBatteryVolt = _movingAverageSumBatteryVolt - _movingAverageBatteryVolt;
	_movingAverageSumBatteryVolt = _movingAverageSumBatteryVolt + analogRead(_highBatPin);
	_movingAverageBatteryVolt = _movingAverageSumBatteryVolt / AverageCount;
	//Shunt volts
	_movingAverageSumShuntVolt = _movingAverageSumShuntVolt - _movingAverageShuntVolt;
	_movingAverageSumShuntVolt = _movingAverageSumShuntVolt + analogRead(_shuntPin);
	_movingAverageShuntVolt = _movingAverageSumShuntVolt / AverageCount;
}
