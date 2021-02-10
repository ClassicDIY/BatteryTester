#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_MCP9808.h"

/* I2C slave Address Scanner
for 5V bus
 * Connect a 4.7k resistor between SDA and Vcc
 * Connect a 4.7k resistor between SCL and Vcc
for 3.3V bus
 * Connect a 2.4k resistor between SDA and Vcc
 * Connect a 2.4k resistor between SCL and Vcc

 */

void scan()
{
	Serial.println(" Scanning I2C Addresses");
	uint8_t cnt = 0;
	for (uint8_t i = 0; i < 128; i++)
	{
		Wire.beginTransmission(i);
		uint8_t ec = Wire.endTransmission(true);
		if (ec == 0)
		{
			if (i < 16)
				Serial.print('0');
			Serial.print(i, HEX);
			cnt++;
		}
		else
			Serial.print("..");
		Serial.print(' ');
		if ((i & 0x0f) == 0x0f)
			Serial.println();
	}
	Serial.print("Scan Completed, ");
	Serial.print(cnt);
	Serial.println(" I2C Devices found.");
}

bool i2cReady(uint8_t adr)
{
	uint32_t timeout = millis();
	bool ready = false;
	while ((millis() - timeout < 100) && (!ready))
	{
		Wire.beginTransmission(adr);
		ready = (Wire.endTransmission() == 0);
	}
	return ready;
}

void eepromSize()
{
	Serial.println("Discovering eeprom sizes 0x50..0x57");
	uint8_t adr = 0x18, i;
	uint16_t size;
	char buf[64];
	while (adr < 0x1F)
	{
		i = 0;
		size = 0x1000; // Start at 4k
		i += sprintf_P(&buf[i], PSTR("0x%02X: "), adr);
		if (i2cReady(adr))
		{ // EEPROM answered
			uint8_t zeroByte;
			Wire.beginTransmission(adr);
			Wire.write((uint8_t)0); // set address ptr to 0, two bytes High
			Wire.write((uint8_t)0); // set address ptr to 0, two bytes Low
			uint8_t err = Wire.endTransmission();
			if (err == 0)
			{ // worked
				err = Wire.requestFrom(adr, (uint8_t)1);
				if (err == 1)
				{ // got the value of the byte at address 0
					zeroByte = Wire.read();
					uint8_t saveByte, testByte;
					do
					{
						if (i2cReady(adr))
						{
							Wire.beginTransmission(adr);
							Wire.write(highByte(size)); // set next test address
							Wire.write(lowByte(size));
							Wire.endTransmission();
							err = Wire.requestFrom(adr, (uint8_t)1);
							if (err == 1)
							{
								saveByte = Wire.read();
								Wire.beginTransmission(adr);
								Wire.write(highByte(size)); // set next test address
								Wire.write(lowByte(size));
								Wire.write((uint8_t)~zeroByte); // change it
								err = Wire.endTransmission();
								if (err == 0)
								{ // changed it
									if (!i2cReady(adr))
									{
										i += sprintf_P(&buf[i], PSTR(" notReady2.\n"));
										Serial.print(buf);
										adr++;
										break;
									}
									Wire.beginTransmission(adr);
									Wire.write((uint8_t)0); // address 0 byte High
									Wire.write((uint8_t)0); // address 0 byte Low
									err = Wire.endTransmission();
									if (err == 0)
									{
										err = Wire.requestFrom(adr, (uint8_t)1);
										if (err == 1)
										{ // now compare it
											testByte = Wire.read();
										}
										else
										{
											testByte = ~zeroByte; // error out
										}
									}
									else
									{
										testByte = ~zeroByte;
									}
								}
								else
								{
									testByte = ~zeroByte;
								}
								//restore byte
								if (!i2cReady(adr))
								{
									i += sprintf_P(&buf[i], PSTR(" notReady4.\n"));
									Serial.print(buf);
									adr++;
									break;
								}

								Wire.beginTransmission(adr);
								Wire.write(highByte(size)); // set next test address
								Wire.write(lowByte(size));
								Wire.write((uint8_t)saveByte); // restore it
								Wire.endTransmission();
							}
							else
								testByte = ~zeroByte;
						}
						else
							testByte = ~zeroByte;
						if (testByte == zeroByte)
						{
							size = size << 1;
						}
					} while ((testByte == zeroByte) && (size > 0));
					if (size == 0)
						i += sprintf_P(&buf[i], PSTR("64k Bytes"));
					else
						i += sprintf_P(&buf[i], PSTR("%dk Bytes"), size / 1024);
					if (!i2cReady(adr))
					{
						i += sprintf_P(&buf[i], PSTR(" notReady3.\n"));
						Serial.print(buf);
						adr++;
						continue;
					}
					Wire.beginTransmission(adr);
					Wire.write((uint8_t)0); // set address ptr to 0, two bytes High
					Wire.write((uint8_t)0); // set address ptr to 0, two bytes Low
					Wire.write(zeroByte);	//Restore
					err = Wire.endTransmission();
				}
				else
					i += sprintf_P(&buf[i], PSTR("Read 0 Failure"));
			}
			else
				i += sprintf_P(&buf[i], PSTR("Write Adr 0 Failure"));
		}
		else
			i += sprintf_P(&buf[i], PSTR("Not Present."));
		Serial.println(buf);
		adr++;
	}
}

#define MCP9808Resolution 3 // 0.0625°C 250 ms
#define MCP9808_1 0x18		// Battery 1
#define MCP9808_2 0x19		// Battery 2
#define logd(format, ...) log_printf(ARDUHAL_LOG_FORMAT(D, format), ##__VA_ARGS__)

Adafruit_MCP9808 _tempsensor1 = Adafruit_MCP9808();
Adafruit_MCP9808 _tempsensor2 = Adafruit_MCP9808();
uint8_t OutputPins[13]{WIFI_STATUS_PIN, DischargeGate1, ChargeCurrent2k1, ChargeCurrent4k1, CE1, LowLoad1, DischargeLed1, DischargeGate2, ChargeCurrent2k2, ChargeCurrent4k2, CE2, LowLoad2, DischargeLed2};

int currentPin = 0;

void setup()
{
	Serial.begin(115200);
	Wire.begin();
	scan();
	Serial.println();
	eepromSize();
	_tempsensor1.begin(MCP9808_1);
	_tempsensor1.setResolution(MCP9808Resolution); // 0.0625°C 250 ms
	_tempsensor1.wake();
	_tempsensor2.begin(MCP9808_2);
	_tempsensor2.setResolution(MCP9808Resolution); // 0.0625°C 250 ms
	_tempsensor2.wake();

	for (int i = 0; i < 13; i++)
	{
		pinMode(OutputPins[i], OUTPUT);
	}
	logd("Temp1: %f Temp2: %f", _tempsensor1.readTempC(), _tempsensor2.readTempC());
	delay(1000);
}

void toggle(uint8_t pin)
{
	digitalWrite(pin, HIGH);
	delay(500);
	digitalWrite(pin, LOW);
	delay(500);
}

void TestChargeCurrent()
{
	toggle(ChargeCurrent2k1);
	toggle(ChargeCurrent4k1);
	toggle(ChargeCurrent2k2);
	toggle(ChargeCurrent4k2);
}

void loop()
{
	logd("Temp1: %f Temp2: %f", _tempsensor1.readTempC(), _tempsensor2.readTempC());
	delay(500);

	// uint8_t pin = OutputPins[currentPin];
	// logd("Pin %d", pin);
	// toggle(pin);
	// currentPin++;
	// currentPin %= 13;

	// TestChargeCurrent();
}
