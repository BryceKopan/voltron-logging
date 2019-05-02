#include "CAN.h"

#include <unistd.h>
#include "Packets.h"
#include "Net.h"
#include "Debug.h"

sem_t* CAN::mutex;
std::ofstream CAN::captureFile;

void CAN::ListenerThread()
{
    int sockfd = createReadSocket(CAN_DATA_PORT);
    if (sockfd < 0)
    {
        Debug::writeDebugMessage("[Logging] ERROR opening CAN socket\n");
        return;
    }
    Debug::writeDebugMessage("[Logging] CAN socket open\n");
    
    while(1)
    {
        sem_wait(mutex);
        
        struct CANDataPacket pkt;
        
        read(sockfd, &pkt, sizeof(pkt));
        
        captureFile.write((char*) &pkt, sizeof(struct CANDataPacket));
        captureFile.flush();
        
        sem_post(mutex);
    }
}
