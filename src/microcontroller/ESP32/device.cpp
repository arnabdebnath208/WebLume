#include "device.h"


// class device constructor and methods
device::device(int id,int groupId,uint8_t gpioPin,char * name,bool isInverted,bool isEnable,bool initState,bool isPWM,int initPwmValue,int pwmMinValue,int pwmMaxValue)
{
    this->id = id;
    this->groupId = groupId;
    this-> gpioPin = gpioPin;  
    this-> name = name;  
    this-> isInverted = isInverted;
    this-> isEnable = isEnable;
    this-> initState = initState;
    this-> isPWM = isPWM;
    this-> initPwmValue = initPwmValue;
    this-> pwmMinValue = pwmMinValue;
    this-> pwmMaxValue = pwmMaxValue;

    this-> setPwmValue(this->initPwmValue); //Set the initial PWM value
    if(this->isEnable)
    {
        pinMode(this->gpioPin,OUTPUT);
        this->setState(this->initState);
    }
}
bool device::on()
{
    if(this->isEnable)
    {
        this->state = true;
        if(this->isPWM)
            this->pwmControl(pwmValue);
        else
        {
            if(this->isInverted)
                digitalWrite(this->gpioPin,LOW);
            else
                digitalWrite(this->gpioPin,HIGH);
        }
        return true;
    }
    return false;
}
bool device::off()
{
    if(this->isEnable)
    {
        this->state = false;
        if(this->isInverted)
        {
            if(this->isPWM)
                analogWrite(this->gpioPin,MCU_PWM_MAX);
            else
                digitalWrite(this->gpioPin,HIGH);
        }
        else
        {
            if(this->isPWM)
                analogWrite(this->gpioPin,MCU_PWM_MIN);
            else
                digitalWrite(this->gpioPin,LOW);
        }
        return true;
    }
    return false;
}
bool device::toggle()
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
void device::setState(bool state)
{
    if(state)
        this->on();
    else
        this->off();
}
bool device::pwmControl(int value)
{
    if(this->isEnable && this->isPWM)
    {
        if(value > this->pwmMaxValue)
            value = this->pwmMaxValue;
        if(value < this->pwmMinValue)
            value = this->pwmMinValue;
        this->pwmValue = value;

        if(this->state)
        {
            int FinalPWMValue;
            if(this->isInverted)
                FinalPWMValue = MCU_PWM_MAX - value;
            else
                FinalPWMValue = value;

            analogWrite(this->gpioPin,FinalPWMValue);
            return true;
        }
    }
    return false;
}
void device::setPwmValue(int value)
{
    if(value > this->pwmMaxValue)
        value = this->pwmMaxValue;
    if(value < this->pwmMinValue)
        value = this->pwmMinValue;
    this->pwmValue = value;
}
bool device::decreasePwmValue(int valueToDecrease)
{
    return this->pwmControl(this->pwmValue - valueToDecrease);
}
bool device::increasePwmValue(int valueToIncrease)
{
    return this->pwmControl(this->pwmValue + valueToIncrease);
}
bool device::setPwmDutyCycle(float dutyCycle)
{
    if(dutyCycle < 0)
        dutyCycle = 0;
    else if(dutyCycle > 100)
        dutyCycle = 100;
    int pwmValue = this->pwmMinValue + ((this->pwmMaxValue - this->pwmMinValue) * dutyCycle * 0.01);
    return this->pwmControl(pwmValue);
}
bool device::increasePwmDutyCycle(float valueToIncrease)
{
    float dutyCycle = (this->pwmValue - this->pwmMinValue) * 100.0 / (this->pwmMaxValue - this->pwmMinValue);
    return this->setPwmDutyCycle(dutyCycle + valueToIncrease);
}
bool device::decreasePwmDutyCycle(float valueToDecrease)
{
    float dutyCycle = (this->pwmValue - this->pwmMinValue) * 100.0 / (this->pwmMaxValue - this->pwmMinValue);
    return this->setPwmDutyCycle(dutyCycle - valueToDecrease);
}
// ------------------------------------


// class deviceManager constructor and methods
deviceManager::deviceManager(device * devices,unsigned int deviceCount)
{
    this->devices = devices;
    this->deviceCount = deviceCount;
}
device * deviceManager::getById(int Id)
{
    for(unsigned int i = 0 ;i<deviceCount;i++)
    {
        if(devices[i].id == Id)
            return &devices[i];
    }
    return NULL;
}
device * deviceManager::getByIndex(int devIndex)
{
    if(devIndex >= 0 && devIndex < deviceCount)
        return &devices[devIndex];
    else
        return NULL;
}

