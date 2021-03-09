#include "Arduino.h"
#include <ThreadController.h>
#include <Thread.h>
#include <BluetoothSerial.h>
#include "esp_task_wdt.h"
#include <sys/time.h>
#include "time.h"
#include "Configuration.h"
#include "IOT.h"
#include "Tester.h"
#include "Log.h"

using namespace BatteryTester;

#define RECEIVE_BUFFER 200
#define WATCHDOG_TIMER 5000 //time in ms to trigger the watchdog

static const char *server_certificate = "-----BEGIN CERTIFICATE-----\n"
										"MIIEkjCCA3qgAwIBAgIQCgFBQgAAAVOFc2oLheynCDANBgkqhkiG9w0BAQsFADA/\n"
										"MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n"
										"DkRTVCBSb290IENBIFgzMB4XDTE2MDMxNzE2NDA0NloXDTIxMDMxNzE2NDA0Nlow\n"
										"SjELMAkGA1UEBhMCVVMxFjAUBgNVBAoTDUxldCdzIEVuY3J5cHQxIzAhBgNVBAMT\n"
										"GkxldCdzIEVuY3J5cHQgQXV0aG9yaXR5IFgzMIIBIjANBgkqhkiG9w0BAQEFAAOC\n"
										"AQ8AMIIBCgKCAQEAnNMM8FrlLke3cl03g7NoYzDq1zUmGSXhvb418XCSL7e4S0EF\n"
										"q6meNQhY7LEqxGiHC6PjdeTm86dicbp5gWAf15Gan/PQeGdxyGkOlZHP/uaZ6WA8\n"
										"SMx+yk13EiSdRxta67nsHjcAHJyse6cF6s5K671B5TaYucv9bTyWaN8jKkKQDIZ0\n"
										"Z8h/pZq4UmEUEz9l6YKHy9v6Dlb2honzhT+Xhq+w3Brvaw2VFn3EK6BlspkENnWA\n"
										"a6xK8xuQSXgvopZPKiAlKQTGdMDQMc2PMTiVFrqoM7hD8bEfwzB/onkxEz0tNvjj\n"
										"/PIzark5McWvxI0NHWQWM6r6hCm21AvA2H3DkwIDAQABo4IBfTCCAXkwEgYDVR0T\n"
										"AQH/BAgwBgEB/wIBADAOBgNVHQ8BAf8EBAMCAYYwfwYIKwYBBQUHAQEEczBxMDIG\n"
										"CCsGAQUFBzABhiZodHRwOi8vaXNyZy50cnVzdGlkLm9jc3AuaWRlbnRydXN0LmNv\n"
										"bTA7BggrBgEFBQcwAoYvaHR0cDovL2FwcHMuaWRlbnRydXN0LmNvbS9yb290cy9k\n"
										"c3Ryb290Y2F4My5wN2MwHwYDVR0jBBgwFoAUxKexpHsscfrb4UuQdf/EFWCFiRAw\n"
										"VAYDVR0gBE0wSzAIBgZngQwBAgEwPwYLKwYBBAGC3xMBAQEwMDAuBggrBgEFBQcC\n"
										"ARYiaHR0cDovL2Nwcy5yb290LXgxLmxldHNlbmNyeXB0Lm9yZzA8BgNVHR8ENTAz\n"
										"MDGgL6AthitodHRwOi8vY3JsLmlkZW50cnVzdC5jb20vRFNUUk9PVENBWDNDUkwu\n"
										"Y3JsMB0GA1UdDgQWBBSoSmpjBH3duubRObemRWXv86jsoTANBgkqhkiG9w0BAQsF\n"
										"AAOCAQEA3TPXEfNjWDjdGBX7CVW+dla5cEilaUcne8IkCJLxWh9KEik3JHRRHGJo\n"
										"uM2VcGfl96S8TihRzZvoroed6ti6WqEBmtzw3Wodatg+VyOeph4EYpr/1wXKtx8/\n"
										"wApIvJSwtmVi4MFU5aMqrSDE6ea73Mj2tcMyo5jMd6jmeWUHK8so/joWUoHOUgwu\n"
										"X4Po1QYz+3dszkDqMp4fklxBwXRsW10KXzPMTZ+sOPAveyxindmjkW8lGy+QsRlG\n"
										"PfZ+G6Z6h7mjem0Y+iWlkYcV4PIWL1iwBi8saCbGS5jN2p8M+X+Q7UNKEkROb3N6\n"
										"KOqkqm57TH2H3eDJAkSnh6/DNFu0Qg==\n"
										"-----END CERTIFICATE-----";

