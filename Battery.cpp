#include "Battery.h"

#include <unistd.h>
#include <fcntl.h>
#include "Packets.h"
#include "Net.h"
#include "Debug.h"
#include "Drive.h"

sem_t* Battery::mutex;
std::ofstream Battery::captureFile;
bool Battery::isCapturing = false;

void Battery::Init()
{
    sem_unlink("LogBattery");
    mutex = sem_open("LogBattery", O_CREAT, 0644, 0);
}

void Battery::ListenerThread()
{
    int sockfd = createReadSocket(BATTERY_PORT);
    if (sockfd < 0)
    {
        Debug::writeDebugMessage("[Logging] ERROR opening Battery Data socket\n");
        return;
    }
    Debug::writeDebugMessage("[Logging] Battery Data socket open\n");
    
    while(1)
    {
        struct BatteryPacket pkt;
    
        read(sockfd, &pkt, sizeof(pkt));
        sem_wait(mutex);
        
        captureFile.write((char*) &pkt, sizeof(struct BatteryPacket));
        captureFile.flush();

        sem_post(mutex);
    }
}

bool Battery::StartCapture(std::string fileName)
{
    if(!isCapturing)
    {
        Debug::writeDebugMessage("[Logging] Starting Battery Capture\n");
        Drive::StartCapture("Battery", fileName);
        
        captureFile.open(fileName.c_str(), std::ofstream::binary);
        sem_post(mutex);
        
        Debug::writeDebugMessage("[Logging] Battery Capture file open\n");
        isCapturing = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool Battery::EndCapture()
{
    if(isCapturing)
    {
        Debug::writeDebugMessage("[Logging] Stoping Battery Capture\n");
        Drive::EndCapture("Battery");
        
        sem_wait(mutex);
        captureFile.close();
        
        Debug::writeDebugMessage("[Logging] Battery Capture file closed\n");
        isCapturing = false;
        return true;
    }
    else
    {
        return false;
    }
}
