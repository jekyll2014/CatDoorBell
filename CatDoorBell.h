#pragma once
#include <WiFi.h>
#include <ArduinoBLE.h>

#ifndef ESP_CAT_DOORBELL
#define ESP_CAT_DOORBELL

#define PROPERTY_DEVICE_NAME								F("DeviceName") //get_sensor
#define PROPERTY_DEVICE_MAC									F("DeviceMAC") //get_sensor
#define PROPERTY_FW_VERSION									F("FwVersion") //get_sensor

#define SWITCH_ON_NUMBER F("1")
#define SWITCH_OFF_NUMBER F("0")
#define SWITCH_ON F("on")
#define SWITCH_OFF F("off")
#define EOL F("\r\n")
#define CSV_DIVIDER F(";")
#define QUOTE F("\"")
#define STAR F("*")
#define SPACE F(" ")
#define COLON F(":")

struct commandTokens
{
	int tokensNumber;
	String command;
	int index;
	String arguments[10];
};

struct BleTokenReport
{
	int userIndex;
	String MacAddress;
	String localName;
	int rssi;
	uint8_t manufacturerData[256] = { 0 };
	int manufacturerDataLength;
	uint8_t advertisementData[256] = { 0 };
	int advertisementDataLength;
	time_t time;
};

//BLE
void bleCentralDiscoverHandler(BLEDevice);

//Button
IRAM_ATTR void onExtButton();

// PWM Outputs
#define PWM_RESOLUTION									10 // 10 bits
#define PWM_CHANNEL_OFFSET								0 // to avoid messing with 3rd party libraries PWM channel usage
const int MAX_DUTY_CYCLE = (int)(pow(2, PWM_RESOLUTION) - 1);
#define LIGHT_PWM_CHANNEL									0 // PWM channel#
#define BUZZER_PWM_CHANNEL									1 // PWM channel#

// Wi-Fi
String getStaSsid();
String getStaPassword();
String getApSsid();
String getApPassword();
wifi_power_t getWiFiPower();
void startWiFi();
void Start_OFF_Mode();
void Start_STA_Mode();
void Start_AP_Mode();
void Start_AP_STA_Mode();

// MQTT
#define MQTT_MAX_PACKET 100
String getMqttServer();
int getMqttPort();
String getMqttUser();
String getMqttPassword();
String getMqttDeviceId();
String getMqttTopicIn();
String getMqttTopicOut();
bool getMqttClean();
bool mqtt_connect();
bool mqtt_send(String&, int, String&);
void mqtt_callback(char*, uint8_t*, uint16_t);

// Telegram
#define TELEGRAM_MESSAGE_MAX_SIZE 1000
#define TELEGRAM_MESSAGE_DELAY 10000
#define TELEGRAM_RETRIES 10
#define TELEGRAM_MESSAGE_BUFFER_SIZE 5

struct telegramMessage
{
	uint8_t retries;
	int64_t user;
	String message;
};

String uint64ToString(uint64_t);
uint64_t StringToUint64(String&);
bool sendToTelegramServer(int64_t, String&);
void addMessageToTelegramOutboundBuffer(int64_t, String, uint8_t);
void removeMessageFromTelegramOutboundBuffer();
void sendToTelegram(int64_t, String&, uint8_t);
void sendBufferToTelegram();
uint64_t getTelegramUser(uint8_t);

// OTA_UPDATE
//void otaStartCallback();
//void otaErrorCallback(ota_error_t);
//void otaProgressCallback(unsigned int, unsigned int);
//void otaEndCallback();

// NTP
#define NTP_PACKET_SIZE 48	// NTP time is in the first 48 bytes of message
#define UDP_LOCAL_PORT 8888 // local port to listen for UDP packets

void sendNTPpacket(IPAddress&);
time_t getNtpTime();
void restartNTP();

// EEPROM
uint32_t collectEepromSize();
String readConfigString(uint16_t, uint16_t);
void readConfigString(uint16_t, uint16_t, char*);
uint32_t readConfigLong(uint16_t);
float readConfigFloat(uint16_t);
void writeConfigString(uint16_t, uint16_t, String);
void writeConfigString(uint16_t, uint16_t, char*, uint8_t);
void writeConfigLong(uint16_t, uint32_t);
void writeConfigFloat(uint16_t, float);
uint16_t calculateEepromCrc();
bool checkEepromCrc();
void refreshEepromCrc();
void clearEeprom();
String getBleUser(uint8_t);
String getBleUserDescription(uint8_t);
int getBleRssiCalibration(uint8_t);

// Help printout
String printConfig(bool);
String printStatus(bool);
String printHelp(bool);

// Command processing
commandTokens parseCommand(String&, char, char, bool);
String processCommand(String&, uint8_t, bool);

//Helpers
String timeToString(uint32_t);
String MacToStr(const uint8_t);
String CompactMac();
void i2c_scan();
unsigned int crc16(unsigned char*, int);
void init_light_pin();
void init_buzzer_pin();
void set_output(uint8_t, int);
void esp32Tone(uint8_t, uint32_t);
void esp32NoTone(uint8_t);
#endif
