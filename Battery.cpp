#include "Battery.h"

#include <unistd.h>
#include "Packets.h"
#include "Net.h"
#include "Debug.h"

sem_t* Battery::mutex;
std::ofstream Battery::captureFile;
bool Battery::isCapturing;

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

void Battery::StartCapture(std::string fileName)
{
    if(!isCapturing)
    {
        std::string str = "[Logging] Starting Battery Capture\n";
        Debug::writeDebugMessage(str.c_str());
        
        captureFile.open(fileName, std::ofstream::binary);
        sem_post(mutex);
        
        str = "[Logging] Battery Capture file open\n";
        Debug::writeDebugMessage(str.c_str());
        isCapturing = true;
    }
}

void Battery::EndCapture()
{
    if(isCapturing)
    {
        std::string str = "[Logging] Stoping Battery Capture\n";
        Debug::writeDebugMessage(str.c_str());
        
        sem_wait(mutex);
        captureFile.close();
        
        str = "[Logging] Battery Capture file closed\n";
        Debug::writeDebugMessage(str.c_str());
        isCapturing = false;
    }
}
