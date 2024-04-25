/*
ToDo:
1) implement alarm activation (pins signal samples)
2) implement calibration command/procedure
4) Improve help()
5) Improve status()
*/
#include <WiFi.h>
#include <SPI.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include "configuration.h"
#include "datastructures.h"
#include "CatDoorBell.h"
#include "command_strings.h"
#include <ArduinoBLE.h>

String deviceName = "";
uint8_t wifi_mode = WIFI_OFF;

bool wifiEnable = true;

bool bleEnable = true;
uint8_t bleReportDistance = 200; // 100 cm. by default
uint32_t bleReportDelay = 60 * 1000; // after 1 minute by default
uint32_t bleReportEndTime = 0;
uint32_t bleAlarmDelay = 3 * 1000; //3 sec. by default
uint32_t bleAlarmEndTime = 0;
uint32_t bleAlarmFreq = 3000; //3kHz by default
bool bleAlarmStarted = false;
uint8_t buzzerPin = 0;
uint8_t lightPin = 0;

String bleUsers[BLE_USERS_NUMBER];
BleTokenReport bleUsersReports[BLE_USERS_NUMBER];
uint8_t reportReadPosition = 0;
uint8_t reportWritePosition = 0;

bool uartReady = false;
String uartCommand = "";

bool timeIsSet = false;
uint8_t autoReport = 0;
uint8_t autoReportAlways = 0;

// OTA UPDATE
#ifdef OTA_UPDATE_ENABLE
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

bool otaEnable = false;

/*
void otaStartCallback()
{
	String type;
	if (ArduinoOTA.getCommand() == U_FLASH)
	{
		Serial.println(F("Start updating sketch"));
	}
	else  // U_FS
	{
		// NOTE: if updating FS this would be the place to unmount FS using FS.end()
		Serial.println(F("Start updating filesystem"));
	}
}

void otaErrorCallback(ota_error_t error)
{
	//Serial.printf("Error[%u]: ", error);
	if (error == OTA_AUTH_ERROR) {
		Serial.println("Auth Failed");
	}
	else if (error == OTA_BEGIN_ERROR) {
		Serial.println("Begin Failed");
	}
	else if (error == OTA_CONNECT_ERROR) {
		Serial.println("Connect Failed");
	}
	else if (error == OTA_RECEIVE_ERROR) {
		Serial.println("Receive Failed");
	}
	else if (error == OTA_END_ERROR) {
		Serial.println("End Failed");
	}
}

void otaProgressCallback(unsigned int progress, unsigned int total)
{
	//Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
}

void otaEndCallback()
{
	Serial.println(F("\nEnd"));
}
*/
#endif

// NTP
#ifdef NTP_TIME_ENABLE
#include <WiFiUdp.h>

WiFiUDP Udp;
int ntpRefreshDelay = 180;
uint32_t lastTimeRefersh = 0;
uint32_t timeRefershPeriod = 24 * 60 * 60 * 1000; //once a day
bool NTPenable = false;

// send an NTP request to the time server at the given MacAddress
void sendNTPpacket(IPAddress& address)
{
	uint8_t packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011; // LI, Version, Mode
	packetBuffer[1] = 0;		  // Stratum, or type of clock
	packetBuffer[2] = 6;		  // Polling Interval
	packetBuffer[3] = 0xEC;		  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;
	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	Udp.beginPacket(address, 123); //NTP requests are to port 123
	Udp.write(packetBuffer, NTP_PACKET_SIZE);
	Udp.endPacket();
}

time_t getNtpTime()
{
	if (!NTPenable || WiFi.status() != WL_CONNECTED)
		return 0;

	while (Udp.parsePacket() > 0)
		yield(); // discard any previously received packets

	IPAddress timeServerIP(129, 6, 15, 28); //129.6.15.28 = time-a.nist.gov

	String ntpServer = readConfigString(NTP_SERVER_addr, NTP_SERVER_size);
	const int timeZone = readConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size).toInt();

	if (!timeServerIP.fromString(ntpServer))
	{
		if (WiFi.hostByName(ntpServer.c_str(), timeServerIP) != 1)
		{
			timeServerIP = IPAddress(129, 6, 15, 28);
		}
	}

	sendNTPpacket(timeServerIP);
	const uint32_t beginWait = millis();
	while (millis() - beginWait < 1500)
	{
		const int size = Udp.parsePacket();
		if (size >= NTP_PACKET_SIZE)
		{
			uint8_t packetBuffer[NTP_PACKET_SIZE];	 //buffer to hold incoming & outgoing packets
			Udp.read(packetBuffer, NTP_PACKET_SIZE); // read packet into the buffer
			timeIsSet = true;
			// convert four bytes starting at location 40 to a long integer
			uint32_t secsSince1900 = (uint32_t)packetBuffer[40] << 24;
			secsSince1900 |= (uint32_t)packetBuffer[41] << 16;
			secsSince1900 |= (uint32_t)packetBuffer[42] << 8;
			secsSince1900 |= (uint32_t)packetBuffer[43];
			//NTPenable = false;
			setSyncProvider(nullptr);
			Udp.stop();
			lastTimeRefersh = millis();
			return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
		}
		yield();
	}
	return 0; // return 0 if unable to get the time
}

void restartNTP()
{
	ntpRefreshDelay = readConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size).toInt();
	if (readConfigString(NTP_ENABLE_addr, NTP_ENABLE_size) == SWITCH_ON_NUMBER)
		NTPenable = true;

	if (NTPenable && wifi_mode != WIFI_OFF)
	{
		timeRefershPeriod = readConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size).toInt() * 1000;
		Udp.begin(UDP_LOCAL_PORT);
		setSyncProvider(getNtpTime);
		setSyncInterval(ntpRefreshDelay); //60 sec. = 1 min.
		lastTimeRefersh = millis();
	}
}

#endif

// MQTT
#ifdef MQTT_ENABLE
#include <PubSubClient.h>
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

String mqttCommand = "";
bool mqttEnable = false;

String getMqttServer()
{
	return readConfigString(MQTT_SERVER_addr, MQTT_SERVER_size);
}

int getMqttPort()
{
	return readConfigString(MQTT_PORT_addr, MQTT_PORT_size).toInt();
}

String getMqttUser()
{
	return readConfigString(MQTT_USER_addr, MQTT_USER_size);
}

String getMqttPassword()
{
	return readConfigString(MQTT_PASS_addr, MQTT_PASS_size);
}

String getMqttDeviceId()
{
	return readConfigString(MQTT_ID_addr, MQTT_ID_size);
}

String getMqttTopicIn()
{
	return readConfigString(MQTT_TOPIC_IN_addr, MQTT_TOPIC_IN_size);
}

String getMqttTopicOut()
{
	return readConfigString(MQTT_TOPIC_OUT_addr, MQTT_TOPIC_OUT_size);
}

bool getMqttClean()
{
	bool mqttClean = false;
	if (readConfigString(MQTT_CLEAN_addr, MQTT_CLEAN_size) == SWITCH_ON_NUMBER)
		mqttClean = true;
	return mqttClean;
}

bool mqtt_connect()
{
	bool result = false;
	if (mqttEnable && WiFi.status() == WL_CONNECTED)
	{
		IPAddress mqtt_ip_server;
		String mqtt_server = getMqttServer();
		uint16_t mqtt_port = getMqttPort();
		String mqtt_User = getMqttUser();
		String mqtt_Password = getMqttPassword();
		String mqtt_device_id = getMqttDeviceId();
		String mqtt_topic_in = getMqttTopicIn();
		bool mqttClean = getMqttClean();

		if (mqtt_ip_server.fromString(mqtt_server))
		{
			mqtt_client.setServer(mqtt_ip_server, mqtt_port);
		}
		else
		{
			mqtt_client.setServer(mqtt_server.c_str(), mqtt_port);
		}

		//mqtt_client.setCallback(mqtt_callback);

		if (mqtt_device_id.length() <= 0)
		{
			mqtt_device_id = deviceName;
			mqtt_device_id += F("_");
			mqtt_device_id += CompactMac();
		}

		if (mqtt_User.length() > 0)
		{
			result = mqtt_client.connect(mqtt_device_id.c_str(), mqtt_User.c_str(), mqtt_Password.c_str(), "", 0, false, "", mqttClean);
		}
		else
		{
			result = mqtt_client.connect(mqtt_device_id.c_str());
		}

		if (!result)
		{
			return result;
		}

		result = mqtt_client.subscribe(mqtt_topic_in.c_str());
	}

	return result;
}

bool mqtt_send(String& message, int dataLength, String& topic)
{
	if (!mqtt_client.connected())
	{
		mqtt_connect();
	}

	bool result = false;
	if (mqttEnable && WiFi.status() == WL_CONNECTED)
	{
		if (topic.length() <= 0)
			topic = getMqttTopicOut();

		if (dataLength > MQTT_MAX_PACKET)
		{
			result = mqtt_client.beginPublish(topic.c_str(), dataLength, false);
			for (uint16_t i = 0; i < dataLength; i++)
			{
				mqtt_client.write(message[i]);
				yield();
			}
			result = mqtt_client.endPublish();
		}
		else
		{
			result = mqtt_client.publish(topic.c_str(), message.c_str(), dataLength);
		}
	}
	return result;
}

void mqtt_callback(char* topic, uint8_t* payload, uint16_t dataLength)
{
	for (uint16_t i = 0; i < dataLength; i++)
	{
		mqttCommand += char(payload[i]);
		yield();
	}
}

#endif

// TELEGRAM
#ifdef TELEGRAM_ENABLE
#include <ArduinoJson.h>
#include <CTBot.h>

CTBot myBot;
telegramMessage telegramOutboundBuffer[TELEGRAM_MESSAGE_BUFFER_SIZE];
uint8_t telegramOutboundBufferPos = 0;
uint32_t telegramLastTime = 0;
bool telegramEnable = false;

