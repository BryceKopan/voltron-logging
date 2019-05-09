#ifndef ZED_H
#define ZED_H

#include <semaphore.h>
#include <fstream>

class ZED
{
    public:
        static void Init();
        static void ListenerThread();
        static bool StartCapture(std::string fileName);
        static bool EndCapture();
        
    private:
        static bool isCapturing;
        static sem_t* mutex;
        static std::ofstream captureFile;
};

#endif
