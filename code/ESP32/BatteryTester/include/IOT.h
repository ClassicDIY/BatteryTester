#pragma once

#include "WiFi.h"
#include "ArduinoJson.h"
extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}
#include "AsyncMqttClient.h"
#include "IotWebConf.h"
#include "Configuration.h"
#include "Constants.h"

#define STR_LEN 64    // general string buffer size

extern BatteryTester::Configuration _config;

namespace BatteryTester
{

    typedef void (*IOTEventCb)();
    class IOT
    {
    public:
        IOT();
        void Init();
        void Run();
        void publish(uint8_t pos, const char *subtopic, const char *value, boolean retained = false);

    private:
        void SetupWifi(const char *ssid, const char *pw);
        bool _clientsConfigured = false;
    };
} // namespace BatteryTester