char *_receiveBuffer;
int _receiveIndex = 0;
int _protectCountdown = 0;
float _recordedWindSpeedAtLastEvent = 0;
time_t _lastWindEvent;
hw_timer_t *timer = NULL;
Configuration _config = Configuration();
ThreadController _controller = ThreadController();
Tester _tester1 = Tester(0, HighBat1, Shunt1, DischargeGate1, Prog1, CE1, MCP9808_1, LowLoad1, STBY1, ChargeCurrent4k1, ChargeCurrent2k1, DischargeLed1);
Tester _tester2 = Tester(1, HighBat2, Shunt2, DischargeGate2, Prog2, CE2, MCP9808_2, LowLoad2, STBY2, ChargeCurrent4k2, ChargeCurrent2k2, DischargeLed2);
Thread *_workerThread = new Thread();
IOT _iot = IOT();

void IRAM_ATTR resetModule()
{
	ets_printf("watchdog timer expired - rebooting\n");
	_config.SaveTime(); // save current time during reboot in case we don't have wifi/ntp
	esp_restart();
}

void init_watchdog()
{
	if (timer == NULL)
	{
		timer = timerBegin(0, 80, true);					  //timer 0, div 80
		timerAttachInterrupt(timer, &resetModule, true);	  //attach callback
		timerAlarmWrite(timer, WATCHDOG_TIMER * 1000, false); //set time in us
		timerAlarmEnable(timer);							  //enable interrupt
	}
}

void feed_watchdog()
{
	if (timer != NULL)
	{
		timerWrite(timer, 0); // feed the watchdog
	}
}

void disable_watchdog()
{
	if (timer != NULL)
	{
		logd("disabled watchdog");
		timerAlarmDisable(timer); // kill the watchdog durring OTA update
		timer = NULL;
	}
}

void setup()
{
	Serial.begin(115200);
	while (!Serial)
	{
		; // wait for serial port to connect.
	}
	logd("Loading configuration");
	_iot.Init();
	_config.Load();
	// Configure main worker thread
	_workerThread->onRun(feed_watchdog);
	_workerThread->setInterval(2000);
	_controller.add(_workerThread);
	logd("Initializing battery");
	_tester1.Setup(&_controller);
	_tester2.Setup(&_controller);
	init_watchdog();
}

void loop()
{
	if (timer == NULL) // performing OTA?
	{
		HttpsOTAStatus_t otastatus = HttpsOTA.status();
		if (otastatus == HTTPS_OTA_SUCCESS)
		{
			logd("Firmware written successfully. Rebooting...");
			ESP.restart();
		}
		else if (otastatus == HTTPS_OTA_FAIL)
		{
			logd("Firmware Upgrade HTTPS_OTA_FAIL");
		}
		else if (otastatus == HTTPS_OTA_IDLE)
		{
			logd("Firmware Upgrade HTTPS_OTA_IDLE");
		}
		else if (otastatus == HTTPS_OTA_ERR)
		{
			logd("Firmware Upgrade HTTPS_OTA_ERR");
		}
		else if (otastatus == HTTPS_OTA_UPDATING)
		{
			ets_printf(".");
		}
		delay(1);
	}
	else
	{
		if (_config.isDirty())
		{
			logd("dirty!!!");
			_config.Save();
			_config.PrintConfiguration();
		}
		_iot.Run();
		_controller.run();
	}
}


void HttpEvent(HttpEvent_t *event)
{
	switch (event->event_id)
	{
	case HTTP_EVENT_ERROR:
		logd("Http Event Error");
		break;
	case HTTP_EVENT_ON_CONNECTED:
		logd("Http Event On Connected");
		break;
	case HTTP_EVENT_HEADER_SENT:
		logd("Http Event Header Sent");
		break;
	case HTTP_EVENT_ON_HEADER:
		logd("Http Event On Header, key=%s, value=%s\n", event->header_key, event->header_value);
		break;
	case HTTP_EVENT_ON_DATA:
		break;
	case HTTP_EVENT_ON_FINISH:
		logd("Http Event On Finish");
		break;
	case HTTP_EVENT_DISCONNECTED:
		logd("Http Event Disconnected");
		break;
	}
}

void doOTA(char* otaUrl)
{
	disable_watchdog();
	disableCore0WDT();
	HttpsOTA.onHttpEvent(HttpEvent);
	HttpsOTA.begin((const char*)otaUrl, server_certificate);
}
