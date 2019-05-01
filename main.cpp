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

#define SQL_HOST "localhost"
#define SQL_USER "root"
#define SQL_PASSWORD ""
#define SQL_DB "Voltron"

#define DRIVES_FILENAME "Drives.json"

sem_t* drivesMutex;
std::ofstream drivesJSONFile;

sem_t* batteryMutex;
std::ofstream batteryCaptureFile;

sem_t* CANMutex;
std::ofstream CANCaptureFile;

sem_t* LIDARMutex;
std::ofstream LIDARCaptureFile;

sem_t* GPSMutex;
std::ofstream GPSCaptureFile;

sem_t* ZEDMutex;
std::ofstream ZEDCaptureFile;

void BatteryDataListener()
{
    int sockfd = createReadSocket(BATTERY_PORT);
    if (sockfd < 0)
    {
        printf("[Logging] ERROR opening Battery Data socket\n");
        return;
    }
    printf("[Logging] Battery Data socket open\n");
    
    while(1)
    {
        sem_wait(batteryMutex);
        
        struct BatteryPacket pkt;
        
        read(sockfd, &pkt, sizeof(pkt));
        
        printf("%i - %f\n", pkt.cellNum, pkt.charge);

        batteryCaptureFile << pkt.cellNum << ":" << pkt.charge << ";";
        batteryCaptureFile.flush();
        
        sem_post(batteryMutex);
    }
}

//This is gross can be fixed with OOP
void StartCapture(bool isCapturing, std::string debugString, std::ofstream* file, std::string fileName, sem_t* mutex)
{
    if(!isCapturing)
    {
        std::string str = "[Logging] Starting " + debugString + " Capture\n";
        printf(str.c_str());
        
        file->open (fileName);
        sem_post(mutex);
        
        str = "[Logging] " + debugString + " Capture file open\n";
        printf(str.c_str());
    }
}

void EndCapture(bool isCapturing, std::string debugString, std::ofstream* file, std::string fileName, sem_t* mutex)
{
    if(isCapturing)
    {
        std::string str = "[Logging] Stoping " + debugString + " Capture\n";
        printf(str.c_str());
        
        sem_wait(mutex);
        file->close();
        
        str = "[Logging] " + debugString + " Capture file closed\n";
        printf(str.c_str());
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
    int sockfd = createReadSocket(LOGGING_CONTROL_PORT);
    printf("[Logging] Logging Control socket open\n");
    
    sem_unlink("LogBattery");
    batteryMutex = sem_open("LogBattery", O_CREAT, 0644, 0);
    sem_unlink("LogCAN");
    CANMutex = sem_open("LogCAN", O_CREAT, 0644, 0);
    sem_unlink("LogLIDAR");
    LIDARMutex = sem_open("LogLIDAR", O_CREAT, 0644, 0);
    sem_unlink("LogGPS");
    GPSMutex = sem_open("LogGPS", O_CREAT, 0644, 0);
    sem_unlink("LogZED");
    ZEDMutex = sem_open("LogZED", O_CREAT, 0644, 0);
    sem_unlink("LogDrives");
    drivesMutex = sem_open("LogDrives", O_CREAT, 0644, 0);
    
    std::thread batteryThread (BatteryDataListener);
//    std::thread CANThread (CANDataListener);
//    std::thread LIDARThread (LIDARDataListener);
//    std::thread GPSThread (GPSDataListener);
//    std::thread ZEDThread (ZEDDataListener);
    
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
                printf("[Logging] Shutting Down\n");
                return EXIT_SUCCESS;
                break;
                
            case StartDrive:
                if(!isDriving)
                {
                    printf("[Logging] Starting Drive\n");
                    isDriving = true;
                    
                    drivesJSONFile.open(DRIVES_FILENAME);
                    
                    Json::Value root;
                    
                    if(!reader.parse( config_doc, root ))
                    
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
                    printf("[Logging] Ending Drive\n");
                    isDriving = false;
                    
                    EndCapture(isCapturingBattery, "Battery", &batteryCaptureFile, "BatteryCapture.txt", batteryMutex);
                    EndCapture(isCapturingCAN, "CAN", &CANCaptureFile, "CANCapture.txt", CANMutex);
                    EndCapture(isCapturingLIDAR,"LIDAR", &LIDARCaptureFile, "LIDARCapture.txt", LIDARMutex);
                    EndCapture(isCapturingGPS, "GPS", &GPSCaptureFile, "GPSCapture.txt", GPSMutex);
                    EndCapture(isCapturingZED, "ZED", &ZEDCaptureFile, "ZEDCapture.txt", ZEDMutex);
                }
                break;
                
            case StartBatteryCapture:
                StartCapture(isCapturingBattery, "Battery", &batteryCaptureFile, "BatteryCapture.txt", batteryMutex);
                break;
                
            case EndBatteryCapture:
                EndCapture(isCapturingBattery, "Battery", &batteryCaptureFile, "BatteryCapture.txt", batteryMutex);
                break;
                
            case StartCANCapture:
                StartCapture(isCapturingCAN, "CAN", &CANCaptureFile, "CANCapture.txt", CANMutex);
                break;
                
            case EndCANCapture:
                EndCapture(isCapturingCAN, "CAN", &CANCaptureFile, "CANCapture.txt", CANMutex);
                break;
                
            case StartLIDARCapture:
                StartCapture(isCapturingLIDAR,"LIDAR", &LIDARCaptureFile, "LIDARCapture.txt", LIDARMutex);
                break;
                
            case EndLIDARCapture:
                EndCapture(isCapturingLIDAR,"LIDAR", &LIDARCaptureFile, "LIDARCapture.txt", LIDARMutex);
                break;
                
            case StartGPSCapture:
                StartCapture(isCapturingGPS, "GPS", &GPSCaptureFile, "GPSCapture.txt", GPSMutex);
                break;
                
            case EndGPSCapture:
                EndCapture(isCapturingGPS, "GPS", &GPSCaptureFile, "GPSCapture.txt", GPSMutex);
                break;
                
            case StartZEDCapture:
                StartCapture(isCapturingZED, "ZED", &ZEDCaptureFile, "ZEDCapture.txt", ZEDMutex);
                break;
                
            case EndZEDCapture:
                EndCapture(isCapturingZED, "ZED", &ZEDCaptureFile, "ZEDCapture.txt", ZEDMutex);
                break;
                
            default:
                printf("[Logging] Logging Control Code not recognized\n");
                break;
        }
    }
    
    return EXIT_SUCCESS;
}
