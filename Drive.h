#ifndef DRIVE_h
#define DRIVE_h

#include <semaphore.h>
#include <fstream>

class Drive
{
    public:
        static void Init(std::string fileName);
        static void StartDrive();
        static void EndDrive();
        static void StartCapture(std::string type, std::string fileName);
        static void EndCapture(std::string type);
        
    private:
        static bool isDriving;
        static sem_t* mutex;
        static int currentDriveIndex;
        static std::string drivesFileName;
    
        static int GetUnixTimeStampAsInt();
};

#endif
