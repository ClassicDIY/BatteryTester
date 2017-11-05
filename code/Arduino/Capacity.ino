/*
  Battery Capacity Checker
  Uses 1 Ohm power resister as shunt - Load can be any suitable resister or lamp

*/


#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include "Discharger.h"

SoftwareSerial _BlueTooth(7, 8);
LiquidCrystal_I2C _lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 20 chars and 4 line display
Adafruit_ADS1115 _ads;
MUX74HC4067 _mux(13, 2, 3, 4, 5);

Discharger _dischargers[4] = { Discharger(&_ads, &_mux, 12, 11) , Discharger(&_ads, &_mux, 13, 10), Discharger(&_ads, &_mux, 14, 9) , Discharger(&_ads, &_mux, 15, 6) };
int _lcdRefresh = 0;

boolean _finished = false;
int printStart = 0;

#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)

/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % 60UL)  
#define numberOfMinutes(_time_) (_time_ / 60UL) 

void setup() {
	_lcd.init();
	_ads.begin();
	_lcd.backlight();
	_lcd.setCursor(0, 0);
	_lcd.print("Capacity Checker");
	Serial.begin(9600);
	_BlueTooth.begin(9600);
	Serial.println("Battery Capacity Checker");
	delay(1000);
	_lcd.clear();
	for (int index = 0; index < 4; index++) {
		_dischargers[index].Init();
	}
	_lcd.print("Measuring resistance");
	delay(500);
	_lcd.clear();
	for (int index = 0; index < 4; index++) {
		if (_dischargers[index].MeasureInternalResistance()) {
			BroadcastInitalState(index);
			_lcd.setCursor(0, index);
			_lcd.print("IR: ");
			_lcd.print(_dischargers[index].InternalResistance());
			_lcd.print("mO ");
			_lcd.print(_dischargers[index].InternalResistanceRanking());
		}
	}
	delay(5000);
}

void loop() {

	if (_lcdRefresh == 0) {
		_lcd.clear();
		_lcdRefresh = 10;
	}
	_lcdRefresh--;
	for (int index = 0; index < 4; index++) {
		_dischargers[index].Discharge();
		UpdateLCD(index);
		BroadcastDischargingState(index);
	}
	delay(1000);
}

void UpdateLCD(int index) {
	unsigned long elapsed = _dischargers[index].ElapsedTime();
	int minutes = numberOfMinutes(elapsed);
	int seconds = numberOfSeconds(elapsed);
	char time[10];
	sprintf(time, "%i:%02i", minutes, seconds);
	char mah[10];
	sprintf(mah, "%5u", _dischargers[index].Capacity());
	_lcd.setCursor(0, index);

	if (_dischargers[index].State() == Complete) {
		_lcd.print("*");
		_lcd.print(_dischargers[index].BatteryVolt());
		_lcd.print(" ");
		_lcd.print(mah);
		_lcd.print(" ");
		_lcd.print(time);
	}
	else if (_dischargers[index].State() == ThermalShutdown) {
		_lcd.print("***ThermalShutdown***");
	}
	else {
		_lcd.print(" ");
		_lcd.print(_dischargers[index].BatteryVolt());
		_lcd.print(" ");
		_lcd.print(_dischargers[index].BatteryCurrent());
		_lcd.print(" ");
		_lcd.print(mah);
	}
}

void BroadcastInitalState(int index) {
	_BlueTooth.print("Cell|{");
	_BlueTooth.print("\"sN\":");
	_BlueTooth.print(index);
	_BlueTooth.print(",\"sV\":");
	_BlueTooth.print(_dischargers[index].BatteryVolt());
	_BlueTooth.print(",\"sR\":");
	_BlueTooth.print(_dischargers[index].InternalResistance());
	_BlueTooth.print(",\"sT\":");
	_BlueTooth.print(_dischargers[index].Temperature());
	_BlueTooth.println("}");
}

void BroadcastDischargingState(int index) {
	_BlueTooth.print("Cell|{");
	_BlueTooth.print("\"sN\":");
	_BlueTooth.print(index);
	_BlueTooth.print(",\"sS\":");
	_BlueTooth.print(_dischargers[index].State());
	_BlueTooth.print(",\"sR\":");
	_BlueTooth.print(_dischargers[index].InternalResistance());
	_BlueTooth.print(",\"sV\":");
	_BlueTooth.print(_dischargers[index].BatteryVolt());
	_BlueTooth.print(",\"sI\":");
	_BlueTooth.print(_dischargers[index].BatteryCurrent());
	_BlueTooth.print(",\"sQ\":");
	_BlueTooth.print(_dischargers[index].Capacity());
	_BlueTooth.print(",\"sT\":");
	_BlueTooth.print(_dischargers[index].Temperature());
	_BlueTooth.print(",\"sE\":");
	_BlueTooth.print(_dischargers[index].ElapsedTime());
	_BlueTooth.println("}");
}