bool deviceManager::on(int index)
{
    device * targetDevice = this->getByIndex(index);
    if(targetDevice != NULL)
        return targetDevice->on();
    return false;
}
bool deviceManager::off(int index)
{
    device * targetDevice = this->getByIndex(index);
    if(targetDevice != NULL)
        return targetDevice->off();
    return false;
}
bool deviceManager::toggle(int index)
{
    device * targetDevice = this->getByIndex(index);
    if(targetDevice != NULL)
        return targetDevice->toggle();
    return false;
}
bool deviceManager::setState(int index,bool state)
{
    device * targetDevice = this->getByIndex(index);
    if(targetDevice != NULL)
    {
        targetDevice->setState(state);
        return true;
    }
    return false;
}
bool deviceManager::pwmControl(int index,int value)
{
    device * targetDevice = this->getByIndex(index);
    if(targetDevice != NULL)
        return targetDevice->pwmControl(value);
    return false;
}
bool deviceManager::setPwmValue(int index,int value)
{
    device * targetDevice = this->getByIndex(index);
    if(targetDevice != NULL)
    {
        targetDevice->setPwmValue(value);
        return true;
    }
    return false;
}
bool deviceManager::decreasePwmValue(int index,int valueToDecrease)
{
    device * targetDevice = this->getByIndex(index);
    if(targetDevice != NULL)
        return targetDevice->decreasePwmValue(valueToDecrease);
    return false;
}
bool deviceManager::increasePwmValue(int index,int valueToIncrease)
{
    device * targetDevice = this->getByIndex(index);
    if(targetDevice != NULL)
        return targetDevice->increasePwmValue(valueToIncrease);
    return false;
}
bool deviceManager::setPwmDutyCycle(int index,float dutyCycle)
{
    device * targetDevice = this->getByIndex(index);
    if(targetDevice != NULL)
        return targetDevice->setPwmDutyCycle(dutyCycle);
    return false;
}
bool deviceManager::increasePwmDutyCycle(int index,float valueToIncrease)
{
    device * targetDevice = this->getByIndex(index);
    if(targetDevice != NULL)
        return targetDevice->increasePwmDutyCycle(valueToIncrease);
    return false;
}
bool deviceManager::decreasePwmDutyCycle(int index,float valueToDecrease)
{
    device * targetDevice = this->getByIndex(index);
    if(targetDevice != NULL)
        return targetDevice->decreasePwmDutyCycle(valueToDecrease);
    return false;
}

bool deviceManager::onById(int Id)
{
    device * targetDevice = this->getById(Id);
    if(targetDevice != NULL)
        return targetDevice->on();
    return false;
}
bool deviceManager::offById(int Id)
{
    device * targetDevice = this->getById(Id);
    if(targetDevice != NULL)
        return targetDevice->off();
    return false;
}
bool deviceManager::toggleById(int Id)
{
    device * targetDevice = this->getById(Id);
    if(targetDevice != NULL)
        return targetDevice->toggle();
    return false;
}
bool deviceManager::setStateById(int Id,bool state)
{
    device * targetDevice = this->getById(Id);
    if(targetDevice != NULL)
    {
        targetDevice->setState(state);
        return true;
    }
    return false;
}
bool deviceManager::pwmControlById(int Id,int value)
{
    device * targetDevice = this->getById(Id);
    if(targetDevice != NULL)
        return targetDevice->pwmControl(value);
    return false;
}
bool deviceManager::setPwmValueById(int Id,int value)
{
    device * targetDevice = this->getById(Id);
    if(targetDevice != NULL)
    {
        targetDevice->setPwmValue(value);
        return true;
    }
    return false;
}
bool deviceManager::decreasePwmValueById(int Id,int valueToDecrease)
{
    device * targetDevice = this->getById(Id);
    if
    (targetDevice != NULL)
        return targetDevice->decreasePwmValue(valueToDecrease);
    return false;
}
bool deviceManager::increasePwmValueById(int Id,int valueToIncrease)
{
    device * targetDevice = this->getById(Id);
    if(targetDevice != NULL)
        return targetDevice->increasePwmValue(valueToIncrease);
    return false;
}
bool deviceManager::setPwmDutyCycleById(int Id,float dutyCycle)
{
    device * targetDevice = this->getById(Id);
    if(targetDevice != NULL)
        return targetDevice->setPwmDutyCycle(dutyCycle);
    return false;
}
bool deviceManager::increasePwmDutyCycleById(int Id,float valueToIncrease)
{
    device * targetDevice = this->getById(Id);
    if(targetDevice != NULL)
        return targetDevice->increasePwmDutyCycle(valueToIncrease);
    return false;
}
bool deviceManager::decreasePwmDutyCycleById(int Id,float valueToDecrease)
{
    device * targetDevice = this->getById(Id);
    if(targetDevice != NULL)
        return targetDevice->decreasePwmDutyCycle(valueToDecrease);
    return false;
}

