#ifndef BATTERY_THREAD_H
#define BATTERY_THREAD_H

#include <semaphore.h>
#include <fstream>

class Battery
{
    public:
        static sem_t* mutex;
        static std::ofstream captureFile;
    
        static void ListenerThread();
};

#endif
