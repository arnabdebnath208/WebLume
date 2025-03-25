#include "intervalTask.h"


// class intervalTask
intervalTask::intervalTask(unsigned int id,unsigned int intervalMillis,void (*callbackFunction)(),const char * name,bool isEnable,bool isSilent)
{
    this->id = id;
    this->interval = intervalMillis;
    this->callbackFunction = callbackFunction;
    this->name = name;
    this->isEnable = isEnable;
    this->isSilent = isSilent;
    
    this-> lastExecutionTime = millis(); //Initial value
}
bool intervalTask::isEligible()
{
    if(this->isEnable)
    {
        if(millis() - this->lastExecutionTime >= this->interval)
            return true;
    }
    return false;
}
bool intervalTask::setState(bool state)
{
    this->isEnable = state;
    return this->isEnable;
}
bool intervalTask::execute()
{
    if(this->isEnable)
    {
        this->lastExecutionTime = millis();
        if(!this->isSilent)
            Serial.println("Inverval Task Executed| ID: "+String(id)+" Name: "+String(name));
        if(this->callbackFunction != NULL)
        {
            this->callbackFunction();
            return true;
        }
    }
    return false;
}
//--------------------------------------------

// class intervalTaskManager
intervalTaskManager::intervalTaskManager(intervalTask * intervalTasks,unsigned int intervalTaskCount)
{
    this->intervalTasks = intervalTasks;
    this->intervalTaskCount = intervalTaskCount;
}
int intervalTaskManager::handle()
{
    int executionCount = 0;
    for(unsigned int i=0;i<this->intervalTaskCount;i++)
    {
        if(this->intervalTasks[i].isEnable == false) //Skip the disabled interval tasks
            continue;
        if(millis()-this->intervalTasks[i].lastExecutionTime >= this->intervalTasks[i].interval)
        {
            if(this->intervalTasks[i].execute())
                executionCount++;
        }
    }
    return executionCount;
}
//--------------------------------------------