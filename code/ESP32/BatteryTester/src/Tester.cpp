#include "Tester.h"
#include "Log.h"
#include <math.h>

namespace BatteryTester
{

	State TestCycleSequence[8]{Begin, FullCharge, Stabilize, Discharge, Stabilize, FullCharge, Stabilize, CycleConplete};
	State ChargeSequence[5]{Begin, FullCharge, Stabilize, InternalResistance, Complete};
	State TestAndStoreSequence[8]{Begin, FullCharge, Stabilize, InternalResistance, Discharge, Stabilize, StorageCharge, Complete};
	State TestAndChargeSequence[8]{Begin, FullCharge, Stabilize, InternalResistance, Discharge, Stabilize, FullCharge, Complete};
	State StorageSequence[5]{Begin, Discharge, Stabilize, StorageCharge, Complete};
	State InternalResistanceSequence[3]{Begin, InternalResistance, Complete};
	State DischargeSequence[3]{Begin, Discharge, Complete};
	State MonitorSequence[2]{Begin, Monitor};

	Tester::Tester(uint8_t batteryPosition, uint8_t highBatPin, uint8_t shuntPin, uint8_t gatePin, uint8_t tp4056Prog,
				   uint8_t tp4056Enable, uint8_t i2cAddress, uint8_t load10mw, uint8_t tp4056Standby, uint8_t chargeCurrent4k, uint8_t chargeCurrent2k, uint8_t dischargeLed)
	{
		_batteryPosition = batteryPosition;
		_pwmChannel = batteryPosition * 2; //pwm channel 0 or 2
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
		ledcSetup(_pwmChannel, PWMfrequency, PWMResolution); //PWM setup, use channel 0 or 1 equal to the battery position
		ledcAttachPin(gatePin, _pwmChannel);
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
		_operation = op;
		switch (op)
		{
		case TestCycleOperation:
			_cycleCount = _config.getChargeDischargeCycleCount();
			_currentOperationArray = TestCycleSequence;
			break;
		case ChargeOperation:
			_currentOperationArray = ChargeSequence;
			break;
		case TestAndStoreOperation:
			_currentOperationArray = TestAndStoreSequence;
			break;
		case TestAndChargeOperation:
			_currentOperationArray = TestAndChargeSequence;
			break;
		case StorageOperation:
			_currentOperationArray = StorageSequence;
			break;
		case InternalResistanceOperation:
			_currentOperationArray = InternalResistanceSequence;
			break;
		case DischargeOperation:
			_currentOperationArray = DischargeSequence;
			break;
		case MonitorOperation:
			_currentOperationArray = MonitorSequence;
			break;
		default:
			return;
		}
		_currentStage = 0;
		setState(NextState());
	}

	State Tester::NextState()
	{
		if (_currentOperationArray != 0)
		{
			State s = _currentOperationArray[_currentStage++];
			logd("Battery(%d): NextState %s", _batteryPosition, States[s]);
			return s;
		}
		return Standby;
	}

