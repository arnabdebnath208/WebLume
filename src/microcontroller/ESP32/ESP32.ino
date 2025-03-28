#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <IRremote.hpp>
#include <DHT.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "dataTypes.cpp"
#include "config.h"
#include "pinConfig.h"
#include "devInfo.h"
#include "device.h"
#include "intervalTask.h"


#define PREFERENCES_NAME NAME

bool AUTO_SYNCRONIZE = SYNCRONIZE;
int deviceStateSyncFailedCount = 0;
bool WIFI = false; // WiFi Status : true = on, false = off
bool AP = false; // AP Status : true = on, false = off
bool AP_WIFI_OVERRIDED = false; // Does the AP turned on while WiFi was on : true = yes, false = no
bool isWiFiConnected = true; // Is the device connected to WiFi : true = yes, false = no
bool isOTAInit = false; // Is the OTA Initialized : true = yes, false = no
bool OTA = false; // OTA Status : true = on, false = off
int configExists = 0; // 0 = Not Exists, 1 = Exists
bool isSyncWithServerDone = false; // Is the device state sync with the server : true = yes, false = no
bool isSyncTheServerDone = false; 
bool debugMode = false; // Debug Mode : true = on, false = off
float temperature=NAN;
float humidity=NAN;

String WIFI_SSID = DEFAULT_WIFI_SSID;
String WIFI_PASSWORD = DEFAULT_WIFI_PASSWORD;
String AP_SSID = DEFAULT_AP_SSID;
String AP_PASSWORD = DEFAULT_AP_PASSWORD;
String HOSTNAME = DEFAULT_HOSTNAME;
String ACCESS_TOKEN = DEFAULT_ACCESS_TOKEN;
String API_KEY = DEFAULT_API_KEY;
String NODE_ID = DEFAULT_NODE_ID;
String OTA_PORT = DEFAULT_OTA_PORT;
String OTA_PASSWORD = DEFAULT_OTA_PASSWORD;
String NETWORK_MODE = DEFAULT_NETWORK_MODE;
String SERVER_API = DEFAULT_SERVER_API;

Preferences preferences;
WebServer server(WEBSERVER_PORT);
DHT dht(DHT_PIN,DHT_TYPE);

void indicate(int onDuration=150,int offDuration=150,uint8_t indicatorDevicePin = INDICATOR_LED,bool isInverted=false);
void pinIOManager();
httpResponse_t httpRequest(String url, String requestMethod, String data, String contentType = "application/x-www-form-urlencoded", String cookie = "",int timeout = 3000);
bool isAuthenticated();
void httpResponse(String response,int code=200,String type="text/plain");
void handelRoot();
void handelApi();
void otaInit();
void otaHandle();
void irSend(uint32_t value);
void irRecivedCheak(uint32_t value);
void irReciveHandle();
bool turnOnMDNS();
bool turnOnWebServer();
void dht11Read();
void turnOnWiFi(bool wait=true);
void turnOffWiFi(bool wait=true);
void turnOnAP(bool wait=true);
void turnOffAP(bool wait=true);
void toggleAPMode(bool wait=true);
void toggleWiFiMode(bool wait=true);
bool loadConfig();
void saveDefaultConfig();
void resetDevice();
void configHandle();
bool restoreDeviceState();
bool storeDeviceState();
void switchHandle();
void intervalHandle();


//device(int id,int groupId,uint8_t gpioPin,char * name = "",bool isInverted = false,bool isEnable = true,bool initState = false,bool isPWM = false,int initPwmValue = MCU_PWM_MAX,int pwmMinValue = MCU_PWM_MIN,int pwmMaxValue = MCU_PWM_MAX)
device devices[] = {
    device(0,0,23,"Power Outlate"),
    device(1,1,19,"Celling Fan"),
    device(2,2,18,"Tube Light"),
    device(3,2,17,"LED Bulb"),
    device(4,3,16,"Celling Light 1"),
    device(5,3,4,"Celling Light 2"),
    device(6,3,27,"Celling Light 3"),
    device(7,3,26,"Celling Light 4"),
    device(8,4,25,"Inbuilt Power LED",false,true,false,true,MCU_PWM_MAX,1,MCU_PWM_MAX),
};
int deviceCount = sizeof(devices)/sizeof(device);

deviceManager devManager(devices,deviceCount);

// inputSwitch(int pin,int triggerState,int longPressTime,void (*singlePressFunction)()=NULL,void (*longPressFunction)()=NULL,char * name="\0",bool isEnable=true)
inputSwitch inputSwitches[] = {
    inputSwitch(AP_TOOGLE_SWITCH,HIGH,2000,NULL,[](){toggleAPMode();},"AP Mode Switch"), //Long Press for 2 seconds to toggle AP mode
    inputSwitch(CONFIG_RESET_SWITCH,LOW,3000,NULL,resetDevice,"Config Reset Switch") //Long Press for 3 seconds to reset the device configuration to default
};
int inputSwitchCount = sizeof(inputSwitches)/sizeof(inputSwitch);

// intervalTask(unsigned int id,unsigned int intervalMillis,void (*callbackFunction)() = NULL,const char * name = "",bool isEnable=true,bool isSilent=false);
intervalTask intervalTasks[] = {
    intervalTask{0,60000,dht11Read,"DHT11 Sensor Data Read"},
    intervalTask{1,30000,[](){if(WiFi.status()==WL_CONNECTED&&AUTO_SYNCRONIZE&&SERVER_API!=""&&isSyncWithServerDone&&!isSyncTheServerDone)storeDeviceState();},"Sync Server With Device State"}, // It will cheak every 30 seconds if any device state is changed and sync with the server
};
int intervalTaskCount = sizeof(intervalTasks)/sizeof(intervalTask);

intervalTaskManager intervalTskManager(intervalTasks,intervalTaskCount);

// irInput(uint32_t value,void (*singlePressFunction)()=NULL,char * name="\0",bool isEnable=true);
irInput irInputs[] = {
    irInput(0x00FF7F80,[](){if(devManager.toggle(0)) isSyncTheServerDone=false;},"Outlate"),
    irInput(0xFE017F80,[](){if(devManager.toggle(1)) isSyncTheServerDone=false;},"Celling Fan"),
    irInput(0xF7087F80,[](){if(devManager.toggle(2)) isSyncTheServerDone=false;},"Tube Light"),
    irInput(0xF6097F80,[](){if(devManager.toggle(3)) isSyncTheServerDone=false;},"LED Bulb"),
    irInput(0xF9067F80,[](){if(devManager.toggle(4)) isSyncTheServerDone=false;},"Celling 1"),
    irInput(0xFD027F80,[](){if(devManager.toggle(5)) isSyncTheServerDone=false;},"Celling 2"),
    irInput(0xFA057F80,[](){if(devManager.toggle(6)) isSyncTheServerDone=false;},"Celling 3"),
    irInput(0xFC037F80,[](){if(devManager.toggle(7)) isSyncTheServerDone=false;},"Celling 4"),
    irInput(0xF8077F80,[](){if(devManager.toggle(8)) isSyncTheServerDone=false;},"Inbuilt Power LED"),
    irInput(0xF50A7F80,[](){if(devManager.decreasePwmValue(8,devManager.getByIndex(8)->pwmValue>10?10:1)) isSyncTheServerDone=false;},"Night Lamp Decrease"),
    irInput(0xE01F7F80,[](){if(devManager.increasePwmValue(8,devManager.getByIndex(8)->pwmValue>15?10:1)) isSyncTheServerDone=false;},"Night Lamp Increase"),
    irInput(0xED127F80,[](){if(devManager.powerOffAll()>0) isSyncTheServerDone=false;},"Power Off All Device"),
    irInput(0xE11E7F80,[](){toggleWiFiMode();},"WIFI Toggle"),
    irInput(0xE51A7F80,[](){toggleAPMode();},"AP Toggle"),
    irInput(0xFB047F80,[](){debugMode=!debugMode;},"Debug Mode Toggle"),
};
int irInputCount = sizeof(irInputs)/sizeof(irInput);