String uint64ToString(uint64_t input)
{
	String result = "";
	uint8_t base = 10;
	do
	{
		char c = input % base;
		input /= base;
		if (c < 10)
			c += '0';
		else
			c += 'A' - 10;
		result = c + result;
		yield();
	} while (input);
	return result;
}

uint64_t StringToUint64(String& value)
{
	for (uint8_t i = 0; i < value.length(); i++)
	{
		if (value[i] < '0' || value[i] > '9')
			return 0;
	}
	uint64_t result = 0;
	const char* p = value.c_str();
	const char* q = p + value.length();
	while (p < q)
	{
		result = (result << 1) + (result << 3) + *(p++) - '0';
		yield();
	}
	return result;
}

bool sendToTelegramServer(int64_t user, String& message)
{
	bool result = false;
	if (telegramEnable && WiFi.status() == WL_CONNECTED)
	{
		result = myBot.testConnection();
		if (result)
		{
			result = myBot.sendMessage(user, message);
			if (result)
			{
				result = true;
			}
		}
	}
	return result;
}

void addMessageToTelegramOutboundBuffer(int64_t user, String message, uint8_t retries)
{
	if (telegramOutboundBufferPos >= TELEGRAM_MESSAGE_BUFFER_SIZE)
	{
		for (uint8_t i = 0; i < TELEGRAM_MESSAGE_BUFFER_SIZE - 1; i++)
		{
			telegramOutboundBuffer[i] = telegramOutboundBuffer[i + 1];
			yield();
		}
		telegramOutboundBufferPos = TELEGRAM_MESSAGE_BUFFER_SIZE - 1;
	}
	telegramOutboundBuffer[telegramOutboundBufferPos].user = user;
	telegramOutboundBuffer[telegramOutboundBufferPos].message = message;
	telegramOutboundBuffer[telegramOutboundBufferPos].retries = retries;
	telegramOutboundBufferPos++;
}

void removeMessageFromTelegramOutboundBuffer()
{
	if (telegramOutboundBufferPos <= 0)
		return;

	for (uint8_t i = 0; i < telegramOutboundBufferPos; i++)
	{
		if (i < TELEGRAM_MESSAGE_BUFFER_SIZE - 1)
			telegramOutboundBuffer[i] = telegramOutboundBuffer[i + 1];
		else
		{
			telegramOutboundBuffer[i].message.clear();
			telegramOutboundBuffer[i].user = 0;
		}
		yield();
	}
	telegramOutboundBufferPos--;
}

void sendToTelegram(int64_t user, String& message, uint8_t retries)
{
	// slice message to pieces
	while (message.length() > 0)
	{
		if (message.length() >= TELEGRAM_MESSAGE_MAX_SIZE)
		{
			addMessageToTelegramOutboundBuffer(user, message.substring(0, TELEGRAM_MESSAGE_MAX_SIZE), retries);
			message = message.substring(TELEGRAM_MESSAGE_MAX_SIZE);
		}
		else
		{
			addMessageToTelegramOutboundBuffer(user, message, retries);
			message.clear();
		}
		yield();
	}
}

void sendBufferToTelegram()
{
	if (telegramEnable && WiFi.status() == WL_CONNECTED && myBot.testConnection())
	{
		/*
		bool isRegisteredUser = false;
		bool isAdmin = false;
		TBMessage msg;
		while (myBot.getNewMessage(msg))
		{
			yield();
			//consider 1st sender an admin if admin is empty
			if (getTelegramUser(0) != 0)
			{
				//check if it's registered user
				for (uint8_t i = 0; i < TELEGRAM_USERS_NUMBER; i++)
				{
					yield();
					if (getTelegramUser(i) == msg.sender.id)
					{
						isRegisteredUser = true;
						if (i == 0)
							isAdmin = true;
						break;
					}
				}
			}
			yield();
			//process data from TELEGRAM
			if (isRegisteredUser && msg.text.length() > 0)
			{
				//sendToTelegram(msg.sender.id, deviceName + ": " + msg.text, telegramRetries);
				String tmpCmd = msg.text;
				String str = processCommand(tmpCmd, CHANNEL_TELEGRAM, isAdmin);
				String message = deviceName;
				message += F(": ");
				message += str;
				sendToTelegram(msg.sender.id, message, TELEGRAM_RETRIES);
			}
		}
		*/
		if (telegramOutboundBuffer[0].message.length() != 0)
		{
			if (sendToTelegramServer(telegramOutboundBuffer[0].user, telegramOutboundBuffer[0].message))
			{
				removeMessageFromTelegramOutboundBuffer();
			}
			else
			{
				telegramOutboundBuffer[0].retries--;
				if (telegramOutboundBuffer[0].retries <= 0)
				{
					removeMessageFromTelegramOutboundBuffer();
				}
			}
		}
	}
}

uint64_t getTelegramUser(uint8_t n)
{
	uint16_t singleUserLength = TELEGRAM_USERS_TABLE_size / TELEGRAM_USERS_NUMBER;
	String userStr = readConfigString((TELEGRAM_USERS_TABLE_addr + n * singleUserLength), singleUserLength);
	userStr.trim();
	return StringToUint64(userStr);
}

#endif

void bleCentralDiscoverHandler(BLEDevice peripheral)
{
	String addr = peripheral.address();

	//Serial.println(addr);
	/*if (name.length() > 0)
		Serial.println(name);

	Serial.println(rssi);

	// print the advertised service UUIDs, if present
	if (peripheral.hasAdvertisedServiceUuid())
	{
		Serial.println("Service UUIDs: ");
		for (int i = 0; i < peripheral.advertisedServiceUuidCount(); i++)
		{
			Serial.print("\t");
			Serial.println(peripheral.advertisedServiceUuid(i));
		}
	}

	Serial.println();*/

	for (int i = 0; i < BLE_USERS_NUMBER; i++)
	{
		if (bleUsers[i] == addr)
		{
			Serial.println("User found");
			BleTokenReport report;
			report.userIndex = i;
			report.MacAddress = addr;

			if (peripheral.hasLocalName())
				report.localName = peripheral.localName();
			else
				report.localName = peripheral.deviceName();

			report.rssi = peripheral.rssi();
			report.time = now();
			if (peripheral.hasManufacturerData())
			{
				const int l = peripheral.manufacturerDataLength();
				report.manufacturerDataLength = peripheral.manufacturerData(report.manufacturerData, l);
			}

			if (peripheral.hasAdvertisementData() && peripheral.advertisementDataLength() > 0)
			{
				const int l = peripheral.advertisementDataLength();
				report.advertisementDataLength = peripheral.advertisementData(report.advertisementData, l);
			}

			bleUsersReports[reportWritePosition] = report;
			reportWritePosition++;

			if (reportWritePosition >= BLE_USERS_NUMBER)
				reportWritePosition = 0;
		}
	}
}

void setup()
{
	//initialize Serial Monitor
	Serial.begin(115200);

#ifdef MQTT_ENABLE
	mqttCommand.reserve(256);
#endif

#ifdef TELEGRAM_ENABLE
	for (uint8_t b = 0; b < TELEGRAM_MESSAGE_BUFFER_SIZE; b++)
	{
		telegramOutboundBuffer[0].message.reserve(TELEGRAM_MESSAGE_MAX_SIZE);
		yield();
	}
#endif
	yield();

	EEPROM.begin(collectEepromSize());
	//clear EEPROM if it's just initialized
	if (!checkEepromCrc())
	{
		clearEeprom();
	}

	for (int m = 0; m < BLE_USERS_NUMBER; m++)
	{
		bleUsers[m] = getBleUser(m);
	}

	//pinMode(LED_BUILTIN, OUTPUT);
	//digitalWrite(LED_BUILTIN, LOW);

	pinMode(PRG_BUTTON, INPUT);
	attachInterrupt(PRG_BUTTON, onExtButton, RISING);

	yield();
	deviceName = readConfigString(DEVICE_NAME_addr, DEVICE_NAME_size);
	autoReport = readConfigString(AUTOREPORT_addr, AUTOREPORT_size).toInt();
	autoReportAlways = readConfigString(AUTOREPORT_ALWAYS_addr, AUTOREPORT_ALWAYS_size).toInt();
	yield();

	// init WiFi
	startWiFi();
	yield();
	String tmpStr;
	tmpStr.reserve(101);
#ifdef OTA_UPDATE_ENABLE
	if (readConfigString(OTA_ENABLE_addr, OTA_ENABLE_size) == SWITCH_ON_NUMBER)
		otaEnable = true;

	uint16_t otaPort = readConfigString(OTA_PORT_addr, OTA_PORT_size).toInt();
	if (otaPort > 0)
	{
		ArduinoOTA.setPort(otaPort);
	}

	tmpStr = readConfigString(OTA_PASSWORD_addr, OTA_PASSWORD_size);
	if (tmpStr.length() > 0)
	{
		ArduinoOTA.setPassword(tmpStr.c_str());
	}

	tmpStr = readConfigString(DEVICE_NAME_addr, DEVICE_NAME_size);
	tmpStr += F("_");
	tmpStr += CompactMac();
	ArduinoOTA.setHostname(tmpStr.c_str());

	//ArduinoOTA.onStart(otaStartCallback);
	//ArduinoOTA.onEnd(otaEndCallback);
	//ArduinoOTA.onProgress(otaProgressCallback);
	//ArduinoOTA.onError(otaErrorCallback);

	if (otaEnable && wifi_mode != WIFI_OFF)
		ArduinoOTA.begin();
#endif
	yield();
#ifdef MQTT_ENABLE
	if (readConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size) == SWITCH_ON_NUMBER)
		mqttEnable = true;

	if (mqttEnable && wifi_mode != WIFI_OFF)
		mqtt_connect();

#endif
	yield();
#ifdef TELEGRAM_ENABLE
	String telegramToken = readConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size);
	if (readConfigString(TELEGRAM_ENABLE_addr, TELEGRAM_ENABLE_size) == SWITCH_ON_NUMBER)
		telegramEnable = true;

	if (telegramEnable && wifi_mode != WIFI_OFF)
	{
		myBot.setTelegramToken(telegramToken);
		myBot.testConnection();
	}
