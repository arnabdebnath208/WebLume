#ifndef CONFIG_H
#define CONFIG_H
#ifndef DHT_H
#include <DHT.h>
#endif

//---------- Preferences------------

#define DEFAULT_WIFI_SSID "" //Default WIFI SSID
#define DEFAULT_WIFI_PASSWORD "" //Default WIFI Password

#define DEFAULT_AP_SSID "WebLume" //Default AP SSID
#define DEFAULT_AP_PASSWORD "12345678" //Default AP Password [Use "" for no password]

#define DEFAULT_HOSTNAME "weblume" //Default Hostname : Used for mDNS and ArduinoOTA hostname

#define DEFAULT_ACCESS_TOKEN "admin" //Default Access Token : Used for API Authentication with this device through http requests
#define DEFAULT_API_KEY "admin" //Default API Key : Used for authenticating with the web server from this device
#define DEFAULT_NODE_ID "1" //Default Node ID : Used for identifying this device in the web server

#define DEFAULT_OTA_PORT "3232" //Default OTA Port : Used for OTA
#define DEFAULT_OTA_PASSWORD "admin" //Default OTA Password : Used for OTA Authentication

#define DEFAULT_NETWORK_MODE "AP" //(WIFI/STA),AP,NONE : Default network mode after boot

#define DEFAULT_SERVER_API "http://192.168.0.10:80/api.php" //Default Server API : Used for syncing device state with the web server

// --------------------------------

#define USB_SERIAL_BAUD_RATE 115200 //USB Serial Baud Rate

#define WEBSERVER_PORT 80 //Web Server Port 
#define DHT_TYPE DHT11 //DHT Sensor Type

#define PWM_MAX_VALUE 255  //PWM Max Value
#define PWM_MIN_VALUE 0 //PWM Min Value

#define SYNCRONIZE true //Auto Sync Device State with the server
#define SYNCRONIZE_FAILED_LIMIT 50 //Max Failed Limit for Auto Sync: If the device state sync with the server failed more than this limit, the auto sync will be disabled

#endif