void setup()
{
    pinIOManager();

    Serial.begin(USB_SERIAL_BAUD_RATE);
    delay(10);
    Serial.println("\n"+String(NAME)+" "+String(VERSION)+"\n"+String(MICROCONTROLLER)+"\n");

    configHandle(); // Load Config: Load presaved config or save default config to NVS
    
    IrReceiver.begin(IR_RECV_PIN, ENABLE_LED_FEEDBACK);
    // IrSender.begin(IR_SEND_PIN);

    if(NETWORK_MODE == "AP")
        turnOnAP(false);
    else if(NETWORK_MODE == "STA" || NETWORK_MODE == "WIFI") //if NETWORK_MODE is STA or WIFI then turn on WiFi
        turnOnWiFi(false);
    else
        Serial.println("No Network Mode is defined");

    turnOnMDNS(); // Turn on mDNS
    turnOnWebServer(); // Turn on Web Server
}

void loop()
{
    if(AP)
    {
        //Things to do when AP is on
        server.handleClient();
        otaHandle();
    }
    else if(WIFI)
    {
        //Things to do when WiFi is on
        if(WiFi.status()==WL_CONNECTED)
        {
            digitalWrite(NETWORK_INDICATOR_LED,LOW);
            if(isWiFiConnected==false)
            {
                Serial.println("WiFi Connected");
                Serial.println("SSID: "+WiFi.SSID());
                Serial.println("RSSI: "+String(WiFi.RSSI())+" dBm");
                Serial.println("IP: "+WiFi.localIP().toString());
                Serial.println("MAC: "+WiFi.macAddress());
                isWiFiConnected = true;
            }
            //Things to do when connected to WiFi
            if(AUTO_SYNCRONIZE&&SERVER_API!="")
            {
                if(!isSyncWithServerDone)
                    restoreDeviceState();
            }
            server.handleClient();
            otaHandle();
        }
        else
        {
            //Things to do when not connected to WiFi
            if(isWiFiConnected==true)
            {
                Serial.println("WiFi Disconnected");
                isWiFiConnected = false;
            }
            digitalWrite(NETWORK_INDICATOR_LED,HIGH);
        }
    }

    irReciveHandle(); // Handle IR Single Recive
    intervalTskManager.handle(); // Handle Interval tasks
    switchHandle(); // Handle Switch input tasks
}



void indicate(int onDuration,int offDuration,uint8_t indicatorDevicePin,bool isInverted)
{
    if(isInverted)
    {    
        digitalWrite(indicatorDevicePin,LOW);
        delay(onDuration);
        digitalWrite(indicatorDevicePin,HIGH);
        delay(offDuration);
    }
    else
    {
        digitalWrite(indicatorDevicePin,HIGH);
        delay(onDuration);
        digitalWrite(indicatorDevicePin,LOW);
        delay(offDuration);
    }
}

void pinIOManager()
{
    pinMode(INDICATOR_LED,OUTPUT);
    pinMode(NETWORK_INDICATOR_LED,OUTPUT);
    pinMode(IR_SEND_PIN,OUTPUT);
    pinMode(BUZZER,OUTPUT);
    pinMode(IR_RECV_PIN,INPUT);
}



httpResponse_t httpRequest(String url, String requestMethod, String data, String contentType, String cookie,int timeout)//This function is used to make http request
{
    // Cheak for network connection type
    WiFiClient client;
    HTTPClient http;
    indicate(25,25,NETWORK_INDICATOR_LED);
    httpResponse_t response;
    http.begin(client, url);
    http.setTimeout(timeout);
    // Handle requestMethod and data
    if (requestMethod == "post" || requestMethod == "POST")
    {
        http.addHeader("Content-Type", contentType);
        if(cookie != "")
            http.addHeader("Cookie", cookie);
        response.code = http.POST(data);
    }
    else if (requestMethod == "get" || requestMethod == "GET")
    {
        if(cookie != "")
            http.addHeader("Cookie", cookie);
        response.code = http.GET();

    }
    // response.header = http.header();
    response.cookie = http.header("Set-Cookie");
    response.text = http.getString();
    http.end();
    client.stop();
    // Detect HTTP Error
    return response;
}

bool isAuthenticated() // Check if the request is authenticated
{
    if(ACCESS_TOKEN=="")
        return true;
    if(server.hasArg("accessToken"))
    {
        if(server.arg("accessToken")==ACCESS_TOKEN)
            return true;
        else
            return false;
    }
    else
        return false;
}

