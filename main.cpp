#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <ctime>
#include <fstream>
#include <thread>
#include <semaphore.h>
#include "Net.h"
#include "Packets.h"
#include "SQLWrapper.h"
#include "JSONWrapper.h"
#include "Debug.h"

#include "Battery.h"
#include "CAN.h"
#include "LIDAR.h"
#include "ZED.h"

#define SQL_HOST "localhost"
#define SQL_USER "root"
#define SQL_PASSWORD ""
#define SQL_DB "Voltron"

#define DRIVES_FILENAME "Drives.json"

sem_t* drivesMutex;
std::ofstream drivesJSONFile;

std::string GetUnixTimeStampAsString()
{
    std::time_t result = std::time(nullptr);
    std::stringstream ss;
    ss << result;
    return ss.str();
}

int main(int argc, const char **argv)
{
    Debug::createDebugPipe();
    
    int sockfd = createReadSocket(LOGGING_CONTROL_PORT);
    Debug::writeDebugMessage("[Logging] Logging Control socket open\n");
    
    sem_unlink("LogDrives");
    drivesMutex = sem_open("LogDrives", O_CREAT, 0644, 0);
    
    bool isDriving = false;
    
    Battery::Init();
    std::thread batteryThread (Battery::ListenerThread);
    
    CAN::Init();
    std::thread CANThread (CAN::ListenerThread);
    
    LIDAR::Init();
    std::thread LIDARThread (LIDAR::ListenerThread);
    
    ZED::Init();
    std::thread ZEDThread (ZED::ListenerThread);
    
    while(1)
    {
        struct LoggingControlPacket pkt;
        read(sockfd, &pkt, sizeof(pkt));
        
        switch (pkt.code)
        {
            case Shutdown:
                Debug::writeDebugMessage("[Logging] Shutting Down\n");
                return EXIT_SUCCESS;
                break;
                
            case StartDrive:
                if(!isDriving)
                {
                    Debug::writeDebugMessage("[Logging] Starting Drive\n");
                    isDriving = true;
                    
                    drivesJSONFile.open(DRIVES_FILENAME);
                    
                    Json::Value root;
                    
                    //if(!reader.parse( config_doc, root ))
                    
                    root["Drives"][0]["StartUNIXTimestamp"] = GetUnixTimeStampAsString();
                    
                    Json::StreamWriterBuilder builder;
                    builder["commentStyle"] = "None";
                    builder["indentation"] = "   ";
                    
                    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
                    writer -> write(root, &drivesJSONFile);
                    
                    drivesJSONFile.close();
                }
                break;
                
            case EndDrive:
                if(isDriving)
                {
                    Debug::writeDebugMessage("[Logging] Ending Drive\n");
                    isDriving = false;
                    
                    Battery::EndCapture();
                    CAN::EndCapture();
                    LIDAR::EndCapture();
                    ZED::EndCapture();
                }
                break;
                
            case StartBatteryCapture:
                Battery::StartCapture("BatteryCapture.txt");
                break;
                
            case EndBatteryCapture:
                Battery::EndCapture();
                break;
                
            case StartCANCapture:
                CAN::StartCapture("BatteryCapture.txt");
                break;
                
            case EndCANCapture:
                CAN::EndCapture();
                break;
                
            case StartLIDARCapture:
                LIDAR::StartCapture("LIDARCapture.txt");
                break;
                
            case EndLIDARCapture:
                LIDAR::EndCapture();
                break;
                
            case StartZEDCapture:
                ZED::StartCapture("ZEDCapture.txt");
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
