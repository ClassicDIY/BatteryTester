#include "IOT.h"
#include <sys/time.h>
#include <EEPROM.h>
#include "time.h"
#include "Log.h"
#include "Tester.h"

extern BatteryTester::Tester _tester1;
extern BatteryTester::Tester _tester2;

namespace BatteryTester
{
	AsyncMqttClient _mqttClient;
	TimerHandle_t mqttReconnectTimer;
	DNSServer _dnsServer;
	WebServer _webServer(80);
	HTTPUpdateServer _httpUpdater;
	IotWebConf _iotWebConf(TAG, &_dnsServer, &_webServer, TAG, CONFIG_VERSION);
	char _mqttRootTopic[STR_LEN];
	char _willTopic[STR_LEN];
	char _mqttServer[IOTWEBCONF_WORD_LEN];
	char _mqttPort[5];
	char _mqttUserName[IOTWEBCONF_WORD_LEN];
	char _mqttUserPassword[IOTWEBCONF_WORD_LEN];
	u_int _uniqueId = 0;
	IotWebConfSeparator seperatorParam = IotWebConfSeparator("MQTT");
	IotWebConfParameter mqttServerParam = IotWebConfParameter("MQTT server", "mqttServer", _mqttServer, IOTWEBCONF_WORD_LEN);
	IotWebConfParameter mqttPortParam = IotWebConfParameter("MQTT port", "mqttSPort", _mqttPort, 5, "text", NULL, "1883");
	IotWebConfParameter mqttUserNameParam = IotWebConfParameter("MQTT user", "mqttUser", _mqttUserName, IOTWEBCONF_WORD_LEN);
	IotWebConfParameter mqttUserPasswordParam = IotWebConfParameter("MQTT password", "mqttPass", _mqttUserPassword, IOTWEBCONF_WORD_LEN, "password");
	IotWebConfParameter mqttRootTopicParam = IotWebConfParameter("MQTT Root Topic", "mqttRootTopic", _mqttRootTopic, IOTWEBCONF_WORD_LEN);
	const char *ntpServer = "pool.ntp.org";

	void onMqttConnect(bool sessionPresent)
	{
		logd("Connected to MQTT. Session present: %d", sessionPresent);
		char mqttCmndTopic[STR_LEN];
		sprintf(mqttCmndTopic, "%s/cmnd/Mode", _mqttRootTopic);
		uint16_t packetIdSub = _mqttClient.subscribe(mqttCmndTopic, 1);
		sprintf(mqttCmndTopic, "%s/cmnd/Config", _mqttRootTopic);
		packetIdSub = _mqttClient.subscribe(mqttCmndTopic, 1);
		logd("MQTT subscribe, QoS 1, packetId: %d", packetIdSub);
		_mqttClient.publish(_willTopic, 0, false, "Online");
		_tester1.setState(Initialize);
		_tester2.setState(Initialize);
	}

	void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
	{
		logw("Disconnected from MQTT. Reason: %d", (int8_t)reason);
	}

	void connectToMqtt()
	{
		logd("Connecting to MQTT...");
		if (WiFi.isConnected())
		{
			_mqttClient.connect();
		}
	}

	void WiFiEvent(WiFiEvent_t event)
	{
		logd("[WiFi-event] event: %d", event);
		String s;
		StaticJsonDocument<128> doc;
		switch (event)
		{
		case SYSTEM_EVENT_STA_GOT_IP:
			// logd("WiFi connected, IP address: %s", WiFi.localIP().toString().c_str());
			doc["IP"] = WiFi.localIP().toString().c_str();
			doc["ApPassword"] = TAG;
			serializeJson(doc, s);
			s += '\n';
			Serial.printf(s.c_str()); // send json to flash tool
			configTime(0, 0, ntpServer);
			printLocalTime();
			xTimerStart(mqttReconnectTimer, 0);
			break;
		case SYSTEM_EVENT_STA_DISCONNECTED:
			logw("WiFi lost connection");
			xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
			break;
		case SYSTEM_EVENT_STA_STOP:
			logw("WiFi stopped");
			break;
		default:
			break;
		}
	}

	void onMqttPublish(uint16_t packetId)
	{
		logd("Publish acknowledged.  packetId: %d", packetId);
	}

