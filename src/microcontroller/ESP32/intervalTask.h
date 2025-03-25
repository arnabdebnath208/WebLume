#ifndef INTERVAL_H
#define INTERVAL_H
#include <Arduino.h>


class intervalTask
{
    public:
        unsigned int id; //Unique
        unsigned int interval; // Inverval in milliseconds
        void (*callbackFunction)(); // Function to be called
        bool isEnable; // If it is true task will be executed
        bool isSilent; // If it is true no verbose will show at the time of execution
        const char * name; // Name of the task

        unsigned long lastExecutionTime = 0;

        intervalTask(unsigned int id,unsigned int intervalMillis,void (*callbackFunction)() = NULL,const char * name = "",bool isEnable=true,bool isSilent=false);
        bool isEligible();
        bool setState(bool state);
        bool execute();
};

class intervalTaskManager
{
    public:
        intervalTask * intervalTasks = NULL;
        unsigned int intervalTaskCount = 0;
        intervalTaskManager(intervalTask * intervalTasks,unsigned int intervalTaskCount);
        int handle();
};

#endif