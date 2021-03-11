#include "IOT.h"
#include <sys/time.h>
#include <EEPROM.h>
#include "time.h"
#include "Log.h"
#include "Tester.h"

extern BatteryTester::Tester _tester1;
extern BatteryTester::Tester _tester2;
extern void doOTA(char *otaURL);

namespace BatteryTester
{
	StaticJsonDocument<MaxMQTTPayload> _JSdoc;
	AsyncMqttClient _mqttClient;
	TimerHandle_t mqttReconnectTimer;
	DNSServer _dnsServer;
	WebServer _webServer(80);
	IotWebConf _iotWebConf(TAG, &_dnsServer, &_webServer, TAG, CONFIG_VERSION);
	char _mqttRootTopic[STR_LEN];
	char _mqttTesterNumber[5];
	char _willTopic[STR_LEN];
	char _mqttServer[IOTWEBCONF_WORD_LEN];
	char _mqttPort[5];
	char _mqttUserName[IOTWEBCONF_WORD_LEN];
	char _mqttUserPassword[IOTWEBCONF_WORD_LEN];
	u_int _uniqueId = 0;
	iotwebconf::ParameterGroup mqttGroup = iotwebconf::ParameterGroup("MQTT");
	iotwebconf::TextParameter mqttServerParam = iotwebconf::TextParameter("MQTT server", "mqttServer", _mqttServer, IOTWEBCONF_WORD_LEN);
	iotwebconf::NumberParameter mqttPortParam = iotwebconf::NumberParameter("MQTT port", "mqttPort", _mqttPort, 5, "text", NULL, "1883");
	iotwebconf::TextParameter mqttUserNameParam = iotwebconf::TextParameter("MQTT user", "mqttUser", _mqttUserName, IOTWEBCONF_WORD_LEN);
	iotwebconf::PasswordParameter mqttUserPasswordParam = iotwebconf::PasswordParameter("MQTT password", "mqttPass", _mqttUserPassword, IOTWEBCONF_WORD_LEN);
	iotwebconf::TextParameter mqttRootTopicParam = iotwebconf::TextParameter("Tester Group Name", "mqttRootTopic", _mqttRootTopic, IOTWEBCONF_WORD_LEN, "text", NULL, "Battery");
	iotwebconf::TextParameter mqttTesterNumberParam = iotwebconf::TextParameter("TesterNumber", "mqttTesterNumber", _mqttTesterNumber, 2, "text", "1,2...");
	const char *ntpServer = "pool.ntp.org";
	char _otaUrl[STR_LEN];

	void doPing()
	{
		char buf[MaxMQTTTopic];
		String s;
		sprintf(buf, "%s/tele/%s/ping", _mqttRootTopic, _mqttTesterNumber);
		_JSdoc.clear();
		_JSdoc["IP"] = WiFi.localIP().toString().c_str();
		_JSdoc["Version"] = CONFIG_VERSION;
		_JSdoc["TesterNumber"] = _mqttTesterNumber;
		StaticJsonDocument<MaxMQTTPayload> config;
		config["LowCutoff"] = _config.getLowCutoff();
		config["ThermalShutdownTemperature"] = _config.getThermalShutdownTemperature();
		config["StorageVoltage"] = _config.getStorageVoltage();
		config["StabilizeDuration"] = _config.getStabilizeDuration();
		config["ChargeCurrent"] = _config.getChargeCurrent();
		config["ChargeDischargeCycleCount"] = _config.getChargeDischargeCycleCount();
		_JSdoc["Config"] = config;
		serializeJson(_JSdoc, s);
		_mqttClient.publish(buf, 0, false, s.c_str());
		_mqttClient.publish(_willTopic, 0, false, "Online");
	}