int deviceManager::onByGroupId(int groupId)
{
    int count = 0;
    for(unsigned int i = 0;i<deviceCount;i++)
    {
        if(devices[i].groupId == groupId)
        {
            if(devices[i].on())
                count++;
        }
    }
    return count;
}
int deviceManager::offByGroupId(int groupId)
{
    int count = 0;
    for(unsigned int i = 0;i<deviceCount;i++)
    {
        if(devices[i].groupId == groupId)
        {
            if(devices[i].off())
                count++;
        }
    }
    return count;
}
int deviceManager::toggleByGroupId(int groupId)
{
    int count = 0;
    for(unsigned int i = 0;i<deviceCount;i++)
    {
        if(devices[i].groupId == groupId)
        {
            if(devices[i].toggle())
                count++;
        }
    }
    return count;
}
int deviceManager::setStatesByGroupId(int groupId,bool state)
{
    int count = 0;
    for(unsigned int i = 0;i<deviceCount;i++)
    {
        if(devices[i].groupId == groupId)
        {
            devices[i].setState(state);
            count++;
        }
    }
    return count;
}
int deviceManager::pwmControlByGroupId(int groupId,int value)
{
    int count = 0;
    for(unsigned int i = 0;i<deviceCount;i++)
    {
        if(devices[i].groupId == groupId)
        {
            if(devices[i].pwmControl(value))
                count++;
        }
    }
    return count;
}
int deviceManager::setPwmValuesByGroupId(int groupId,int value)
{
    int count = 0;
    for(unsigned int i = 0;i<deviceCount;i++)
    {
        if(devices[i].groupId == groupId)
        {
            devices[i].setPwmValue(value);
            count++;
        }
    }
    return count;
}
int deviceManager::decreasePwmValuesByGroupId(int groupId,int valueToDecrease)
{
    int count = 0;
    for(unsigned int i = 0;i<deviceCount;i++)
    {
        if(devices[i].groupId == groupId)
        {
            if(devices[i].decreasePwmValue(valueToDecrease))
                count++;
        }
    }
    return count;
}
int deviceManager::increasePwmValuesByGroupId(int groupId,int valueToIncrease)
{
    int count = 0;
    for(unsigned int i = 0;i<deviceCount;i++)
    {
        if(devices[i].groupId == groupId)
        {
            if(devices[i].increasePwmValue(valueToIncrease))
                count++;
        }
    }
    return count;
}
int deviceManager::setPwmDutyCyclesByGroupId(int groupId,float dutyCycle)
{
    int count = 0;
    for(unsigned int i = 0;i<deviceCount;i++)
    {
        if(devices[i].groupId == groupId)
        {
            if(devices[i].setPwmDutyCycle(dutyCycle))
                count++;
        }
    }
    return count;
}
int deviceManager::increasePwmDutyCyclesByGroupId(int groupId,float valueToIncrease)
{
    int count = 0;
    for(unsigned int i = 0;i<deviceCount;i++)
    {
        if(devices[i].groupId == groupId)
        {
            if(devices[i].increasePwmDutyCycle(valueToIncrease))
                count++;
        }
    }
    return count;
}
int deviceManager::decreasePwmDutyCyclesByGroupId(int groupId,float valueToDecrease)
{
    int count = 0;
    for(unsigned int i = 0;i<deviceCount;i++)
    {
        if(devices[i].groupId == groupId)
        {
            if(devices[i].decreasePwmDutyCycle(valueToDecrease))
                count++;
        }
    }
    return count;
}

int deviceManager::powerOnAll()
{
    int count = 0;
    for(unsigned int i = 0;i<deviceCount;i++)
    {
        if(devices[i].on())
            count++;
    }
    return count;
}
int deviceManager::powerOffAll()
{
    int count = 0;
    for(unsigned int i = 0;i<deviceCount;i++)
    {
        if(devices[i].off())
            count++;
    }
    return count;
}
int deviceManager::toggleAll()
{
    int count = 0;
    for(unsigned int i = 0;i<deviceCount;i++)
    {
        if(devices[i].toggle())
            count++;
    }
    return count;
}
// ------------------------------------