#pragma once
// EEPROM config string position and size
#define DEVICE_NAME_addr	0
#define DEVICE_NAME_size	20

#define STA_SSID_addr	DEVICE_NAME_addr + DEVICE_NAME_size
#define STA_SSID_size	32

#define STA_PASS_addr	STA_SSID_addr + STA_SSID_size
#define STA_PASS_size	32

#define AP_SSID_addr	STA_PASS_addr + STA_PASS_size
#define AP_SSID_size	32

#define AP_PASS_addr	AP_SSID_addr + AP_SSID_size
#define AP_PASS_size	32

#define WIFI_POWER_addr	AP_PASS_addr + AP_PASS_size
#define WIFI_POWER_size	4

#define WIFI_MODE_addr	WIFI_POWER_addr + WIFI_POWER_size
#define WIFI_MODE_size	1

#define CONNECT_TIME_addr	WIFI_MODE_addr + WIFI_MODE_size
#define CONNECT_TIME_size	5

#define CONNECT_PERIOD_addr	CONNECT_TIME_addr + CONNECT_TIME_size
#define CONNECT_PERIOD_size	5

#define AUTOREPORT_addr  CONNECT_PERIOD_addr + CONNECT_PERIOD_size
#define AUTOREPORT_size  3

#define AUTOREPORT_ALWAYS_addr  AUTOREPORT_addr + AUTOREPORT_size
#define AUTOREPORT_ALWAYS_size  3

#define NTP_SERVER_addr	AUTOREPORT_ALWAYS_addr + AUTOREPORT_ALWAYS_size
#define NTP_SERVER_size	100

#define NTP_TIME_ZONE_addr	NTP_SERVER_addr + NTP_SERVER_size
#define NTP_TIME_ZONE_size	2

#define NTP_REFRESH_DELAY_addr	NTP_TIME_ZONE_addr + NTP_TIME_ZONE_size
#define NTP_REFRESH_DELAY_size	5

#define NTP_REFRESH_PERIOD_addr	NTP_REFRESH_DELAY_addr + NTP_REFRESH_DELAY_size
#define NTP_REFRESH_PERIOD_size	7

#define NTP_ENABLE_addr	NTP_REFRESH_PERIOD_addr + NTP_REFRESH_PERIOD_size
#define NTP_ENABLE_size	1

#define HTTP_PORT_addr	NTP_ENABLE_addr + NTP_ENABLE_size
#define HTTP_PORT_size	5

#define HTTP_ENABLE_addr	HTTP_PORT_addr + HTTP_PORT_size
#define HTTP_ENABLE_size	1

#define MQTT_SERVER_addr  HTTP_ENABLE_addr + HTTP_ENABLE_size
#define MQTT_SERVER_size  100

#define MQTT_PORT_addr  MQTT_SERVER_addr + MQTT_SERVER_size
#define MQTT_PORT_size  5

#define MQTT_USER_addr  MQTT_PORT_addr + MQTT_PORT_size
#define MQTT_USER_size  100

#define MQTT_PASS_addr  MQTT_USER_addr + MQTT_USER_size
#define MQTT_PASS_size  20

#define MQTT_ID_addr  MQTT_PASS_addr + MQTT_PASS_size
#define MQTT_ID_size  100

#define MQTT_TOPIC_IN_addr  MQTT_ID_addr + MQTT_ID_size
#define MQTT_TOPIC_IN_size  100

#define MQTT_TOPIC_OUT_addr  MQTT_TOPIC_IN_addr + MQTT_TOPIC_IN_size
#define MQTT_TOPIC_OUT_size  100

#define MQTT_CLEAN_addr  MQTT_TOPIC_OUT_addr + MQTT_TOPIC_OUT_size
#define MQTT_CLEAN_size  1

#define MQTT_ENABLE_addr  MQTT_CLEAN_addr + MQTT_CLEAN_size
#define MQTT_ENABLE_size  1

#define TELEGRAM_TOKEN_addr	MQTT_ENABLE_addr + MQTT_ENABLE_size
#define TELEGRAM_TOKEN_size	50

#define TELEGRAM_USERS_TABLE_addr	TELEGRAM_TOKEN_addr + TELEGRAM_TOKEN_size
#define TELEGRAM_USERS_TABLE_size	TELEGRAM_USERS_NUMBER * TELEGRAM_USERS_NUMBER

#define TELEGRAM_ENABLE_addr	TELEGRAM_USERS_TABLE_addr + TELEGRAM_USERS_TABLE_size
#define TELEGRAM_ENABLE_size	1

#define OTA_PORT_addr	TELEGRAM_ENABLE_addr + TELEGRAM_ENABLE_size
#define OTA_PORT_size	5

#define OTA_PASSWORD_addr	OTA_PORT_addr + OTA_PORT_size
#define OTA_PASSWORD_size	20

#define OTA_ENABLE_addr	OTA_PASSWORD_addr + OTA_PASSWORD_size
#define OTA_ENABLE_size	1

#define BLE_USERS_TABLE_addr	OTA_ENABLE_addr + OTA_ENABLE_size
#define BLE_USERS_TABLE_size	17 * BLE_USERS_NUMBER

#define BLE_USERS_DESC_TABLE_addr	BLE_USERS_TABLE_addr + BLE_USERS_TABLE_size
#define BLE_USERS_DESC_TABLE_size	64 * BLE_USERS_NUMBER

#define BLE_RSSI_TABLE_addr	BLE_USERS_DESC_TABLE_addr + BLE_USERS_DESC_TABLE_size
#define BLE_RSSI_TABLE_size	3 * BLE_USERS_NUMBER

#define BLE_REPORT_DISTANCE_addr	BLE_RSSI_TABLE_addr + BLE_RSSI_TABLE_size
#define BLE_REPORT_DISTANCE_size	5

#define BLE_REPORT_DELAY_addr	BLE_REPORT_DISTANCE_addr + BLE_REPORT_DISTANCE_size
#define BLE_REPORT_DELAY_size	3

#define LIGHT_PIN_addr	BLE_REPORT_DELAY_addr + BLE_REPORT_DELAY_size
#define LIGHT_PIN_size	2

#define BUZZER_PIN_addr	LIGHT_PIN_addr + LIGHT_PIN_size
#define BUZZER_PIN_size	2

#define ALARM_DELAY_addr	BUZZER_PIN_addr + BUZZER_PIN_size
#define ALARM_DELAY_size	2

#define ALARM_FREQ_addr	ALARM_DELAY_addr + ALARM_DELAY_size
#define ALARM_FREQ_size	5

#define FINAL_CRC_addr	ALARM_FREQ_addr + ALARM_FREQ_size
#define FINAL_CRC_size	2

#define ON true
#define OFF false

const String wifiModes[4] = { "Off", "Station", "Access point",  "Access point + Station" };

#define CHANNELS_NUMBER 3
#define CHANNEL_UART 0
#define CHANNEL_MQTT 1
#define CHANNEL_TELEGRAM 2

const String channels[CHANNELS_NUMBER] = { "UART", "MQTT", "TELEGRAM" };
