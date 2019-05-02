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
#include <sys/mman.h>
#include "Debug.h"

#include "Battery.h"
#include "CAN.h"

#define SQL_HOST "localhost"
#define SQL_USER "root"
#define SQL_PASSWORD ""
#define SQL_DB "Voltron"

#define DRIVES_FILENAME "Drives.json"

sem_t* drivesMutex;
std::ofstream drivesJSONFile;

sem_t* LIDARMutex;
std::ofstream LIDARCaptureFile;

sem_t* GPSMutex;
std::ofstream GPSCaptureFile;

sem_t* ZEDMutex;
std::ofstream ZEDCaptureFile;

void LIDARListenerThread()
{
    int sharedMemoryFD;
    struct LIDARData* memoryRegions;
    
    sharedMemoryFD = shm_open(LIDAR_MEMORY_NAME, O_RDONLY, 0777);
    if (sharedMemoryFD == -1)
    {
        Debug::writeDebugMessage("[Logging] ERROR LIDAR shared memory could not be established");
    }
    
    size_t dataSize = sizeof(struct LIDARData) * LIDAR_DATA_NUM_REGIONS;
    memoryRegions = (LIDARData*)mmap(NULL, dataSize, PROT_READ, MAP_SHARED, sharedMemoryFD, 0);
    if (memoryRegions == MAP_FAILED)
    {
        Debug::writeDebugMessage("[Logging] ERROR LIDAR shared memory was established, but could not be mapped");
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
        sem_wait(LIDARMutex);
        
        struct LIDARPacket pkt;
        
        read(sockfd, &pkt, sizeof(pkt));
        
        LIDARCaptureFile.write((char*) &memoryRegions[pkt.updated], sizeof(struct LIDARPacket));
        LIDARCaptureFile.flush();
        
        sem_post(LIDARMutex);
    }
}

//This is gross can be fixed with OOP
void StartCapture(bool isCapturing, std::string debugString, std::ofstream& file, std::string fileName, sem_t* mutex)
{
    if(!isCapturing)
    {
        std::string str = "[Logging] Starting " + debugString + " Capture\n";
        Debug::writeDebugMessage(str.c_str());
        
        std::ofstream file (fileName, std::ofstream::binary);
        sem_post(mutex);
        
        str = "[Logging] " + debugString + " Capture file open\n";
        Debug::writeDebugMessage(str.c_str());
    }
}

void EndCapture(bool isCapturing, std::string debugString, std::ofstream& file, std::string fileName, sem_t* mutex)
{
    if(isCapturing)
    {
        std::string str = "[Logging] Stoping " + debugString + " Capture\n";
        Debug::writeDebugMessage(str.c_str());
        
        sem_wait(mutex);
        file.close();
        
        str = "[Logging] " + debugString + " Capture file closed\n";
        Debug::writeDebugMessage(str.c_str());
    }
}

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
    
    sem_unlink("LogBattery");
    Battery::mutex = sem_open("LogBattery", O_CREAT, 0644, 0);
    sem_unlink("LogCAN");
    CAN::mutex = sem_open("LogCAN", O_CREAT, 0644, 0);
    sem_unlink("LogLIDAR");
    LIDARMutex = sem_open("LogLIDAR", O_CREAT, 0644, 0);
    sem_unlink("LogGPS");
    GPSMutex = sem_open("LogGPS", O_CREAT, 0644, 0);
    sem_unlink("LogZED");
    ZEDMutex = sem_open("LogZED", O_CREAT, 0644, 0);
    sem_unlink("LogDrives");
    drivesMutex = sem_open("LogDrives", O_CREAT, 0644, 0);
    
    std::thread batteryThread (Battery::ListenerThread);
    std::thread CANThread (CAN::ListenerThread);
    std::thread LIDARThread (LIDARListenerThread);
//    std::thread GPSThread (GPSListenerThread);
//    std::thread ZEDThread (ZEDListenerThread);
    
    bool isDriving = false;
    bool isCapturingBattery = false;
    bool isCapturingCAN = false;
    bool isCapturingLIDAR = false;
    bool isCapturingGPS = false;
    bool isCapturingZED = false;
    
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
                    
                    EndCapture(isCapturingBattery, "Battery", Battery::captureFile, "BatteryCapture.txt", Battery::mutex);
                    EndCapture(isCapturingCAN, "CAN", CAN::captureFile, "CANCapture.txt", CAN::mutex);
                    EndCapture(isCapturingLIDAR,"LIDAR", LIDARCaptureFile, "LIDARCapture.txt", LIDARMutex);
                    EndCapture(isCapturingGPS, "GPS", GPSCaptureFile, "GPSCapture.txt", GPSMutex);
                    EndCapture(isCapturingZED, "ZED", ZEDCaptureFile, "ZEDCapture.txt", ZEDMutex);
                }
                break;
                
            case StartBatteryCapture:
                StartCapture(isCapturingBattery, "Battery", Battery::captureFile, "BatteryCapture.txt", Battery::mutex);
                break;
                
            case EndBatteryCapture:
                EndCapture(isCapturingBattery, "Battery", Battery::captureFile, "BatteryCapture.txt", Battery::mutex);
                break;
                
            case StartCANCapture:
                StartCapture(isCapturingCAN, "CAN", CAN::captureFile, "CANCapture.txt", CAN::mutex);
                break;
                
            case EndCANCapture:
                EndCapture(isCapturingCAN, "CAN", CAN::captureFile, "CANCapture.txt", CAN::mutex);
                break;
                
            case StartLIDARCapture:
                StartCapture(isCapturingLIDAR,"LIDAR", LIDARCaptureFile, "LIDARCapture.txt", LIDARMutex);
                break;
                
            case EndLIDARCapture:
                EndCapture(isCapturingLIDAR,"LIDAR", LIDARCaptureFile, "LIDARCapture.txt", LIDARMutex);
                break;
                
            case StartGPSCapture:
                StartCapture(isCapturingGPS, "GPS",GPSCaptureFile, "GPSCapture.txt", GPSMutex);
                break;
                
            case EndGPSCapture:
                EndCapture(isCapturingGPS, "GPS", GPSCaptureFile, "GPSCapture.txt", GPSMutex);
                break;
                
            case StartZEDCapture:
                StartCapture(isCapturingZED, "ZED", ZEDCaptureFile, "ZEDCapture.txt", ZEDMutex);
                break;
                
            case EndZEDCapture:
                EndCapture(isCapturingZED, "ZED", ZEDCaptureFile, "ZEDCapture.txt", ZEDMutex);
                break;
                
            default:
                Debug::writeDebugMessage("[Logging] Logging Control Code not recognized\n");
                break;
        }
    }
    
    return EXIT_SUCCESS;
}
