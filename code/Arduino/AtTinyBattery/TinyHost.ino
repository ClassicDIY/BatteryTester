
#include <SoftwareSerial.h>

#define RECEIVE_BUFFER 256

const int Rx = 2;
const int Tx = 3;

SoftwareSerial swSerial(Rx, Tx);

char* _receiveBuffer;
int _receiveIndex = 0;
int _topIndex = -1;

void setup() {
	_receiveBuffer = (char*)malloc(RECEIVE_BUFFER);
	swSerial.begin(9600);
	Serial.begin(9600);
	delay(200);
	swSerial.println("TinyHost");
	Serial.print("[?]");
	delay(1000);
	Serial.print("[!]");
	delay(1000);

}

void loop() {
	if (_topIndex != -1) {
		char buffer[8];
		sprintf(buffer, "[%x]", _receiveIndex);
		Serial.print(buffer);
		_receiveIndex++;
		if (_receiveIndex > _topIndex) {
			_receiveIndex = 0;
		}
		delay(600);
	}
	serialEvent();
	delay(600);
}

void serialEvent() {
	char src[8];

	if (Serial.available()) {
		String receiver = Serial.readStringUntil(']');
		Serial.read();
		swSerial.print(receiver); swSerial.println("]");

		if (receiver.length() < 5) {
			int j = 0;
			for (int i = 0; i < receiver.length(); i++) {
				if (receiver[i] == '?') {
					_topIndex = -1;
					return;
				}
				else if ((receiver[i] >= 0x30 && receiver[i] <= 0x39) || (receiver[i] >= 0x41 && receiver[i] <= 0x46)) {
					src[j++] = receiver[i];
				}
			}
			src[j] = 0;
			if (_topIndex == -1) {
				receiver = src;
				_topIndex = receiver.toInt();
			}
		}

	}
}