void httpResponse(String response,int code,String type)
{
    server.send(code,type,response);
    indicate(25,25,NETWORK_INDICATOR_LED);
}
void handelRoot()
{
    String response = "Hello From WebLume!";
    httpResponse(response);
}
void handelApi()
{
    if(server.method()==HTTP_GET)
    {
        if(server.hasArg("fetch"))
        {
            String fetch = server.arg("fetch");
            if(fetch == "dht11")
            {
                float temperature = dht.readTemperature();
                float humidity = dht.readHumidity();
                if(isnan(temperature) || isnan(humidity))
                {
                    String response = "{\"status\":\"failed\",\"message\":\"Failed to read DHT11 Sensor\"}";
                    httpResponse(response,500, "application/json");
                }
                else
                {
                    String response = "{\"status\":\"success\",\"temperature\":"+String(temperature)+",\"humidity\":"+String(humidity)+"}";
                    httpResponse(response,200, "application/json");
                }            
            }
            else if(fetch == "temperature")
            {
                float temperature = dht.readTemperature();
                if(isnan(temperature))
                {
                    String response = "{\"status\":\"failed\",\"message\":\"Failed to read DHT11 Sensor\"}";
                    httpResponse(response,500, "application/json");
                }
                else
                {
                    String response = "{\"status\":\"success\",\"temperature\":"+String(temperature)+"}";
                    httpResponse(response,200, "application/json");
                }
            }
            else if(fetch == "humidity")
            {
                float humidity = dht.readHumidity();
                if(isnan(humidity))
                {
                    String response = "{\"status\":\"failed\",\"message\":\"Failed to read DHT11 Sensor\"}";
                    httpResponse(response,500, "application/json");
                }
                else
                {
                    String response = "{\"status\":\"success\",\"humidity\":"+String(humidity)+"}";
                    httpResponse(response,200, "application/json");
                }
            }
            else if(fetch == "sensors")            
            {
                float dhtTemperature = dht.readTemperature();
                float dhtHumidity = dht.readHumidity();
                String response = "{\"status\":\"success\",\"dht11\":{\"temperature\":"+String(dhtTemperature)+",\"humidity\":"+String(dhtHumidity)+"}}";
                httpResponse(response,200, "application/json");
            }
            else
            {
                String response = "{\"status\":\"failed\",\"message\":\"Invalid Fetch Key!\"}";
                httpResponse(response,400, "application/json");
            }
        }
        else
        {
            String response = "Hello From WebLume API!";
            httpResponse(response,200);
        }
        return;
    }
    else if(server.method()==HTTP_POST)
    {
        if(!isAuthenticated())
        {
            String response = "{\"status\":\"failed\",\"message\":\"Authentication Failed\",\"code\":401}";
            httpResponse(response,401, "application/json");
            return;
        }

        if(server.hasArg("action")) // Perform a action
        {
            String action = server.arg("action");
            if(action=="onAP" || action=="onap")
            {
                if(AP)
                {
                    String response = "{\"status\":\"failed\",\"message\":\"AP Already Turned On\"}";
                    httpResponse(response,400, "application/json");
                }
                else
                {
                    httpResponse("{\"status\":\"success\",\"message\":\"AP Turning On\"}",200, "application/json");
                    turnOnAP();
                }
            }
            else if(action=="offAP" || action =="offap")
            {
                if(!AP)
                {
                    String response = "{\"status\":\"failed\",\"message\":\"AP Already Turned Off\"}";
                    httpResponse(response,400, "application/json");
                }
                else
                {
                    httpResponse("{\"status\":\"success\",\"message\":\"AP Turning Off\"}",200, "application/json");
                    turnOffAP();
                }
            }
            else if(action == "onWiFi" || action == "onwifi")
            {
                if(WIFI)
                {
                    String response = "{\"status\":\"failed\",\"message\":\"WiFi Already Turned On\"}";
                    httpResponse(response,400, "application/json");
                }
                else
                {
                    httpResponse("{\"status\":\"success\",\"message\":\"WiFi Turning On\"}",200, "application/json");
                    turnOnWiFi();
                }
            }
            else if(action == "offWiFi" || action == "offwifi")
            {
                if(!WIFI)
                {
                    String response = "{\"status\":\"failed\",\"message\":\"WiFi Already Turned Off\"}";
                    httpResponse(response,400, "application/json");
                }
                else
                {
                    httpResponse("{\"status\":\"success\",\"message\":\"WiFi Turning Off\"}",200, "application/json");
                    turnOffWiFi();
                }
            }
            else if (action == "onOTA" || action == "onota")
            {
                if(OTA)
                {
                    String response = "{\"status\":\"failed\",\"message\":\"OTA Already Turned On\"}";
                    httpResponse(response,400, "application/json");
                }
                else
                {
                    if(!isOTAInit)
                        otaInit();
                    OTA = true;
                    String response = "{\"status\":\"success\",\"message\":\"OTA Turned On\"}";
                    httpResponse(response,200, "application/json");
                }
            }
            else if (action == "offOTA" || action == "offota")
            {
                if(OTA)
                {
                    OTA = false;
                    String response = "{\"status\":\"success\",\"message\":\"OTA Turned Off\"}";
                    httpResponse(response,200, "application/json");
                }
                else
                {
                    String response = "{\"status\":\"failed\",\"message\":\"OTA Already Turned Off\"}";
                    httpResponse(response,400, "application/json");
                }
            }
            else if(action == "reboot" || action == "restart")
            {
                String response = "{\"status\":\"success\",\"message\":\"Rebooting...\"}";
                httpResponse(response,200, "application/json");
                delay(1000);
                ESP.restart();
            }
            else if(action == "poweroff" || action == "shutdown")
            {
                // Turn on the power saver mode
            }
            else if(action == "reset" || action == "configReset" || action == "configreset")
            {
                httpResponse("{\"status\":\"success\",\"message\":\"Resetting...\"}",200, "application/json");
                resetDevice();
            }
            else
            {
                String response = "{\"status\":\"failed\",\"message\":\"Invalid action\"}";
                httpResponse(response,400,"application/json");
            }
        }
        else if(server.hasArg("control")) // Control the devices
        {
            device * targetDevice = NULL;
            if(server.hasArg("devIndex"))
            {
                int devIndex = server.arg("devIndex").toInt();
                targetDevice = devManager.getByIndex(devIndex);
                if(targetDevice == NULL)
                {
                    String response = "{\"status\":\"failed\",\"message\":\"Invalid device index!\"}";
                    httpResponse(response,400, "application/json");
                    return;
                }
            }
            else if(server.hasArg("devId"))
            {
                int devId = server.arg("devId").toInt();
                targetDevice = devManager.getById(devId);
                if(targetDevice == NULL)
                {
                    String response = "{\"status\":\"failed\",\"message\":\"Invalid device id!\"}";
                    httpResponse(response,400, "application/json");
                    return;
                }
            }
            
            if(targetDevice != NULL)
            {
                if(server.hasArg("state"))
                {
                    String state = server.arg("state");
                    if(state == "on" || state == "true" || state == "1")
                    {
                        if(targetDevice->on())
                        {
                            isSyncTheServerDone = false;
                            String response = "{\"status\":\"success\",\"message\":\""+String(targetDevice->name)+" turned on\"}";
                            httpResponse(response,200, "application/json");
                            return;
                        }
                        else
                        {
                            String response;
                            if(targetDevice->isEnable)
                                response = "{\"status\":\"failed\",\"message\":\"Something went wrong!\"}";
                            else
                                response = "{\"status\":\"failed\",\"message\":\"This deviced is disabled!\"}";
                            httpResponse(response,400, "application/json");
                            return;
                        }
                    }
                    else if(state == "off" || state == "false" || state == "0")
                    {
                        if(targetDevice->off())
                        {
                            isSyncTheServerDone = false;
                            String response = "{\"status\":\"success\",\"message\":\""+String(targetDevice->name)+" turned off\"}";
                            httpResponse(response,200, "application/json");
                            return;
                        }
                        else
                        {
                            String response;
                            if(targetDevice->isEnable)
                                response = "{\"status\":\"failed\",\"message\":\"Something went wrong!\"}";
                            else
                                response = "{\"status\":\"failed\",\"message\":\"This deviced is disabled!\"}";
                            httpResponse(response,400, "application/json");
                            return;
                        }
                    }
                    else if(state == "toggle")
                    {
                        if(targetDevice->toggle())
                        {
                            isSyncTheServerDone = false;
                            String response = "{\"status\":\"success\",\"message\":\""+String(targetDevice->name)+" toggled["+String(targetDevice->state?"on":"off")+"]\"}";
                            httpResponse(response,200, "application/json");
                            return;
                        }
                        else
                        {
                            String response;
                            if(targetDevice->isEnable)
                                response = "{\"status\":\"failed\",\"message\":\"Something went wrong!\"}";
                            else
                                response = "{\"status\":\"failed\",\"message\":\"This deviced is disabled!\"}";
                            httpResponse(response,400, "application/json");
                            return;
                        }
                    }
                    else
                    {
                        String response = "{\"status\":\"failed\",\"message\":\"Invalid state!\"}";
                        httpResponse(response,400, "application/json");       
                        return;
                    }
                }
                else if(server.hasArg("pwmValue"))
                {
                    int pwmValue = server.arg("pwmValue").toInt();
                    if(targetDevice->pwmControl(pwmValue))
                    {
                        isSyncTheServerDone = false;
                        String response = "{\"status\":\"success\",\"message\":\""+String(targetDevice->name)+" PWM value set to "+String(targetDevice->pwmValue)+"\"}";
                        httpResponse(response,200, "application/json");
                        return;
                    }
                    else
                    {
                        String response;
                        if(targetDevice->isEnable)
                        {
                            if(targetDevice->isPWM)
                            {
                                if(targetDevice->state)
                                    response = "{\"status\":\"failed\",\"message\":\"Something went wrong!\"}";
                                else
                                {
                                    isSyncTheServerDone = false;
                                    response = "{\"status\":\"failed\",\"message\":\"PWM value saved but the device is off!\"}";
                                }
                            }
                            else
                                response = "{\"status\":\"failed\",\"message\":\"This is not a pwm device!\"}";
                        }
                        else
                            response = "{\"status\":\"failed\",\"message\":\"This deviced is disabled!\"}";
                        httpResponse(response,400, "application/json");
                        return;
                    }
                }
                else if(server.hasArg("increasePwmValue"))
                {
                    int valueToIncrease = server.arg("increasePwmValue").toInt();
                    if(targetDevice->increasePwmValue(valueToIncrease))
                    {
                        isSyncTheServerDone = false;
                        String response = "{\"status\":\"success\",\"message\":\""+String(targetDevice->name)+" PWM value set to "+String(targetDevice->pwmValue)+"\"}";
                        httpResponse(response,200, "application/json");
                        return;
                    }
                    else
                    {
                        String response;
                        if(targetDevice->isEnable)
                        {
                            if(targetDevice->isPWM)
                            {
                                if(targetDevice->state)
                                    response = "{\"status\":\"failed\",\"message\":\"Something went wrong!\"}";
                                else
                                {
                                    isSyncTheServerDone = false;
                                    response = "{\"status\":\"failed\",\"message\":\"PWM value saved but the device is off!\"}";
                                }
                            }
                            else
                                response = "{\"status\":\"failed\",\"message\":\"This is not a pwm device!\"}";
                        }
                        else
                            response = "{\"status\":\"failed\",\"message\":\"This deviced is disabled!\"}";
                        httpResponse(response,400, "application/json");
                        return;
                    }

                }
                else if(server.hasArg("decreasePwmValue"))
                {
                    int valueToDecrease = server.arg("decreasePwmValue").toInt();
                    if(targetDevice->decreasePwmValue(valueToDecrease))
                    {
                        isSyncTheServerDone = false;
                        String response = "{\"status\":\"success\",\"message\":\""+String(targetDevice->name)+" PWM value set to "+String(targetDevice->pwmValue)+"\"}";
                        httpResponse(response,200, "application/json");
                        return;
                    }
                    else
                    {
                        String response;
                        if(targetDevice->isEnable)
                        {
                            if(targetDevice->isPWM)
                            {
                                if(targetDevice->state)
                                    response = "{\"status\":\"failed\",\"message\":\"Something went wrong!\"}";
                                else
                                {
                                    isSyncTheServerDone = false;
                                    response = "{\"status\":\"failed\",\"message\":\"PWM value saved but the device is off!\"}";
                                }
                            }
                            else
                                response = "{\"status\":\"failed\",\"message\":\"This is not a pwm device!\"}";
                        }
                        else
                            response = "{\"status\":\"failed\",\"message\":\"This deviced is disabled!\"}";
                        httpResponse(response,400, "application/json");
                        return;
                    }
                }
                else
                {
                    String response = "{\"status\":\"failed\",\"message\":\"Invalid control action!\"}";
                    httpResponse(response,400, "application/json");
                }
            }
            else if(server.hasArg("groupId"))
            {
                int groupId = server.arg("groupId").toInt();
                if(server.hasArg("state"))
                {
                    String state = server.arg("state");
                    if(state == "on" || state == "true" || state == "1")
                    {
                        int controlledDeviceCount = devManager.onByGroupId(groupId);
                        if(controlledDeviceCount>0)
                        {
                            isSyncTheServerDone = false;
                            String response = "{\"status\":\"failed\",\"message\":\""+String(controlledDeviceCount)+" devices has turned on sucessfully\"}";
                            httpResponse(response,200, "application/json");       
                            return;
                        }
                        else
                        {
                            String response = "{\"status\":\"failed\",\"message\":\"0 devices has turned on!\"}";
                            httpResponse(response,400, "application/json");       
                            return;
                        }
                    }
                    else if(state == "off" || state == "false" || state == "0")
                    {
                        int controlledDeviceCount = devManager.offByGroupId(groupId);
                        if(controlledDeviceCount>0)
                        {
                            isSyncTheServerDone = false;
                            String response = "{\"status\":\"failed\",\"message\":\""+String(controlledDeviceCount)+" devices has turned off sucessfully\"}";
                            httpResponse(response,200, "application/json");       
                            return;
                        }
                        else
                        {
                            String response = "{\"status\":\"failed\",\"message\":\"0 devices has turned off!\"}";
                            httpResponse(response,400, "application/json");       
                            return;
                        }
                    }
                    else if(state == "toggle")
                    {
                        int controlledDeviceCount = devManager.toggleByGroupId(groupId);
                        if(controlledDeviceCount>0)
                        {
                            isSyncTheServerDone = false;
                            String response = "{\"status\":\"failed\",\"message\":\""+String(controlledDeviceCount)+" devices has toggled sucessfully\"}";
                            httpResponse(response,200, "application/json");       
                            return;
                        }
                        else
                        {
                            String response = "{\"status\":\"failed\",\"message\":\"0 devices has toggled!\"}";
                            httpResponse(response,400, "application/json");       
                            return;
                        }
                    }
                    else
                    {
                        String response = "{\"status\":\"failed\",\"message\":\"Invalid state!\"}";
                        httpResponse(response,400, "application/json");       
                        return;
                    }

                }
                else if(server.hasArg("pwmValue"))
                {
                    int pwmValue = server.arg("pwmValue").toInt();
                    int controlledDeviceCount = 0;
                    controlledDeviceCount = devManager.pwmControlByGroupId(groupId,pwmValue);
                    isSyncTheServerDone = false;
                    if(controlledDeviceCount>0)
                    {
                        String response = "{\"status\":\"failed\",\"message\":\""+String(controlledDeviceCount)+" devices pwm value cotrolled\"}";
                        httpResponse(response,200, "application/json");       
                        return;
                    }
                    else
                    {
                        String response = "{\"status\":\"failed\",\"message\":\"0 devices pwm value cotrolled\"}";
                        httpResponse(response,400, "application/json");       
                        return;
                    }
                }
                else if(server.hasArg("increasePwmValue"))
                {
                    int valueToIncrease = server.arg("increasePwmValue").toInt();
                    int controlledDeviceCount = 0;
                    controlledDeviceCount = devManager.increasePwmValuesByGroupId(groupId,valueToIncrease);
                    isSyncTheServerDone = false;
                    if(controlledDeviceCount>0)
                    {
                        String response = "{\"status\":\"failed\",\"message\":\""+String(controlledDeviceCount)+" devices pwm value cotrolled\"}";
                        httpResponse(response,200, "application/json");       
                        return;
                    }
                    else
                    {
                        String response = "{\"status\":\"failed\",\"message\":\"0 devices pwm value cotrolled\"}";
                        httpResponse(response,400, "application/json");       
                        return;
                    }
                }
                else if(server.hasArg("decreasePwmValue"))
                {
                    int valueToDecrease = server.arg("decreasePwmValue").toInt();
                    int controlledDeviceCount = 0;
                    controlledDeviceCount = devManager.decreasePwmValuesByGroupId(groupId,valueToDecrease);
                    isSyncTheServerDone = false;
                    if(controlledDeviceCount>0)
                    {
                        String response = "{\"status\":\"failed\",\"message\":\""+String(controlledDeviceCount)+" devices pwm value cotrolled\"}";
                        httpResponse(response,200, "application/json");       
                        return;
                    }
                    else
                    {
                        String response = "{\"status\":\"failed\",\"message\":\"0 devices pwm value cotrolled\"}";
                        httpResponse(response,400, "application/json");       
                        return;
                    }
                }
                else
                {
                    String response = "{\"status\":\"failed\",\"message\":\"Invalid control action!\"}";
                    httpResponse(response,400, "application/json");
                    return;
                }
            }
            else
            {
                String response = "{\"status\":\"failed\",\"message\":\"Invalid control request!!\"}";
                httpResponse(response,400, "application/json");
                return;
            }
        }
        else if(server.hasArg("fetch")) // Fetch the data
        {
            String fetch = server.arg("fetch");
            if(fetch == "dht11")
            {
                float temperature = dht.readTemperature();
                float humidity = dht.readHumidity();
                if(isnan(temperature) || isnan(humidity))
                {
                    String response = "{\"status\":\"failed\",\"message\":\"Failed to read DHT11 Sensor\"}";
                    httpResponse(response,500, "application/json");
                }
                else
                {
                    String response = "{\"status\":\"success\",\"temperature\":"+String(temperature)+",\"humidity\":"+String(humidity)+"}";
                    httpResponse(response,200, "application/json");
                }            
            }
            else if(fetch == "temperature")
            {
                float temperature = dht.readTemperature();
                if(isnan(temperature))
                {
                    String response = "{\"status\":\"failed\",\"message\":\"Failed to read DHT11 Sensor\"}";
                    httpResponse(response,500, "application/json");
                }
                else
                {
                    String response = "{\"status\":\"success\",\"temperature\":"+String(temperature)+"}";
                    httpResponse(response,200, "application/json");
                }
            }
            else if(fetch == "humidity")
            {
                float humidity = dht.readHumidity();
                if(isnan(humidity))
                {
                    String response = "{\"status\":\"failed\",\"message\":\"Failed to read DHT11 Sensor\"}";
                    httpResponse(response,500, "application/json");
                }
                else
                {
                    String response = "{\"status\":\"success\",\"humidity\":"+String(humidity)+"}";
                    httpResponse(response,200, "application/json");
                }
            }
            else if(fetch == "sensors")            
            {
                float dhtTemperature = dht.readTemperature();
                float dhtHumidity = dht.readHumidity();
                String response = "{\"status\":\"success\",\"dht11\":{\"temperature\":"+String(dhtTemperature)+",\"humidity\":"+String(dhtHumidity)+"}}";
                httpResponse(response,200, "application/json");
            }
            else if(fetch == "devices")
            {
                String response = "{\"status\":\"success\",\"devices\":[";
                int i = 0;
                device * targetDevice = devManager.getByIndex(i);
                while(targetDevice!=NULL)
                {
                    response += "{\"id\":"+String(targetDevice->id)+",\"name\":\""+String(targetDevice->name)+"\",\"state\":"+String(targetDevice->state)+",\"pwmValue\":"+String(targetDevice->pwmValue)+"}";
                    device * nextDevice = devManager.getByIndex(++i);
                    if(nextDevice!=NULL)
                        response += ",";
                    targetDevice = nextDevice;
                }
                response += "]}";
                httpResponse(response,200, "application/json");
            }
            else if(fetch == "rssi")
            {
                if(WiFi.status()==WL_CONNECTED)
                {
                    int rssi = WiFi.RSSI();
                    String ssid = WiFi.SSID();
                    String response = "{\"status\":\"success\",\"ssid\":\""+ssid+"\",\"rssi\":"+String(rssi)+"}";
                    httpResponse(response,200, "application/json");
                }
                else
                {
                    String response = "{\"status\":\"failed\",\"message\":\"Not Connected to WiFi\"}";
                    httpResponse(response,400, "application/json");
                }
            }
            else
            {
                String response = "{\"status\":\"failed\",\"message\":\"Invalid Fetch Key!\"}";
                httpResponse(response,400, "application/json");
            }
        }
        else if(server.hasArg("configGet")) // Get the config value
        {
            String configGet = server.arg("configGet");
            preferences.begin(PREFERENCES_NAME, true);
            if(preferences.isKey(configGet.c_str()))
            {
                String response = "{\"status\":\"success\",\""+configGet+"\":\""+preferences.getString(configGet.c_str(),"")+"\"}";
                preferences.end();
                httpResponse(response,200, "application/json");
            }
            else
            {
                preferences.end();
                String response = "{\"status\":\"failed\",\"message\":\"Invalid Config Key!\"}";
                httpResponse(response,400, "application/json");
            }

        }
        else if(server.hasArg("configSet")) // Set the config value
        {
            String configSet = server.arg("configSet");
            // Cheak if the config key is valid
            preferences.begin(PREFERENCES_NAME, false);
            if(preferences.isKey(configSet.c_str()))
            {
                if(server.hasArg("value"))
                {
                    preferences.putString(configSet.c_str(),server.arg("value"));
                    preferences.end();
                    String response = "{\"status\":\"success\",\"message\":\""+configSet+" Updated\"}";
                    httpResponse(response,200, "application/json");
                }
                else
                {
                    preferences.end();
                    String response = "{\"status\":\"failed\",\"message\":\"Value is required!\"}";
                    httpResponse(response,400, "application/json");
                }
            }
            else
            {
                preferences.end();
                String response = "{\"status\":\"failed\",\"message\":\"Invalid Config Key!\"}";
                httpResponse(response,400, "application/json");
            }

        }
        else
        {
            String response = "{\"status\":\"success\",\"message\":\"Hello From WebLume API!\"}";
            httpResponse(response,200, "application/json");
        }
    }
    else
    {
        String response = "{\"status\":\"failed\",\"message\":\"Invalid Request Method\"}";
        httpResponse(response,400, "application/json");
    }
}
bool turnOnMDNS()
{
    if(!MDNS.begin(HOSTNAME))
    {
        Serial.println("mDNS Failed[Hostname: "+String(HOSTNAME)+"]");
        return false;
    }
    else
    {
        Serial.println("mDNS Started[Hostname: "+String(HOSTNAME)+"]");
        MDNS.addService("weblume", "tcp", 80);
        MDNS.addServiceTxt("weblume", "tcp", "version", VERSION);
        MDNS.addServiceTxt("weblume","tcp","id",ID);
        MDNS.addServiceTxt("weblume","tcp","mac",WiFi.macAddress());
        return true;
    }
}
bool turnOnWebServer()
{
    server.on("/",handelRoot);
    server.on("/api",handelApi);
    server.on("/ip",[](){
        if(WiFi.status()==WL_CONNECTED)
        {
            String ip = WiFi.localIP().toString();
            String response = "{\"status\":\"success\",\"ip\":\""+ip+"\",\"type\":\"station\"}";
            httpResponse(response,200,"application/json");
        }
        else
        {
            if(AP)
            {
                String ip = WiFi.softAPIP().toString();
                String response = "{\"status\":\"success\",\"ip\":\""+ip+"\",\"type\":\"ap\"}";
                httpResponse(response,200,"application/json");
            }
            else
            {
                // String response = "Not connected to a network";
                String response = "{\"status\":\"failed\",\"error\":\"not connected to a network\"}";
                httpResponse(response,400,"application/json");
            }
        }
    });
    server.on("/devInfo",[](){
        String response = "{\"id\":\""+String(ID)+"\",\"version\":\""+String(VERSION)+"\",\"microcontroller\":\""+String(MICROCONTROLLER)+"\"}";
        httpResponse(response,200,"application/json");
    });
    server.onNotFound([](){
        String response = "404 Not Found!";
        httpResponse(response,404);
    });
    server.begin();
    return true;
}