	void onMqttConnect(bool sessionPresent)
	{
		logd("Connected to MQTT. Session present: %d", sessionPresent);
		char mqttCmndTopic[STR_LEN];
		sprintf(mqttCmndTopic, "%s/cmnd/%s", _mqttRootTopic, Subtopics[Subtopic::operation]);
		uint16_t packetIdSub = _mqttClient.subscribe(mqttCmndTopic, 1);
		logd("MQTT subscribing to: %s", mqttCmndTopic);
		sprintf(mqttCmndTopic, "%s/cmnd/%s", _mqttRootTopic, Subtopics[Subtopic::config]);
		packetIdSub = _mqttClient.subscribe(mqttCmndTopic, 1);
		logd("MQTT subscribing to: %s", mqttCmndTopic);
		sprintf(mqttCmndTopic, "%s/cmnd/%s", _mqttRootTopic, Subtopics[Subtopic::ping]);
		packetIdSub = _mqttClient.subscribe(mqttCmndTopic, 1);
		logd("MQTT subscribing to: %s", mqttCmndTopic);
		sprintf(mqttCmndTopic, "%s/cmnd/%s", _mqttRootTopic, Subtopics[Subtopic::outcome]);
		packetIdSub = _mqttClient.subscribe(mqttCmndTopic, 1);
		logd("MQTT subscribing to: %s", mqttCmndTopic);
		sprintf(mqttCmndTopic, "%s/cmnd/%s", _mqttRootTopic, Subtopics[Subtopic::monitor]);
		packetIdSub = _mqttClient.subscribe(mqttCmndTopic, 1);
		logd("MQTT subscribing to: %s", mqttCmndTopic);
		sprintf(mqttCmndTopic, "%s/cmnd/%s/%s", _mqttRootTopic, _mqttTesterNumber, Subtopics[Subtopic::flash]);
		packetIdSub = _mqttClient.subscribe(mqttCmndTopic, 1);
		logd("MQTT subscribing to: %s", mqttCmndTopic);
		doPing();
		_tester1.setState(Standby);
		_tester2.setState(Standby);
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
		switch (event)
		{
		case SYSTEM_EVENT_STA_GOT_IP:
			logd("WiFi connected, IP address: %s", WiFi.localIP().toString().c_str());
			_JSdoc.clear();
			_JSdoc["IP"] = WiFi.localIP().toString().c_str();
			_JSdoc["ApPassword"] = TAG;
			serializeJson(_JSdoc, s);
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
			char tbuf[16];
			sprintf(tbuf, "%s/%s", _mqttTesterNumber, Subtopics[Subtopic::flash]);
			logd("Payload: %s", buf);
			if (strcmp(subtopic, Subtopics[Subtopic::ping]) == 0) // ping telemetry response with IP address
			{
				doPing();
			}
			else if (strcmp(subtopic, Subtopics[Subtopic::operation]) == 0) // mode of operation
			{
				Operation op = MonitorOperation;
				if (strncmp(payload, Operations[MonitorOperation], len) == 0)
				{
					op = MonitorOperation;
				}
				else if (strncmp(payload, Operations[TestCycleOperation], len) == 0)
				{
					op = TestCycleOperation;
				}
				else if (strncmp(payload, Operations[ChargeOperation], len) == 0)
				{
					op = ChargeOperation;
				}
				else if (strncmp(payload, Operations[TestAndStoreOperation], len) == 0)
				{
					op = TestAndStoreOperation;
				}
				else if (strncmp(payload, Operations[TestAndChargeOperation], len) == 0)
				{
					op = TestAndChargeOperation;
				}
				else if (strncmp(payload, Operations[StorageOperation], len) == 0)
				{
					op = StorageOperation;
				}
				else if (strncmp(payload, Operations[InternalResistanceOperation], len) == 0)
				{
					op = InternalResistanceOperation;
				}
				else if (strncmp(payload, Operations[DischargeOperation], len) == 0)
				{
					op = DischargeOperation;
				}
				_tester1.Perform(op);
				_tester2.Perform(op);
			}
			else if (strcmp(subtopic, Subtopics[Subtopic::outcome]) == 0)
			{
				_tester1.PublishOutcome();
				_tester2.PublishOutcome();
			}
			else if (strcmp(subtopic, Subtopics[Subtopic::monitor]) == 0)
			{
				_tester1.PublishUpdate();
				_tester2.PublishUpdate();
			}
			else if (strcmp(subtopic, Subtopics[Subtopic::config]) == 0)
			{
				if (strncmp(payload, "LoadDefaultSettings", len) == 0) // reset config to factory default
				{
					_config.LoadFactoryDefault();
				}
				else // set configuration
				{
					_JSdoc.clear();
					DeserializationError error = deserializeJson(_JSdoc, payload, len);
					if (!error)
					{
						if (_JSdoc.containsKey("LowCutoff"))
						{
							uint16_t val = _JSdoc["LowCutoff"];
							_config.setLowCutoff(val);
						}
						if (_JSdoc.containsKey("ThermalShutdownTemperature"))
						{
							uint16_t val = _JSdoc["ThermalShutdownTemperature"];
							_config.setThermalShutdownTemperature(val);
						}
						if (_JSdoc.containsKey("StorageVoltage"))
						{
							uint16_t val = _JSdoc["StorageVoltage"];
							_config.setStorageVoltage(val);
						}
						if (_JSdoc.containsKey("StabilizeDuration"))
						{
							uint16_t val = _JSdoc["StabilizeDuration"];
							_config.setStabilizeDuration(val);
						}
						if (_JSdoc.containsKey("ChargeCurrent"))
						{
							uint8_t val = _JSdoc["ChargeCurrent"];
							_config.setChargeCurrent(val);
						}
						if (_JSdoc.containsKey("ChargeDischargeCycleCount"))
						{
							uint8_t val = _JSdoc["ChargeDischargeCycleCount"];
							_config.setChargeDischargeCycleCount(val);
						}
					}
					else
					{
						logw("JSON error!");
					}
				}
			}
			else if (strcmp(subtopic, tbuf) == 0) // testerNumber/flash
			{
				_JSdoc.clear();
				DeserializationError error = deserializeJson(_JSdoc, payload, len);
				if (!error)
				{
					if (_JSdoc.containsKey("ServerUrl"))
					{
						strncpy(_otaUrl, _JSdoc["ServerUrl"], STR_LEN);
						doOTA(_otaUrl);
					}
				}
			}
		}
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
		s += "<li>Tester Group Name: ";
		s += _mqttRootTopic;
		s += "</ul>";
		s += "<ul>";
		s += "<li>Tester Number: ";
		s += _mqttTesterNumber;
		s += "</ul>";
		s += "Go to <a href='config'>configure page</a> to change values.";
		s += "</body></html>\n";
		_webServer.send(200, "text/html", s);
	}

