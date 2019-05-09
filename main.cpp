#include <thread>
#include "Packets.h"
#include "Debug.h"
#include "SQLWrapper.h"
#include "Net.h"

#include <sstream>
#include "Drive.h"
#include "Battery.h"
#include "CAN.h"
#include "LIDAR.h"
#include "ZED.h"
#include "SQLThread.h"

#define DRIVES_FILENAME "Drives.json"
#define BATTERY_FILENAME "BatteryCapture.cap"
#define CAN_FILENAME "CANCapture.cap"
#define LIDAR_FILENAME "LIDARCapture.cap"
#define ZED_FILENAME "ZEDCapture.cap"

int main(int argc, const char **argv)
{
    Debug::createDebugPipe();
    
    int sockfd = createReadSocket(LOGGING_CONTROL_PORT);
    Debug::writeDebugMessage("[Logging] Logging Control socket open\n");
    
    Drive::Init(DRIVES_FILENAME);
    
    Battery::Init();
    std::thread batteryThread (Battery::ListenerThread);
    
    CAN::Init();
    std::thread CANThread (CAN::ListenerThread);
    
    LIDAR::Init();
    std::thread LIDARThread (LIDAR::ListenerThread);
    
    ZED::Init();
    std::thread ZEDThread (ZED::ListenerThread);
    
    std::thread sqlThread (SQLThread::ListenerThread, DRIVES_FILENAME);
    
    while(1)
    {
        srand(time(NULL));
        struct LoggingControlPacket pkt;
        read(sockfd, &pkt, sizeof(pkt));
        std::stringstream ss;
        
        switch (pkt.code)
        {
            case Shutdown:
                Debug::writeDebugMessage("[Logging] Shutting Down\n");
                return EXIT_SUCCESS;
                break;
                
            case StartDrive:
                Drive::StartDrive();
                break;
                
            case EndDrive:
                Drive::EndDrive();
                break;
                
            case StartBatteryCapture:
                ss.clear();
                ss << rand() % 9999999999 << BATTERY_FILENAME;
                Battery::StartCapture(ss.str());
                break;
                
            case EndBatteryCapture:
                Battery::EndCapture();
                break;
                
            case StartCANCapture:
                ss.clear();
                ss << rand() % 9999999999 << CAN_FILENAME;
                CAN::StartCapture(ss.str());
                break;
                
            case EndCANCapture:
                CAN::EndCapture();
                break;
                
            case StartLIDARCapture:
                ss.clear();
                ss << rand() % 9999999999 << LIDAR_FILENAME;
                LIDAR::StartCapture(ss.str());
                break;
                
            case EndLIDARCapture:
                LIDAR::EndCapture();
                break;
                
            case StartZEDCapture:
                ss.clear();
                ss << rand() % 9999999999 << ZED_FILENAME;
                ZED::StartCapture(ss.str());
                break;
                
            case EndZEDCapture:
                ZED::EndCapture();
                break;
                
            default:
                Debug::writeDebugMessage("[Logging] Logging Control Code not recognized\n");
                break;
        }
    }
    
    return EXIT_SUCCESS;
}
