#include "Arduino.h"
#include <ThreadController.h>
#include <Thread.h>
#include <BluetoothSerial.h>

#include <sys/time.h>
#include "time.h"
#include "Configuration.h"
#include "IOT.h"
#include "Battery.h"
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
Battery _battery = Battery();
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

bool hasClient = false;
void runWorker()
{
	feed_watchdog();
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
	_workerThread->onRun(runWorker);
	_workerThread->setInterval(2000);
	_controller.add(_workerThread);
	logi("Initializing battery");
	_battery.Initialize(&_controller);
	init_watchdog();
}

void loop()
{
	_iot.Run();
	_controller.run();
}
