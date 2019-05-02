#include "Battery.h"

#include <unistd.h>
#include "Packets.h"
#include "Net.h"
#include "Debug.h"

sem_t* Battery::mutex;
std::ofstream Battery::captureFile;

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
        sem_wait(mutex);
        
        struct BatteryPacket pkt;
        
        read(sockfd, &pkt, sizeof(pkt));
        
        captureFile.write((char*) &pkt, sizeof(struct BatteryPacket));
        captureFile.flush();
        
        sem_post(mutex);
    }
}