#endif

#ifdef NTP_TIME_ENABLE
	restartNTP();
#endif

	bleReportDistance = readConfigString(BLE_REPORT_DISTANCE_addr, BLE_REPORT_DISTANCE_size).toInt();
	bleReportDelay = readConfigString(BLE_REPORT_DELAY_addr, BLE_REPORT_DELAY_size).toInt() * 1000;
	bleAlarmDelay = readConfigString(ALARM_DELAY_addr, ALARM_DELAY_size).toInt() * 1000;
	bleAlarmFreq = readConfigString(ALARM_FREQ_addr, ALARM_FREQ_size).toInt();

	lightPin = readConfigString(LIGHT_PIN_addr, LIGHT_PIN_size).toInt();
	init_light_pin();
	/*ledcSetup(LIGHT_PWM_CHANNEL + PWM_CHANNEL_OFFSET, 500, PWM_RESOLUTION);
	ledcAttachPin(lightPin, LIGHT_PWM_CHANNEL + PWM_CHANNEL_OFFSET); // Attach the LED PWM Channel to the GPIO Pin
	set_output(LIGHT_PWM_CHANNEL, 0);*/

	buzzerPin = readConfigString(BUZZER_PIN_addr, BUZZER_PIN_size).toInt();
	init_buzzer_pin();

	// start BlueTooth
	if (!BLE.begin())
	{
		Serial.println(F("starting Bluetooth Low Energy module failed!"));
		bleEnable = false;
	}
	else
	{
		BLE.setEventHandler(BLEDiscovered, bleCentralDiscoverHandler);
		// start scanning for peripheral
		BLE.scan(true);
		bleEnable = true;
	}
}

void loop()
{
	if (bleEnable)
		BLE.poll();

	if (reportWritePosition != reportReadPosition)
	{

		BleTokenReport report = bleUsersReports[reportReadPosition];
		reportReadPosition++;
		if (reportReadPosition >= BLE_USERS_NUMBER)
			reportReadPosition = 0;

		String delimiter = F("\",\r\n\"");
		String eq = F("\":\"");
		String str;
		str.reserve(1024);
		str = F("{\r\n\"");

		str += F("RSSI");
		str += eq;
		str += report.rssi;
		str += delimiter;

		str += F("MAC");
		str += eq;
		str += report.MacAddress;
		str += delimiter;

		str += F("LocalName");
		str += eq;
		str += report.localName;
		str += delimiter;

		str += F("Description");
		str += eq;
		str += getBleUserDescription(report.userIndex);
		str += delimiter;

		if (report.manufacturerDataLength > 0)
		{
			str += F("MfgData[");
			str += String(report.manufacturerDataLength);
			str += F("]");
			str += eq;
			for (int i = 0; i < report.manufacturerDataLength; i++)
			{
				str += F(" 0x");
				str += String((unsigned int)report.manufacturerData[i], HEX);
			}
			str += delimiter;
		}

		float calibratedPower = getBleRssiCalibration(report.userIndex);
		if (calibratedPower == 0)
		{
			//Serial.println(F("Token doesn't have RSSI calibration"));
			// try to get 1 meter RSSI from manufacturer data of the advertizement packet
			if (report.advertisementDataLength >= 36)
			{
				/*Serial.print(F("Advertisement data: ["));
				Serial.print(report.advertisementDataLength);
				Serial.print(F("]:"));
				for (int i = 0; i < report.advertisementDataLength; i++)
				{
					Serial.print(F(" 0x"));
					Serial.print((unsigned int)report.advertisementData[i], HEX);
				}
				Serial.println();*/

				if (report.advertisementData[6] == 0x02 && report.advertisementData[7] == 0x01 && report.advertisementData[8] == 0x1a)
				{
					//Serial.println(F("Get token RSSI calibration from advertisement data"));
					calibratedPower = -report.advertisementData[35];
				}
			}
		}

		if (calibratedPower == 0)
		{
			//Serial.println(F("Token RSSI calibration set to default"));
			calibratedPower = -63.0f; // default value
		}

		str += F("Calibrated_1m_Power");
		str += eq;
		str += String(calibratedPower, 2);
		str += delimiter;

		uint16_t distance = pow(10, ((calibratedPower - report.rssi) / (10.0f * 2.0f))) * 100;
		str += F("Distance");
		str += eq;
		str += String(distance);
		str += delimiter;

		str += F("Time");
		str += eq;
		str += timeToString(report.time);
		//str += delimiter;

		str += F("\"\r\n}");

		Serial.print(F("["));
		Serial.print(report.localName);
		Serial.print(F("] token found: "));
		Serial.print(String(distance));
		Serial.println(F(" cm."));

		if (distance <= bleReportDistance && bleReportEndTime == 0)
		{
			Serial.println(F("Token within range"));

			bleReportEndTime = millis() + bleReportDelay;
			bleAlarmEndTime = millis() + bleAlarmDelay;

			Serial.println("Start alarm");

			// activate light
			if (lightPin > 0)
				digitalWrite(lightPin, HIGH);

			// activate buzzer
			if (buzzerPin > 0)
				esp32Tone(BUZZER_PWM_CHANNEL, bleAlarmFreq);

			// report to server
			if (autoReport > 0)
			{
				for (uint8_t b = 0; b < CHANNELS_NUMBER; b++)
				{
					if (bitRead(autoReport, b))
					{
						if (b == CHANNEL_UART)
						{
							Serial.println(str);
						}
						else if (b == CHANNEL_MQTT && mqttEnable)
						{
							mqtt_connect();

							String topic = "";
							mqtt_send(str, str.length(), topic);
							mqtt_client.disconnect();
						}
						else if (b == CHANNEL_TELEGRAM && telegramEnable)
						{
							for (int n = 0; n < TELEGRAM_USERS_NUMBER; n++)
							{
								int64_t user = getTelegramUser(n);
								if (user > 0)
								{
									sendToTelegram(user, str, 3);
								}
								yield();
							}
						}
					}
					yield();
				}
			}
		}

		if (autoReportAlways > 0)
		{
			for (uint8_t b = 0; b < CHANNELS_NUMBER; b++)
			{
				if (bitRead(autoReportAlways, b))
				{
					if (b == CHANNEL_UART)
					{
						Serial.println(str);
					}
					else if (b == CHANNEL_MQTT && mqttEnable)
					{
						if (!mqtt_client.connected())
						{
							mqtt_connect();
						}
						else
						{
							mqtt_client.loop();
						}

						String topic = "";
						mqtt_send(str, str.length(), topic);
					}
					else if (b == CHANNEL_TELEGRAM && telegramEnable)
					{
						for (int n = 0; n < TELEGRAM_USERS_NUMBER; n++)
						{
							int64_t user = getTelegramUser(n);
							if (user > 0)
							{
								sendToTelegram(user, str, 3);
							}
							yield();
						}
					}
				}
				yield();
			}
		}
	}

	//stop light/buzzer
	if (bleAlarmEndTime != 0 && millis() > bleAlarmEndTime)
	{
		Serial.println("Stop alarm");
		bleAlarmEndTime = 0;
		if (lightPin > 0)
		{
			digitalWrite(lightPin, LOW);
		}

		if (buzzerPin > 0)
		{
			esp32NoTone(BUZZER_PWM_CHANNEL);
		}
	}

	// reset report delay 
	if (bleReportEndTime != 0 && millis() > bleReportEndTime)
	{
		bleReportEndTime = 0;
	}

#ifdef NTP_TIME_ENABLE
	//refersh NTP time if time has come
	if (NTPenable && millis() - lastTimeRefersh > timeRefershPeriod)
	{
		restartNTP();
	}
#endif
	yield();

#ifdef HARD_UART_ENABLE
	//check UART for data
	while (Serial.available() && !uartReady)
	{
		char c = Serial.read();
		if (c == '\r' || c == '\n')
			uartReady = true;
		else
			uartCommand += (char)c;
		yield();
	}
	//process data from UART
	if (uartReady)
	{
		uartReady = false;
		if (uartCommand.length() > 0)
		{
			String str = processCommand(uartCommand, CHANNEL_UART, true);
			Serial.println(str);
		}
		uartCommand.clear();
	}
#endif
	yield();

	//process data from MQTT/TELEGRAM if available
	if (WiFi.status() == WL_CONNECTED || WiFi.softAPgetStationNum() > 0)
	{
#ifdef OTA_UPDATE_ENABLE
		if (otaEnable) ArduinoOTA.handle();
#endif
		yield();
#ifdef MQTT_ENABLE
		if (mqttEnable)
		{
			if (!mqtt_client.connected())
				mqtt_connect();
			else
				mqtt_client.loop();

			if (mqttCommand.length() > 0)
			{
				String str = processCommand(mqttCommand, CHANNEL_MQTT, true);
				mqttCommand.clear();
				String topic = "";
				mqtt_send(str, str.length(), topic);
			}
			yield();
		}
#endif
		yield();
#ifdef TELEGRAM_ENABLE
		//check TELEGRAM for data
		if (telegramEnable && WiFi.getMode() != WIFI_AP && millis() - telegramLastTime > TELEGRAM_MESSAGE_DELAY)
		{
			sendBufferToTelegram();
			telegramLastTime = millis();
		}
#endif
	}
	yield();
}

IRAM_ATTR void onExtButton()
{
	//sending = !sending;
}

float readBatteryVoltage()
{
	float v = 0;

#ifdef VBAT_ADC
	for (int i = 0; i < 20; i++)
	{
		if (VBAT_ADC == 35)
		{
			v += (float)(analogRead(VBAT_ADC)) / 4095.0 * 2.0 * 3.3 * 1.1;
		}
		else if (VBAT_ADC == 01)
		{
			v += (100.0 / (100.0 + 390.0) * (float)(analogRead(VBAT_ADC)));
		}
		else
			break;

		if (i > 0)
			v = v / 2;

		delay(10);
	}
#endif

	return v;
}