	void Tester::setState(State state)
	{
		if (_state != state)
		{
			logd("Battery(%d): SetState %s", _batteryPosition, States[state]);
			_state = state;
			_stateChangeTimeStamp = millis(); // state change time
			_lastReadingTimeStamp = millis();
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
				Load_Off();
				TP4056_On();
				DischargeLed_Off();
				SetChargeCurrent();
				break;
			case Discharge:
				_dutyCycle = 128;
				TP4056_Off();
				Load_On();
				DischargeLed_On();
				break;
			case StorageCharge:
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
				_currentStage = 0;
				Load_Off();
				TP4056_Off();
				DischargeLed_Off();
				break;
			case Stabilize:
				Load_Off();
				TP4056_Off();
				DischargeLed_Off();
				break;
			case Initialize:
				_dutyCycle = 64;
				_errorState = Tester_Ok;
				_currentOperationArray = 0;
				_currentStage = 0;
				Load_Off();
				TP4056_Off();
				DischargeLed_Off();
				_pBattery->Reset();
				_pBattery->MMA();
				enabled = false;
				_pBattery->enabled = false;
				break;
			}
			StaticJsonDocument<MaxMQTTPayload> doc;
			doc[Elements[Id::state]] = States[_state];
			_iot.publish(_batteryPosition, Subtopics[Subtopic::mode], &doc, false);
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

	void Tester::PublishUpdate()
	{
		uint16_t temp = _pBattery->Temperature();
		StaticJsonDocument<MaxMQTTPayload> doc;
		uint16_t current = (_state == Discharge) ? _pBattery->DischargeCurrent() : (_state == FullCharge || _state == StorageCharge) ? _pBattery->ChargeCurrent() : 0;
		doc[Elements[Id::state]] = States[_state];
		doc[Elements[Id::voltage]] = _pBattery->Voltage();
		doc[Elements[Id::current]] = current;
		doc[Elements[Id::temperature]] = temp;
		doc[Elements[Id::stage]] = _currentStage;
		doc[Elements[Id::maxTemperature]] = _MaxTemperature;
		doc[Elements[Id::duration]] = _duration == 0 ? millis() - _operationTimeStamp : _duration;
		_iot.publish(_batteryPosition, Subtopics[Subtopic::update], &doc, false);
	}
	
	// publish result of test with retain on
	void Tester::PublishOutcome()
	{
		StaticJsonDocument<MaxMQTTPayload> doc;
		if (_cyclesCompleted > 1)
		{
			doc[Elements[Id::capacity]] = _capacity / _cyclesCompleted;
			doc[Elements[Id::internalResistance]] = _internalResistanceSummation / _cyclesCompleted;
		}
		else
		{
			doc[Elements[Id::capacity]] = _capacity;
			doc[Elements[Id::internalResistance]] = _internalResistanceSummation;
		}
		_iot.publish(_batteryPosition, Subtopics[Subtopic::outcome], &doc, true);
	}

	void Tester::MQTTMonitor()
	{
		_modulo++;
		_modulo %= 10;
		if (_modulo == 0)
		{
			PublishUpdate();
		}
	}

	void Tester::run()
	{
		runned();
		if (_pBattery->CheckForBattery() == false)
		{
			setState(NoBatteryFound);
		}
		uint16_t temp = _pBattery->Temperature();
		if (temp < 1000) // skip bad MCP9808 readings?
		{
			if (temp > _MaxTemperature) //max temperature of current function
			{
				_MaxTemperature = temp;
			}
			if (temp >= _config.getThermalShutdownTemperature())
			{
				logw("Battery(%d): ThermalShutdown at %d", _batteryPosition, temp);
				setState(ThermalShutdown);
			}
		}
		switch (_state)
		{
		case Monitor:
		{
			MQTTMonitor();
		}
		break;
		case ThermalShutdown:
		{
			float temp = _pBattery->Temperature();
			logd("Battery(%d) Monitor: %d mV %d mv %d mA %2.1f °C", _batteryPosition, _pBattery->Voltage(), _pBattery->ShuntVoltage(), _pBattery->ChargeCurrent(), temp / 10);
		}
		break;
		case Begin:
		{
			// initialize variables for operation
			_MaxTemperature = 0;
			_internalResistance = 0;
			_cyclesCompleted = 0;
			_capacity = 0;
			_duration = 0;
			_internalResistanceSummation = 0;
			PublishOutcome(); // clear retain data for new test run
			_operationTimeStamp = millis();
			setState(NextState());
			break;
		}
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
			unsigned long t = millis() - _stateChangeTimeStamp;
			MQTTMonitor();
			if (t > _config.getStabilizeDuration() * 1000)
			{
				setState(NextState());
			}
		}
		break;
		case FullCharge:
		{
			uint16_t c = _pBattery->ChargeCurrent();
			unsigned long sinceLastReading = millis() - _lastReadingTimeStamp;
			_mAs += c * sinceLastReading / 1000;
			_lastReadingTimeStamp = millis();
			MQTTMonitor();
			if (TP4056_OnStandby())
			{
				TP4056_Off();
				delay(500);
				if (_pBattery->CheckForBattery())
				{
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
			ledcWrite(_pwmChannel, 255); // full load
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
				_internalResistanceSummation += _internalResistance;
			}
			setState(NextState());
		}
		break;
		case Discharge:
		{
			MQTTMonitor();
			if (_pBattery->Voltage() >= _config.getLowCutoff())
			{
				int32_t current = _pBattery->DischargeCurrent();
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
				unsigned long sinceLastReading = millis() - _lastReadingTimeStamp;
				_mAs += _pBattery->DischargeCurrent() * sinceLastReading / 1000;
				_lastReadingTimeStamp = millis();
			}
			else
			{
				Load_Off();
				DischargeLed_Off();
				if (_pBattery->CheckForBattery())
				{
					_capacity = _mAs / 3600;
					logd(" Battery(%d): Discharge done! duration %d capacity %d", _batteryPosition, duration, _capacity);
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
				setState(NextState());
			}
		}
		break;
		case CycleConplete:
		{
			_cycleCount--;
			if (_cycleCount > 0)
			{
				_currentStage = 1; // start over, skip initialize
				_cyclesCompleted++;
				setState(NextState());
			}
			else
			{
				setState(Complete);
			}
		}
		break;
		case Complete:
		{
			_cyclesCompleted++;
			_duration = millis() - _operationTimeStamp;
			PublishUpdate();
			setState(Standby);
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
		ledcWrite(_pwmChannel, 0);
	}

	void Tester::Load_On()
	{
		ledcWrite(_pwmChannel, _dutyCycle);
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
		// digitalWrite(_chargeCurrent4k, (cc & 0x01) ? LOW : HIGH);
		// digitalWrite(_chargeCurrent2k, (cc & 0x02) ? LOW : HIGH);
		if (cc & 0x01)
		{
			pinMode(_chargeCurrent4k, OUTPUT);
			digitalWrite(_chargeCurrent4k, LOW);
		}
		else
		{
			pinMode(_chargeCurrent4k, INPUT);
		}
		if (cc & 0x02)
		{
			pinMode(_chargeCurrent2k, OUTPUT);
			digitalWrite(_chargeCurrent2k, LOW);
		}
		else
		{
			pinMode(_chargeCurrent2k, INPUT);
		}
	}


} // namespace BatteryTester
