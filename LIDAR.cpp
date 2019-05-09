#include "LIDAR.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "Packets.h"
#include "Net.h"
#include "Debug.h"
#include "Drive.h"

sem_t* LIDAR::mutex;
std::ofstream LIDAR::captureFile;
bool LIDAR::isCapturing = false;

void LIDAR::Init()
{
    sem_unlink("LogLIDAR");
    mutex = sem_open("LogLIDAR", O_CREAT, 0644, 0);
}

void LIDAR::ListenerThread()
{
    int sharedMemoryFD;
    struct LIDARData* memoryRegions;
    
    sharedMemoryFD = shm_open(LIDAR_MEMORY_NAME, O_RDONLY, 0777);
    if (sharedMemoryFD == -1)
    {
        Debug::writeDebugMessage("[Logging] ERROR LIDAR shared memory could not be established\n");
        return;
    }
    
    size_t dataSize = sizeof(struct LIDARData) * LIDAR_DATA_NUM_REGIONS;
    memoryRegions = (LIDARData*)mmap(NULL, dataSize, PROT_READ, MAP_SHARED, sharedMemoryFD, 0);
    if (memoryRegions == MAP_FAILED)
    {
        Debug::writeDebugMessage("[Logging] ERROR LIDAR shared memory was established, but could not be mapped\n");
    }
    
    int sockfd = createReadSocket(LIDAR_PORT);
    if (sockfd < 0)
    {
        Debug::writeDebugMessage("[Logging] ERROR opening LIDAR socket\n");
        return;
    }
    Debug::writeDebugMessage("[Logging] LIDAR socket open\n");
    
    while(1)
    {
        sem_wait(mutex);
        
        struct LIDARPacket pkt;
        
        read(sockfd, &pkt, sizeof(pkt));
        
        captureFile.write((char*) &pkt.timestamp, sizeof(std::time_t));
        captureFile.write((char*) &memoryRegions[pkt.updated], sizeof(struct LIDARPacket));
        captureFile.flush();
        
        sem_post(mutex);
    }
}

bool LIDAR::StartCapture(std::string fileName)
{
    if(!isCapturing)
    {
        Debug::writeDebugMessage("[Logging] Starting LIDAR Capture\n");
        Drive::StartCapture("LIDAR", fileName);
        
        captureFile.open(fileName.c_str(), std::ofstream::binary);
        sem_post(mutex);
        
        Debug::writeDebugMessage("[Logging] LIDAR Capture file open\n");
        isCapturing = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool LIDAR::EndCapture()
{
    if(isCapturing)
    {
        Debug::writeDebugMessage("[Logging] Stoping LIDAR Capture\n");
        Drive::EndCapture("LIDAR");
        
        sem_wait(mutex);
        captureFile.close();
        
        Debug::writeDebugMessage("[Logging] LIDAR Capture file closed\n");
        isCapturing = false;
        return true;
    }
    else
    {
        return false;
    }
}
