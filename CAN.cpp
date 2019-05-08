#include "CAN.h"

#include <unistd.h>
#include "Packets.h"
#include "Net.h"
#include "Debug.h"

sem_t* CAN::mutex;
std::ofstream CAN::captureFile;
bool CAN::isCapturing;

void CAN::Init()
{
    sem_unlink("LogCAN");
    mutex = sem_open("LogCAN", O_CREAT, 0644, 0);
}

void CAN::ListenerThread()
{
    int sockfd = createReadSocket(CAN_DATA_PORT);
    if (sockfd < 0)
    {
        Debug::writeDebugMessage("[Logging] ERROR opening CAN Data socket\n");
        return;
    }
    Debug::writeDebugMessage("[Logging] CAN Data socket open\n");
    
    while(1)
    {
        struct CANDataPacket pkt;
        
        read(sockfd, &pkt, sizeof(pkt));
        sem_wait(mutex);
        
        captureFile.write((char*) &pkt, sizeof(struct CANDataPacket));
        captureFile.flush();
        
        sem_post(mutex);
    }
}

void CAN::StartCapture(std::string fileName)
{
    if(!isCapturing)
    {
        std::string str = "[Logging] Starting CAN Capture\n";
        Debug::writeDebugMessage(str.c_str());
        
        captureFile.open(fileName, std::ofstream::binary);
        sem_post(mutex);
        
        str = "[Logging] CAN Capture file open\n";
        Debug::writeDebugMessage(str.c_str());
        isCapturing = true;
    }
}

void CAN::EndCapture()
{
    if(isCapturing)
    {
        std::string str = "[Logging] Stoping CAN Capture\n";
        Debug::writeDebugMessage(str.c_str());
        
        sem_wait(mutex);
        captureFile.close();
        
        str = "[Logging] CAN Capture file closed\n";
        Debug::writeDebugMessage(str.c_str());
        isCapturing = false;
    }
}
