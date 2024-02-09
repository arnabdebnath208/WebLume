#include <ArduinoOTA.h>
#include <ESP8266mDNS.h> 
#include <WiFiUdp.h> 
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <string.h>

const char *WIFI_SSID = "My WIFI"; //WI-FI SSID
const char *WIFI_PASSWORD = "veryVeryStrongPassword"; //WI-FI PASSWORD
const char *OTA_HOSTNAME = "WebLume"; //OTA HOSTNAME
const char *OTA_PASSWORD = "password";  //OTA PASSWORD
const String WEBSERVER_API_URL = "http://192.168.0.0:8012/api.php"; //Here will be your web api url
// webapi is used to get the pin state and sync the pin state with the server

bool SIPO_REGISTER_MODE = false; //This is used to tell that the nodemcu is using the SIPO register or not
bool SYNC_MODE = true; //This is used to tell that the nodemcu is using the sync fetures or not
bool SYNC_STATE = false;
bool SYNC_FIRST = false;
bool NO_WIFI_MODE=false; 
bool WIFI_SLEEP = false;
bool WIFI_STATE = false;
bool OTA_INS = false;
bool OTA_FIRST = false;
int syncFailedCount = 0;
int SYNC_FAILED_LIMIT = 50;

bool HTTP_ERROR = false; //This is used to tell that the http request is failed or not


//Mode Fetures
bool DEBUG_MODE = false;
bool SERIAL_PRINT = false;

//PIN Description
const int latchPin = D3;  // Connect to the 74HC595 pin 12 (Latch) if SIPO_REGISTER_MODE is true
const int clockPin = D5;  // Connect to the 74HC595 pin 11 (Clock) if SIPO_REGISTER_MODE is true
const int dataPin = D6;   // Connect to the 74HC595 pin 14 (Data) if SIPO_REGISTER_MODE is true
const int irReciverPin = D7; //Connect to the IR reciver signal pin
const int indicatorPin = D8; //Connect to the indicator led +ve pin
const int maskedPins[] = { D1, D2, D3, D5, D6, -1, -1, -1}; //will be use if SIPO_REGISTER_MODE is false
// --------------

bool PIN_STATES[] = { false, false, false, false, false, false, false, false};
const int PIN_COUNT = 8;


ESP8266WebServer server(80); // This is used for the WebServer
IRrecv irrecv(irReciverPin);     // This is used for IR reciver
decode_results results;      // This is used for IR reciver

String getPinStateCodes()
{
  String response="";
  for(int i=0;i<PIN_COUNT;i++)
  {
    if(PIN_STATES[i])
      response+="1";
    else
      response+="0";
  }
  return response;
}

void println(String text)
{
  if (DEBUG_MODE)
  {
    // Here will be the process to send the text through http request
    httpRequest(WEBSERVER_API_URL+"?message="+text);
  }
  else if (SERIAL_PRINT)
    Serial.println(text);
}

void indicate(int interval=75,int offInterval=-1) //Seems ok
{
  if(offInterval==-1)
    offInterval=interval;
  digitalWrite(indicatorPin, HIGH);
  delay(interval);
  digitalWrite(indicatorPin, LOW);
  delay(offInterval);
}

String httpRequest(String url)
{
  HTTP_ERROR=false;
  indicate( 30, 20);
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClient client;
    HTTPClient http;

    http.begin(client, url);
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200 || httpResponseCode == 201)
    {
      String response = http.getString();
      http.end();
      client.stop();
      return response;
    }
    else
    {
      HTTP_ERROR=true;
      http.end();
      client.stop();
      return String(httpResponseCode);
    }
  }
  else
  {
    HTTP_ERROR=true;
    return "W0";
  }
}
void httpResponse(String text , int responseCode=200, String contentType="text/html")
{
  server.send(responseCode, contentType, text);
  indicate(50);
}


void sync(int x=0)
{
  indicate(10,20);
  if(SYNC_STATE)
    return;
  if(NO_WIFI_MODE)
  {
    //Here is the process to sync the nodemcu with eeprom of the device
    // Here might use the eeprom to store the pin state
    SYNC_STATE=true;
    // return true;
  }
  else
  {
    if(WiFi.status() == WL_CONNECTED)
    {
      bool isSyncFailed=false;
      if(!SYNC_FIRST)
      {
        //Here is the process to sync the nodemcu with the server
        String response=httpRequest(WEBSERVER_API_URL+"?state");
        if(!HTTP_ERROR)
        {
          bool is_changed=false;
          // verify the response then update it
          if(response.length()==PIN_COUNT)
          {
            for(int i=0;i<PIN_COUNT;i++)
            {
              if(response[i]=='1' && !PIN_STATES[i])
              {
                PIN_STATES[i]=true;
                is_changed=true;
              }
            }
            if(is_changed)
              updatePinStates();
            SYNC_STATE=true;
            SYNC_FIRST=true;
            syncFailedCount=0;
            SYNC_MODE=true; //It will turn on the sync mode
            // return true;
          }
          else
            isSyncFailed=true;
        }
        else
          isSyncFailed=true;
      }
      else
      {
        //Here is the process to sync the server with nodemcu
        // Here is the process to make the sync data
        char data[PIN_COUNT+1];
        for(int i=0;i<PIN_COUNT;i++)
        {
          if(PIN_STATES[i])
            data[i]='1';
          else
            data[i]='0';
        }
        data[PIN_COUNT]='\0'; //It will add the null character at the end of the string
        String response=httpRequest(WEBSERVER_API_URL+"?sync="+data);
        if(!HTTP_ERROR)
        {
          if(response=="true")
          {
            SYNC_STATE=true;
            syncFailedCount=0;
            SYNC_MODE=true; //It will turn on the sync mode
            // return true;
          }
          else
            isSyncFailed=true;
        }
        else
          isSyncFailed=true;
      }

      // It will count the failed sync state if it reached the limit of failed it will turn off the sync fetures
      if(isSyncFailed)
      {
        syncFailedCount++;
        if(syncFailedCount>=SYNC_FAILED_LIMIT)
          SYNC_MODE=false;
      }
    }
  }
  // return false;
}

