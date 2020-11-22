#include "Tester.h"
#include "Log.h"
#include <math.h>

namespace BatteryTester
{

	State TestCycleSequence[7]{FullCharge, Stabilize, Discharge, Stabilize, FullCharge, Stabilize, CycleConplete};
	State ChargeSequence[4]{FullCharge, Stabilize, InternalResistance, Standby};
	State StorageSequence[4]{Discharge, Stabilize, StorageCharge, Standby};
	State InternalResistanceSequence[2]{InternalResistance, Standby};
	State DischargeSequence[2]{Discharge, Standby};

	Tester::Tester(uint8_t batteryPosition, uint8_t highBatPin, uint8_t shuntPin, uint8_t gatePin, uint8_t tp4056Prog,
				   uint8_t tp4056Enable, uint8_t i2cAddress, uint8_t load10mw, uint8_t tp4056Standby, uint8_t chargeCurrent4k, uint8_t chargeCurrent2k, uint8_t dischargeLed)
	{
		_batteryPosition = batteryPosition;
		_tp4056Enable = tp4056Enable;
		_tp4056Standby = tp4056Standby;
		_chargeCurrent4k = chargeCurrent4k;
		_chargeCurrent2k = chargeCurrent2k;
		_dischargeLed = dischargeLed;
		_pBattery = new Battery(highBatPin, shuntPin, tp4056Prog, i2cAddress, load10mw);
		pinMode(_tp4056Enable, OUTPUT);
		pinMode(_chargeCurrent4k, OUTPUT);
		pinMode(_chargeCurrent2k, OUTPUT);
		pinMode(_dischargeLed, OUTPUT);
		pinMode(_tp4056Standby, INPUT);
		ledcSetup(PWMChannel, PWMfrequency, PWMResolution); //PWM setup
		ledcAttachPin(gatePin, PWMChannel);
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
		setInterval(MonitorRate);
		setState(Initialize);
		controller->add(this);
		_pBattery->setInterval(MMASampleRate);
		controller->add(_pBattery);
		SetChargeCurrent();
	}

	void Tester::Charge()
	{
		_currentOperation = ChargeSequence;
		_currentStage = 0;
		setState(NextState());
	}

	void Tester::Storage()
	{
		_currentOperation = StorageSequence;
		_currentStage = 0;
		setState(NextState());
	}

	void Tester::Cycle()
	{
		_cycleCount = _config.getChargeDischargeCycleCount();
		_currentOperation = TestCycleSequence;
		_currentStage = 0;
		setState(NextState());
	}

	void Tester::MeasureInternalResistance()
	{
		_currentOperation = InternalResistanceSequence;
		_currentStage = 0;
		setState(NextState());
	}

	void Tester::DoDischarge()
	{
		_currentOperation = DischargeSequence;
		_currentStage = 0;
		setState(NextState());
	}

	State Tester::NextState()
	{
		if (_currentOperation != 0)
		{
			State s = _currentOperation[_currentStage++];
			logd("Battery(%d): NextState %s", _batteryPosition, StateText(s));
			return s;
		}
		return Standby;
	}

	const char *Tester::StateText()
	{
		return StateText(_state);
	}

	const char *Tester::StateText(State s)
	{
		switch (s)
		{
		case Standby:
			return "Standby";
		case NoBatteryFound:
			return "NoBatteryFound";
		case InternalResistance:
			return "InternalResistance";
		case FullCharge:
			return "FullCharge";
		case Discharge:
			return "Discharge";
		case StorageCharge:
			return "StorageCharge";
		case ThermalShutdown:
			return "ThermalShutdown";
		case CycleConplete:
			return "CycleConplete";
		case Monitor:
			return "Monitor";
		case Stabilize:
			return "Stabilize";
		case Initialize:
		default:
			return "Unknown";
		}
	}

