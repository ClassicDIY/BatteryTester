#define ESPVoltageRef 3300	 // ESP32 ref voltage in mV

#define AverageCount 1000 //MMA modified moving average count
#define MMASampleRate 2000 // mS
#define MMASamplePeriod 500 // mS
#define ADC_Resolution 4095.0 // ESP32 ADC resolution

#define LowCutoff 2900 // discharge cutoff in mV, factory default
#define StorageVoltage 3700 // factory default storage voltage
#define ThermalShutdownTemperature 450 // Thermal shutdown factory setting
#define PWMfrequency 1000000 // discharger frequency
#define PWMChannel 0
#define PWMResolution 8 // duty cycle 0 -> 255
#define MaxMQTTPayload 255
#define MaxMQTTTopic 64
#define DefaultChargeCurrent 3 // charge current factory setting

#define MCP9808Resolution 3 // 0.0625°C 250 ms
#define MCP9808_1 0x18 // Battery 1
#define MCP9808_2 0x19 // Battery 2