void updateShiftRegister(bool *STATES)
{
  byte data = 0; //make it dynamic to PIN_COUNT

  // Convert the array of boolean values to a single byte
  for(int i=0;i<PIN_COUNT;i++)
  {
    if(STATES[i])
      bitSet(data, i);
  }

  // Send the data to the 74HC595 shift register
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, data);
  digitalWrite(latchPin, HIGH);
}

void updatePinStates() //This function is used to update the pin state in hardware level
{
  bool is_changed=false;

  if(SIPO_REGISTER_MODE)
  {
    updateShiftRegister(PIN_STATES);
    is_changed=true; 
  }
  else
  {
    for(int i=0;i<PIN_COUNT;i++)
    {
      if((maskedPins[i]!=-1 )&& digitalRead(maskedPins[i])!=PIN_STATES[i])
      {
        digitalWrite(maskedPins[i],PIN_STATES[i]);
        is_changed=true;
      }
    }
  }
  
  if(is_changed)
  {
    SYNC_STATE=false;
    // Here might be some indicate to show that the pin state is changed
    // indicate(50);
  }
}
void pinStateSet(int pin,bool mode) //This function is used to set a pin state
{
  if(pin<PIN_COUNT&&pin>=0)
  {
    PIN_STATES[pin]=mode;
    updatePinStates();
  }
}
void pinStateToggle(int pin) //This function is used to toggle the pin state
{
  pinStateSet(pin,!PIN_STATES[pin]);
}


void turnOffAllPins(int x=0)
{
  for(int i=0;i<PIN_COUNT;i++) 
    PIN_STATES[i]=false;
  updatePinStates();
}
void wifiSleepToggle(int x=0) 
{
  if(WIFI_SLEEP)
  {
    // Turning off the WiFi sleep mode and Trun on the WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WIFI_SLEEP = false;
    for(int i=0;i<10;i++)
      indicate(250);
  }
  else
  {
    // Turning on the WiFi sleep mode and Trun off the WiFi
    if(WIFI_STATE)
      WiFi.disconnect();
    WIFI_SLEEP = true;
    WIFI_STATE = false;
    for(int i=0;i<12;i++)
      indicate(80,200);
  }
}
void restartNodemcu(int x=0) 
{
  ESP.restart();
}

void otaModeSet(bool mode)
{
  if(mode)
    OTA_INS = true;
  else if(OTA_FIRST && mode==false)
    restartNodemcu();
  else
    OTA_INS = false;
}



// HTTP HANDLER Start
void ota1()
{
  if(OTA_INS)
   httpResponse("OTA is already running");
  else
  {
    otaModeSet(true);
    httpResponse("OTA turned on");
  }
}
void ota0()
{
  if(OTA_INS)
  {
    httpResponse("OTA turned off");
    otaModeSet(false);
  }
  else
    httpResponse("OTA is already off");
}
void debug1()
{
  if(DEBUG_MODE)
    httpResponse("Debug mode is already on");
  else
  {
    DEBUG_MODE = true;
    httpResponse("Debug mode is turned on");
  }
}
void debug0()
{
  if(DEBUG_MODE)
  {
    DEBUG_MODE = false;
    httpResponse("Debug mode is turned off");
  }
  else
    httpResponse("Debug mode is already off");
}
// HTTP HANDLER End




