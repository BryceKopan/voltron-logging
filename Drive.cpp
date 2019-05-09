#include "Drive.h"

//Ubuntu include path
#include <jsoncpp/json/json.h>
//OSXinclude path
//#include <json/json.h>

#include "Debug.h"
#include "JSONWrapper.h"
#include "Battery.h"
#include "CAN.h"
#include "LIDAR.h"
#include "ZED.h"

sem_t* Drive::mutex;
bool Drive::isDriving = false;
int Drive::currentDriveIndex = 0;
std::string Drive::drivesFileName;

void Drive::Init(std::string fileName)
{
    sem_unlink("LogDrive");
    mutex = sem_open("LogDrive", O_CREAT, 0644, 1);
    JSONWrapper::InitializeWriter();
    drivesFileName = fileName;
}

void Drive::StartDrive()
{
    if(!isDriving)
    {
        Debug::writeDebugMessage("[Logging] Starting Drive\n");
        isDriving = true;
        
        Json::Value root;
        
        sem_wait(mutex);
        try
        {
            JSONWrapper::ReadJSONFile(root, drivesFileName);
        } catch(Json::RuntimeError) {
            //errors in reading json mean no file exists, which is fine
        }
        sem_post(mutex);
        
        for (const Json::Value& drive : root["Drives"])
        {
            currentDriveIndex++;
        }
        currentDriveIndex = 0;
        
        root["Drives"][currentDriveIndex]["IsCompleted"] = false;
        root["Drives"][currentDriveIndex]["StartUnixTimestamp"] = GetUnixTimeStampAsInt();
        root["Drives"][currentDriveIndex]["EndUnixTimestamp"] = 0;
        root["Drives"][currentDriveIndex]["Captures"] = Json::Value(Json::arrayValue);
        
        sem_wait(mutex);
        JSONWrapper::WriteJSONToFile(root, drivesFileName);
        sem_post(mutex);
    }
}

void Drive::EndDrive()
{
    if(isDriving)
    {
        Debug::writeDebugMessage("[Logging] Ending Drive\n");
        isDriving = false;
        
        Battery::EndCapture();
        CAN::EndCapture();
        LIDAR::EndCapture();
        ZED::EndCapture();
        
        Json::Value root;
        
        sem_wait(mutex);
        JSONWrapper::ReadJSONFile(root, drivesFileName);
        sem_post(mutex);
        
        root["Drives"][currentDriveIndex]["IsCompleted"] = true;
        root["Drives"][currentDriveIndex]["EndUnixTimestamp"] = GetUnixTimeStampAsInt();
        
        sem_wait(mutex);
        JSONWrapper::WriteJSONToFile(root, drivesFileName);
        sem_post(mutex);
    }
}

void Drive::StartCapture(std::string type, std::string fileName)
{
    Json::Value root;
    
    sem_wait(mutex);
    JSONWrapper::ReadJSONFile(root, drivesFileName);
    sem_post(mutex);
    
    int captureIndex = 0;
    for (const Json::Value& capture : root["Drives"][currentDriveIndex]["Captures"])
    {
        captureIndex++;
    }

    root["Drives"][currentDriveIndex]["Captures"][captureIndex]["Type"] = type;
    root["Drives"][currentDriveIndex]["Captures"][captureIndex]["File"] = fileName;
    root["Drives"][currentDriveIndex]["Captures"][captureIndex]["StartUnixTimestamp"] = GetUnixTimeStampAsInt();
    root["Drives"][currentDriveIndex]["Captures"][captureIndex]["EndUnixTimestamp"] = 0;
    
    sem_wait(mutex);
    JSONWrapper::WriteJSONToFile(root, drivesFileName);
    sem_post(mutex);
}

void Drive::EndCapture(std::string type)
{
    Json::Value root;
    
    sem_wait(mutex);
    JSONWrapper::ReadJSONFile(root, drivesFileName);
    sem_post(mutex);
    
    int targetIndex, captureIndex = 0;
    for (const Json::Value& capture : root["Drives"][0]["Captures"])
    {
        captureIndex++;
        if(capture["Type"].asString() == type)
        {
            targetIndex = captureIndex;
        }
    }
    
    root["Drives"][currentDriveIndex]["Captures"][targetIndex]["EndUnixTimestamp"] = GetUnixTimeStampAsInt();
    
    sem_wait(mutex);
    JSONWrapper::WriteJSONToFile(root, drivesFileName);
    sem_post(mutex);
}

int Drive::GetUnixTimeStampAsInt()
{
    int result = static_cast<int>(std::time(NULL));
    return result;
}
