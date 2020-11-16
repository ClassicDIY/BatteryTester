#include "Battery.h"
#include "Log.h"

namespace BatteryTester
{

	Battery::Battery(uint8_t highBatPin, uint8_t shuntPin, uint8_t tp4056Prog, uint8_t thermistorPin)
	{
		_highBatPin = highBatPin;
		_shuntPin = shuntPin;
		_tp4056Prog = tp4056Prog;
		_thermistorPin = thermistorPin;
		pinMode(_tp4056Prog, INPUT);
		pinMode(_highBatPin, INPUT);
		pinMode(_shuntPin, INPUT);
		pinMode(_thermistorPin, INPUT);
		Reset();
	}

	Battery::~Battery()
	{
	}

	void Battery::Reset()
	{
		_movingAverageChargeCurrent = 0;
		_movingAverageSumChargeCurrent = 0;
		_movingAverageTemperature = 0;
		_movingAverageSumTemperature = 0;
		_movingAverageBatteryVolt = 0;
		_movingAverageSumBatteryVolt = 0;
		_movingAverageShuntVolt = 0;
        _movingAverageSumShuntVolt = 0;
	}

	void Battery::Calibrate()
	{
		int count = AverageCount*5;
		while (--count != 0) //Get 5 * AverageCount 
		{													 
			run();
			delay(3); // let ADC settle before next sample 3ms
		}
	}

	float Battery::Scale(uint32_t reading)
	{
		if (reading < 1 || reading > ADC_Resolution)
		{
			return 0;
		}
		// The constants used in this calculation are taken from
		// https://github.com/G6EJD/ESP32-ADC-Accuracy-Improvement-function
		// and improves the default ADC reading accuracy to within 1%.
		float sensorReading = -0.000000000000016 * pow(reading, 4) + 0.000000000118171 * pow(reading, 3) - 0.000000301211691 * pow(reading, 2) + 0.001109019271794 * reading + 0.034143524634089;
		return sensorReading;
	}

		// in mV
	uint16_t Battery::Voltage()
	{
		float f = Scale(_movingAverageBatteryVolt) * 25000 / 15; // 10k and 15k divider * 1000 mV
		return f + 0.5;										// round float by adding 0.5 before cast
	}

	// TP4056 Prog pin (in mA)
	int16_t Battery::ChargeCurrent()
	{
		float f = Scale(_movingAverageChargeCurrent) * 1000; //mA
		return f + 0.5; // round float by adding 0.5 before cast
	}

	// Shunt voltage / 1Ω (in mA)
	uint32_t Battery::DischargeCurrent()
	{
		_movingAverageBatteryVolt = 0;
		_movingAverageSumBatteryVolt = 0;
		_movingAverageShuntVolt = 0;
		_movingAverageSumShuntVolt = 0;
		unsigned long timeStamp = millis();
		int i = 0;
		while (millis() - timeStamp < 200)
		{
			_movingAverageSumBatteryVolt = _movingAverageSumBatteryVolt - _movingAverageBatteryVolt;
			_movingAverageSumBatteryVolt = _movingAverageSumBatteryVolt + analogRead(_highBatPin);
			_movingAverageBatteryVolt = _movingAverageSumBatteryVolt / AverageCount;
			_movingAverageSumShuntVolt = _movingAverageSumShuntVolt - _movingAverageShuntVolt;
			_movingAverageSumShuntVolt = _movingAverageSumShuntVolt + analogRead(_shuntPin);
			_movingAverageShuntVolt = _movingAverageSumShuntVolt / AverageCount;
			i++;
		}
		return _movingAverageBatteryVolt - _movingAverageShuntVolt;
	}

	// °C * 10
	uint16_t Battery::Temperature()
	{
		float vsens = Scale(_movingAverageTemperature) * 1000; // vsens in mV
		float rtherm = RSeries * vsens / (ESPVoltageRef - vsens);
		float steinhart;
		steinhart = rtherm / THERMISTORNOMINAL;			  // (R/Ro)
		steinhart = log(steinhart);						  // ln(R/Ro)
		steinhart /= BCOEFFICIENT;						  // 1/B * ln(R/Ro)
		steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
		steinhart = 1.0 / steinhart;					  // Invert
		steinhart -= 273.15;
		return steinhart * 10; // °C X 10 as uint16
	}

	void Battery::run()
	{
		_movingAverageSumTemperature = _movingAverageSumTemperature - _movingAverageTemperature;  // Remove previous sample from the sum
		_movingAverageSumTemperature = _movingAverageSumTemperature + analogRead(_thermistorPin); // Replace it with the current sample
		_movingAverageTemperature = _movingAverageSumTemperature / AverageCount;				  // Recalculate moving average
		// charge current
		_movingAverageSumChargeCurrent = _movingAverageSumChargeCurrent - _movingAverageChargeCurrent;
		_movingAverageSumChargeCurrent = _movingAverageSumChargeCurrent + analogRead(_tp4056Prog);
		_movingAverageChargeCurrent = _movingAverageSumChargeCurrent / AverageCount;
		//Battery volts
		_movingAverageSumBatteryVolt = _movingAverageSumBatteryVolt - _movingAverageBatteryVolt;
		_movingAverageSumBatteryVolt = _movingAverageSumBatteryVolt + analogRead(_highBatPin);
		_movingAverageBatteryVolt = _movingAverageSumBatteryVolt / AverageCount;
		// //Shunt volts
		_movingAverageSumShuntVolt = _movingAverageSumShuntVolt - _movingAverageShuntVolt;
		_movingAverageSumShuntVolt = _movingAverageSumShuntVolt + analogRead(_shuntPin);
		_movingAverageShuntVolt = _movingAverageSumShuntVolt / AverageCount;
	}

} // namespace BatteryTester