uint32_t collectEepromSize()
{
	return FINAL_CRC_addr + FINAL_CRC_size + 10; // add 10 bytes as a reserve
}

String readConfigString(uint16_t startAt, uint16_t maxlen)
{
	String str = "";
	str.reserve(maxlen);
	for (uint16_t i = 0; i < maxlen; i++)
	{
		char c = (char)EEPROM.read(startAt + i);
		if (c == 0 || c == 0xff)
			i = maxlen;
		else
			str += c;
		yield();
	}

	return str;
}

void readConfigString(uint16_t startAt, uint16_t maxlen, char* array)
{
	for (uint16_t i = 0; i < maxlen; i++)
	{
		array[i] = (char)EEPROM.read(startAt + i);
		if (array[i] == 0 || array[i] == 0xff)
			i = maxlen;
		yield();
	}
}

uint32_t readConfigLong(uint16_t startAt)
{
	union Convert
	{
		uint32_t number = 0;
		uint8_t byteNum[4];
	} p;
	for (uint8_t i = 0; i < 4; i++)
	{
		p.byteNum[i] = EEPROM.read(startAt + i);
		yield();
	}
	return p.number;
}

float readConfigFloat(uint16_t startAt)
{
	union Convert
	{
		float number = 0;
		uint8_t byteNum[4];
	} p;
	for (uint8_t i = 0; i < 4; i++)
	{
		p.byteNum[i] = EEPROM.read(startAt + i);
		yield();
	}
	return p.number;
}

void writeConfigString(uint16_t startAt, uint16_t maxLen, String str)
{
	for (uint16_t i = 0; i < maxLen; i++)
	{
		if (i < str.length())
			EEPROM.write(startAt + i, (uint8_t)str[i]);
		else
		{
			EEPROM.write(startAt + i, 0);
			break;
		}
		yield();
	}
	refreshEepromCrc();
}

void writeConfigString(uint16_t startAt, uint16_t maxlen, char* array, uint8_t length)
{
	for (uint16_t i = 0; i < maxlen; i++)
	{
		if (i >= length)
		{
			EEPROM.write(startAt + i, 0);
			break;
		}
		EEPROM.write(startAt + i, (uint8_t)array[i]);
		yield();
	}
	refreshEepromCrc();
}

void writeConfigLong(uint16_t startAt, uint32_t data)
{
	union Convert
	{
		uint32_t number = 0;
		uint8_t byteNum[4];
	} p;
	p.number = data;
	for (uint16_t i = 0; i < 4; i++)
	{
		EEPROM.write(startAt + i, p.byteNum[i]);
		yield();
	}
	refreshEepromCrc();
}

void writeConfigFloat(uint16_t startAt, float data)
{
	union Convert
	{
		float number = 0;
		uint8_t byteNum[4];
	} p;

	p.number = data;
	for (uint16_t i = 0; i < 4; i++)
	{
		EEPROM.write(startAt + i, p.byteNum[i]);
		yield();
	}
	refreshEepromCrc();
}

uint16_t calculateEepromCrc()
{
	uint16_t crc = 0xffff;
	for (int i = 0; i < FINAL_CRC_addr + FINAL_CRC_size; i++)
	{
		uint8_t b = EEPROM.read(i);
		crc ^= b;
		for (int x = 0; x < 8; x++)
		{
			if (crc & 0x0001)
			{
				crc >>= 1;
				crc ^= 0xA001;
			}
			else
			{
				crc >>= 1;
			}
			yield();
		}
		yield();
	}

	return crc;
}

bool checkEepromCrc()
{
	union Convert
	{
		uint32_t number = 0;
		uint8_t byteNum[4];
	} savedCrc;

	for (uint8_t i = 0; i < 2; i++)
	{
		savedCrc.byteNum[i] = EEPROM.read(FINAL_CRC_addr + i);
		yield();
	}
	uint16_t realCrc = calculateEepromCrc();

	return savedCrc.number == realCrc;
}

void refreshEepromCrc()
{
	union Convert
	{
		uint16_t number = 0;
		uint8_t byteNum[4];
	} realCrc;
	realCrc.number = calculateEepromCrc();
	for (uint16_t i = 0; i < 2; i++)
	{
		EEPROM.write(FINAL_CRC_addr + i, realCrc.byteNum[i]);
		yield();
	}
	EEPROM.commit();
}

void clearEeprom()
{
	/*for (uint32_t i = 0; i < EEPROM.length(); i++)
	{
		EEPROM.write(i, 0);
		yield();
	}
	EEPROM.commit();
	refreshEepromCrc();*/
}

String getBleUser(uint8_t n)
{
	uint16_t singleUserLength = BLE_USERS_TABLE_size / BLE_USERS_NUMBER;
	String userStr = readConfigString((BLE_USERS_TABLE_addr + n * singleUserLength), singleUserLength);
	userStr.trim();
	return userStr;
}

String getBleUserDescription(uint8_t n)
{
	uint16_t singleUserLength = BLE_USERS_DESC_TABLE_size / BLE_USERS_NUMBER;
	String userStr = readConfigString((BLE_USERS_DESC_TABLE_addr + n * singleUserLength), singleUserLength);
	userStr.trim();
	return userStr;
}

int getBleRssiCalibration(uint8_t n)
{
	uint16_t singleUserLength = BLE_RSSI_TABLE_size / BLE_USERS_NUMBER;
	String userStr = readConfigString((BLE_RSSI_TABLE_addr + n * singleUserLength), singleUserLength);
	userStr.trim();
	return -userStr.toInt();
}

// input string format: ****nn="****",nnn,"****",nnn
commandTokens parseCommand(String& commandString, char cmd_divider, char arg_divider, bool findIndex)
{
	commandTokens params;
	params.index = -1;
	params.tokensNumber = 0;

	if (commandString.length() <= 0)
		return params;

	int equalSignPos = commandString.indexOf(cmd_divider);
	if (equalSignPos > 0)
	{
		int lastIndex = equalSignPos + 1;
		do // get argument tokens
		{
			int t = commandString.indexOf(arg_divider, lastIndex);
			if (t > equalSignPos)
			{
				params.arguments[params.tokensNumber] = commandString.substring(lastIndex, t);
			}
			else
			{
				params.arguments[params.tokensNumber] = commandString.substring(lastIndex);
			}

			params.arguments[params.tokensNumber].trim();
			params.tokensNumber++;
			lastIndex = t + 1;
			yield();
		} while (lastIndex > equalSignPos);
	}
	else
	{
		equalSignPos = commandString.length();
	}

	int cmdEnd = equalSignPos - 1;
	if (findIndex)
	{
		for (; cmdEnd >= 0; cmdEnd--)
		{
			if (commandString[cmdEnd] < '0' || commandString[cmdEnd] > '9')
			{
				cmdEnd++;
				break;
			}
			yield();
		}
		// get command index
		if (cmdEnd < equalSignPos)
			params.index = commandString.substring(cmdEnd, equalSignPos).toInt();
	}

	// get command name
	if (cmdEnd > 0)
		params.command = commandString.substring(0, cmdEnd);

	return params;
}