// OTA Functions
void otaInit()
{
    ArduinoOTA.setPort(OTA_PORT.toInt());
    ArduinoOTA.setHostname(HOSTNAME.c_str());
    ArduinoOTA.setPassword(OTA_PASSWORD.c_str());
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";
        Serial.println("Start OTA updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nUpdate Complete");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
    Serial.println("OTA Initialized");
    isOTAInit = true;
}
void otaHandle()
{
    if(OTA)
    {
        ArduinoOTA.handle();
        indicate(50,50);
    }
}
// ------------


// IR Functions
void irSend(uint32_t value)
{
    IrSender.sendNEC(value,32);
}
void irRecivedCheak(uint32_t value)
{
    if(value==0x0)
        return;
    for(int i=0;i<irInputCount;i++)
    {
        if(irInputs[i].isEnable && irInputs[i].value==value)
        {
            Serial.println(String(irInputs[i].name)+" IR Signal Recived");
            irInputs[i].action();
            indicate(20,20);
        }
    }
}
void irReciveHandle()
{
    if(IrReceiver.decode())
    {
        Serial.println(IrReceiver.decodedIRData.decodedRawData,HEX);
        if(debugMode)
            indicate(50,50);
        irRecivedCheak(IrReceiver.decodedIRData.decodedRawData);
        IrReceiver.resume();
    }
}
// -----------


