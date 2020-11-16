#include "Tester.h"
#include "Log.h"
#include <math.h>

namespace BatteryTester
{

	Tester::Tester(uint8_t batteryPosition, uint8_t highBatPin, uint8_t shuntPin, uint8_t gatePin, uint8_t tp4056Prog,
				   uint8_t tp4056Enable, uint8_t thermistorPin, uint8_t load10mw, uint8_t tp4056Standby, uint8_t chargeCurrent4k, uint8_t chargeCurrent2k, uint8_t dischargeLed)
	{
		_batteryPosition = batteryPosition;
		_tp4056Enable = tp4056Enable;
		_lowLoad = load10mw;
		_tp4056Standby = tp4056Standby;
		_chargeCurrent4k = chargeCurrent4k;
		_chargeCurrent2k = chargeCurrent2k;
		_dischargeLed = dischargeLed;
		_pBattery = new Battery(highBatPin, shuntPin, tp4056Prog, thermistorPin);
		pinMode(_tp4056Enable, OUTPUT);
		pinMode(_chargeCurrent4k, OUTPUT);
		pinMode(_chargeCurrent2k, OUTPUT);
		pinMode(_dischargeLed, OUTPUT);
		pinMode(_lowLoad, OUTPUT_OPEN_DRAIN);
		pinMode(_tp4056Standby, INPUT);
		ledcSetup(PWMChannel, PWMfrequency, PWMResolution); //PWM setup
		ledcAttachPin(gatePin, PWMChannel);
		LowLoad_Off();
		TP4056_Off();
		Load_Off();
		DischargeLed_Off();
	}

	Tester::~Tester()
	{
	}

	void Tester::Setup(ThreadController *controller)
	{
		enabled = false;
		setInterval(2000);
		setState(Initialize);
		controller->add(this);
		_pBattery->setInterval(MMASampleRate);
		controller->add(_pBattery);
		SetChargeCurrent();
	}

	void Tester::Enable()
	{
		setState(Initialize);
	}

	TesterState Tester::getState()
	{
		return _state;
	}

	TesterState Tester::NextState()
	{
		logd("NextState");
		return Monitor;
	}

	void Tester::setState(TesterState state)
	{
		if (_state != state)
		{
			_state = state;
			String mode;
			switch (state)
			{
			case Standby:
				enabled = false;
				TP4056_Off();
				Load_Off();
				LowLoad_Off();
				DischargeLed_Off();
				mode = "Standby";
				break;
			case NoBatteryFound:
				enabled = false;
				TP4056_Off();
				Load_Off();
				LowLoad_Off();
				DischargeLed_Off();
				mode = "NoBatteryFound";
				break;
			case InternalResistance:
				TP4056_Off();
				Load_Off();
				LowLoad_Off();
				enabled = true;
				mode = "InternalResistance";
				break;
			case Charge:
				Load_Off();
				LowLoad_Off();
				TP4056_On();
				DischargeLed_Off();
				SetChargeCurrent();
				enabled = true;
				mode = "Charge";
				break;
			case Discharge:
				_dutyCycle = 128;
				_previousMillis = millis();
				_totalMillis = millis();
				TP4056_Off();
				LowLoad_Off();
				Load_On();
				DischargeLed_On();
				enabled = true;
				mode = "Discharge";
				break;
			case Storage:
				Load_Off();
				LowLoad_Off();
				TP4056_On();
				SetChargeCurrent();
				enabled = true;
				mode = "Storage";
				break;
			case ThermalShutdown:
				enabled = false;
				TP4056_Off();
				LowLoad_Off();
				Load_Off();
				DischargeLed_Off();
				mode = "ThermalShutdown";
				break;
			case Complete:
				enabled = false;
				TP4056_Off();
				LowLoad_Off();
				Load_Off();
				DischargeLed_Off();
				mode = "Complete";
				break;
			case Monitor:
				Load_Off();
				LowLoad_Off();
				TP4056_Off();
				DischargeLed_Off();
				enabled = true;
				mode = "Monitor";
				break;
			case Initialize:
			default:
				_dutyCycle = 64;
				_errorState = Tester_Ok;
				_internalResistance = 0;
				_previousMillis = 0;
				_totalMillis = 0;
				_mAs = 0;
				Load_Off();
				LowLoad_Off();
				TP4056_Off();
				DischargeLed_Off();
				_MaxTemperature = 0;
				_pBattery->Reset();
				_pBattery->Calibrate();
				enabled = false;
				mode = "Initialize";
				break;
			}
			_iot.publish(_batteryPosition, "mode", mode.c_str(), false);
		}
		return;
	}