	void Tester::setState(State state)
	{
		if (_state != state)
		{
			_state = state;
			switch (state)
			{
			case Standby:
				enabled = true;
				_currentStage = 0;
				TP4056_Off();
				Load_Off();
				DischargeLed_Off();
				break;
			case NoBatteryFound:
				enabled = true;
				_currentStage = 0;
				TP4056_Off();
				Load_Off();
				DischargeLed_Off();
				break;
			case InternalResistance:
				TP4056_Off();
				Load_Off();
				DischargeLed_Off();
				enabled = true;
				break;
			case FullCharge:
				_mAs = 0;
				_previousMillis = millis();
				_MaxTemperature = 0;
				Load_Off();
				TP4056_On();
				DischargeLed_Off();
				SetChargeCurrent();
				_totalMillis = millis();
				enabled = true;
				break;
			case Discharge:
				_mAs = 0;
				_MaxTemperature = 0;
				_dutyCycle = 128;
				_previousMillis = millis();
				_totalMillis = millis();
				TP4056_Off();
				Load_On();
				DischargeLed_On();
				enabled = true;
				break;
			case StorageCharge:
				_MaxTemperature = 0;
				Load_Off();
				TP4056_On();
				SetChargeCurrent();
				enabled = true;
				break;
			case ThermalShutdown:
				enabled = false;
				TP4056_Off();
				Load_Off();
				DischargeLed_Off();
				break;
			case CycleConplete:
				enabled = false;
				TP4056_Off();
				Load_Off();
				DischargeLed_Off();
				break;
			case Monitor:
				_MaxTemperature = 0;
				_currentStage = 0;
				Load_Off();
				TP4056_Off();
				DischargeLed_Off();
				enabled = true;
				break;
			case Stabilize:
				_totalMillis = millis();
				_MaxTemperature = 0;
				Load_Off();
				TP4056_Off();
				DischargeLed_Off();
				enabled = true;
				break;
			case Initialize:
			default:
				_dutyCycle = 64;
				_errorState = Tester_Ok;
				_internalResistance = 0;
				_previousMillis = 0;
				_totalMillis = 0;
				_currentOperation = 0;
				_currentStage = 0;
				_mAs = 0;
				Load_Off();
				TP4056_Off();
				DischargeLed_Off();
				_MaxTemperature = 0;
				_pBattery->Reset();
				_pBattery->MMA();
				enabled = false;
				break;
			}
			_iot.publish(_batteryPosition, "mode", StateText(), false);
		}
		return;
	}

