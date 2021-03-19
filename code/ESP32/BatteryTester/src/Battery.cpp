#include "Battery.h"
#include "Log.h"

namespace BatteryTester
{
	Battery::Battery(uint8_t highBatPin, uint8_t shuntPin, uint8_t tp4056ProgPin, uint8_t i2cAddress, uint8_t lowLoad)
	{
		_lowLoad = lowLoad;
		pinMode(tp4056ProgPin, INPUT);
		pinMode(highBatPin, INPUT_PULLUP);
		pinMode(shuntPin, INPUT);
		pinMode(_lowLoad, OUTPUT_OPEN_DRAIN);
		LowLoad_On();
		_tempsensor.begin(i2cAddress);
		_highBat.attach(highBatPin);
		_shunt.attach(shuntPin);
		_tp4056Prog.attach(tp4056ProgPin);
	}

	Battery::~Battery()
	{
	}

	void Battery::Reset()
	{
		_movingAverageChargeCurrent = 0;
		_movingAverageSumChargeCurrent = 0;
		_movingAverageBatteryVolt = 0;
		_movingAverageSumBatteryVolt = 0;
		_movingAverageShuntVolt = 0;
		_movingAverageSumShuntVolt = 0;
		_tempsensor.setResolution(MCP9808Resolution); // 0.0625°C 250 ms
	}

	void Battery::LowLoad_Off()
	{
		digitalWrite(_lowLoad, HIGH);
	}

	void Battery::LowLoad_On()
	{
		digitalWrite(_lowLoad, LOW);
	}

	boolean Battery::CheckForBattery()
	{
		return OpenVoltage() >= MinimumBatteryVoltageForDetection;
	}

	// float Battery::Scale(uint32_t reading)
	// {
	// 	if (reading < 1 || reading > ADC_Resolution)
	// 	{
	// 		return 0;
	// 	}
	// 	// The constants used in this calculation are taken from
	// 	// https://github.com/G6EJD/ESP32-ADC-Accuracy-Improvement-function
	// 	// and improves the default ADC reading accuracy to within 1%.
	// 	float sensorReading = -0.000000000000016 * pow(reading, 4) + 0.000000000118171 * pow(reading, 3) - 0.000000301211691 * pow(reading, 2) + 0.001109019271794 * reading + 0.034143524634089;
	// 	return sensorReading;
	// }

	uint16_t Battery::OpenVoltage()
	{
		LowLoad_On();
		delay(100);
		MMA();
		LowLoad_Off();
		return Voltage();
	}

	// in mV
	uint16_t Battery::Voltage()
	{
		// float f = Scale(_movingAverageBatteryVolt) * 25000 / 15; // 10k and 15k divider * 1000 mV
		// return f + 0.5;											 // round float by adding 0.5 before cast
		return _movingAverageBatteryVolt * 25 / 15;
	}

	uint16_t Battery::ShuntVoltage()
	{
		// float f = Scale(_movingAverageShuntVolt) * 25000 / 15; // 10k and 15k divider * 1000 mV
		// return f + 0.5;										   // round float by adding 0.5 before cast
		return _movingAverageShuntVolt * 25 / 15;
	}

	// TP4056 Prog pin (in mA)
	int16_t Battery::ChargeCurrent()
	{
		// float f = Scale(_movingAverageChargeCurrent) * 1000; //mA
		// return f + 0.5;										 // round float by adding 0.5 before cast
		return _movingAverageChargeCurrent;
	}

	// Shunt voltage delta / 1Ω (in mA)
	uint32_t Battery::DischargeCurrent()
	{
		if (Voltage() > ShuntVoltage())
			return Voltage() - ShuntVoltage();
		else
			return 0;
	}

	// °C * 10
	uint16_t Battery::Temperature()
	{
		try
		{
			_tempsensor.wake();
			float c = _tempsensor.readTempC();
			return c * 10; // °C X 10 as uint16
		}
		catch  (const std::exception& ex)
		{
			loge("Exception from MCP9808: %s", ex.what());
		}

	}

	void Battery::run()
	{
		runned();
		MMA();
	}

	void Battery::MMA()
	{
		unsigned long timeStamp = millis();
		while (millis() - timeStamp < MMASamplePeriod)
		{
			// charge current
			_movingAverageSumChargeCurrent = _movingAverageSumChargeCurrent - _movingAverageChargeCurrent; // Remove previous sample from the sum
			_movingAverageSumChargeCurrent = _movingAverageSumChargeCurrent + _tp4056Prog.readMiliVolts(); // Replace it with the current sample
			_movingAverageChargeCurrent = _movingAverageSumChargeCurrent / AverageCount;				   // Recalculate moving average
			//Battery volts
			_movingAverageSumBatteryVolt = _movingAverageSumBatteryVolt - _movingAverageBatteryVolt;
			_movingAverageSumBatteryVolt = _movingAverageSumBatteryVolt + _highBat.readMiliVolts();
			;
			_movingAverageBatteryVolt = _movingAverageSumBatteryVolt / AverageCount;
			// //Shunt volts
			_movingAverageSumShuntVolt = _movingAverageSumShuntVolt - _movingAverageShuntVolt;
			_movingAverageSumShuntVolt = _movingAverageSumShuntVolt + _shunt.readMiliVolts();
			;
			_movingAverageShuntVolt = _movingAverageSumShuntVolt / AverageCount;
		}
	}

} // namespace BatteryTester