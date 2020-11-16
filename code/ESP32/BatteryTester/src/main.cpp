#include "Arduino.h"
#include <ThreadController.h>
#include <Thread.h>
#include <BluetoothSerial.h>

#include <sys/time.h>
#include "time.h"
#include "Configuration.h"
#include "IOT.h"
#include "Tester.h"
#include "Log.h"

using namespace BatteryTester;

#define RECEIVE_BUFFER 200
#define WATCHDOG_TIMER 60000 //time in ms to trigger the watchdog

char *_receiveBuffer;
int _receiveIndex = 0;
int _protectCountdown = 0;
float _recordedWindSpeedAtLastEvent = 0;
time_t _lastWindEvent;
hw_timer_t *timer = NULL;
Configuration _config = Configuration();
ThreadController _controller = ThreadController();
Tester _tester1 = Tester(0, HighBat1, Shunt1, DischargeGate1, Prog1, CE1, Temp1, LowLoad1, STBY1, ChargeCurrent4k1, ChargeCurrent2k1, DischargeLed1);
Tester _tester2 = Tester(1, HighBat2, Shunt2, DischargeGate2, Prog2, CE2, Temp2, LowLoad2, STBY2, ChargeCurrent4k2, ChargeCurrent2k2, DischargeLed2);
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

void setup()
{
	Serial.begin(115200);
	while (!Serial)
	{
		; // wait for serial port to connect.
	}
	logi("Loading configuration");
	_iot.Init();
	_config.Load();
	// Configure main worker thread
	_workerThread->onRun(feed_watchdog);
	_workerThread->setInterval(2000);
	_controller.add(_workerThread);
	logi("Initializing battery");
	_tester1.Setup(&_controller);
	_tester2.Setup(&_controller);
	init_watchdog();
}

void loop()
{
	if (_config.isDirty())
	{
		logi("dirty!!!");
		_config.Save();
		_config.PrintConfiguration();
	}
	_iot.Run();
	_controller.run();
}