	void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
	{
		logd("MQTT Message arrived [%s]  qos: %d len: %d index: %d total: %d", topic, properties.qos, len, index, total);
		printHexString(payload, len);
		int l = strlen(_mqttRootTopic) + 6;
		if (l < strlen(topic) && len < MaxMQTTPayload)
		{
			char *subtopic = &topic[l];
			char buf[MaxMQTTTopic + 1];
			strncpy(buf, payload, len);
			buf[len] = 0;
			logd("Payload: %s", buf);
			if (strcmp(subtopic, "Mode") == 0) // mode of operation
			{
				if (strncmp(payload, "Monitor", len) == 0)
				{
					_tester1.setState(Monitor);
					// _tester2.setState(Monitor);
				}
				if (strncmp(payload, "InternalResistance", len) == 0)
				{
					_tester1.setState(InternalResistance);
					_tester2.setState(InternalResistance);
				}
				if (strncmp(payload, "Charge", len) == 0)
				{
					_tester1.setState(Charge);
					_tester2.setState(Charge);
				}
				if (strncmp(payload, "Discharge", len) == 0)
				{
					_tester1.setState(Discharge);
					// _tester2.setState(Discharge);
				}
				if (strncmp(payload, "Storage", len) == 0)
				{
					_tester1.setState(Storage);
					_tester2.setState(Storage);
				}
				if (strncmp(payload, "Standby", len) == 0)
				{
					_tester1.setState(Standby);
					_tester2.setState(Standby);
				}
			}
			else if (strcmp(subtopic, "Config") == 0)
			{
				if (strncmp(payload, "LoadDefaultSettings", len) == 0) // reset config to factory default
				{
					_config.LoadFactoryDefault();
				}
				else // set configuration
				{
					StaticJsonDocument<MaxMQTTPayload> doc;
					DeserializationError error = deserializeJson(doc, payload, len);
					if (!error)
					{
						if (doc.containsKey("LowCutoff"))
						{
							uint16_t val = doc["LowCutoff"];
							_config.setLowCutoff(val);
						}
						if (doc.containsKey("ThermalShutdownTemperature"))
						{
							uint16_t val = doc["ThermalShutdownTemperature"];
							_config.setThermalShutdownTemperature(val);
						}
						if (doc.containsKey("StorageVoltage"))
						{
							uint16_t val = doc["StorageVoltage"];
							_config.setStorageVoltage(val);
						}
						if (doc.containsKey("ChargeCurrent"))
						{
							uint8_t val = doc["ChargeCurrent"];
							_config.setChargeCurrent(val);
						}
					}
					else
					{
						logw("JSON error!");
					}
				}
			}
		}
	}

	IOT::IOT()
	{
	}

	/**
 * Handle web requests to "/" path.
 */
	void handleRoot()
	{
		// -- Let IotWebConf test and handle captive portal requests.
		if (_iotWebConf.handleCaptivePortal())
		{
			logd("Captive portal");
			// -- Captive portal request were already served.
			return;
		}
		logd("handleRoot");
		String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
		s += "<title>";
		s += _iotWebConf.getThingName();
		s += "</title></head><body>";
		s += _iotWebConf.getThingName();
		s += "<ul>";
		s += "<li>MQTT server: ";
		s += _mqttServer;
		s += "</ul>";
		s += "<ul>";
		s += "<li>MQTT port: ";
		s += _mqttPort;
		s += "</ul>";
		s += "<ul>";
		s += "<li>MQTT user: ";
		s += _mqttUserName;
		s += "</ul>";
		s += "<ul>";
		s += "<li>MQTT root topic: ";
		s += _mqttRootTopic;
		s += "</ul>";
		s += "Go to <a href='config'>configure page</a> to change values.";
		s += "</body></html>\n";
		_webServer.send(200, "text/html", s);
	}