// Network Functions 
void turnOnWiFi(bool wait)
{
    Serial.println("Turning on WiFi");
    if(AP)
    {
        Serial.println("Turning off AP to start WiFi");
        turnOffAP(wait);
    }
    WiFi.mode(WIFI_STA);
    WIFI=true;
    if(WIFI_SSID=="")
    {
        Serial.println("No WiFi SSID Defined");
        isWiFiConnected=false;
        digitalWrite(NETWORK_INDICATOR_LED,HIGH);
    }
    else
    {
        WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
        WiFi.setHostname(HOSTNAME.c_str());
        Serial.print("Connecting to "+WIFI_SSID+" ");
        if(wait)
        {
            for(uint8_t i=0;i<10;i++)
            {
                if(WiFi.status()==WL_CONNECTED)
                {
                    Serial.println();
                    Serial.println("WiFi Connected");
                    Serial.println("WIFI SSID: "+WIFI_SSID);
                    Serial.println("RSSI: "+String(WiFi.RSSI())+" dBm");
                    Serial.println("IP Address: "+WiFi.localIP().toString());
                    Serial.println("MAC Address: "+WiFi.macAddress());
                    isWiFiConnected=true;
                    digitalWrite(NETWORK_INDICATOR_LED,LOW);
                    break;
                }
                digitalWrite(NETWORK_INDICATOR_LED,HIGH);
                delay(250);
                digitalWrite(NETWORK_INDICATOR_LED,LOW);
                delay(250);
                Serial.print(".");
            }
        }
        Serial.println();
        isWiFiConnected=false;
        digitalWrite(NETWORK_INDICATOR_LED,HIGH);
        // WiFi.setSleep(WIFI_PS_NONE);
    }
}
void turnOffWiFi(bool wait)
{
    if(AP)
    {
        Serial.println("Unable to turn off WiFi while AP is on");
        return;
    }
    else if(WiFi.getMode() == WIFI_MODE_NULL)
    {
        Serial.println("WiFi Already Off");
        AP_WIFI_OVERRIDED = false;
        return;
    }
    else
    {
        Serial.println("Disconnecting from WiFi");
        WiFi.disconnect(true);
        if(wait)
        {
            indicate(250,250,NETWORK_INDICATOR_LED);
            indicate(250,250,NETWORK_INDICATOR_LED);
        }
        WiFi.mode(WIFI_OFF);
        WIFI=false;
        AP_WIFI_OVERRIDED = false;
        Serial.println("WiFi Turned Off");
        digitalWrite(NETWORK_INDICATOR_LED,LOW);
    }
}
void turnOnAP(bool wait)
{
    if(WIFI)
    {
        Serial.println("Turning off WiFi to start AP");
        turnOffWiFi(wait);
        AP_WIFI_OVERRIDED = true;
    }
    if(wait)
    {
        indicate(100,100,NETWORK_INDICATOR_LED);
        indicate(100,100,NETWORK_INDICATOR_LED);
        indicate(100,100,NETWORK_INDICATOR_LED);
        indicate(100,100,NETWORK_INDICATOR_LED);
        indicate(100,100,NETWORK_INDICATOR_LED);
    }
    Serial.println("Turning on AP Mode");
    WiFi.mode(WIFI_AP);
    if(AP_PASSWORD!="")
        WiFi.softAP(AP_SSID.c_str(),AP_PASSWORD.c_str());
    else
        WiFi.softAP(AP_SSID.c_str());
    AP = true;
    Serial.println("Access Point Started");
    Serial.println("SSID: "+String(AP_SSID));
    Serial.println("Password: "+String(AP_PASSWORD));
    Serial.println("IP Address: "+WiFi.softAPIP().toString());
    Serial.println("MAC Address: "+WiFi.softAPmacAddress());
    if(wait)
        delay(1000);
}
void turnOffAP(bool wait)
{
    WiFi.mode(WIFI_OFF);
    if(wait)
    {
        indicate(200,200,NETWORK_INDICATOR_LED);
        indicate(200,200,NETWORK_INDICATOR_LED);
    }
    AP = false;
    Serial.println("AP Mode Stopped");
    if(AP_WIFI_OVERRIDED)
    {
        AP_WIFI_OVERRIDED = false;
        Serial.println("Restoring WiFi");
        turnOnWiFi(wait);
    }
    if(wait)
        delay(1000);
}
void toggleAPMode(bool wait)
{
    Serial.println("AP Mode Toggle");
    if(AP)
    {
        Serial.println("AP Mode was on");
        turnOffAP(wait);
    }
    else
    {
        Serial.println("AP Mode was off");
        turnOnAP(wait);
    }
}
void toggleWiFiMode(bool wait)
{
    if(WIFI)
    {
        Serial.println("Turning off WiFi");
        turnOffWiFi(wait);
    }
    else
    {
        Serial.println("Turing on WiFi");
        if(AP)
            turnOffAP(wait);
        turnOnWiFi(wait);
    }
}
// ----------------