	void Tester::run()
	{
		runned();
		switch (_state)
		{
		case Initialize:
		{
			uint32_t voc = _pBattery->Voltage();
			if (voc < 2000)
			{ // battery inserted?

				setState(NoBatteryFound);
			}
			else
			{
				setState(Monitor);
			}
		}
		break;
		case Monitor:
		{
			float temp = _pBattery->Temperature();
			logi("Monitor: Battery(%d) %d mV %d mA %2.1f °C", _batteryPosition, _pBattery->Voltage(), _pBattery->ChargeCurrent(), temp / 10);
		}
		break;
		case Charge:
		{
			float temp = _pBattery->Temperature();
			logi("InitialCharge: Battery(%d) %d mV %d mA %2.1f °C", _batteryPosition, _pBattery->Voltage(), _pBattery->ChargeCurrent(), temp / 10);
			if (TP4056_OnStandby())
			{
				setState(NextState());
			}
		}
		break;
		case InternalResistance:
		{
			logi("InternalResistance");
			_pBattery->enabled = false;
			Load_Off();
			LowLoad_On();
			delay(200);
			uint16_t voc = _pBattery->Voltage();
			_pBattery->enabled = false;
			uint16_t iMin = _pBattery->DischargeCurrent();
			LowLoad_Off();
			ledcWrite(PWMChannel, 255); // full load
			delay(200);
			uint32_t iMax = _pBattery->DischargeCurrent();
			_pBattery->enabled = true;
			uint32_t vLoad = _pBattery->Voltage();
			Load_Off();
			_internalResistance = (voc - vLoad);
			_internalResistance *= 1000;
			_internalResistance /= (iMax - iMin);
			logd("voc %d, vLoad %d, iMin %d, iMax %d", voc, vLoad, iMin, iMax);
			logi("InternalResistance Battery(%d), %d", _batteryPosition, _internalResistance);
			if (_internalResistance < 1000)
			{
				setState(NextState());
				_previousMillis = millis();
				_totalMillis = millis();
				_mAs = 0;
			}
		}
		break;
		case Discharge:
		{
			if (_pBattery->Voltage() >= _config.getLowCutoff())
			{
				int32_t current = _pBattery->DischargeCurrent();
				unsigned long t = ((millis() - _previousMillis));
				_mAs += abs(current) * t / 1000;
				_previousMillis = millis();
				if (current > 550 && _dutyCycle > 0)
				{
					--_dutyCycle;
				}
				else if (current < 450 && _dutyCycle < 255)
				{
					_dutyCycle++;
				}
				float temp = _pBattery->Temperature();
				logi("Discharge: Battery(%d) %d mV %d mA %2.1f °C dutyCycle %2.0f %%", _batteryPosition, _pBattery->Voltage(), current, temp / 10, _dutyCycle / 2.55);
			}
			else
			{
				Load_Off();
				DischargeLed_Off();
				_totalMillis = millis() - _totalMillis;
				setState(NextState());
			}
		}
		break;
		case Storage:
		{
			if (_pBattery->Voltage() >= _config.getStorageVoltage())
			{
				setState(NextState());
			}
		}
		break;
		}
		uint32_t temp = _pBattery->Temperature();
		if (temp > _MaxTemperature)
		{
			_MaxTemperature = temp;
			if (temp >= _config.getThermalShutdownTemperature())
			{
				setState(ThermalShutdown);
			}
		}
	}

	// in Seconds
	uint32_t Tester::DischargeTime()
	{
		if (_state == Discharge)
		{
			return (millis() - _totalMillis) / 1000;
		}
		else if (_state == Charge || _state == Complete)
		{
			return _totalMillis / 1000;
		}
		else
		{
			return 0;
		}
	}

	void Tester::TP4056_Off()
	{
		digitalWrite(_tp4056Enable, LOW);
	}

	boolean Tester::TP4056_OnStandby()
	{
		return digitalRead(_tp4056Standby) == LOW;
	}

	void Tester::TP4056_On()
	{
		digitalWrite(_tp4056Enable, HIGH);
	}

	void Tester::Load_Off()
	{
		ledcWrite(PWMChannel, 0);
	}

	void Tester::Load_On()
	{
		ledcWrite(PWMChannel, _dutyCycle);
	}

	void Tester::LowLoad_Off()
	{
		digitalWrite(_lowLoad, HIGH);
	}

	void Tester::LowLoad_On()
	{
		digitalWrite(_lowLoad, LOW);
	}

	void Tester::DischargeLed_Off()
	{
		digitalWrite(_dischargeLed, HIGH);
	}

	void Tester::DischargeLed_On()
	{
		digitalWrite(_dischargeLed, LOW);
	}

	void Tester::SetChargeCurrent()
	{
		uint8_t cc = _config.getChargeCurrent(); // 0:100 mA 1:400 mA 2: 700mA 3:1000 mA
		digitalWrite(_chargeCurrent4k, (cc & 0x01) ? LOW : HIGH);
		digitalWrite(_chargeCurrent2k, (cc & 0x02) ? LOW : HIGH);
	}
} // namespace BatteryTester
