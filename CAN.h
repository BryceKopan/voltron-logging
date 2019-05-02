#ifndef CAN_THREAD_H
#define CAN_THREAD_H

#include <semaphore.h>
#include <fstream>

class CAN
{
    public:
        static sem_t* mutex;
        static std::ofstream captureFile;
    
        static void ListenerThread();
};

#endif
