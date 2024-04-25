#pragma once
//data requests
#define CMD_GET_CONFIG						F("get_config")

#define CMD_GET_STATUS						F("get_status")

#define CMD_HELP									F("help")

#define CMD_RESET									F("reset")

//device settings
#define CMD_DEVICE_NAME						F("device_name")
#define REPLY_DEVICE_NAME					F("Device name")

#define CMD_AUTOREPORT						F("autoreport")
#define REPLY_AUTOREPORT					F("Autoreport channels")

#define CMD_AUTOREPORT_ALWAYS			F("autoreport_always")
#define REPLY_AUTOREPORT_ALWAYS		F("Autoreport always channels")

#define CMD_TIME_SET							F("set_time")
#define REPLY_TIME_SET						F("Time")

//WiFi settings
#define CMD_WIFI_MODE							F("wifi_mode") //wifi_mode
#define REPLY_WIFI_MODE						F("WiFi mode") //

#define CMD_STA_SSID							F("sta_ssid") //sta_ssid
#define REPLY_STA_SSID						F("WiFi STA SSID") //

#define CMD_STA_PASS							F("sta_pass") //sta_pass
#define REPLY_STA_PASS						F("WiFi STA password") //

#define CMD_AP_SSID								F("ap_ssid") //ap_ssid
#define REPLY_AP_SSID							F("WiFi AP SSID") //

#define CMD_AP_PASS								F("ap_pass") //ap_pass
#define REPLY_AP_PASS							F("WiFi AP password") //

#define CMD_WIFI_POWER						F("wifi_power") //wifi_power
#define REPLY_WIFI_POWER					F("WiFi power") //

#define CMD_WIFI_ENABLE						F("wifi_enable") //wifi_enable
#define REPLY_WIFI_ENABLE					F("WiFi service") //

//NTP settings
#define CMD_NTP_SERVER						F("ntp_server") //ntp_server
#define REPLY_NTP_SERVER					F("NTP server") //

#define CMD_NTP_REFRESH_DELAY			F("ntp_refresh_delay") //ntp_refresh_delay
#define REPLY_NTP_REFRESH_DELAY		F("NTP refresh delay") //

#define CMD_NTP_REFRESH_PERIOD		F("ntp_refresh_period") //ntp_refresh_period
#define REPLY_NTP_REFRESH_PERIOD	F("NTP refresh period") //

#define CMD_NTP_TIME_ZONE					F("ntp_time_zone") //ntp_time_zone
#define REPLY_NTP_TIME_ZONE				F("NTP time zone") //

#define CMD_NTP_ENABLE						F("ntp_enable") //ntp_enable
#define REPLY_NTP_ENABLE					F("NTP service") //

//OTA settings
#define CMD_OTA_PORT							F("ota_port") //display_refresh
#define REPLY_OTA_PORT						F("OTA port") //

#define CMD_OTA_PASSWORD					F("ota_pass") //display_refresh
#define REPLY_OTA_PASSWORD				F("OTA password") //

#define CMD_OTA_ENABLE						F("ota_enable") //display_refresh
#define REPLY_OTA_ENABLE					F("OTA service") //

//MQTT settings
#define CMD_MQTT_SERVER						F("mqtt_server") //mqtt_server
#define REPLY_MQTT_SERVER					F("MQTT server") //

#define CMD_MQTT_PORT							F("mqtt_port") //mqtt_port
#define REPLY_MQTT_PORT						F("MQTT port") //

#define CMD_MQTT_LOGIN						F("mqtt_login") //mqtt_login
#define REPLY_MQTT_LOGIN					F("MQTT login") //

#define CMD_MQTT_PASS							F("mqtt_pass") //mqtt_pass
#define REPLY_MQTT_PASS						F("MQTT password") //

#define CMD_MQTT_ID								F("mqtt_id") //mqtt_id
#define REPLY_MQTT_ID							F("MQTT ID") //

#define CMD_MQTT_TOPIC_IN					F("mqtt_topic_in") //mqtt_topic_in
#define REPLY_MQTT_TOPIC_IN				F("MQTT receive topic") //

#define CMD_MQTT_TOPIC_OUT				F("mqtt_topic_out") //mqtt_topic_out
#define REPLY_MQTT_TOPIC_OUT			F("MQTT post topic") //

#define CMD_MQTT_CLEAN						F("mqtt_clean") //mqtt_clean
#define REPLY_MQTT_CLEAN					F("MQTT clean session") //

#define CMD_MQTT_ENABLE						F("mqtt_enable") //mqtt_enable
#define REPLY_MQTT_ENABLE					F("MQTT service") //

#define CMD_SEND_MQTT							F("send_mqtt") //send_mqtt
#define REPLY_SEND_MQTT						F("MQTT sent") //send_mqtt

//Telegram settings
#define CMD_TELEGRAM_TOKEN				F("telegram_token") //telegram_token
#define REPLY_TELEGRAM_TOKEN			F("TELEGRAM token") //

#define CMD_TELEGRAM_USER					F("telegram_user") //telegram_user
#define REPLY_TELEGRAM_USER				F("Telegram user #") //

#define CMD_TELEGRAM_ENABLE				F("telegram_enable") //telegram_enable
#define REPLY_TELEGRAM_ENABLE			F("Telegram service") //

#define CMD_SEND_TELEGRAM					F("send_telegram") //send_telegram
#define REPLY_SEND_TELEGRAM				F("Telegram sent") //send_telegram

// Bluetooth settings
#define CMD_BLE_USER							F("ble_user") //send_telegram
#define REPLY_BLE_USER						F("BLE user #") //send_telegram

#define CMD_BLE_USER_DESC					F("ble_user_desc") //send_telegram
#define REPLY_BLE_USER_DESC				F("BLE user description #")

#define CMD_BLE_USER_RSSI					F("ble_user_rssi") //BLE user 1m. calibrated RSSI
#define REPLY_BLE_USER_RSSI				F("BLE user RSSI #")

#define CMD_BLE_REPORT_DISTANCE		F("ble_report_distance") //distance cm.
#define REPLY_BLE_REPORT_DISTANCE	F("BLE report distance")

#define CMD_BLE_REPORT_DELAY			F("ble_report_delay") //delay sec.
#define REPLY_BLE_REPORT_DELAY		F("BLE report delay")

#define CMD_LIGHT_PIN							F("light_pin") //
#define REPLY_LIGHT_PIN						F("Light pin")

#define CMD_BUZZER_PIN						F("buzzer_pin") //
#define REPLY_BUZZER_PIN					F("Buzzer pin")

#define CMD_ALARM_DELAY						F("alarm_delay") //sec.
#define REPLY_ALARM_DELAY					F("Alarm delay")

#define CMD_ALARM_FREQ						F("alarm_freq") //kHz.
#define REPLY_ALARM_FREQ					F("Alarm frequency")

// Incorrect request reply
#define REPLY_INCORRECT						F("Incorrect ")