	void Tester::BlinkLED()
	{
		if (_blinker)
		{
			_blinker = false;
			DischargeLed_Off();
		}
		else
		{
			_blinker = true;
			DischargeLed_On();
		}
	}
	void Tester::run()
	{
		runned();
		// logd(" Battery(%d): Running state %s", _batteryPosition, StateText());
		switch (_state)
		{
		case Monitor:
		{
			float temp = _pBattery->Temperature();
			logi("Battery(%d) Monitor: %d mV %d mv %d mA %2.1f °C", _batteryPosition, _pBattery->Voltage(), _pBattery->ShuntVoltage(), _pBattery->ChargeCurrent(), temp / 10);
		}
		break;
		case Standby:
		{
			BlinkLED();
		}
		break;
		case NoBatteryFound:
		{
			if (_pBattery->CheckForBattery()) // battery has been inserted
			{
				setState(NextState());
			}
			else
			{
				BlinkLED();
			}
			
		}
		break;
		case Stabilize:
		{
			if (millis() - _totalMillis > _config.getStabilizeDuration() * 1000)
			{
				StaticJsonDocument<1024> doc;
				doc["mode"] = StateText();
				doc["voltage"] = _pBattery->Voltage();
				doc["maxTemperature"] = _MaxTemperature;
				String s;
				serializeJson(doc, s);
				_iot.publish(_batteryPosition, "result", s.c_str(), false);
				setState(NextState());
			}
			float temp = _pBattery->Temperature();
			logi("Battery(%d): Stabilize  %d mV %d mv %d mA %2.1f °C", _batteryPosition, _pBattery->Voltage(), _pBattery->ShuntVoltage(), _pBattery->ChargeCurrent(), temp / 10);
		}
		break;
		case FullCharge:
		{
			unsigned long t = ((millis() - _previousMillis));
			_mAs += _pBattery->ChargeCurrent() * t / 1000;
			_previousMillis = millis();
			float temp = _pBattery->Temperature();
			logi("Battery(%d): FullCharge  %d mV %d mA %2.1f °C", _batteryPosition, _pBattery->Voltage(), _pBattery->ChargeCurrent(), temp / 10);
			if (TP4056_OnStandby())
			{
				TP4056_Off();
				delay(500);
				if (_pBattery->CheckForBattery())
				{
					_totalMillis = millis() - _totalMillis;
					StaticJsonDocument<1024> doc;
					doc["mode"] = StateText();
					doc["charge_time"] = _totalMillis / 1000;
					doc["Energy"] = _mAs / 3600;
					doc["maxTemperature"] = _MaxTemperature;
					String s;
					serializeJson(doc, s);
					_iot.publish(_batteryPosition, "result", s.c_str(), false);
					setState(NextState());
				}
				else
				{
					setState(NoBatteryFound);
				}
			}
		}
		break;
		case InternalResistance:
		{

			_pBattery->enabled = false;
			Load_Off();
			uint16_t voc = _pBattery->OpenVoltage();
			ledcWrite(PWMChannel, 255); // full load
			delay(200);
			_pBattery->MMA();
			uint32_t iMax = _pBattery->DischargeCurrent();
			uint32_t vLoad = _pBattery->Voltage();
			Load_Off();
			_pBattery->enabled = true;
			_internalResistance = (voc - vLoad);
			_internalResistance *= 1000;
			if (iMax <= 0)
			{
				logw("Battery(%d): InternalResistance failed!!! voc %d, vLoad %d, iMax %d", _batteryPosition, voc, vLoad, iMax);
				if (_pBattery->CheckForBattery() == false)
				{
					break;
				}
			}
			else
			{
				_internalResistance /= (iMax);
				logi("Battery(%d): InternalResistance  %d", _batteryPosition, _internalResistance);
				logd("Battery(%d): voc %d, vLoad %d, iMax %d", _batteryPosition, voc, vLoad, iMax);
				StaticJsonDocument<1024> doc;
				doc["mode"] = StateText();
				doc["reading"] = _internalResistance;
				doc["maxTemperature"] = _MaxTemperature;
				String s;
				serializeJson(doc, s);
				_iot.publish(_batteryPosition, "result", s.c_str(), false);
			}
			setState(NextState());
		}
		break;
		case Discharge:
		{
			if (_pBattery->Voltage() >= _config.getLowCutoff())
			{
				int32_t current = _pBattery->DischargeCurrent();
				unsigned long t = ((millis() - _previousMillis));
				_mAs += current * t / 1000;
				_previousMillis = millis();
				if (current > 550 && _dutyCycle > 0)
				{
					// --_dutyCycle;
					_dutyCycle -= 10;
					Load_On();
				}
				else if (current < 450 && _dutyCycle < 255)
				{
					// _dutyCycle++;
					_dutyCycle += 10;
					Load_On();
				}
				float temp = _pBattery->Temperature();
				logi("Discharge: Battery(%d) %d mV %d mV %d mA %2.1f °C dutyCycle %2.0f %%", _batteryPosition, _pBattery->Voltage(), _pBattery->ShuntVoltage(), current, temp / 10, _dutyCycle / 2.55);
			}
			else
			{

				Load_Off();
				DischargeLed_Off();
				if (_pBattery->CheckForBattery())
				{
					_totalMillis = millis() - _totalMillis;
					StaticJsonDocument<1024> doc;
					doc["mode"] = StateText();
					doc["cischarge_Time"] = _totalMillis / 1000;
					doc["capacity"] = _mAs / 3600;
					doc["maxTemperature"] = _MaxTemperature;
					String s;
					serializeJson(doc, s);
					_iot.publish(_batteryPosition, "result", s.c_str(), false);
					logi(" Battery(%d): Discharge done! duration %d capacity %d", _batteryPosition, _totalMillis, _mAs / 3600);
					setState(NextState());
				}
			}
		}
		break;
		case StorageCharge:
		{
			if (_pBattery->Voltage() >= _config.getStorageVoltage())
			{
				StaticJsonDocument<1024> doc;
				doc["mode"] = StateText();
				doc["voltage"] = _pBattery->Voltage();
				doc["maxTemperature"] = _MaxTemperature;
				String s;
				serializeJson(doc, s);
				_iot.publish(_batteryPosition, "result", s.c_str(), false);
				setState(NextState());
			}
		}
		break;
		case CycleConplete:
		{
			_cycleCount--;
			if (_cycleCount > 0)
			{
				StaticJsonDocument<1024> doc;
				doc["mode"] = StateText();
				doc["voltage"] = _pBattery->Voltage();
				doc["cycle"] = _cycleCount;
				doc["maxTemperature"] = _MaxTemperature;
				String s;
				serializeJson(doc, s);
				_iot.publish(_batteryPosition, "result", s.c_str(), false);
				_currentStage = 0; // start over
				setState(NextState());
			}
			setState(Standby);
		}
		break;
		}
		uint16_t temp = _pBattery->Temperature();
		if (temp < 1000) // skip bad MCP9808 readings?
		{
			if (temp > _MaxTemperature)
			{
				_MaxTemperature = temp;
			}
			if (temp >= _config.getThermalShutdownTemperature())
			{
				logd("Battery(%d): ThermalShutdown at %d", _batteryPosition, temp);
				setState(ThermalShutdown);
				StaticJsonDocument<1024> doc;
				doc["mode"] = StateText();
				doc["voltage"] = _pBattery->Voltage();
				doc["maxTemperature"] = _MaxTemperature;
				String s;
				serializeJson(doc, s);
				_iot.publish(_batteryPosition, "result", s.c_str(), false);
			}
		}
		if (_pBattery->CheckForBattery() == false)
		{
			setState(NoBatteryFound);
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