	IOT::IOT()
	{
	}

	void IOT::Init()
	{
		_iotWebConf.setStatusPin(WIFI_STATUS_PIN);
		_iotWebConf.setConfigPin(WIFI_AP_PIN);
		if (FACTORY_RESET_PIN != -1)
		{
			pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
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
		}
		mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(5000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
		WiFi.onEvent(WiFiEvent);
		mqttGroup.addItem(&mqttServerParam);
		mqttGroup.addItem(&mqttPortParam);
		mqttGroup.addItem(&mqttUserNameParam);
		mqttGroup.addItem(&mqttUserPasswordParam);
		mqttGroup.addItem(&mqttRootTopicParam);
		mqttGroup.addItem(&mqttTesterNumberParam);
		_iotWebConf.addParameterGroup(&mqttGroup);
		boolean validConfig = _iotWebConf.init();
		if (!validConfig)
		{
			logw("!invalid configuration!");
			_mqttServer[0] = '\0';
			_mqttPort[0] = '\0';
			_mqttUserName[0] = '\0';
			_mqttUserPassword[0] = '\0';
			strcpy(_mqttRootTopic, _iotWebConf.getThingName());
			_mqttTesterNumber[0] = '\0';
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
					sprintf(_willTopic, "%s/tele/%s/LWT", _mqttRootTopic, _mqttTesterNumber);
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
		if (!_clientsConfigured || !WiFi.isConnected())
		{
			// process serial command from flasher tool to setup wifi
			if (Serial.peek() == '{')
			{
				String s = Serial.readStringUntil('}');
				s += "}";
				_JSdoc.clear();
				DeserializationError err = deserializeJson(_JSdoc, s);
				if (err)
				{
					loge("deserializeJson() failed: %s", err.c_str());
				}
				else
				{
					if (_JSdoc.containsKey("ssid") && _JSdoc.containsKey("password"))
					{
						SetupWifi(_JSdoc["ssid"], _JSdoc["password"]);
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

	void IOT::SetupWifi(const char *ssid, const char *pw)
	{
		iotwebconf::Parameter *p = _iotWebConf.getWifiSsidParameter();
		strcpy(p->valueBuffer, ssid);
		logd("Setting ssid: %s", p->valueBuffer);
		p = _iotWebConf.getWifiPasswordParameter();
		strcpy(p->valueBuffer, pw);
		logd("Setting password: %s", p->valueBuffer);
		p = _iotWebConf.getApPasswordParameter();
		strcpy(p->valueBuffer, TAG); // reset to default AP password
		logd("Setting AP password: %s", p->valueBuffer);
		_iotWebConf.saveConfig();
		esp_restart(); // force reboot
	}

	void IOT::publish(uint8_t pos, const char *subtopic, StaticJsonDocument<MaxMQTTPayload> *doc, boolean retained)
	{
		if (_mqttClient.connected())
		{
			char buf[MaxMQTTTopic];
			sprintf(buf, "%s/stat/%s/%s", _mqttRootTopic, _mqttTesterNumber, subtopic);
			int testerIndex = (atoi(_mqttTesterNumber) - 1) * 2; // 2 cells per tester * testerNumber origin 0
			(*doc)[Elements[Id::index]] = testerIndex + pos;	 // cell index (origin 0) out of all testers
			(*doc)[Elements[Id::cell]] = pos;					 // battery cell position (0 or 1) in this tester
			String s;
			serializeJson(*doc, s);
			logd("publish %s|%s", buf, s.c_str());
			_mqttClient.publish(buf, 0, retained, s.c_str());
		}
	}

} // namespace BatteryTester