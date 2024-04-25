#pragma once
/*
TTGO LORA Series BOARD PINS
Name			V1.0	V1.2(T-Fox)	V1.6	V2.0	Heltec V2
Vbat ADC	35?		35?					35?		35		01

ESP32 strapping pins:
GPIO 0  - must be LOW to enter boot mode)
GPIO 2  - must be floating or LOW during boot)
GPIO 4
GPIO 5  - must be HIGH during boot)
GPIO 12 - must be LOW during boot)
GPIO 15 - must be HIGH during boot)

ESP32 active at boot:
GPIO 1  - Sends the ESP32 boot logs via the UART
GPIO 3  - 3.3V voltage at boot time
GPIO 5  - Sends a PWM signal at boot time
GPIO 6-11 - connected to the ESP32 integrated SPI flash memory – not recommended to use).
GPIO 14 - Sends a PWM signal at boot time
GPIO 15 - Sends the ESP32 boot logs via the UART

ESP32-C3 strapping pins:
GPIO 2
GPIO 8
GPIO 9

ESP32-C3 active at boot:
GPIO 0-8
GPIO 10
GPIO 11-17 - connected to the ESP32 integrated SPI flash memory
GPIO 18
GPIO 19 (input disabled, pull-up resistor enabled)
*/

#define FW_VERSION							F("1.0.0")

//#define DEBUG_MODE							HARD_UART // Feature UNSTABLE - WIFI problem	//SOFT_UART //not implemented yet
#define HARD_UART_ENABLE					115200

#define OTA_UPDATE_ENABLE
#define NTP_TIME_ENABLE
#define MQTT_ENABLE
#define TELEGRAM_ENABLE

//TELEGRAM
#define TELEGRAM_USERS_NUMBER 10

//Bluetooth LE
#define BLE_USERS_NUMBER 10

// Button
#define PRG_BUTTON 00

//Battery voltage
// TTGO LoRa32-OLED V1 board
//#define VBAT_ADC				35

// TTGO LoRa32-OLED V1.6 board
//#define VBAT_ADC				35

// HELTEC V2 board
//#define VBAT_ADC				01
