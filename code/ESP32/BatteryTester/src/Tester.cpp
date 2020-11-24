#include "Tester.h"
#include "Log.h"
#include <math.h>

namespace BatteryTester
{

	State TestCycleSequence[7]{FullCharge, Stabilize, Discharge, Stabilize, FullCharge, Stabilize, CycleConplete};
	State ChargeSequence[4]{FullCharge, Stabilize, InternalResistance, Standby};
	State TestAndStoreSequence[7]{FullCharge, Stabilize, InternalResistance, Discharge, Stabilize, StorageCharge, Standby};
	State TestAndChargeSequence[7]{FullCharge, Stabilize, InternalResistance, Discharge, Stabilize, FullCharge, Standby};
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
		_pBattery->enabled = false;
		controller->add(this);
		_pBattery->setInterval(MMASampleRate);
		controller->add(_pBattery);
		setInterval(MonitorRate);
		setState(Initialize);
		SetChargeCurrent();
	}

	void Tester::Perform(Operation op)
	{
		switch (op)
		{
		case TestCycleOperation:
			_cycleCount = _config.getChargeDischargeCycleCount();
			_currentOperation = TestCycleSequence;
			break;
		case ChargeOperation:
			_currentOperation = ChargeSequence;
			break;
		case TestAndStoreOperation:
			_currentOperation = TestAndStoreSequence;
			break;
		case TestAndChargeOperation:
			_currentOperation = TestAndChargeSequence;
			break;
		case StorageOperation:
			_currentOperation = StorageSequence;
			break;
		case InternalResistanceOperation:
			_currentOperation = InternalResistanceSequence;
			break;
		case DischargeOperation:
			_currentOperation = DischargeSequence;
			break;
		case NoOp:
		default:
			return;
		}
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
			return "Initialize";
		default:
			return "Unknown";
		}
	}

	void Tester::setState(State state)
	{
		if (_state != state)
		{
			logd("Battery(%d): SetState %s", _batteryPosition, StateText(state));
			_state = state;
			_timeStamp = millis(); // state change time
			_mAs = 0;
			_modulo = -1;
			switch (state)
			{
			case Standby:
				enabled = true;
				_pBattery->enabled = true;
				_currentStage = 0;
				TP4056_Off();
				Load_Off();
				DischargeLed_Off();
				break;
			case NoBatteryFound:
				enabled = true;
				_pBattery->enabled = true;
				_currentStage = 0;
				TP4056_Off();
				Load_Off();
				DischargeLed_Off();
				break;
			case InternalResistance:
				TP4056_Off();
				Load_Off();
				DischargeLed_Off();
				break;
			case FullCharge:
				_MaxTemperature = 0;
				Load_Off();
				TP4056_On();
				DischargeLed_Off();
				SetChargeCurrent();
				break;
			case Discharge:
				_MaxTemperature = 0;
				_dutyCycle = 128;
				TP4056_Off();
				Load_On();
				DischargeLed_On();
				break;
			case StorageCharge:
				_MaxTemperature = 0;
				Load_Off();
				TP4056_On();
				SetChargeCurrent();
				break;
			case ThermalShutdown:
				TP4056_Off();
				Load_Off();
				DischargeLed_Off();
				break;
			case CycleConplete:
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
				break;
			case Stabilize:
				_MaxTemperature = 0;
				Load_Off();
				TP4056_Off();
				DischargeLed_Off();
				break;
			case Initialize:
			default:
				_dutyCycle = 64;
				_errorState = Tester_Ok;
				_internalResistance = 0;
				_currentOperation = 0;
				_currentStage = 0;
				Load_Off();
				TP4056_Off();
				DischargeLed_Off();
				_MaxTemperature = 0;
				_pBattery->Reset();
				_pBattery->MMA();
				enabled = false;
				_pBattery->enabled = false;
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

	void Tester::MQTTMonitor()
	{
		_modulo = ++_modulo % 10;
		if (_modulo == 0)
		{
			float temp = _pBattery->Temperature();
			if (temp < 1000) // skip bad MCP9808 readings?
			{
				uint16_t current = (_state == Discharge) ? _pBattery->DischargeCurrent() : _pBattery->ChargeCurrent();
				_doc["mode"] = StateText();
				_doc["voltage"] = _pBattery->Voltage();
				_doc["current"] = current;
				_doc["temp"] = temp / 10;
				String s;
				serializeJson(_doc, s);
				_iot.publish(_batteryPosition, "monitor", s.c_str(), false);
			}
		}
	}

	void Tester::run()
	{
		runned();

		switch (_state)
		{
		case Monitor:
		case ThermalShutdown:
		{
			float temp = _pBattery->Temperature();
			logd("Battery(%d) Monitor: %d mV %d mv %d mA %2.1f °C", _batteryPosition, _pBattery->Voltage(), _pBattery->ShuntVoltage(), _pBattery->ChargeCurrent(), temp / 10);
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
			unsigned long t = millis() - _timeStamp;
			MQTTMonitor();
			if (t > _config.getStabilizeDuration() * 1000)
			{
				_doc["mode"] = StateText();
				_doc["voltage"] = _pBattery->Voltage();
				_doc["maxTemperature"] = _MaxTemperature;
				_doc["duration"] = t / 1000;
				String s;
				serializeJson(_doc, s);
				_iot.publish(_batteryPosition, "result", s.c_str(), false);
				setState(NextState());
			}
		}
		break;
		case FullCharge:
		{
			unsigned long t = millis() - _timeStamp;
			_mAs += _pBattery->ChargeCurrent() * t / 1000;
			MQTTMonitor();
			if (TP4056_OnStandby())
			{
				TP4056_Off();
				delay(500);
				if (_pBattery->CheckForBattery())
				{
					_doc["mode"] = StateText();
					_doc["Energy"] = _mAs / 3600;
					_doc["maxTemperature"] = _MaxTemperature;
					_doc["duration"] = t / 1000;
					String s;
					serializeJson(_doc, s);
					_iot.publish(_batteryPosition, "result", s.c_str(), false);
					setState(NextState());
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
				logd("Battery(%d): InternalResistance  %d", _batteryPosition, _internalResistance);
				logd("Battery(%d): voc %d, vLoad %d, iMax %d", _batteryPosition, voc, vLoad, iMax);
				_doc["mode"] = StateText();
				_doc["reading"] = _internalResistance;
				_doc["maxTemperature"] = _MaxTemperature;
				String s;
				serializeJson(_doc, s);
				_iot.publish(_batteryPosition, "result", s.c_str(), false);
			}
			setState(NextState());
		}
		break;
		case Discharge:
		{
			unsigned long t = millis() - _timeStamp;
			MQTTMonitor();
			if (_pBattery->Voltage() >= _config.getLowCutoff())
			{
				int32_t current = _pBattery->DischargeCurrent();
				_mAs += current * t / 1000;
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
			}
			else
			{
				Load_Off();
				DischargeLed_Off();
				if (_pBattery->CheckForBattery())
				{
					_doc["mode"] = StateText();
					_doc["capacity"] = _mAs / 3600;
					_doc["maxTemperature"] = _MaxTemperature;
					_doc["duration"] = t / 1000;
					String s;
					serializeJson(_doc, s);
					_iot.publish(_batteryPosition, "result", s.c_str(), false);
					logd(" Battery(%d): Discharge done! duration %d capacity %d", _batteryPosition, t / 1000, _mAs / 3600);
					setState(NextState());
				}
			}
		}
		break;
		case StorageCharge:
		{
			MQTTMonitor();
			if (_pBattery->Voltage() >= _config.getStorageVoltage())
			{
				unsigned long t = millis() - _timeStamp;
				_doc["mode"] = StateText();
				_doc["voltage"] = _pBattery->Voltage();
				_doc["maxTemperature"] = _MaxTemperature;
				_doc["duration"] = t / 1000;
				String s;
				serializeJson(_doc, s);
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
				unsigned long t = millis() - _timeStamp;
				_doc["mode"] = StateText();
				_doc["voltage"] = _pBattery->Voltage();
				_doc["cycle"] = _cycleCount;
				_doc["maxTemperature"] = _MaxTemperature;
				_doc["duration"] = t / 1000;
				String s;
				serializeJson(_doc, s);
				_iot.publish(_batteryPosition, "result", s.c_str(), false);
				_currentStage = 0; // start over
				setState(NextState());
			}
			setState(Standby);
		}
		break;
		}
		if (_pBattery->CheckForBattery() == false)
		{
			setState(NoBatteryFound);
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
				logw("Battery(%d): ThermalShutdown at %d", _batteryPosition, temp);
				setState(ThermalShutdown);
				_doc["mode"] = StateText();
				_doc["voltage"] = _pBattery->Voltage();
				_doc["maxTemperature"] = _MaxTemperature;
				String s;
				serializeJson(_doc, s);
				_iot.publish(_batteryPosition, "result", s.c_str(), false);
			}
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