// Config Functions
bool loadConfig() // Load Config: Load presaved config to variables from NVS
{
    preferences.begin(PREFERENCES_NAME, true);
    configExists = preferences.getInt("exists",0);
    if(configExists==1)
    {
        Serial.println("Configurations Exists");
        // Here is the process to check for version change and update the config
        if(String(VERSION)!=preferences.getString("VERSION","NOVERSION"))
        {
            Serial.println("Version Changed from "+preferences.getString("VERSION","NOVERSION")+" to "+VERSION);
            preferences.end();
            saveNewConfig();
            preferences.begin(PREFERENCES_NAME, true);
        }
        WIFI_SSID = preferences.getString("WIFI_SSID",DEFAULT_WIFI_SSID);
        WIFI_PASSWORD = preferences.getString("WIFI_PASSWORD",DEFAULT_WIFI_PASSWORD);
        AP_SSID = preferences.getString("AP_SSID",DEFAULT_AP_SSID);
        AP_PASSWORD = preferences.getString("AP_PASSWORD",DEFAULT_AP_PASSWORD);
        HOSTNAME = preferences.getString("HOSTNAME",DEFAULT_HOSTNAME);
        ACCESS_TOKEN = preferences.getString("ACCESS_TOKEN",DEFAULT_ACCESS_TOKEN);
        API_KEY = preferences.getString("API_KEY",DEFAULT_API_KEY);
        NODE_ID = preferences.getString("NODE_ID",DEFAULT_NODE_ID);
        OTA_PORT = preferences.getString("OTA_PORT",DEFAULT_OTA_PORT);
        OTA_PASSWORD = preferences.getString("OTA_PASSWORD",DEFAULT_OTA_PASSWORD);
        NETWORK_MODE = preferences.getString("NETWORK_MODE",DEFAULT_NETWORK_MODE);
        SERVER_API = preferences.getString("SERVER_API",DEFAULT_SERVER_API);
        preferences.end();
        Serial.println("Configurations Loaded");
        return true;
    }
    else if(configExists==0)
    {
        preferences.end();
        Serial.println("Configurations Not Exists");
        return false; 
    }
    else
    {
        preferences.end();
        Serial.println("Configurations Error");
        return false;
    }
}
void saveDefaultConfig() // Save Default Config: Save the default config to NVS
{
    preferences.begin(PREFERENCES_NAME, false);
    preferences.putInt("exists",1);
    preferences.putString("VERSION",VERSION);
    preferences.putString("WIFI_SSID",WIFI_SSID);
    preferences.putString("WIFI_PASSWORD",WIFI_PASSWORD);
    preferences.putString("AP_SSID",AP_SSID);
    preferences.putString("AP_PASSWORD",AP_PASSWORD);
    preferences.putString("HOSTNAME",HOSTNAME);
    preferences.putString("ACCESS_TOKEN",ACCESS_TOKEN);
    preferences.putString("API_KEY",API_KEY);
    preferences.putString("NODE_ID",NODE_ID);
    preferences.putString("OTA_PORT",OTA_PORT);
    preferences.putString("OTA_PASSWORD",OTA_PASSWORD);
    preferences.putString("NETWORK_MODE",NETWORK_MODE);
    preferences.putString("SERVER_API",SERVER_API);
    preferences.end();
    Serial.println("Configurations Saved");
}
void saveNewConfig() // Save New Config which was not saved before
{
    Serial.println("Saving New Configurations");
    preferences.begin(PREFERENCES_NAME, false);
    preferences.putInt("exists",1);
    preferences.putString("VERSION",VERSION);
    if(!preferences.isKey("WIFI_SSID"))
        preferences.putString("WIFI_SSID",WIFI_SSID);
    if(!preferences.isKey("WIFI_PASSWORD"))
        preferences.putString("WIFI_PASSWORD",WIFI_PASSWORD);
    if(!preferences.isKey("AP_SSID"))
        preferences.putString("AP_SSID",AP_SSID);
    if(!preferences.isKey("AP_PASSWORD"))
        preferences.putString("AP_PASSWORD",AP_PASSWORD);
    if(!preferences.isKey("HOSTNAME"))
        preferences.putString("HOSTNAME",HOSTNAME);
    if(!preferences.isKey("ACCESS_TOKEN"))
        preferences.putString("ACCESS_TOKEN",ACCESS_TOKEN);
    if(!preferences.isKey("API_KEY"))
        preferences.putString("API_KEY",API_KEY);
    if(!preferences.isKey("NODE_ID"))
        preferences.putString("NODE_ID",NODE_ID);
    if(!preferences.isKey("OTA_PORT"))
        preferences.putString("OTA_PORT",OTA_PORT);
    if(!preferences.isKey("OTA_PASSWORD"))
        preferences.putString("OTA_PASSWORD",OTA_PASSWORD);
    if(!preferences.isKey("NETWORK_MODE"))
        preferences.putString("NETWORK_MODE",NETWORK_MODE);
    if(!preferences.isKey("SERVER_API"))
        preferences.putString("SERVER_API",SERVER_API);
    preferences.end();
    Serial.println("New Configurations Saved");
}
void resetDevice() // Reset Device: Reset the device config to default
{
    preferences.begin(PREFERENCES_NAME, false);
    preferences.clear();
    preferences.end();
    Serial.println("Device Reseted");
    delay(1000);
    ESP.restart();
}
void configHandle() // Load Config: Load presaved config or save default config to NVS
{
    if(!loadConfig())
    {
        Serial.println("Loading Default Configurations");
        saveDefaultConfig();
    }
}
// ----------------