String processCommand(String& command, uint8_t channel, bool isAdmin)
{
	commandTokens cmd = parseCommand(command, '=', ',', true);
	cmd.command.toLowerCase();
	String eq = F("=");
	String str;
	str.reserve(2048);

	if (cmd.command == CMD_GET_STATUS)
	{
		str = printStatus(false);
	}
	else if (cmd.command == CMD_HELP)
	{
		str = printHelp(isAdmin);
	}
	else if (isAdmin)
	{
		if (cmd.command == CMD_GET_CONFIG)
		{
			str = printConfig(false);
		}
		else if (cmd.command == CMD_TIME_SET)
		{
			uint8_t _hr = cmd.arguments[0].substring(11, 13).toInt();
			uint8_t _min = cmd.arguments[0].substring(14, 16).toInt();
			uint8_t _sec = cmd.arguments[0].substring(17, 19).toInt();
			uint8_t _day = cmd.arguments[0].substring(8, 10).toInt();
			uint8_t _month = cmd.arguments[0].substring(5, 7).toInt();
			uint16_t _yr = cmd.arguments[0].substring(0, 4).toInt();
			setTime(_hr, _min, _sec, _day, _month, _yr);
			timeIsSet = true;
#ifdef NTP_TIME_ENABLE
			NTPenable = false;
#endif

			str = REPLY_TIME_SET;
			str += eq;
			str += String(_yr);
			str += F(".");
			str += String(_month);
			str += F(".");
			str += String(_day);
			str += SPACE;
			str += String(_hr);
			str += COLON;
			str += String(_min);
			str += COLON;
			str += String(_sec);
		}
		else if (cmd.command == CMD_DEVICE_NAME)
		{
			deviceName = cmd.arguments[0];
			writeConfigString(DEVICE_NAME_addr, DEVICE_NAME_size, deviceName);

			str = REPLY_DEVICE_NAME;
			str += eq;
			str += deviceName;
		}
		else if (cmd.command == CMD_STA_SSID)
		{
			writeConfigString(STA_SSID_addr, STA_SSID_size, cmd.arguments[0]);

			str = REPLY_STA_SSID;
			str += eq;
			str += cmd.arguments[0];
		}
		else if (cmd.command == CMD_STA_PASS)
		{
			writeConfigString(STA_PASS_addr, STA_PASS_size, cmd.arguments[0]);

			str = REPLY_STA_PASS;
			str += eq;
			str += cmd.arguments[0];
		}
		else if (cmd.command == CMD_AP_SSID)
		{
			writeConfigString(AP_SSID_addr, AP_SSID_size, cmd.arguments[0]);

			str = REPLY_AP_SSID;
			str += eq;
			str += cmd.arguments[0];
		}
		else if (cmd.command == CMD_AP_PASS)
		{
			writeConfigString(AP_PASS_addr, AP_PASS_size, cmd.arguments[0]);

			str = REPLY_AP_PASS;
			str += eq;
			str += cmd.arguments[0];
		}
		else if (cmd.command == CMD_WIFI_POWER)
		{
			str = REPLY_WIFI_POWER;
			str += eq;
			float power = cmd.arguments[0].toFloat();
			if (power >= -1 && power <= 19.5)
			{
				str += String(power);
				writeConfigFloat(WIFI_POWER_addr, power);
			}
			else
			{
				str += REPLY_INCORRECT;
				str += cmd.arguments[0];
			}
		}
		else if (cmd.command == CMD_WIFI_MODE)
		{
			cmd.arguments[0].toUpperCase();
			uint8_t wifi_mode = 255;
			str = REPLY_WIFI_MODE;
			str += eq;
			if (cmd.arguments[0] == F("OFF"))
			{
				wifi_mode = WIFI_OFF;
			}
			else if (cmd.arguments[0] == F("STATION"))
			{
				wifi_mode = WIFI_STA;
			}
			else if (cmd.arguments[0] == F("AP"))
			{
				wifi_mode = WIFI_AP;
			}
			else if (cmd.arguments[0] == F("APSTATION"))
			{
				wifi_mode = WIFI_AP_STA;
			}
			else
			{
				str += REPLY_INCORRECT;
				str += cmd.arguments[0];
			}

			if (wifi_mode != 255)
			{
				writeConfigString(WIFI_MODE_addr, WIFI_MODE_size, String(wifi_mode));

				str += wifiModes[wifi_mode];
			}
		}
		else if (cmd.command == CMD_WIFI_ENABLE)
		{
			str = REPLY_WIFI_ENABLE;
			str += eq;

			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				wifiEnable = false;
				Start_OFF_Mode();
				str += SWITCH_OFF;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				wifiEnable = true;
				startWiFi();
				str += SWITCH_ON;
			}
			else
			{
				str += REPLY_INCORRECT;
				str += cmd.arguments[0];
			}
		}
		else if (cmd.command == CMD_RESET)
		{
			if (channel == CHANNEL_TELEGRAM)
			{
				uartCommand = F("reset");
				uartReady = true;
			}
			else
			{
#ifdef DEBUG_MODE
				//Serial.println(F("Resetting..."));
				//Serial.flush();
#endif
				ESP.restart();
			}
		}
		else if (cmd.command == CMD_AUTOREPORT)
		{
			autoReport = 0;
			str += REPLY_AUTOREPORT;
			str += eq;
			for (uint8_t i = 0; i < cmd.tokensNumber; i++)
			{
				for (uint8_t b = 0; b < CHANNELS_NUMBER; b++)
				{
					if (cmd.arguments[i].indexOf(channels[b], 0) > -1)
					{
						bitSet(autoReport, b);
						str += channels[b];
						str += F(",");
					}
					yield();
				}
			}
			writeConfigString(AUTOREPORT_addr, AUTOREPORT_size, String(autoReport));
		}
		else if (cmd.command == CMD_AUTOREPORT_ALWAYS)
		{
			autoReportAlways = 0;
			str += REPLY_AUTOREPORT_ALWAYS;
			str += eq;
			for (uint8_t i = 0; i < cmd.tokensNumber; i++)
			{
				for (uint8_t b = 0; b < CHANNELS_NUMBER; b++)
				{
					if (cmd.arguments[i].indexOf(channels[b], 0) > -1)
					{
						bitSet(autoReportAlways, b);
						str += channels[b];
						str += F(",");
					}
					yield();
				}
			}
			writeConfigString(AUTOREPORT_ALWAYS_addr, AUTOREPORT_ALWAYS_size, String(autoReportAlways));
		}
		else if (cmd.command == CMD_BLE_USER)
		{
			str = REPLY_BLE_USER;
			str += String(cmd.index);
			str += eq;

			if (cmd.index >= 0 && cmd.index < BLE_USERS_NUMBER)
			{
				uint16_t m = BLE_USERS_TABLE_size / BLE_USERS_NUMBER;
				writeConfigString(BLE_USERS_TABLE_addr + cmd.index * m, m, cmd.arguments[0]);
				str += cmd.arguments[0];
			}
			else
			{
				str += REPLY_INCORRECT;
				str += F(" out of range [0,");
				str += String(BLE_USERS_NUMBER - 1);
				str += F("]");
			}
		}
		else if (cmd.command == CMD_BLE_USER_DESC)
		{
			str = REPLY_BLE_USER_DESC;
			str += String(cmd.index);
			str += eq;

			if (cmd.index >= 0 && cmd.index < BLE_USERS_NUMBER)
			{
				uint16_t m = BLE_USERS_DESC_TABLE_size / BLE_USERS_NUMBER;
				writeConfigString(BLE_USERS_DESC_TABLE_addr + cmd.index * m, m, cmd.arguments[0]);
				str += cmd.arguments[0];
			}
			else
			{
				str += REPLY_INCORRECT;
				str += F(" out of range [0,");
				str += String(BLE_USERS_NUMBER - 1);
				str += F("]");
			}
		}
		else if (cmd.command == CMD_BLE_USER_RSSI)
		{
			str = REPLY_BLE_USER_RSSI;
			str += String(cmd.index);
			str += eq;

			if (cmd.index >= 0 && cmd.index < BLE_USERS_NUMBER)
			{
				uint16_t m = BLE_RSSI_TABLE_size / BLE_USERS_NUMBER;
				uint8_t v = abs(cmd.arguments[0].toInt());
				writeConfigString(BLE_RSSI_TABLE_addr + cmd.index * m, m, String(v));
				str += cmd.arguments[0];
			}
			else
			{
				str += REPLY_INCORRECT;
				str += F(" out of range [0,");
				str += String(BLE_USERS_NUMBER - 1);
				str += F("]");
			}
		}
		else if (cmd.command == CMD_BLE_REPORT_DISTANCE)
		{
			bleReportDistance = cmd.arguments[0].toInt();
			writeConfigString(BLE_REPORT_DISTANCE_addr, BLE_REPORT_DISTANCE_size, String(bleReportDistance));

			str = REPLY_BLE_REPORT_DISTANCE;
			str += eq;
			str += String(bleReportDistance);
		}
		else if (cmd.command == CMD_BLE_REPORT_DELAY)
		{
			bleReportDelay = cmd.arguments[0].toInt();
			writeConfigString(BLE_REPORT_DELAY_addr, BLE_REPORT_DELAY_size, String(bleReportDelay));

			str = REPLY_BLE_REPORT_DELAY;
			str += eq;
			str += String(bleReportDelay);
			bleReportDelay = bleReportDelay * 1000;
		}
		else if (cmd.command == CMD_LIGHT_PIN)
		{
			lightPin = cmd.arguments[0].toInt();
			writeConfigString(LIGHT_PIN_addr, LIGHT_PIN_size, String(lightPin));

			str = REPLY_LIGHT_PIN;
			str += eq;
			str += String(lightPin);
			pinMode(lightPin, OUTPUT);
			digitalWrite(lightPin, LOW);
		}
		else if (cmd.command == CMD_BUZZER_PIN)
		{
			buzzerPin = cmd.arguments[0].toInt();
			writeConfigString(BUZZER_PIN_addr, BUZZER_PIN_size, String(buzzerPin));

			str = REPLY_BUZZER_PIN;
			str += eq;
			str += String(buzzerPin);
		}
		else if (cmd.command == CMD_ALARM_DELAY)
		{
			bleAlarmDelay = cmd.arguments[0].toInt();
			writeConfigString(ALARM_DELAY_addr, ALARM_DELAY_size, String(bleAlarmDelay));

			str = REPLY_ALARM_DELAY;
			str += eq;
			str += String(bleAlarmDelay);
			bleAlarmDelay = bleAlarmDelay * 1000;
		}
		else if (cmd.command == CMD_ALARM_FREQ)
		{
			bleAlarmFreq = cmd.arguments[0].toInt();
			writeConfigString(ALARM_FREQ_addr, ALARM_FREQ_size, String(bleAlarmFreq));

			str = REPLY_ALARM_FREQ;
			str += eq;
			str += String(bleAlarmFreq);
		}

#ifdef MQTT_ENABLE
		// send_mqtt=topic,message - to be implemented
		else if (cmd.command == CMD_SEND_MQTT)
		{
			str = REPLY_SEND_MQTT;
			str += eq;
			if (mqtt_send(cmd.arguments[1], cmd.arguments[1].length(), cmd.arguments[0]))
			{
				str += F("Message sent");
			}
			else
			{
				str += F("Message not sent");
			}
		}
		else if (cmd.command == CMD_MQTT_SERVER)
		{
			IPAddress tmpAddr;
			writeConfigString(MQTT_SERVER_addr, MQTT_SERVER_size, cmd.arguments[0]);

			str = REPLY_MQTT_SERVER;
			str += eq;
			if (tmpAddr.fromString(cmd.arguments[0]))
			{
				IPAddress mqtt_ip_server = tmpAddr;
				str += mqtt_ip_server.toString();
			}
			else
			{
				str += cmd.arguments[0];
			}
		}
		else if (cmd.command == CMD_MQTT_PORT)
		{
			uint16_t mqtt_port = cmd.arguments[0].toInt();
			writeConfigString(MQTT_PORT_addr, MQTT_PORT_size, String(mqtt_port));

			str = REPLY_MQTT_PORT;
			str += eq;
			str += String(mqtt_port);
			//mqtt_client.setServer(mqtt_ip_server, mqtt_port);
		}
		else if (cmd.command == CMD_MQTT_LOGIN)
		{
			writeConfigString(MQTT_USER_addr, MQTT_USER_size, cmd.arguments[0]);

			str = REPLY_MQTT_LOGIN;
			str += eq;
			str += cmd.arguments[0];
		}
		else if (cmd.command == CMD_MQTT_PASS)
		{
			writeConfigString(MQTT_PASS_addr, MQTT_PASS_size, cmd.arguments[0]);

			str = REPLY_MQTT_PASS;
			str += eq;
			str += cmd.arguments[0];
		}
		else if (cmd.command == CMD_MQTT_ID)
		{
			writeConfigString(MQTT_ID_addr, MQTT_ID_size, cmd.arguments[0]);

			str = REPLY_MQTT_ID;
			str += eq;
			str += cmd.arguments[0];
		}
		else if (cmd.command == CMD_MQTT_TOPIC_IN)
		{
			writeConfigString(MQTT_TOPIC_IN_addr, MQTT_TOPIC_IN_size, cmd.arguments[0]);

			str = REPLY_MQTT_TOPIC_IN;
			str += eq;
			str += cmd.arguments[0];
		}
		else if (cmd.command == CMD_MQTT_TOPIC_OUT)
		{
			writeConfigString(MQTT_TOPIC_OUT_addr, MQTT_TOPIC_OUT_size, cmd.arguments[0]);

			str = REPLY_MQTT_TOPIC_OUT;
			str += eq;
			str += cmd.arguments[0];
		}
		else if (cmd.command == CMD_MQTT_CLEAN)
		{
			str = REPLY_MQTT_CLEAN;
			str += eq;
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size, SWITCH_OFF_NUMBER);
				str += SWITCH_OFF;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(MQTT_CLEAN_addr, MQTT_CLEAN_size, SWITCH_ON_NUMBER);
				str += SWITCH_ON;
			}
			else
			{
				str += REPLY_INCORRECT;
			}
		}
		else if (cmd.command == CMD_MQTT_ENABLE)
		{
			str = REPLY_MQTT_ENABLE;
			str += eq;
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size, SWITCH_OFF_NUMBER);
				mqttEnable = false;
				str += SWITCH_OFF;
				mqtt_client.disconnect();
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size, SWITCH_ON_NUMBER);
				mqttEnable = true;
				str += SWITCH_ON;
				mqtt_connect();
			}
			else
			{
				str += REPLY_INCORRECT;
				str += cmd.arguments[0];
			}
		}
