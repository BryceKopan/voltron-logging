#ifndef LIDAR_H
#define LIDAR_H

#include <semaphore.h>
#include <fstream>

class LIDAR
{
    public:
        static sem_t* mutex;
        static std::ofstream captureFile;
        
        static void Init();
        static void ListenerThread();
        static void StartCapture(std::string fileName);
        static void EndCapture();
        
    private:
        static bool isCapturing;
};

#endif