void setup()
{
  // pinMode(D0,OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  // pinMode(D4,OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D8, OUTPUT);
  // digitalWrite(D0,LOW);
  digitalWrite(D1, LOW);
  digitalWrite(D2, LOW);
  digitalWrite(D3, LOW);
  // digitalWrite(D4,LOW);
  digitalWrite(D5, LOW);
  digitalWrite(D6, LOW);
  digitalWrite(D8, LOW); 

  if (SERIAL_PRINT)
  {
    //Here is the process to start serial print mode
    Serial.begin(115200);
    delay(10);
  }

  // println("Connecting to " + WIFI_SSID);

  irrecv.enableIRIn(); // Start the ir receiver


  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // Start the WiFi connection

  // wait for 3 sec to connect to the wifi
  for(int i=0;i<20;i++)
  {
    if(WiFi.status() == WL_CONNECTED)
      break;
    indicate(100);
  }

  server.on("/debug1", debug1);
  server.on("/debug0", debug0);
  server.on("/ota1", ota1);
  server.on("/ota0", ota0);

  server.on("/0", HTTP_GET,[](){
    turnOffAllPins();
    httpResponse("true");
    indicate(30);
  });

  server.on("/state", HTTP_GET,[](){
    httpResponse(getPinStateCodes());
  });
   
  server.on("/d00", HTTP_GET, [](){
    pinStateSet(0,false);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d01", HTTP_GET, [](){
    pinStateSet(0,true);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d10", HTTP_GET, [](){
    pinStateSet(1,false);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d11", HTTP_GET, [](){
    pinStateSet(1,true);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d20", HTTP_GET, [](){
    pinStateSet(2,false);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d21", HTTP_GET, [](){
    pinStateSet(2,true);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d30", HTTP_GET, [](){
    pinStateSet(3,false);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d31", HTTP_GET, [](){
    pinStateSet(3,true);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d40", HTTP_GET, [](){
    pinStateSet(4,false);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d41", HTTP_GET, [](){
    pinStateSet(4,true);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d50", HTTP_GET, [](){
    pinStateSet(5,false);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d51", HTTP_GET, [](){
    pinStateSet(5,true);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d60", HTTP_GET, [](){
    pinStateSet(6,false);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d61", HTTP_GET, [](){
    pinStateSet(6,true);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d70", HTTP_GET, [](){
    pinStateSet(7,false);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d71", HTTP_GET, [](){
    pinStateSet(7,true);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d0", HTTP_GET, [](){
    pinStateToggle(0);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d1", HTTP_GET, [](){
    pinStateToggle(1);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d2", HTTP_GET, [](){
    pinStateToggle(2);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d3", HTTP_GET, [](){
    pinStateToggle(3);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d4", HTTP_GET, [](){
    pinStateToggle(4);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d5", HTTP_GET, [](){
    pinStateToggle(5);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d6", HTTP_GET, [](){
    pinStateToggle(6);
    httpResponse("true");
    indicate(30);
  });
  server.on("/d7", HTTP_GET, [](){
    pinStateToggle(7);
    httpResponse("true");
    indicate(30);
  });


  server.begin(); //Start the http server
}

void loop()
{
  //WIFI BASED SYSTEM
  if(!NO_WIFI_MODE && !WIFI_SLEEP)
  {
    if(WiFi.status() == WL_CONNECTED)
    {
      // Here is the process of telling that the wifi is connected;
      WIFI_STATE = true;
      digitalWrite(indicatorPin, LOW);
      server.handleClient(); // This is used for http request recive
      if(SYNC_MODE && !SYNC_STATE)
        sync();
    }
    else
    {
      WIFI_STATE = false;
      digitalWrite(indicatorPin, HIGH);
    }
  }
  
  // IR SYSTEM
  if (irrecv.decode(&results))
  {
    int receivedValue = static_cast<int>(results.value);
    irReciveHandel(receivedValue);
    irrecv.resume();
  }

  // OTA UPDATE SYSTEM
  if (OTA_INS)
  {
    if (OTA_FIRST)
    {
      ArduinoOTA.handle(); //Lesting for the OTA updates
      indicate(50);
    }
    else
    {
      ArduinoOTA.setHostname(OTA_HOSTNAME);
      // ArduinoOTA.setPassword(OTA_PASSWORD);
      ArduinoOTA.begin();
      OTA_FIRST = true;
    }
  }
}


bool tellMyIp()
{
  if(WiFi.status() == WL_CONNECTED)
  {
    String response=httpRequest(WEBSERVER_API_URL+"?ip="+WiFi.localIP().toString());
    if(!HTTP_ERROR)
    {
      if(response=="true")
        return true;
      else
        return false;
    }
    else
      return false;
  }
  else
    return false;
}



//Here will be some ir code for ir functionality
const int IR_CODES_COUNT = 12; 
const int IR_CODES[] = { 0, 33456255, 33441975, 33431775, 33480735, 33427695, 33460335, 33444015, 33478695, 0, 0, 0};
void (*IR_CODE_TOOGLE_FUNC[])(int) = { restartNodemcu, wifiSleepToggle, turnOffAllPins, sync, pinStateToggle, pinStateToggle, pinStateToggle, pinStateToggle, pinStateToggle, pinStateToggle, pinStateToggle, pinStateToggle}; 
const int IR_TOOGLE_FUNC_DATA[] = { -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7}; 

void irReciveHandel(int irValue)
{
  for(int i=0;i<IR_CODES_COUNT;i++) 
  {
    if(irValue==IR_CODES[i])
    {
      IR_CODE_TOOGLE_FUNC[i](IR_TOOGLE_FUNC_DATA[i]);
      indicate(50);
      break;
    }
  }
  println(String(irValue));
}