#endif

#ifdef TELEGRAM_ENABLE
		// send_telegram=[*/user#],message
		else if (cmd.command == CMD_SEND_TELEGRAM)
		{
			uint8_t i = 0;
			uint8_t j = 0;
			if (cmd.arguments[0] == STAR) //if * then all
			{
				i = 0;
				j = TELEGRAM_USERS_NUMBER;
			}
			else
			{
				i = cmd.arguments[0].toInt();
				j = i + 1;
			}

			str = REPLY_SEND_TELEGRAM;
			str += eq;

			if (i <= TELEGRAM_USERS_NUMBER)
			{
				for (; i < j; i++)
				{
					int64_t telegramUser = getTelegramUser(i);
					if (telegramUser != 0)
					{
						if (sendToTelegramServer(telegramUser, cmd.arguments[1]))
						{
							str += F("Message sent;");
						}
						else
						{
							str += F("Message not sent;");
						}
					}
					yield();
				}
			}
		}
		else if (cmd.command == CMD_TELEGRAM_USER)
		{
			uint64_t newUser = StringToUint64(cmd.arguments[0]);
			str = REPLY_TELEGRAM_USER;
			str += String(cmd.index);
			str += eq;

			if (cmd.index >= 0 && cmd.index < TELEGRAM_USERS_NUMBER)
			{
				uint16_t m = TELEGRAM_USERS_TABLE_size / TELEGRAM_USERS_NUMBER;
				writeConfigString(TELEGRAM_USERS_TABLE_addr + cmd.index * m, m, uint64ToString(newUser));
				str += uint64ToString(newUser);
			}
			else
			{
				str += REPLY_INCORRECT;
				str += F(" out of range [0,");
				str += String(TELEGRAM_USERS_NUMBER - 1);
				str += F("]");
			}
		}
		else if (cmd.command == CMD_TELEGRAM_TOKEN)
		{
			writeConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size, cmd.arguments[0]);

			str = REPLY_TELEGRAM_TOKEN;
			str += eq;
			str += cmd.arguments[0];
			//myBot.setTelegramToken(telegramToken);
		}
		else if (cmd.command == CMD_TELEGRAM_ENABLE)
		{
			str = REPLY_TELEGRAM_ENABLE;
			str += eq;
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(TELEGRAM_ENABLE_addr, TELEGRAM_ENABLE_size, SWITCH_OFF_NUMBER);
				//telegramEnable = false;
				str += SWITCH_OFF;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(TELEGRAM_ENABLE_addr, TELEGRAM_ENABLE_size, SWITCH_ON_NUMBER);
				//telegramEnable = true;
				//myBot.wifiConnect(STA_Ssid, STA_Password);
				//myBot.setTelegramToken(telegramToken);
				//myBot.testConnection();
				str += SWITCH_ON;
			}
			else
			{
				str += REPLY_INCORRECT;
				str += cmd.arguments[0];
			}
		}
#endif

#ifdef NTP_TIME_ENABLE
		else if (cmd.command == CMD_NTP_SERVER)
		{
			writeConfigString(NTP_SERVER_addr, NTP_SERVER_size, cmd.arguments[0]);

			str = REPLY_NTP_SERVER;
			str += eq;
			str += cmd.arguments[0];
		}
		else if (cmd.command == CMD_NTP_TIME_ZONE)
		{
			int timeZone = cmd.arguments[0].toInt();
			writeConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size, String(timeZone));

			str = REPLY_NTP_TIME_ZONE;
			str += eq;
			str += String(timeZone);
		}
		else if (cmd.command == CMD_NTP_REFRESH_DELAY)
		{
			ntpRefreshDelay = cmd.arguments[0].toInt();
			writeConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size, String(ntpRefreshDelay));
			setSyncInterval(ntpRefreshDelay);

			str = REPLY_NTP_REFRESH_DELAY;
			str += eq;
			str += String(ntpRefreshDelay);
		}
		else if (cmd.command == CMD_NTP_REFRESH_PERIOD)
		{
			timeRefershPeriod = cmd.arguments[0].toInt();
			writeConfigString(NTP_REFRESH_PERIOD_addr, NTP_REFRESH_PERIOD_size, String(timeRefershPeriod));

			str = REPLY_NTP_REFRESH_PERIOD;
			str += eq;
			str += String(timeRefershPeriod);
		}
		else if (cmd.command == CMD_NTP_ENABLE)
		{
			str = REPLY_NTP_ENABLE;
			str += eq;
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(NTP_ENABLE_addr, NTP_ENABLE_size, SWITCH_OFF_NUMBER);
				NTPenable = false;
				setSyncProvider(nullptr);
				Udp.stop();

				str += SWITCH_OFF;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(NTP_ENABLE_addr, NTP_ENABLE_size, SWITCH_ON_NUMBER);
				NTPenable = true;
				Udp.begin(UDP_LOCAL_PORT);
				setSyncProvider(getNtpTime);
				setSyncInterval(ntpRefreshDelay);

				str += SWITCH_ON;
			}
			else
			{
				str += REPLY_INCORRECT;
				str += cmd.arguments[0];
			}
		}
#endif

#ifdef OTA_UPDATE_ENABLE
		else if (cmd.command == CMD_OTA_PORT)
		{
			uint16_t ota_port = cmd.arguments[0].toInt();
			writeConfigString(OTA_PORT_addr, OTA_PORT_size, String(ota_port));

			str = REPLY_OTA_PORT;
			str += eq;
			str += String(ota_port);
		}
		else if (cmd.command == CMD_OTA_PASSWORD)
		{
			writeConfigString(OTA_PASSWORD_addr, OTA_PASSWORD_size, cmd.arguments[0]);

			str = REPLY_OTA_PASSWORD;
			str += eq;
			str += cmd.arguments[0];
		}
		else if (cmd.command == CMD_OTA_ENABLE)
		{
			str = REPLY_OTA_ENABLE;
			str += eq;
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(OTA_ENABLE_addr, OTA_ENABLE_size, SWITCH_OFF_NUMBER);
				otaEnable = false;
				ArduinoOTA.end();

				str += SWITCH_OFF;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(OTA_ENABLE_addr, OTA_ENABLE_size, SWITCH_ON_NUMBER);
				otaEnable = true;
				ArduinoOTA.begin();

				str += SWITCH_ON;
			}
			else
			{
				str += REPLY_INCORRECT;
				str += cmd.arguments[0];
			}
		}
#endif
		else
		{
			str = REPLY_INCORRECT;
			str += F("command: \"");
			str += command;
			str += QUOTE;
		}
	}
	else
	{
		str = REPLY_INCORRECT;
		str += F("command: \"");
		str += command;
		str += QUOTE;
	}
	str += EOL;

	return str;
}

