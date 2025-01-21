#include <Arduino.h>
#ifndef DEVICE_H
#define DEVICE_H

// 8 bit resolution
#define MCU_PWM_MAX 255
#define MCU_PWM_MIN 0

class device
{
    public:
        int id; // The device ID [Unique]
        int groupId; // The device Group ID [Group of devices]
        uint8_t gpioPin; // The GPIO pin of the device
        char * name; // The name of the device [for debugging]
        bool isInverted; // If the device is inverted it will be ON when the GPIO pin is LOW and OFF when the GPIO pin is HIGH
        bool isEnable; // Enable/Disable the device [true:Enable/false:Disable]
        bool initState; // The initial state of the device [true:ON/false:OFF]
        bool isPWM = false; // If the device is PWM controlled [true:PWM Controlled/false:ON-OFF]
        int initPwmValue = MCU_PWM_MAX; // The initial PWM value of the device [range: pwmMinValue-pwmMaxValue]
        int pwmMinValue = MCU_PWM_MIN; // The minimum PWM value of the device
        int pwmMaxValue = MCU_PWM_MAX; // The maximum PWM value of the device

        int pwmValue; // The PWM value of the device 
        bool state; // The state of the device [true:ON/false:OFF]
    device(int id,int groupId,uint8_t gpioPin,char * name = "",bool isInverted = false,bool isEnable = true,bool initState = false,bool isPWM = false,int initPwmValue = MCU_PWM_MAX,int pwmMinValue = MCU_PWM_MIN,int pwmMaxValue = MCU_PWM_MAX);
    bool on(); //This function is used to turn on the device
    bool off(); //This function is used to turn off the device
    bool toggle(); //This function is used to toggle the device state
    void setState(bool state); //This function is used to set the device state [ON/OFF]
    bool pwmControl(int value); //This function is used to control the PWM value of the device
    void setPwmValue(int value); //This function is used to set the PWM value of the device
    bool decreasePwmValue(int valueToDecrease); //This function is used to decrease the PWM value of the device [Current PWM Value - valueToDecrease]
    bool increasePwmValue(int valueToIncrease); //This function is used to increase the PWM value of the device [Current PWM Value + valueToIncrease]
    bool setPwmDutyCycle(float dutyCycle);
    bool increasePwmDutyCycle(float valueToIncrease);
    bool decreasePwmDutyCycle(float valueToDecrease);
};

class deviceManager
{
    public:
        device * devices = NULL;
        unsigned int deviceCount = 0;
        deviceManager(device * devices,unsigned int deviceCount);
        device * getById(int Id);
        device * getByIndex(int devIndex);
        
        bool on(int index);
        bool off(int index);
        bool toggle(int index);
        bool setState(int index,bool state);
        bool pwmControl(int index,int value);
        bool setPwmValue(int index,int value);
        bool decreasePwmValue(int index,int valueToDecrease);
        bool increasePwmValue(int index,int valueToIncrease);
        bool setPwmDutyCycle(int index,float dutyCycle);
        bool increasePwmDutyCycle(int index,float valueToIncrease);
        bool decreasePwmDutyCycle(int index,float valueToDecrease);
       
        bool onById(int Id);
        bool offById(int Id);
        bool toggleById(int Id);
        bool setStateById(int Id,bool state);
        bool pwmControlById(int Id,int value);
        bool setPwmValueById(int Id,int value);
        bool decreasePwmValueById(int Id,int valueToDecrease);
        bool increasePwmValueById(int Id,int valueToIncrease);
        bool setPwmDutyCycleById(int Id,float dutyCycle);
        bool increasePwmDutyCycleById(int Id,float valueToIncrease);
        bool decreasePwmDutyCycleById(int Id,float valueToDecrease);

        int onByGroupId(int groupId);
        int offByGroupId(int groupId);
        int toggleByGroupId(int groupId);
        int setStatesByGroupId(int groupId,bool state);
        int pwmControlByGroupId(int groupId,int value);
        int setPwmValuesByGroupId(int groupId,int value);
        int decreasePwmValuesByGroupId(int groupId,int valueToDecrease);
        int increasePwmValuesByGroupId(int groupId,int valueToIncrease);
        int setPwmDutyCyclesByGroupId(int groupId,float dutyCycle);
        int increasePwmDutyCyclesByGroupId(int groupId,float valueToIncrease);
        int decreasePwmDutyCyclesByGroupId(int groupId,float valueToDecrease);

        int powerOnAll();
        int powerOffAll();
        int toggleAll();
};

#endif