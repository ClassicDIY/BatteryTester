#define ESPVoltageRef 3300	 // ESP32 ref voltage in mV
#define RSeries 10000 // resistor in series with thermistor
#define AverageCount 32 //MMA modified moving average count
#define MMASampleRate 200 // mS
#define ADC_Resolution 4095.0 // ESP32 ADC resolution
#define THERMISTORNOMINAL 10000 // thermistor resistance nominal in Ω
#define TEMPERATURENOMINAL 25 // temp. for nominal resistance (almost always 25 C)
#define BCOEFFICIENT 3700 // The beta coefficient of the thermistor (usually 3000-4000)
#define LowCutoff 2900 // discharge cutoff in mV, factory default
#define StorageVoltage 3700 // factory default storage voltage
#define ThermalShutdownTemperature 450 // Thermal shutdown factory setting
#define PWMfrequency 1000000 // discharger frequency
#define PWMChannel 0
#define PWMResolution 8 // duty cycle 0 -> 255
#define MaxMQTTPayload 255
#define MaxMQTTTopic 64
#define DefaultChargeCurrent 3 // charge current factory setting