String printConfig(bool toJson = false)
{
	String eq = F(": ");
	String delimiter = F("\r\n");
	String str = "";
	//str.reserve(3000);
	if (toJson)
	{
		delimiter = F("\",\r\n\"");
		eq = F("\":\"");
		str = F("{\"");
	}

	str += PROPERTY_DEVICE_NAME;
	str += eq;
	str += readConfigString(DEVICE_NAME_addr, DEVICE_NAME_size);
	str += delimiter;

	str += PROPERTY_DEVICE_MAC;
	str += eq;
	str += WiFi.macAddress();
	str += delimiter;

	str += PROPERTY_FW_VERSION;
	str += eq;
	str += FW_VERSION;
	str += delimiter;

	str += REPLY_STA_SSID;
	str += eq;
	str += readConfigString(STA_SSID_addr, STA_SSID_size);
	str += delimiter;

	str += REPLY_STA_PASS;
	str += eq;
	str += readConfigString(STA_PASS_addr, STA_PASS_size);
	str += delimiter;

	str += REPLY_AP_SSID;
	str += eq;
	str += readConfigString(AP_SSID_addr, AP_SSID_size);
	str += delimiter;

	str += REPLY_AP_PASS;
	str += eq;
	str += readConfigString(AP_PASS_addr, AP_PASS_size);
	str += delimiter;

	str += REPLY_WIFI_POWER;
	str += eq;
	str += String(readConfigFloat(WIFI_POWER_addr));
	str += delimiter;

	str += REPLY_WIFI_MODE;
	str += eq;
	long m = readConfigString(WIFI_MODE_addr, WIFI_MODE_size).toInt();
	if (m < 0 || m >= wifiModes->length())
		m = WIFI_AP_STA;
	str += wifiModes[m];
	str += delimiter;

	str += REPLY_AUTOREPORT;
	str += eq;
	m = readConfigString(AUTOREPORT_addr, AUTOREPORT_size).toInt();
	for (uint8_t b = 0; b < CHANNELS_NUMBER; b++)
	{
		if (bitRead(m, b))
		{
			str += channels[b];
			str += F(",");
		}
		yield();
	}
	str += delimiter;

	str += REPLY_AUTOREPORT_ALWAYS;
	str += eq;
	m = readConfigString(AUTOREPORT_ALWAYS_addr, AUTOREPORT_ALWAYS_size).toInt();
	for (uint8_t b = 0; b < CHANNELS_NUMBER; b++)
	{
		if (bitRead(m, b))
		{
			str += channels[b];
			str += F(",");
		}
		yield();
	}
	str += delimiter;

	for (m = 0; m < BLE_USERS_NUMBER; m++)
	{
		str += REPLY_BLE_USER;
		str += String(m);
		str += eq;
		str += getBleUser(m);
		str += delimiter;

		str += REPLY_BLE_USER_DESC;
		str += String(m);
		str += eq;
		str += getBleUserDescription(m);
		str += delimiter;

		str += REPLY_BLE_USER_RSSI;
		str += String(m);
		str += eq;
		str += String(getBleRssiCalibration(m));
		str += delimiter;
	}

	str += REPLY_BLE_REPORT_DISTANCE;
	str += eq;
	str += readConfigString(BLE_REPORT_DISTANCE_addr, BLE_REPORT_DISTANCE_size);
	str += delimiter;

	str += REPLY_BLE_REPORT_DELAY;
	str += eq;
	str += readConfigString(BLE_REPORT_DELAY_addr, BLE_REPORT_DELAY_size);
	str += delimiter;

	str += REPLY_LIGHT_PIN;
	str += eq;
	str += readConfigString(LIGHT_PIN_addr, LIGHT_PIN_size);
	str += delimiter;

	str += REPLY_BUZZER_PIN;
	str += eq;
	str += readConfigString(BUZZER_PIN_addr, BUZZER_PIN_size);
	str += delimiter;

	str += REPLY_ALARM_DELAY;
	str += eq;
	str += readConfigString(ALARM_DELAY_addr, ALARM_DELAY_size);
	str += delimiter;

	str += REPLY_ALARM_FREQ;
	str += eq;
	str += readConfigString(ALARM_FREQ_addr, ALARM_FREQ_size);
	str += delimiter;

	yield();
#ifdef OTA_UPDATE_ENABLE
	str += REPLY_OTA_PORT;
	str += eq;
	str += readConfigString(OTA_PORT_addr, OTA_PORT_size);
	str += delimiter;

	str += REPLY_OTA_PASSWORD;
	str += eq;
	str += readConfigString(OTA_PASSWORD_addr, OTA_PASSWORD_size);
	str += delimiter;

	str += REPLY_OTA_ENABLE;
	str += eq;
	str += String(readConfigString(OTA_ENABLE_addr, OTA_ENABLE_size));
	str += delimiter;
#endif
	yield();
#ifdef MQTT_ENABLE
	str += REPLY_MQTT_SERVER;
	str += eq;
	str += readConfigString(MQTT_SERVER_addr, MQTT_SERVER_size);
	str += delimiter;

	str += REPLY_MQTT_PORT;
	str += eq;
	str += readConfigString(MQTT_PORT_addr, MQTT_PORT_size);
	str += delimiter;

	str += REPLY_MQTT_LOGIN;
	str += eq;
	str += readConfigString(MQTT_USER_addr, MQTT_USER_size);
	str += delimiter;

	str += REPLY_MQTT_PASS;
	str += eq;
	str += readConfigString(MQTT_PASS_addr, MQTT_PASS_size);
	str += delimiter;

	str += REPLY_MQTT_ID;
	str += eq;
	str += readConfigString(MQTT_ID_addr, MQTT_ID_size);
	str += delimiter;

	str += REPLY_MQTT_TOPIC_IN;
	str += eq;
	str += readConfigString(MQTT_TOPIC_IN_addr, MQTT_TOPIC_IN_size);
	str += delimiter;

	str += REPLY_MQTT_TOPIC_OUT;
	str += eq;
	str += readConfigString(MQTT_TOPIC_OUT_addr, MQTT_TOPIC_OUT_size);
	str += delimiter;

	str += REPLY_MQTT_CLEAN;
	str += eq;
	str += readConfigString(MQTT_CLEAN_addr, MQTT_CLEAN_size);
	str += delimiter;

	str += REPLY_MQTT_ENABLE;
	str += eq;
	str += readConfigString(MQTT_ENABLE_addr, MQTT_ENABLE_size);
	str += delimiter;
#endif
	yield();
#ifdef TELEGRAM_ENABLE
	str += REPLY_TELEGRAM_TOKEN;
	str += eq;
	str += readConfigString(TELEGRAM_TOKEN_addr, TELEGRAM_TOKEN_size);
	str += delimiter;

	for (m = 0; m < TELEGRAM_USERS_NUMBER; m++)
	{
		str += REPLY_TELEGRAM_USER;
		str += String(m);
		if (m == 0)
		{
			str += F("(admin)");
		}
		str += eq;
		str += uint64ToString(getTelegramUser(m));
		str += delimiter;
		yield();
	}

	str += REPLY_TELEGRAM_ENABLE;
	str += eq;
	str += readConfigString(TELEGRAM_ENABLE_addr, TELEGRAM_ENABLE_size);
	str += delimiter;
#endif
	yield();
#ifdef NTP_TIME_ENABLE
	str += REPLY_NTP_SERVER;
	str += eq;
	str += readConfigString(NTP_SERVER_addr, NTP_SERVER_size);
	str += delimiter;

	str += REPLY_NTP_TIME_ZONE;
	str += eq;
	str += readConfigString(NTP_TIME_ZONE_addr, NTP_TIME_ZONE_size);
	str += delimiter;

	str += REPLY_NTP_REFRESH_DELAY;
	str += eq;
	str += readConfigString(NTP_REFRESH_DELAY_addr, NTP_REFRESH_DELAY_size);
	str += delimiter;

	str += REPLY_NTP_REFRESH_PERIOD;
	str += eq;
	str += readConfigString(NTP_REFRESH_PERIOD_addr, NTP_REFRESH_PERIOD_size);
	str += delimiter;

	str += REPLY_NTP_ENABLE;
	str += eq;
	str += readConfigString(NTP_ENABLE_addr, NTP_ENABLE_size);
	str += delimiter;
#endif
	yield();
	str += F("EEPROM size");
	str += eq;
	str += String(collectEepromSize());
	str += delimiter;
	yield();
	if (toJson)
		str += F("\"}");

	return str;
}

String printStatus(bool toJson = false)
{
	String delimiter = F("\r\n");
	String eq = F(": ");
	if (toJson)
	{
		delimiter = F("\",\r\n\"");
		eq = F("\":\"");
	}

	String str;
	str.reserve(1024);
	if (toJson)
		str = F("{\"");

	str += PROPERTY_DEVICE_NAME;
	str += eq;
	str += readConfigString(DEVICE_NAME_addr, DEVICE_NAME_size);
	str += delimiter;

	str += PROPERTY_DEVICE_MAC;
	str += eq;
	str += WiFi.macAddress();
	str += delimiter;

	str += PROPERTY_FW_VERSION;
	str += eq;
	str += FW_VERSION;
	str += delimiter;

	str += F("Time");
	str += eq;
	if (timeIsSet)
		str += timeToString(now());
	else
		str += F("not set");
	str += delimiter;

	str += F("WiFi enabled");
	str += eq;
	str += String(wifiEnable);
	str += delimiter;

	str += F("WiFi mode");
	str += eq;
	str += wifiModes[WiFi.getMode()];
	str += delimiter;

	str += F("WiFi connection");
	str += eq;
	if (WiFi.status() == WL_CONNECTED)
	{
		str += F("connected");
		str += delimiter;

		str += F("AP");
		str += eq;
		str += getStaSsid();
		str += delimiter;

		str += F("IP");
		str += eq;
		str += WiFi.localIP().toString();
		str += delimiter;
	}
	else
	{
		str += F("not connected");
		str += delimiter;
	}

	if (WiFi.softAPgetStationNum() > 0)
	{
		str += F("AP Clients connected");
		str += eq;
		str += String(WiFi.softAPgetStationNum());
		str += delimiter;
	}

#ifdef TELEGRAM_ENABLE
	str += F("TELEGRAM buffer usage");
	str += eq;
	str += String(telegramOutboundBufferPos);
	str += F("/");
	str += String(TELEGRAM_MESSAGE_BUFFER_SIZE);
	str += delimiter;
#endif

	str += F("Free memory");
	str += eq;
	str += String(ESP.getFreeHeap());
	str += delimiter;

#ifdef VBAT_ADC
	str += F("Battery voltage");
	str += eq;
	str += String(readBatteryVoltage());
	str += delimiter;
#endif // VBAT_ADC

	str += F("MQTT connected");
	str += eq;
	str += String(mqtt_client.connected());
	str += delimiter;

	if (toJson)
		str += F("\"}");

	return str;
}