// Device State Sync Functions
bool restoreDeviceState() //This function is used to restore the devices state from the server or NVS
{
    httpResponse_t response = httpRequest(SERVER_API,"POST","apiKey="+API_KEY+"&nodeId="+NODE_ID+"&action=restoreState","application/x-www-form-urlencoded","",1000);
    if(response.code!=200)
    {
        Serial.println("Failed to Restore Device State: HTTP ERROR "+String(response.code));
        deviceStateSyncFailedCount++;
        if(deviceStateSyncFailedCount>=SYNCRONIZE_FAILED_LIMIT)
        {
            Serial.println("Device State Sync Failed Limit Reached");
            AUTO_SYNCRONIZE = false;
            return false;
        }
        return false;
    }
    else
    {
        DynamicJsonDocument doc(500);
        DeserializationError error = deserializeJson(doc, response.text);
        if (error) {
            Serial.println("Failed to restore device state: Deserialization Error");
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            deviceStateSyncFailedCount++;
            if(deviceStateSyncFailedCount>=SYNCRONIZE_FAILED_LIMIT)
            {
                Serial.println("Device State Sync Failed Limit Reached");
                AUTO_SYNCRONIZE = false;
            }
            return false; 
        }

        if (doc.containsKey("devices")) 
        {
            bool isServerSynced = true;
            JsonArray devicesArray = doc["devices"].as<JsonArray>();
            for (JsonVariant deviceVariant : devicesArray) 
            {
                JsonObject deviceObject = deviceVariant.as<JsonObject>();
                int deviceId = deviceObject["id"].as<int>();
                int deviceState = deviceObject["state"].as<int>();
                int devicePwmValue = deviceObject["pwmValue"].as<int>();
                device * targetDevice = devManager.getById(deviceId);
                if(targetDevice!=NULL)
                {
                    targetDevice->setPwmValue(devicePwmValue);
                    if(targetDevice->state != targetDevice->initState) //If any changes in device state made by the user after booting 
                        isServerSynced = false;
                    else
                        targetDevice->setState(deviceState);
                }
            }
            Serial.println("Device State Restored");
            deviceStateSyncFailedCount = 0;
            isSyncWithServerDone = true;
            isSyncTheServerDone = isServerSynced;
            return true;
        }
        else
        {
            Serial.println("Failed to Restore Device State: Invalid Response: "+response.text);
            deviceStateSyncFailedCount++;
            if(deviceStateSyncFailedCount>=SYNCRONIZE_FAILED_LIMIT)
            {
                Serial.println("Device State Sync Failed Limit Reached");
                AUTO_SYNCRONIZE = false;
            }
            return false;
        }
    }
}
bool storeDeviceState() //This function is used to store the devices state to the server or NVS
{
    String deviceState = "";
    int i = 0;
    device * targetDevice = devManager.getByIndex(i);
    while(targetDevice!=NULL)
    {
        deviceState += String(targetDevice->id)+":"+String(targetDevice->state)+":"+String(targetDevice->pwmValue);
        device * nextDevice = devManager.getByIndex(++i);
        if(nextDevice!=NULL)
            deviceState += ",";
        targetDevice = nextDevice;
    }
    httpResponse_t response = httpRequest(SERVER_API,"POST","apiKey="+API_KEY+"&nodeId="+NODE_ID+"&action=storeState&deviceState="+deviceState,"application/x-www-form-urlencoded","",1000);
    if(response.code!=200)
    {
        Serial.println("Failed to Store Device State: HTTP ERROR "+String(response.code));
        deviceStateSyncFailedCount++;
        if(deviceStateSyncFailedCount>=SYNCRONIZE_FAILED_LIMIT)
        {
            Serial.println("Device State Sync Failed Limit Reached");
            AUTO_SYNCRONIZE = false;
        }
        return false;
    }
    else
    {
        DynamicJsonDocument doc(100);
        DeserializationError error = deserializeJson(doc, response.text);
        if (error) {
            Serial.println("Failed to store device state: Deserialization Error");
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            deviceStateSyncFailedCount++;
            if(deviceStateSyncFailedCount>=SYNCRONIZE_FAILED_LIMIT)
            {
                Serial.println("Device State Sync Failed Limit Reached");
                AUTO_SYNCRONIZE = false;
            }
            return false; 
        }

        if(doc["status"]=="success")
        {
            Serial.println("Device State Stored");
            deviceStateSyncFailedCount = 0;
            isSyncTheServerDone = true;
            return true;
        }
        else
        {
            Serial.println("Failed to Store Device State: Server Error");
            deviceStateSyncFailedCount++;
            if(deviceStateSyncFailedCount>=SYNCRONIZE_FAILED_LIMIT)
            {
                Serial.println("Device State Sync Failed Limit Reached");
                AUTO_SYNCRONIZE = false;
            }
            return false;
        }
    }
}
// ---------------------------



