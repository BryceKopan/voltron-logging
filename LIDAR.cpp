#include "LIDAR.h"

#include <unistd.h>
#include <sys/mman.h>
#include "Packets.h"
#include "Net.h"
#include "Debug.h"

sem_t* LIDAR::mutex;
std::ofstream LIDAR::captureFile;
bool LIDAR::isCapturing;

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
        
        captureFile.write((char*) &memoryRegions[pkt.updated], sizeof(struct LIDARPacket));
        captureFile.flush();
        
        sem_post(mutex);
    }
}

void LIDAR::StartCapture(std::string fileName)
{
    if(!isCapturing)
    {
        std::string str = "[Logging] Starting LIDAR Capture\n";
        Debug::writeDebugMessage(str.c_str());
        
        captureFile.open(fileName, std::ofstream::binary);
        sem_post(mutex);
        
        str = "[Logging] LIDAR Capture file open\n";
        Debug::writeDebugMessage(str.c_str());
        isCapturing = true;
    }
}

void LIDAR::EndCapture()
{
    if(isCapturing)
    {
        std::string str = "[Logging] Stoping LIDAR Capture\n";
        Debug::writeDebugMessage(str.c_str());
        
        sem_wait(mutex);
        captureFile.close();
        
        str = "[Logging] LIDAR Capture file closed\n";
        Debug::writeDebugMessage(str.c_str());
        isCapturing = false;
    }
}
