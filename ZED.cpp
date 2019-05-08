#include "ZED.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "Packets.h"
#include "Net.h"
#include "Debug.h"

sem_t* ZED::mutex;
std::ofstream ZED::captureFile;
bool ZED::isCapturing;

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
        
        captureFile.write((char*) &memoryRegions[pkt.updated], sizeof(struct CameraPacket));
        captureFile.flush();
        
        sem_post(mutex);
    }
}

void ZED::StartCapture(std::string fileName)
{
    if(!isCapturing)
    {
        std::string str = "[Logging] Starting ZED Capture\n";
        Debug::writeDebugMessage(str.c_str());
        
        captureFile.open(fileName, std::ofstream::binary);
        sem_post(mutex);
        
        str = "[Logging] ZED Capture file open\n";
        Debug::writeDebugMessage(str.c_str());
        isCapturing = true;
    }
}

void ZED::EndCapture()
{
    if(isCapturing)
    {
        std::string str = "[Logging] Stoping ZED Capture\n";
        Debug::writeDebugMessage(str.c_str());
        
        sem_wait(mutex);
        captureFile.close();
        
        str = "[Logging] ZED Capture file closed\n";
        Debug::writeDebugMessage(str.c_str());
        isCapturing = false;
    }
}
