#pragma once

/*
    workaround when using both WebServer.h and esp_http_server.h at the same time
    https://github.com/tzapu/WiFiManager/issues/1184
*/

#define _HTTP_Method_H_ 

typedef enum {
  jHTTP_GET     = 0b00000001,
  jHTTP_POST    = 0b00000010,
  jHTTP_DELETE  = 0b00000100,
  jHTTP_PUT     = 0b00001000,
  jHTTP_PATCH   = 0b00010000,
  jHTTP_HEAD    = 0b00100000,
  jHTTP_OPTIONS = 0b01000000,
  jHTTP_ANY     = 0b01111111,
} HTTPMethod;


#include "WiFi.h"
#include "ArduinoJson.h"
extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}
#include "AsyncMqttClient.h"
#include "IotWebConf.h"
#include "HttpsOTAUpdate.h"
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
        void publish(uint8_t pos, const char *subtopic, StaticJsonDocument<MaxMQTTPayload> *doc, boolean retained = false);

    private:
        void SetupWifi(const char *ssid, const char *pw);
        bool _clientsConfigured = false;
    };
} // namespace BatteryTester