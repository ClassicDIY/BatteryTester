#include "Battery.h"

#include <arduino.h>
#include <avr/io.h>
#include <string.h>
#include <SoftwareSerial.h>

const int Rx = 3; // this is physical pin 2
const int Tx = 4; // this is physical pin 3

const unsigned long  voltRef = 48;

SoftwareSerial swSerial(Rx, Tx);
//Battery(unsigned long  voltRef, uint8_t highBatPin, uint8_t gatePin, uint8_t tp4056Enable, uint8_t tp4056Prog, uint8_t tp4056Standby, uint8_t mcp9700)
Battery _Battery(voltRef, 0xff, 0xff, 1, A1, 0, 0xff);

int _index = -1; // cell index

char* ToA(char* src, uint16_t val, uint8_t precision) {
	utoa(val, src, 10);
	uint8_t len = strlen(src);
	if (precision < len) {
		return &src[len - precision];
	}
	else if (len == precision) {
		return src;
	}
	else {
		char dest[8];
		size_t zeros = (len > precision) ? 0 : precision - len;
		memset(dest, '0', zeros);
		strcpy(dest + zeros, src);
		return strcpy(src, dest);
	}
}

void setup()
{
	OSCCAL = 134;
	swSerial.begin(9600);
	delay(200);
	swSerial.println("ATtiny 1.0");
	delay(500);
	_Battery.Init();
	_index = -1;
	swSerial.println("Ready");
}

void loop()
{
	_Battery.Loop();
	if (swSerial.available()) {
		swSerial.readStringUntil('[');
		String received = swSerial.readStringUntil(']');
		swSerial.read();
		if (received.length() < 5) {
			char src[8];
			if (received[0] == '?') { // reset request
				swSerial.print("[?]");
				_index = -1;
				_Battery.Reset();
			}
			else if (_index == -1) {

				if (received[0] == '!') { // first one in chain
					_index = 0;
				}
				else {
					int i = received.substring(0, 2).toInt();
					_index = i + 1;
				}
				swSerial.print("[");
				swSerial.print(ToA(src, _index, 2));
				swSerial.print("]");
				_Battery.Charge();
			}
			else {
				//swSerial.println(); swSerial.print("***"); swSerial.print(received); swSerial.println("***");
				int receivedIndex = received.substring(0, 2).toInt();
				if (receivedIndex == _index) { // request for this cell
					swSerial.print("[");
					swSerial.print(ToA(src, _index, 2));
					swSerial.print(",");
					swSerial.print(ToA(src, _Battery.GetState(), 1));
					swSerial.print(",");
					swSerial.print(ToA(src, _Battery.InternalResistance(), 3));
					swSerial.print(",");
					swSerial.print(ToA(src, _Battery.Volt(), 3));
					swSerial.print(",");
					swSerial.print(ToA(src, _Battery.Current(), 3));
					swSerial.print(",");
					swSerial.print(ToA(src, _Battery.Capacity(), 4));
					swSerial.print(",");
					swSerial.print(ToA(src, _Battery.Temperature(), 3));
					swSerial.print(",");
					swSerial.print(ToA(src, _Battery.DischargeTime(), 4));
					swSerial.print("]");
				}
				else { // next cell request
					swSerial.print("[");
					swSerial.print(ToA(src, receivedIndex, 2));
					swSerial.print("]");
				}
			}
		}
		else {
			swSerial.print("[");
			swSerial.print(received); // forward previous cell data
			swSerial.print("]");
		}
	}
	delay(300);
}
