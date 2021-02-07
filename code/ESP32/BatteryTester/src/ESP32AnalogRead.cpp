/*
 * ESP32AnalogRead.cpp
 *
 *  Created on: Apr 10, 2020
 *      Author: hephaestus
 *      https://github.com/madhephaestus/ESPMutexDemo/blob/DSPTest/ESPMutexDemo.ino
 */

#include "ESP32AnalogRead.h"

ESP32AnalogRead::ESP32AnalogRead(int pinNum) {
	if (!(pinNum < 0)) {
		attach(pinNum);
	}
}
void ESP32AnalogRead::attach(int pin) {
	myPin = pin;
	channel = (adc_channel_t) digitalPinToAnalogChannel(myPin);
	attached = true;
}

uint32_t ESP32AnalogRead::readMiliVolts() {
	if (!attached)
		return 0;
	analogRead(myPin);
	// Configure ADC
	if (myPin > 27) {
		adc1_config_width(ADC_WIDTH_12Bit);
		adc1_channel_t chan= ADC1_CHANNEL_0;
		switch (myPin) {

		case 36:
			chan = ADC1_CHANNEL_0;
			break;
		case 37:
			chan = ADC1_CHANNEL_1;
			break;
		case 38:
			chan = ADC1_CHANNEL_2;
			break;
		case 39:
			chan = ADC1_CHANNEL_3;
			break;
		case 32:
			chan = ADC1_CHANNEL_4;
			break;
		case 33:
			chan = ADC1_CHANNEL_5;
			break;
		case 34:
			chan = ADC1_CHANNEL_6;
			break;
		case 35:
			chan = ADC1_CHANNEL_7;
			break;
		}
		adc1_config_channel_atten(chan, ADC_ATTEN_11db);
	} else {
		adc2_channel_t chan= ADC2_CHANNEL_0;
		switch (myPin) {
		case 4:
			chan = ADC2_CHANNEL_0;
			break;
		case 0:
			chan = ADC2_CHANNEL_1;
			break;
		case 2:
			chan = ADC2_CHANNEL_2;
			break;
		case 15:
			chan = ADC2_CHANNEL_3;
			break;
		case 13:
			chan = ADC2_CHANNEL_4;
			break;
		case 12:
			chan = ADC2_CHANNEL_5;
			break;
		case 14:
			chan = ADC2_CHANNEL_6;
			break;
		case 27:
			chan = ADC2_CHANNEL_7;
			break;
		case 25:
			chan = ADC2_CHANNEL_8;
			break;
		case 26:
			chan = ADC2_CHANNEL_9;
			break;
		}
		adc2_config_channel_atten(chan, ADC_ATTEN_11db);
	}
	// Calculate ADC characteristics i.e. gain and offset factors
	esp_adc_cal_get_characteristics(V_REF, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12,&characteristics);
	uint32_t voltage = 0;
	// Read ADC and obtain result in mV
	esp_adc_cal_get_voltage(channel, &characteristics, &voltage);
	return voltage;
}
