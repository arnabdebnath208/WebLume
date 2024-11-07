#include <Arduino.h>
#include "config.h"

class device;
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


class device //This class is used to create a device object
{
    public:
        int id; // The device ID
        int pin; // The GPIO pin to which the device is connected
        const char * name; // The name of the device [for debugging]
        bool isEnable; // Enable/Disable the device
        bool isInverted; // If the device is inverted it will be turned on when the pin is LOW and turned off when the pin is HIGH
        bool isPWM; // If the device is PWM controlled


        int pwmValue; // The PWM value of the device 
        bool state; // The state of the device [true:ON/false:OFF]
    device(int id,int pin,char * name,bool isEnable=true,bool isInverted=false,bool isPWM=false)
    {
        this->id = id;
        this-> pin = pin;  
        this -> name = name;  
        this-> isEnable = isEnable;
        this-> isInverted = isInverted;
        this -> isPWM = isPWM;

        if(this->isInverted)
            this->pwmValue = PWM_MIN_VALUE;
        else 
            this->pwmValue = PWM_MAX_VALUE;
        this->state = false;
    }
    bool on() //This function is used to turn on the device
    {
        if(this->isEnable)
        {
            if(this->state==false)
            {
                if(this->isPWM)
                {
                    if(this->isInverted)
                        analogWrite(this->pin,PWM_MAX_VALUE-pwmValue);
                    else
                        analogWrite(this->pin,pwmValue);
                }
                else
                {
                    if(this->isInverted)
                        digitalWrite(this->pin,LOW);
                    else
                        digitalWrite(this->pin,HIGH);
                }
                this->state = true;
                return true;
            }
        }
        return false;
    }
    bool off() //This function is used to turn off the device
    {
        if(this->isEnable)
        {
            if(this->state)
            {
                if (this->isPWM)
                {
                    if(this->isInverted)
                        analogWrite(this->pin,PWM_MAX_VALUE);
                    else
                        analogWrite(this->pin,PWM_MIN_VALUE);
                }
                else
                {
                    if(this->isInverted)
                        digitalWrite(this->pin,HIGH);
                    else
                        digitalWrite(this->pin,LOW);
                }
                this->state = false;
                return true;
            }
        }
        return false;
    }
    bool toggle() //This function is used to toggle the device state
    {
        if(this->isEnable)
        {
            if(this->state)
                return this->off();
            else
                return this->on();
        }
        return false;
    }
    bool pwmControl(int value) //This function is used to control the PWM value of the device
    {
        if(this->isEnable)
        {
            if(this->isPWM)
            {
                if(value>PWM_MAX_VALUE)
                    value = PWM_MAX_VALUE;
                if(value< PWM_MIN_VALUE)
                    value = PWM_MIN_VALUE;
                this->pwmValue = value;
                if(this->state)
                {
                    if(this->isInverted)
                        analogWrite(this->pin,PWM_MAX_VALUE-value);
                    else
                        analogWrite(this->pin,value);
                }
                return true;
            }
        }
        return false;
    }
    void setState(int state) //This function is used to set the device state [ON/OFF]
    {
        if(state==1)
            this->on();
        else if(state==0)
            this->off();
    }
    void setPwmValue(int value) //This function is used to set the PWM value of the device
    {
        this->pwmControl(value);
    }
    bool decreasePwmValue(int valueToDecrease) //This function is used to decrease the PWM value of the device [Current PWM Value - valueToDecrease]
    {
        if(this->isEnable)
        {
            if(this->isPWM)
            {
                if(this->pwmValue-valueToDecrease>PWM_MIN_VALUE)
                {
                    this->pwmValue -= valueToDecrease;
                    if(this->state)
                        this->pwmControl(this->pwmValue);
                    return true;
                }
                else
                {
                    this->pwmValue = PWM_MIN_VALUE;
                    if(this->state)
                        this->pwmControl(this->pwmValue);
                    return true;
                }
            }
        }       
        return false; 
    }
    bool increasePwmValue(int valueToIncrease) //This function is used to increase the PWM value of the device [Current PWM Value + valueToIncrease]
    {
        if(this->isEnable)
        {
            if(this->isPWM)
            {
                if(this->pwmValue+valueToIncrease<PWM_MAX_VALUE)
                {
                    this->pwmValue += valueToIncrease;
                    if(this->state)
                        this->pwmControl(this->pwmValue);
                    return true;
                }
                else
                {
                    this->pwmValue = PWM_MAX_VALUE;
                    if(this->state)
                        this->pwmControl(this->pwmValue);
                    return true;
                }
            }
        }       
        return false;
    }
};

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