String printHelp(bool isAdmin)
{
	String reply = F("Commands:\r\n"
		"help\r\n"
		"get_status\r\n");

	if (isAdmin)
	{
		reply += F("get_config\r\n"

			"[ADMIN]        set_time=yyyy.mm.dd hh:mm:ss\r\n"

			"[ADMIN][FLASH] autoreport=UART | MQTT | TELEGRAM)\r\n"
			"[ADMIN][FLASH] autoreport_always=UART | MQTT | TELEGRAM)\r\n"

			"[ADMIN][FLASH] device_name=****\r\n"

			"[ADMIN][FLASH] lora_id=****\r\n"
			"[ADMIN][FLASH] master_lora_id=****\r\n"

			"[ADMIN][FLASH] sta_ssid=****\r\n"
			"[ADMIN][FLASH] sta_pass=****\r\n"
			"[ADMIN][FLASH] ap_ssid=**** (empty for device_name+MAC)\r\n"
			"[ADMIN][FLASH] ap_pass=****\r\n"
			"[ADMIN][FLASH] wifi_power=n (0..20.5)\r\n"
			"[ADMIN][FLASH] wifi_mode=AUTO/STATION/APSTATION/AP/OFF\r\n"
			"[ADMIN][FLASH] wifi_connect_time=n (sec.)\r\n"
			"[ADMIN][FLASH] wifi_reconnect_period=n (sec.)\r\n"
			"[ADMIN]        wifi_enable=on/off\r\n"

#ifdef OTA_UPDATE_ENABLE
			"[ADMIN][FLASH] ota_port=n\r\n"
			"[ADMIN][FLASH] ota_pass=****\r\n"
			"[ADMIN][FLASH] ota_enable=on/off\r\n"
#endif

#ifdef NTP_TIME_ENABLE
			"[ADMIN][FLASH] ntp_server=****\r\n"
			"[ADMIN][FLASH] ntp_time_zone=n\r\n"
			"[ADMIN][FLASH] ntp_refresh_delay=n (sec.)\r\n"
			"[ADMIN][FLASH] ntp_refresh_period=n (sec.)\r\n"
			"[ADMIN][FLASH] ntp_enable=on/off\r\n"
#endif

#ifdef MQTT_ENABLE
			"[ADMIN][FLASH] mqtt_server=****\r\n"
			"[ADMIN][FLASH] mqtt_port=n\r\n"
			"[ADMIN][FLASH] mqtt_login=****\r\n"
			"[ADMIN][FLASH] mqtt_pass=****\r\n"
			"[ADMIN][FLASH] mqtt_id=**** (empty for device_name+MAC)\r\n"
			"[ADMIN][FLASH] mqtt_topic_in=****\r\n"
			"[ADMIN][FLASH] mqtt_topic_out=****\r\n"
			"[ADMIN][FLASH] mqtt_clean=on/off\r\n"
			"[ADMIN][FLASH] mqtt_enable=on/off\r\n"
#endif

#ifdef TELEGRAM_ENABLE
			"[ADMIN][FLASH] telegram_token=****\r\n"
			"[ADMIN][FLASH] telegram_user?=n\r\n"
			"[ADMIN][FLASH] telegram_enable=on/off\r\n"
#endif

			"[ADMIN]        reset");
	}

	return reply;
}

String timeToString(uint32_t time)
{
	// digital clock display of the time
	String tmp = String(hour(time));
	tmp += COLON;
	tmp += String(minute(time));
	tmp += COLON;
	tmp += String(second(time));
	tmp += SPACE;
	tmp += String(year(time));
	tmp += F(".");
	tmp += String(month(time));
	tmp += F(".");
	tmp += String(day(time));
	return tmp;
}

String getStaSsid()
{
	return readConfigString(STA_SSID_addr, STA_SSID_size);
}

String getStaPassword()
{
	return readConfigString(STA_PASS_addr, STA_PASS_size);
}

String getApSsid()
{
	String AP_Ssid = readConfigString(AP_SSID_addr, AP_SSID_size);
	if (AP_Ssid.length() <= 0)
		AP_Ssid = deviceName + F("_") + CompactMac();
	return AP_Ssid;
}

String getApPassword()
{
	return readConfigString(AP_PASS_addr, AP_PASS_size);
}

wifi_power_t getWiFiPower()
{
	float power = readConfigFloat(WIFI_POWER_addr);

	if (power <= -1)
		return WIFI_POWER_MINUS_1dBm;
	else if (power > -1 && power <= 2)
		return WIFI_POWER_2dBm;
	else if (power > 2 && power <= 5)
		return WIFI_POWER_5dBm;
	else if (power > 5 && power <= 7)
		return WIFI_POWER_7dBm;
	else if (power > 7 && power <= 8.5)
		return WIFI_POWER_8_5dBm;
	else if (power > 8.5 && power <= 11)
		return WIFI_POWER_11dBm;
	else if (power > 11 && power <= 13)
		return WIFI_POWER_13dBm;
	else if (power > 13 && power <= 15)
		return WIFI_POWER_15dBm;
	else if (power > 15 && power <= 17)
		return WIFI_POWER_17dBm;
	else if (power > 17 && power <= 18.5)
		return WIFI_POWER_18_5dBm;
	else if (power > 18.5 && power <= 19)
		return WIFI_POWER_19dBm;

	return WIFI_POWER_19_5dBm;
}

void startWiFi()
{
	wifi_mode = readConfigString(WIFI_MODE_addr, WIFI_MODE_size).toInt();
	if (wifi_mode < 0 || wifi_mode > 4)
		wifi_mode = WIFI_AP_STA;

	if (wifi_mode == WIFI_STA)
	{
		Start_STA_Mode();
	}
	else if (wifi_mode == WIFI_AP)
	{
		Start_AP_Mode();
	}
	else if (wifi_mode == WIFI_OFF)
	{
		Start_OFF_Mode();
	}
	else if (wifi_mode == WIFI_AP_STA)
	{
		Start_AP_STA_Mode();
	}
	else // default mode for fresh start configuration
	{
		Start_AP_STA_Mode();
	}
}

void Start_OFF_Mode()
{
	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.persistent(true);
	WiFi.mode(WIFI_OFF);
	delay(1000UL);
}

void Start_STA_Mode()
{
	String STA_Password = getStaPassword();
	String STA_Ssid = getStaSsid();
	wifi_power_t wifi_OutputPower = getWiFiPower();	   //[-1 - 19.5]dBm

	WiFi.disconnect(false);
	WiFi.softAPdisconnect(false);
	WiFi.persistent(true);
	//WiFi.mode(WIFI_OFF);
	delay(1000UL);
	WiFi.setTxPower(wifi_OutputPower);
	//WiFi.setSleep(WIFI_PS_NONE); // not allowed for BLE+WiFi all-together
	WiFi.mode(WIFI_MODE_STA);
	WiFi.hostname(deviceName);
	WiFi.begin(STA_Ssid.c_str(), STA_Password.c_str());
}

void Start_AP_Mode()
{
	String AP_Ssid = getApSsid();
	String AP_Password = getApPassword();
	wifi_power_t wifi_OutputPower = getWiFiPower();	   //[0 - 20.5]dBm

	WiFi.disconnect(false);
	WiFi.softAPdisconnect(false);
	WiFi.persistent(true);
	//WiFi.mode(WIFI_OFF);
	delay(1000UL);
	WiFi.setTxPower(wifi_OutputPower);
	WiFi.mode(WIFI_AP);
	WiFi.hostname(deviceName);
	WiFi.softAP(AP_Ssid.c_str(), AP_Password.c_str());
	//WiFi.softAPConfig(apIP, apIPgate, IPAddress(192, 168, 4, 1));
}

void Start_AP_STA_Mode()
{
	String STA_Password = getStaPassword();
	String STA_Ssid = getStaSsid();
	String AP_Ssid = getApSsid();
	String AP_Password = getApPassword();
	wifi_power_t wifi_OutputPower = getWiFiPower();	   //[0 - 20.5]dBm

	WiFi.disconnect(false);
	WiFi.softAPdisconnect(false);
	WiFi.persistent(true);
	//WiFi.mode(WIFI_OFF);
	delay(1000UL);
	WiFi.setTxPower(wifi_OutputPower);
	WiFi.mode(WIFI_AP_STA);
	WiFi.hostname(deviceName);
	WiFi.softAP(AP_Ssid.c_str(), AP_Password.c_str());
	WiFi.begin(STA_Ssid.c_str(), STA_Password.c_str());
	//WiFi.softAPConfig(apIP, apIPgate, IPAddress(192, 168, 4, 1));
}

String MacToStr(const uint8_t mac[6])
{
	return String(mac[0]) + String(mac[1]) + String(mac[2]) + String(mac[3]) + String(mac[4]) + String(mac[5]);
}

String CompactMac()
{
	String mac = WiFi.macAddress();
	mac.replace(COLON, F(""));
	return mac;
}

unsigned int crc16(unsigned char* buffer, int length)
{
	unsigned int crc = 0xffff;
	for (int i = 0; i < length; i++)
	{
		unsigned char b = buffer[i];
		crc ^= b;
		for (int x = 0; x < 8; x++)
		{
			if (crc & 0x0001)
			{
				crc >>= 1;
				crc ^= 0xA001;
			}
			else
			{
				crc >>= 1;
			}
			yield();
		}
		yield();
	}

	return crc;
}

void init_light_pin()
{
	pinMode(lightPin, OUTPUT);
	digitalWrite(lightPin, LOW);
}

void init_buzzer_pin()
{
	ledcSetup(BUZZER_PWM_CHANNEL + PWM_CHANNEL_OFFSET, 10000, PWM_RESOLUTION);
	ledcAttachPin(buzzerPin, BUZZER_PWM_CHANNEL + PWM_CHANNEL_OFFSET); // Attach the LED PWM Channel to the GPIO Pin
	set_output(BUZZER_PWM_CHANNEL, 0);
}

void set_output(uint8_t pwmChannelNum, int outValue)
{
	if (outValue < 0)
		outValue = 0;

	if (outValue > MAX_DUTY_CYCLE)
		outValue = MAX_DUTY_CYCLE;

	ledcWrite(pwmChannelNum + PWM_CHANNEL_OFFSET, outValue);
}

void esp32Tone(uint8_t pwmChannelNum, uint32_t freq)
{
	ledcWriteTone(pwmChannelNum, freq);    // channel, frequency
}

void esp32NoTone(uint8_t pwmChannelNum)
{
	ledcWriteTone(pwmChannelNum, 0);
}