#include <ThreadController.h>
#include <Thread.h>
#include "Constants.h"

namespace BatteryTester
{
    class Battery : public Thread
    {
    public:
        Battery(uint8_t highBatPin, uint8_t shuntPin, uint8_t tp4056Prog, uint8_t thermistorPin);
        ~Battery();
        void run();
        void Reset();
        void Calibrate();

        uint16_t Voltage();
        int16_t ChargeCurrent();
        uint32_t DischargeCurrent();
        uint16_t Temperature();

    private:
        float Scale(uint32_t v);
        uint8_t _highBatPin;
        uint8_t _shuntPin;
        uint8_t _tp4056Prog;
        uint8_t _thermistorPin;
        uint32_t _AcsOffset; // ACS712 vout when no current
        uint32_t _movingAverageChargeCurrent;
        uint32_t _movingAverageSumChargeCurrent;
        uint32_t _movingAverageTemperature;
        uint32_t _movingAverageSumTemperature;
        uint32_t _movingAverageBatteryVolt;
        uint32_t _movingAverageSumBatteryVolt;
        uint32_t _movingAverageShuntVolt;
        uint32_t _movingAverageSumShuntVolt;
    };

} // namespace BatteryTester