	void IOT::Init()
	{
		pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
		_iotWebConf.setStatusPin(WIFI_STATUS_PIN);
		_iotWebConf.setConfigPin(WIFI_AP_PIN);
		if (digitalRead(FACTORY_RESET_PIN) == LOW)
		{
			EEPROM.begin(IOTWEBCONF_CONFIG_START + IOTWEBCONF_CONFIG_VERSION_LENGTH);
			for (byte t = 0; t < IOTWEBCONF_CONFIG_VERSION_LENGTH; t++)
			{
				EEPROM.write(IOTWEBCONF_CONFIG_START + t, 0);
			}
			EEPROM.commit();
			EEPROM.end();
			logw("Factory Reset!");
		}
		mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(5000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
		WiFi.onEvent(WiFiEvent);
		_iotWebConf.setupUpdateServer(&_httpUpdater);
		_iotWebConf.addParameter(&seperatorParam);
		_iotWebConf.addParameter(&mqttServerParam);
		_iotWebConf.addParameter(&mqttPortParam);
		_iotWebConf.addParameter(&mqttUserNameParam);
		_iotWebConf.addParameter(&mqttUserPasswordParam);
		_iotWebConf.addParameter(&mqttRootTopicParam);
		boolean validConfig = _iotWebConf.init();
		if (!validConfig)
		{
			logw("!invalid configuration!");
			_mqttServer[0] = '\0';
			_mqttPort[0] = '\0';
			_mqttUserName[0] = '\0';
			_mqttUserPassword[0] = '\0';
			strcpy(_mqttRootTopic, _iotWebConf.getThingName());
			_iotWebConf.resetWifiAuthInfo();
		}
		else
		{
			_iotWebConf.skipApStartup(); // Set WIFI_AP_PIN to gnd to force AP mode
			if (_mqttServer[0] != '\0')	 // skip if factory reset
			{
				logd("Valid configuration!");
				_clientsConfigured = true;
				// setup MQTT
				_mqttClient.onConnect(onMqttConnect);
				_mqttClient.onDisconnect(onMqttDisconnect);
				_mqttClient.onMessage(onMqttMessage);
				_mqttClient.onPublish(onMqttPublish);
				IPAddress ip;
				if (ip.fromString(_mqttServer))
				{
					int port = atoi(_mqttPort);
					_mqttClient.setServer(ip, port);
					_mqttClient.setCredentials(_mqttUserName, _mqttUserPassword);
					sprintf(_willTopic, "%s/tele/LWT", _mqttRootTopic);
					_mqttClient.setWill(_willTopic, 0, false, "Offline");
				}
			}
		}
		// Set up required URL handlers on the web server.
		_webServer.on("/", handleRoot);
		_webServer.on("/config", [] { _iotWebConf.handleConfig(); });
		_webServer.onNotFound([]() { _iotWebConf.handleNotFound(); });
	}

	void IOT::Run()
	{
		_iotWebConf.doLoop();
		if (_clientsConfigured && WiFi.isConnected())
		{
			// ToDo MQTT monitoring
			// if (_mqttClient.connected())
			// {
			// 	if (_lastPublishTimeStamp < millis())
			// 	{
			// 		_lastPublishTimeStamp = millis() + _currentPublishRate;
			// 		_publishCount++;
			// 		publishReadings();
			// 		// Serial.printf("%d ", _publishCount);
			// 	}
			// 	if (!_stayAwake && _publishCount >= WAKE_COUNT)
			// 	{
			// 		_publishCount = 0;
			// 		_currentPublishRate = SNOOZE_PUBLISH_RATE;
			// 		logd("Snoozing!");
			// 	}
			// }
		}
		else
		{
			// process serial command from flasher tool to setup wifi
			if (Serial.peek() == '{')
			{
				String s = Serial.readStringUntil('}');
				s += "}";
				StaticJsonDocument<128> doc;
				DeserializationError err = deserializeJson(doc, s);
				if (err)
				{
					loge("deserializeJson() failed: %s", err.c_str());
				}
				else
				{
					if (doc.containsKey("ssid") && doc.containsKey("password"))
					{
						IotWebConfParameter *p = _iotWebConf.getWifiSsidParameter();
						strcpy(p->valueBuffer, doc["ssid"]);
						logd("Setting ssid: %s", p->valueBuffer);
						p = _iotWebConf.getWifiPasswordParameter();
						strcpy(p->valueBuffer, doc["password"]);
						logd("Setting password: %s", p->valueBuffer);
						p = _iotWebConf.getApPasswordParameter();
						strcpy(p->valueBuffer, TAG); // reset to default AP password
						_iotWebConf.configSave();
						esp_restart(); // force reboot
					}
					else
					{
						logw("Received invalid json: %s", s.c_str());
					}
				}
			}
			else
			{
				Serial.read(); // discard data
			}
		}
	}

	void IOT::publish(uint8_t pos, const char *subtopic, const char *value, boolean retained)
	{
		if (_mqttClient.connected())
		{
			char buf[MaxMQTTTopic];
			sprintf(buf, "%s/%d/stat/%s", _mqttRootTopic, pos, subtopic);
			_mqttClient.publish(buf, 0, retained, value);
		}
	}


} // namespace BatteryTester