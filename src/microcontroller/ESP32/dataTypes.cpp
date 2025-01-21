#include <Arduino.h>
#include "config.h"

class irInput;

typedef struct input_switch_struct
{
    int pin; //The GPIO pin to which the switch is connected
    int triggerState; //The state of the pin when the switch is pressed [HIGH/LOW]
    int longPressTime; //The time in milliseconds to consider the switch press as long press
    void (*singlePressFunction)(); //The function to call when the switch is single pressed
    void (*longPressFunction)(); //The function to call when the switch is long pressed
    const char * name; //The name of the switch [for debugging]
    bool isEnable; //Enable/Disable the switch
    bool longPressBug=false; //Internal variable to fix the long press bug

    input_switch_struct(int pin,int triggerState,int longPressTime,void (*singlePressFunction)()=NULL,void (*longPressFunction)()=NULL,char * name="\0",bool isEnable=true)
    {
        this->pin = pin;
        this->triggerState = triggerState;
        this->longPressTime = longPressTime;
        this->singlePressFunction = singlePressFunction;
        this->longPressFunction = longPressFunction;
        this->name = name;
        this->isEnable = isEnable;
        if(this->isEnable)
            pinMode(this->pin,INPUT);
    }
} inputSwitch;

typedef struct interval_task_struct
{
    int long interval; //The interval in milliseconds
    void (*function)(); //The function to call after the interval
    bool isEnable; //Enable/Disable the interval task
    const char * name; //The name of the interval task [for debugging]
    unsigned long lastTime=0; //Internal variable to store the last time the function was called

    interval_task_struct(int interval,void (*function)()=NULL,char * name="\0",bool isEnable=true)
    {
        this->interval = interval;
        this->function = function;
        this->name = name;
        this->isEnable = isEnable;
    }
} intervalTask;

typedef struct http_response_struct
{
    int code; //The HTTP response code
    String header;
    String cookie;
    String text; //The HTTP response text
} httpResponse_t;



class irInput //This class is used to create an IR input object
{
    public:
        uint32_t value; //The IR value to be received
        const char * name; //The name of the IR input [for debugging]
        void (*singlePressFunction)(); //The function to call when the IR input is received
        bool isEnable; //Enable/Disable the IR input

        irInput(uint32_t value,void (*singlePressFunction)()=NULL,char * name="\0",bool isEnable=true)
        {
            this->value = value;
            this->name = name;
            this->singlePressFunction = singlePressFunction;
            this->isEnable = isEnable;
        }
        void action() //This function is used to call the single press function when the IR input is received
        {
            if(this->isEnable)
            {
                if(this->singlePressFunction!=NULL)
                    this->singlePressFunction();
            }
        }
};
