#include <ThreadController.h>
#include <Thread.h>
#include "Constants.h"
#include "IOT.h"
#include "Adafruit_MCP9808.h"
#include "ESP32AnalogRead.h"

extern BatteryTester::IOT _iot;


namespace BatteryTester
{
    class Battery : public Thread
    {
    public:
        Battery(uint8_t highBatPin, uint8_t shuntPin, uint8_t tp4056ProgPin, uint8_t i2cAddress, uint8_t lowLoad);
        ~Battery();
        void run();
        void Reset();
        void MMA();
		boolean CheckForBattery();
        uint16_t Voltage();
        uint16_t OpenVoltage();
        uint16_t ShuntVoltage();
        int16_t ChargeCurrent();
        uint32_t DischargeCurrent();
        uint16_t Temperature();

    private:
        Adafruit_MCP9808 _tempsensor = Adafruit_MCP9808();
        // float Scale(uint32_t v);
		void LowLoad_Off();
		void LowLoad_On();
        
        uint8_t _lowLoad;
        ESP32AnalogRead _highBat;
        ESP32AnalogRead _shunt;
        ESP32AnalogRead _tp4056Prog;
        uint32_t _AcsOffset; // ACS712 vout when no current
        uint32_t _movingAverageChargeCurrent;
        uint32_t _movingAverageSumChargeCurrent;
        uint32_t _movingAverageBatteryVolt;
        uint32_t _movingAverageSumBatteryVolt;
        uint32_t _movingAverageShuntVolt;
        uint32_t _movingAverageSumShuntVolt;

    };

} // namespace BatteryTester