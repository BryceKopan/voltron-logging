#include "ZED.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <ctime>
#include "Packets.h"
#include "Net.h"
#include "Debug.h"
#include "Drive.h"

sem_t* ZED::mutex;
std::ofstream ZED::captureFile;
bool ZED::isCapturing = false;

void ZED::Init()
{
    sem_unlink("LogZED");
    mutex = sem_open("LogZED", O_CREAT, 0644, 0);
}

void ZED::ListenerThread()
{
    int sharedMemoryFD;
    struct CAMData* memoryRegions;
    
    sharedMemoryFD = shm_open(CAM_MEMORY_NAME, O_RDONLY, 0777);
    if (sharedMemoryFD == -1)
    {
        Debug::writeDebugMessage("[Logging] ERROR ZED shared memory could not be established\n");
        return;
    }
    
    size_t dataSize = sizeof(struct CAMData) * CAM_NUM_IMAGES;
    memoryRegions = (CAMData*)mmap(NULL, dataSize, PROT_READ, MAP_SHARED, sharedMemoryFD, 0);
    if (memoryRegions == MAP_FAILED)
    {
        Debug::writeDebugMessage("[Logging] ERROR ZED shared memory was established, but could not be mapped\n");
    }
    
    int sockfd = createReadSocket(CAMERA_PORT);
    if (sockfd < 0)
    {
        Debug::writeDebugMessage("[Logging] ERROR opening ZED socket\n");
        return;
    }
    Debug::writeDebugMessage("[Logging] ZED socket open\n");
    
    while(1)
    {
        sem_wait(mutex);
        
        struct CameraPacket pkt;
        
        read(sockfd, &pkt, sizeof(pkt));
        
        captureFile.write((char*) &pkt.timestamp, sizeof(std::time_t));
        captureFile.write((char*) &memoryRegions[pkt.updated], sizeof(struct CameraPacket));
        captureFile.flush();
        
        sem_post(mutex);
    }
}

bool ZED::StartCapture(std::string fileName)
{
    if(!isCapturing)
    {
        Debug::writeDebugMessage("[Logging] Starting ZED Capture\n");
        Drive::StartCapture("ZED", fileName);
        
        captureFile.open(fileName.c_str(), std::ofstream::binary);
        sem_post(mutex);
        
        Debug::writeDebugMessage("[Logging] ZED Capture file open\n");
        isCapturing = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool ZED::EndCapture()
{
    if(isCapturing)
    {
        Debug::writeDebugMessage("[Logging] Stoping ZED Capture\n");
        Drive::EndCapture("ZED");
        
        sem_wait(mutex);
        captureFile.close();
        
        Debug::writeDebugMessage("[Logging] ZED Capture file closed\n");
        isCapturing = false;
        return true;
    }
    else
    {
        return false;
    }
}
