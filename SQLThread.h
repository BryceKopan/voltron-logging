#ifndef SQL_THREAD_H
#define SQL_THREAD_H

#include <string>
#include <unistd.h>
#include <semaphore.h>

class SQLThread
{
    public:
        static void ListenerThread(std::string drivesFileName);
    
    private:
        static sem_t* mutex;
    
        static void UploadDrive(int driveIndex, std::string drivesFileName);
};

#endif
