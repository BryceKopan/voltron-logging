#include "CAN.h"

#include <unistd.h>
#include <fcntl.h>
#include "Packets.h"
#include "Net.h"
#include "Debug.h"
#include "Drive.h"

sem_t* CAN::mutex;
std::ofstream CAN::captureFile;
bool CAN::isCapturing = false;

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

bool CAN::StartCapture(std::string fileName)
{
    if(!isCapturing)
    {
        Debug::writeDebugMessage("[Logging] Starting CAN Capture\n");
        Drive::StartCapture("CAN", fileName);
        
        captureFile.open(fileName.c_str(), std::ofstream::binary);
        sem_post(mutex);
        
        Debug::writeDebugMessage("[Logging] CAN Capture file open\n");
        isCapturing = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool CAN::EndCapture()
{
    if(isCapturing)
    {
        Debug::writeDebugMessage("[Logging] Stoping CAN Capture\n");
        Drive::EndCapture("CAN");
        
        sem_wait(mutex);
        captureFile.close();
        
        Debug::writeDebugMessage("[Logging] CAN Capture file closed\n");
        isCapturing = false;
        return true;
    }
    else
    {
        return false;
    }
}