void switchHandle() //This function is used to handle the switch inputs
{
    for(int i=0;i<inputSwitchCount;i++)
    {
        if(inputSwitches[i].isEnable==false)
            continue;
        if(inputSwitches[i].longPressBug) 
        {
            if(digitalRead(inputSwitches[i].pin)==inputSwitches[i].triggerState) 
            {
                continue;
            }
            else
                inputSwitches[i].longPressBug=false;
        }
        if(digitalRead(inputSwitches[i].pin)==inputSwitches[i].triggerState)
        {
            unsigned long switchPressStartTime = millis();
            bool longPressed = true;
            while(millis()-switchPressStartTime<inputSwitches[i].longPressTime)
            {
                if(digitalRead(inputSwitches[i].pin)!=inputSwitches[i].triggerState)
                {
                    longPressed = false;
                    break;
                }
                delay(100);
            }
            if(longPressed)
            {
                Serial.println(String(inputSwitches[i].name)+" Long Pressed");
                inputSwitches[i].longPressBug = true;
                if(inputSwitches[i].longPressFunction!=NULL)
                {
                    indicate();
                    indicate();
                    inputSwitches[i].longPressFunction();
                }
            }
            else
            {
                Serial.println(String(inputSwitches[i].name)+" Single Pressed");
                if(inputSwitches[i].singlePressFunction!=NULL)
                {
                    indicate();
                    inputSwitches[i].singlePressFunction();
                }
            }
        }
    }
}

void dht11Read()
{
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    Serial.println("Temperature: "+String(temperature)+"°C, Humidity: "+String(humidity)+"%");
}
